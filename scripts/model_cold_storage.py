"""Four-vial cryogenic storage canister, with separate animation-ready meshes."""
import bpy, math
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'assets/exports';OUT.mkdir(exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
parts=[]
def mat(hex):
    name='Cryo '+hex
    if name in bpy.data.materials:return bpy.data.materials[name]
    m=bpy.data.materials.new(name);m.diffuse_color=tuple(int(hex[i:i+2],16)/255 for i in (0,2,4))+(1,);m.use_nodes=True
    bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=m.diffuse_color;bs.inputs['Metallic'].default_value=.65;bs.inputs['Roughness'].default_value=.28
    return m
def finish_part(o,name,color):
    o.name=name;o.data.materials.append(mat(color));attr=o.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
    for c in attr.data:c.color=mat(color).diffuse_color
    parts.append(o);return o
def cyl(name,z,r,depth,color,xy=(0,0),top=None,verts=32):
    bpy.ops.mesh.primitive_cone_add(vertices=verts,radius1=r,radius2=r if top is None else top,depth=depth,location=(*xy,z))
    return finish_part(bpy.context.object,name,color)
def box(name,pos,size,color):
    bpy.ops.mesh.primitive_cube_add(size=1,location=pos);o=bpy.context.object;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);return finish_part(o,name,color)
def join(name):
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=parts[0];o.name=name;bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');parts.clear();return o
# Heavy, ribbed vacuum flask with machined collar and eight latches.
cyl('octagonal foot',.09,1.03,.18,'87979d',verts=12)
cyl('lower tapered shoulder',.25,.96,.18,'424f57',top=.83)
cyl('vacuum flask',1.0,.84,1.4,'26323b')
cyl('upper tapered shoulder',1.79,.84,.22,'657780',top=1.0)
for z in (1.94,2.10,2.26):cyl('machined cooling ring',z,1.02,.09,'bdc8cb')
cyl('black collar',2.10,.90,.43,'354854')
cyl('top seating rim',2.39,1.02,.15,'82949c')
cyl('dark inner neck',2.50,.66,.16,'182f3d')
for i in range(8):
    a=i*math.tau/8;x,y=math.cos(a)*.93,math.sin(a)*.93
    o=box('locking latch',(x,y,2.36),(.10,.10,.28),'bcc6ca');o.rotation_euler.z=a
    cyl('foot fastener',.2,.05,.035,'d1dadd',(math.cos(a)*.87,math.sin(a)*.87),verts=8)
for z in (.6,1.32):
    o=cyl('front fitting',z,.09,.08,'96aeb7');o.rotation_euler.y=math.pi/2;o.location.x=-.85
body=join('SM_ColdBody')
# Removable rotating rack and central handle shaft.
cyl('rack disk',.055,.65,.11,'b6c8ce')
cyl('central lift shaft',.55,.08,1.05,'a9c3cf',verts=16)
cyl('upper rack disk',1.07,.65,.06,'839ba6')
for i in range(4):
    a=i*math.tau/4
    cyl('vial socket',.13,.145,.13,'344d5c',(math.cos(a)*.46,math.sin(a)*.46),verts=16)
rack=join('SM_ColdRack')
# Lid with tiered silver dome and handle.
cyl('lid sealing ring',.035,.72,.07,'293f4a')
cyl('lid skirt',.11,.88,.10,'bfcdd0')
cyl('lid dome',.24,.86,.20,'9daeb5',top=.37)
cyl('lid top',.36,.37,.05,'c6d3d7')
cyl('handle neck',.43,.07,.12,'536975',verts=16)
cyl('handle grip',.51,.15,.05,'a7bac3',verts=16)
lid=join('SM_ColdLid')
# Frosted vial with amber sample, dark cap and retaining bands.
cyl('vial body',.49,.095,.78,'9bd8e8',verts=20)
cyl('amber sample',.44,.099,.24,'e6c456',verts=20)
for z in (.12,.29,.63,.84):cyl('vial metal band',z,.112,.05,'d6e2e2',verts=20)
cyl('vial cap',.91,.125,.10,'607f91',verts=20)
cyl('cap crown',.97,.10,.03,'badce5',verts=20)
cyl('vial foot',.055,.105,.11,'7099ac',verts=20)
vial=join('SM_ColdVial')
for o in (body,rack,lid,vial):
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    bpy.ops.export_scene.fbx(filepath=str(OUT/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,mesh_smooth_type='FACE',colors_type='LINEAR',use_triangles=True)
rack.location.z=2.52;lid.location.z=3.73
for i in range(4):
    o=vial if i==0 else vial.copy()
    if i: o.data=vial.data;bpy.context.collection.objects.link(o)
    a=i*math.tau/4;o.location=(math.cos(a)*.46,math.sin(a)*.46,2.65)
bpy.ops.object.camera_add(location=(7,-10,7));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,2.1))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=5.4;bpy.context.scene.camera=cam
for pos,power,size in [((3,-5,7),1200,5),((-4,-2,4),800,4),((0,4,6),1300,3)]:
    bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector((0,0,2))-o.location).to_track_quat('-Z','Y').to_euler()
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24;scene.world.color=(.04,.055,.08);scene.render.resolution_x=800;scene.render.resolution_y=900;scene.render.resolution_percentage=100
scene.render.filepath=str(ROOT/'assets/blender/cold-storage-review.png')
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'assets/blender/cold-storage.blend'));bpy.ops.render.render(write_still=True)
