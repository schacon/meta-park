"""Build articulated limb pieces from the existing Blender dinosaur sculptures.
Run with Blender --background --python scripts/build_dinosaur_rigs.py.
Exports rigid segments with joint origins, plus rest-joint metadata for runtime IK.
"""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'scripts/build_park_assets.py').read_text()
# Reuse the sculpting helpers and species bodies without rebuilding the island.
exec(compile(source[:source.index("for species,c in [('brachiosaurus'")],str(ROOT/'scripts/build_park_assets.py'),'exec'))
scene.name='Park • Articulated Dinosaur Workshop'
rigs=[]; exported={}
LEG_NAMES=('pillar leg','ivory nail','bent weight-bearing leg','broad foot','hoof','muscular thigh','angled shin','splayed toe','claw')
def knee_at(hip,ankle,l1,l2,pole):
    hip,ankle=Vector(hip),Vector(ankle)
    axis=(ankle-hip).normalized();distance=(ankle-hip).length
    bend=Vector(pole)-axis*Vector(pole).dot(axis);bend.normalize()
    along=(l1*l1-l2*l2+distance*distance)/(2*distance)
    return hip+axis*along+bend*math.sqrt(max(0,l1*l1-along*along))
def segment(name,a,b,r1,r2,color,origin):
    begin();tube('articulated limb',[a,b],[r1,r2],color,10)
    ell('rounded joint',a,(r1,r1,r1),color,10,6)
    ell('rounded joint',b,(r2,r2,r2),color,10,6)
    for o in parts:
        for v in o.data.vertices:v.co-=Vector(origin)
    exported[name]=finish(name)
def foot(name,ankle,biped,color):
    begin();a=Vector(ankle)
    if biped:
        tube('raised hock',[a,(a.x+.1,a.y,.28)],[.22,.23],color,9)
        for j in (-1,0,1):
            y=a.y+j*.19
            tube('articulated toe',[(a.x+.03,y,.25),(a.x+.46,y+j*.09,.12),(a.x+.77,y+j*.13,.11)],[.16,.12,.07],tone(color,.88),8)
            tube('ivory claw',[(a.x+.71,y+j*.13,.13),(a.x+.98,y+j*.14,.09)],[.085,0],IVORY,7)
    else:
        ell('weight bearing foot',(a.x+.12,a.y,.24),(.5,.4,.24),color,10,6)
        for j in (-1,0,1):ell('rounded hoof',(a.x+.48,a.y+j*.2,.16),(.16,.12,.16),IVORY,8,5)
    # Precisely ground the soles; all foot meshes use the ankle as their origin.
    bottom=min(v.co.z for o in parts for v in o.data.vertices)
    for o in parts:
        for v in o.data.vertices:v.co.z-=bottom;v.co-=a
    exported[name]=finish(name)
for species,color in [('brachiosaurus','#379489'),('triceratops','#cab26b'),('tyrannosaurus','#e86731'),('velociraptor','#66876c')]:
    dinosaur(species,color)
    original=assets['SM_'+species]
    body=bpy.data.collections.new('SM_'+species+'_Body')
    for o in original.objects:
        if not o.name.startswith(LEG_NAMES):body.objects.link(o)
    exported['SM_'+species+'_Body']=body
    quad=species in ('brachiosaurus','triceratops');bronto=species=='brachiosaurus'
    rig={'species':species,'body':'SM_'+species+'_Body','stride':.65 if not quad else .6,'duty':.64 if not quad else .78,'lift':.34 if not quad else .22,'legs':[]}
    for row,x in enumerate(((-1.15,1.3) if quad else (-.55,))):
        for side in (-1,1):
            label=('hind' if row==0 else 'fore')+('_l' if side==-1 else '_r')
            hip=(x,side*(.9 if quad else .65),3 if bronto or not quad else 2.5)
            ankle=(x+(.12 if quad else .6),side*(1.08 if quad else .94),.24 if quad else .55)
            l1,l2=(1.7,1.7) if bronto else (1.4,1.45) if quad else (1.55,1.65)
            pole=(-1,0,0) if quad and row==1 else (1,0,0)
            knee=knee_at(hip,ankle,l1,l2,pole)
            prefix='SM_'+species+'_'+label
            segment(prefix+'_Upper',hip,knee,.52 if quad else .67,.35 if quad else .38,color,hip)
            segment(prefix+'_Lower',knee,ankle,.35 if quad else .33,.29 if quad else .21,color,knee)
            foot(prefix+'_Foot',ankle,not quad,color)
            offset=({(0,-1):.75,(0,1):.25,(1,-1):.5,(1,1):0}[(row,side)]) if quad else (0 if side==-1 else .5)
            rig['legs'].append({'name':label,'hip':list(hip),'knee':list(knee),'ankle':list(ankle),'pole':list(pole),'offset':offset,
                'upper':prefix+'_Upper','lower':prefix+'_Lower','foot':prefix+'_Foot'})
    rigs.append(rig)
# Same explicit handedness conversion as the island asset exporter.
for name,col in exported.items():
    scene.collection.children.link(col)
    deps=bpy.context.evaluated_depsgraph_get();verts=[];faces=[];colors=[]
    for o in col.objects:
        ev=o.evaluated_get(deps);m=ev.to_mesh();base=len(verts)
        for v in m.vertices:
            p=o.matrix_world@v.co;verts.append((p.x,-p.y,p.z))
        attr=m.color_attributes.get('Color')
        for f in m.polygons:
            loops=list(reversed(f.loop_indices));faces.append(tuple(base+m.loops[k].vertex_index for k in loops));colors.extend(tuple(attr.data[k].color) for k in loops)
        ev.to_mesh_clear()
    obj=mesh(name+' export',verts,faces,WHITE)
    for i,c in enumerate(colors):obj.data.color_attributes['Color'].data[i].color=c
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,mesh_smooth_type='FACE',colors_type='LINEAR',use_triangles=True)
    bpy.data.objects.remove(obj,do_unlink=True);parts.clear();scene.collection.children.unlink(col)
(OUT/'dinosaur-rigs.json').write_text(json.dumps({'version':1,'rigs':rigs,'assets':list(exported)},indent=2)+'\n')
# Editable joint hierarchy for inspecting all four assembled rigs in Blender.
for i,rig in enumerate(rigs):
    root=bpy.data.objects.new(rig['species']+' • articulated rig',None);scene.collection.objects.link(root)
    root.location=((i%2)*18,(i//2)*18,0)
    def joint_instance(name,asset,parent,position):
        obj=bpy.data.objects.new(name,None);obj.instance_type='COLLECTION';obj.instance_collection=exported[asset]
        scene.collection.objects.link(obj);obj.parent=parent;obj.location=position;return obj
    joint_instance('body',rig['body'],root,(0,0,0))
    for leg in rig['legs']:
        hip=joint_instance(leg['name']+' • hip',leg['upper'],root,leg['hip'])
        knee=joint_instance(leg['name']+' • knee',leg['lower'],hip,Vector(leg['knee'])-Vector(leg['hip']))
        joint_instance(leg['name']+' • ankle',leg['foot'],knee,Vector(leg['ankle'])-Vector(leg['knee']))
for area in bpy.context.screen.areas if bpy.context.screen else []:
    if area.type=='VIEW_3D':
        area.spaces.active.region_3d.view_location=Vector((9,9,3));area.spaces.active.region_3d.view_distance=45
bpy.data.libraries.write(str(ROOT/'assets/blender/dinosaur-rigs.blend'),{scene,*exported.values()},fake_user=True)
print('Articulated dinosaurs:',len(rigs),'rigs,',len(exported),'mesh parts')
