"""Original park diorama assets. Execute through Blender MCP; source coordinates are metres.
Each asset is a named collection, exported independently with flat normals + vertex colours.
The existing Blender scene is preserved. The assembled scene is saved as park.blend.
"""
import bpy, math, random, json, os
from pathlib import Path
from mathutils import Vector, Matrix
from mathutils.geometry import delaunay_2d_cdt
from mathutils.bvhtree import BVHTree
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets/exports'
OUT.mkdir(parents=True, exist_ok=True)
rng = random.Random(1993)
scene = bpy.data.scenes.new('Park • Asset Workshop')
bpy.context.window.scene = scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1
scene.render.engine = 'BLENDER_EEVEE'
scene.render.resolution_x = 1600
scene.render.resolution_y = 1200
scene.render.resolution_percentage = 100
scene.world = bpy.data.worlds.new('Park • Daylight')
scene.world.use_nodes = True
scene.world.node_tree.nodes['Background'].inputs[0].default_value = (.32,.48,.62,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value = .5
scene.view_settings.view_transform = 'Standard'
mat = bpy.data.materials.new('Park • Painted facets')
mat.use_nodes = True
bsdf = mat.node_tree.nodes.get('Principled BSDF')
bsdf.inputs['Roughness'].default_value = .82
vc = mat.node_tree.nodes.new('ShaderNodeVertexColor'); vc.layer_name = 'Color'
mat.node_tree.links.new(vc.outputs['Color'], bsdf.inputs['Base Color'])
mat.node_tree.links.new(vc.outputs['Color'], bsdf.inputs['Emission Color'])
glow=mat.node_tree.nodes.new('ShaderNodeMath');glow.operation='MULTIPLY';glow.inputs[1].default_value=3
mat.node_tree.links.new(vc.outputs['Alpha'],glow.inputs[0]);mat.node_tree.links.new(glow.outputs[0],bsdf.inputs['Emission Strength'])
WHITE='#e4e3ce'; IVORY='#ffedb8'; DARK='#273f3e'; WOOD='#876042'; GRAY='#a5a397'; GLASS='#3f7180'
parts=[]; assets={}; placements=[]
def rgb(c):
    if isinstance(c,str): return tuple(int(c[i:i+2],16)/255 for i in (1,3,5))
    return tuple(c[:3])
def lin(c): return tuple(v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4 for v in rgb(c))+(1,)
def tone(c,t): return tuple(min(1,v*t) for v in rgb(c))
def mesh(name, verts, faces, color, face_colors=None):
    m=bpy.data.meshes.new(name); m.from_pydata(verts,[],faces); m.update()
    o=bpy.data.objects.new(name,m); scene.collection.objects.link(o); m.materials.append(mat)
    colors=m.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
    for p in m.polygons:
        c=lin(face_colors[p.index] if face_colors else color)
        for k in p.loop_indices: colors.data[k].color=c[:3]+(1 if 'flame' in name.lower() or 'beacon red lens' in name.lower() else 0,)
    parts.append(o); return o

def box(name,p,s,c,bevel=0):
    x,y,z=(v/2 for v in s)
    o=mesh(name,[(p[0]+a*x,p[1]+b*y,p[2]+d*z) for a,b,d in [(-1,-1,-1),(-1,-1,1),(-1,1,-1),(-1,1,1),(1,-1,-1),(1,-1,1),(1,1,-1),(1,1,1)]],[(0,4,6,2),(1,3,7,5),(0,1,5,4),(2,6,7,3),(0,2,3,1),(4,5,7,6)],c)
    if bevel:
        mod=o.modifiers.new('Hand-cut edges','BEVEL'); mod.width=bevel; mod.segments=1
    return o

def tube(name,points,radii,c,n=8):
    # Swept cross-sections: anatomy, horns, branching trunks and curved tails.
    vs=[]
    for i,p in enumerate(points):
        tangent=Vector(points[min(i+1,len(points)-1)])-Vector(points[max(0,i-1)])
        tangent.normalize(); side=tangent.cross(Vector((0,1,0)))
        if side.length<.01: side=tangent.cross(Vector((1,0,0)))
        side.normalize(); up=tangent.cross(side).normalized()
        r=radii[i]; r=(r,r) if isinstance(r,(int,float)) else r
        for j in range(n):
            a=2*math.pi*j/n; vs.append(Vector(p)+side*(math.cos(a)*r[0])+up*(math.sin(a)*r[1]))
    fs=[tuple(reversed(range(n)))]; cs=[tone(c,.85)]
    for i in range(len(points)-1):
        for j in range(n):
            fs.append((i*n+j,i*n+(j+1)%n,(i+1)*n+(j+1)%n,(i+1)*n+j)); cs.append(tone(c,.94+.04*(j%3)))
    fs.append(tuple((len(points)-1)*n+j for j in range(n))); cs.append(c)
    return mesh(name,vs,fs,c,cs)

def ell(name,p,s,c,n=10,rings=5):
    vs=[(p[0],p[1],p[2]-s[2])]
    for i in range(1,rings):
        a=-math.pi/2+math.pi*i/rings
        for j in range(n):
            b=2*math.pi*j/n; vs.append((p[0]+s[0]*math.cos(a)*math.cos(b),p[1]+s[1]*math.cos(a)*math.sin(b),p[2]+s[2]*math.sin(a)))
    vs.append((p[0],p[1],p[2]+s[2])); fs=[]
    for j in range(n):fs.append((0,1+(j+1)%n,1+j))
    for i in range(rings-2):
        for j in range(n):
            a=1+i*n+j;b=1+i*n+(j+1)%n
            fs.extend([(a,b,b+n),(a,b+n,a+n)])
    for j in range(n): fs.append((len(vs)-1,1+(rings-2)*n+j,1+(rings-2)*n+(j+1)%n))
    return mesh(name,vs,fs,c)

def beam(name,a,b,r,c,n=6):return tube(name,[a,b],[r,r],c,n)
def slab(name,poly,z,thickness,c):
    n=len(poly); vs=[(x,y,z) for x,y in poly]+[(x,y,z-thickness) for x,y in poly]
    fs=[tuple(range(n)),tuple(reversed(range(n,2*n)))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    return mesh(name,vs,fs,c)
def text(name,value,p,size,c,rot=(math.pi/2,0,math.pi),depth=.025,bevel=0,offset=0,font=None):
    curve=bpy.data.curves.new(name,'FONT');curve.body=value;
    font_path=Path('/System/Library/Fonts/Supplemental/Arial Bold.ttf')
    if font_path.exists():curve.font=bpy.data.fonts.load(str(font_path),check_existing=True)
    curve.align_x='CENTER';curve.size=size;curve.extrude=depth;curve.bevel_depth=bevel;curve.bevel_resolution=1;curve.offset=offset
    if font:curve.font=bpy.data.fonts.load(font,check_existing=True)
    o=bpy.data.objects.new(name,curve);scene.collection.objects.link(o);o.location=p;o.rotation_euler=rot
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.convert(target='MESH')
    o=bpy.context.object; o.data.materials.append(mat)
    colors=o.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
    for d in colors.data:d.color=lin(c)[:3]+(0,)
    parts.append(o);return o

def begin():parts.clear()
def finish(name):
    col=bpy.data.collections.new(name);col.use_fake_user=True
    for o in list(parts):
        col.objects.link(o);scene.collection.objects.unlink(o)
    assets[name]=col;parts.clear();return col

def instance(name,p=(0,0,0),yaw=0,scale=1):
    o=bpy.data.objects.new(name+' • placed',None);o.instance_type='COLLECTION';o.instance_collection=assets[name]
    scene.collection.objects.link(o);o.location=p;o.rotation_euler.z=math.radians(yaw);o.scale=(scale,)*3
    placements.append(dict(asset=name,position=list(p),yaw=yaw,scale=scale));return o

# Dinosaur sculpture library: deliberate silhouettes, tapered limbs, separate jaws and details.
def dinosaur(species,c):
    begin(); quad=species in ('brachiosaurus','triceratops','stegosaurus')
    if species=='brachiosaurus':
        tube('barrel torso',[(-2,0,2.8),(-1.3,0,3.2),(.1,0,3.4),(1.4,0,3.5),(2,0,3.8)],[(.5,.65),(1.4,1.15),(1.65,1.3),(1.45,1.1),(.85,.75)],c,10)
        tube('sweeping neck',[(1.3,0,3.8),(2,0,4.6),(2.35,0,6),(2.75,0,7.4),(3.1,0,8.4),(3.7,0,8.8)],[.95,.8,.63,.5,.43,.38],c,9)
        tube('gentle muzzle',[(3.35,0,8.7),(3.9,0,8.7),(4.4,0,8.55)],[.48,(.44,.52),(.27,.35)],c,8)
        head=(3.9,0,8.85); eye_x=3.82; eye_z=8.94; eye_y=.48
        tube('long counterbalance tail',[(-1.7,0,3),(-3.1,0,2.8),(-4.5,.2,2.6),(-6,.5,2.5),(-7,.7,2.7)],[.72,.52,.33,.17,.025],c)
        for x in (-1.15,1.3):
            for side in (-1,1):
                y=side*.87; xx=x+(.15 if side==1 else 0)
                tube('pillar leg',[(xx,y,3),(xx+.12,y*1.15,1.8),(xx+.03,y*1.2,.5),(xx+.15,y*1.2,.18)],[.55,.42,.35,.4],c)
                for toe in (-1,0,1):ell('ivory nail',(xx+.49,y*1.2+toe*.18,.16),(.15,.11,.13),IVORY,6,3)
    elif species in ('triceratops','stegosaurus'):
        stego=species=='stegosaurus'
        tube('armoured torso',[(-2,0,2),(-1.25,0,2.4),(0,0,2.6),(1.4,0,2.4),(2,0,2)],[(.65,.6),(1.25,1.1),(1.45,1.3),(1.05,.95),(.65,.6)],c,10)
        for x in (-1.1,1.2):
            for s in (-1,1):
                tube('bent weight-bearing leg',[(x,s*.8,2.5),(x-.25,s*1.05,1.2),(x+.12,s*1.08,.25)],[.55,.4,.3],c)
                ell('broad foot',(x+.25,s*1.08,.22),(.53,.36,.25),tone(c,.86),8,3)
                for toe in (-1,0,1):ell('hoof',(x+.64,s*1.08+toe*.18,.14),(.17,.1,.13),IVORY,6,3)
        tube('tapering tail',[(-1.8,0,2.2),(-3,0,1.9),(-4.3,.2,1.6),(-5.3,.5,1.75)],[.65,.42,.22,.025],c)
        if stego:
            tube('low browsing head',[(1.7,0,2),(2.5,0,1.5),(3.2,0,1.4)],[(.55,.5),(.45,.4),(.23,.3)],c)
            eye_x=2.65;eye_z=1.75;eye_y=.35
            for j in range(9):
                x=-2.4+j*.51; h=1.1+1.25*math.sin(math.pi*(j+1)/10)
                for s in (-1,1):
                    xx=x+(s+1)*.1; yy=s*.38; z=3.35-.18*abs(x)
                    mesh('stegosaur plate',[(xx-.36,yy,z),(xx-.45,yy,z+h*.62),(xx-.06,yy,z+h),(xx+.42,yy,z+h*.55),(xx+.33,yy,z),(xx,yy+s*.19,z+h*.46)],[(0,1,5),(1,2,5),(2,3,5),(3,4,5),(4,0,5),(4,3,2,1,0)],'#deb978')
            for x in (-4.2,-4.8):
                for s in (-1,1): tube('tail spike',[(x,s*.1,1.7),(x-.3,s*.75,2),(x-.55,s*1.1,2.2)],[.16,.1,0],IVORY,7)
        else:
            tube('cheek and muzzle',[(1.5,0,2.5),(2.35,0,2.35),(3.25,0,1.8),(3.65,0,1.7)],[(1,.85),(.75,.78),(.42,.45),(.18,.28)],c,9)
            # Fan-shaped bony frill with a bevelled, scalloped perimeter.
            rim=[]
            for j in range(14):
                a=2*math.pi*j/14;rim.append((1.6-.32*math.sin(a),1.38*math.cos(a),2.6+1.42*math.sin(a)))
            vs=[(1.95,0,2.65)]+rim+[(x-.16,y*.86,2.6+(z-2.6)*.86) for x,y,z in rim]
            fs=[]; cs=[]
            for j in range(14):fs.extend([(0,j+1,(j+1)%14+1),(j+1,15+j,15+(j+1)%14,(j+1)%14+1)]);cs.extend([tone(c,.9),IVORY])
            mesh('scalloped shield',vs,fs,c,cs)
            for s in (-1,1):tube('brow horn',[(2.25,s*.52,2.95),(2.75,s*.6,3.7),(3.35,s*.62,4.1)],[.23,.14,.015],IVORY,8)
            tube('nasal horn',[(3.15,0,2.12),(3.55,0,2.7)],[.18,.015],IVORY,8)
            eye_x=2.45;eye_z=2.6;eye_y=.72
            tube('dark beak',[(3.35,0,1.82),(3.68,0,1.65),(3.5,0,1.42)],[.28,.21,.03],tone(c,.54),7)
    else:
        rex=species=='tyrannosaurus'; galli=species=='gallimimus'; para=species=='parasaurolophus'; dilo=species=='dilophosaurus'
        size=1 if rex else .8
        tube('balanced torso',[(-1.7,0,2.6),(-.8,0,3.1),(.35,0,3.5),(1.25,0,3.85)],[(.55,.6),(1.05,.86),(1,.8),(.62,.5)],c,10)
        tube('athletic neck',[(.9,0,3.7),(1.55,0,4.35),(1.85,0,5)],[(.65,.55),(.5,.42),(.42,.37)],c)
        if rex:
            tube('sculpted skull',[(1.55,0,5),(2.05,0,5.25),(2.95,0,5.22),(3.65,0,5.05)],[(.48,.5),(.65,.68),(.6,.62),(.44,.5)],c,8)
            tube('mouth interior',[(2.05,0,4.9),(3.5,0,4.82)],[(.11,.53),(.09,.46)],'#6e392c',8)
            tube('lower jaw',[(1.85,0,4.8),(2.6,0,4.56),(3.5,0,4.65)],[(.25,.45),(.2,.49),(.16,.4)],tone(c,1.12),8)
            for s in (-1,1):
                for j in range(5):tube('tooth',[(2.4+j*.22,s*.48,4.93),(2.44+j*.22,s*.48,4.71)],[.065,.003],IVORY,5)
            eye_x=2.04;eye_z=5.5;eye_y=.61
        else:
            tube('streamlined head',[(1.7,0,5),(2.15,0,5.2),(2.75,0,5.13),(3.25,0,4.96)],[(.38,.38),(.43,.42),(.3,.32),(.14,.2)],c,8)
            tube('beak edge',[(2.65,0,4.98),(3.26,0,4.9)],[(.05,.28),(.025,.16)],tone(c,.5),6)
            eye_x=2.1;eye_z=5.36;eye_y=.4
            if para:tube('swept crest',[(1.9,0,5.5),(1.15,0,5.8),(.45,0,5.75),(.12,0,5.4)],[.24,.2,.16,.06],'#d5b06e',8)
            if dilo:
                for s in (-1,1):
                    tube('cranial crest',[(1.75,s*.2,5.45),(2.25,s*.2,5.9),(2.72,s*.16,5.38)],[.15,.19,.05],'#d29447',7)
                    vs=[(1.15,0,4.8)]+[(1.05, s*(.3+1.25*math.sin(math.pi*j/7)),4+1.2*math.cos(math.pi*j/7)) for j in range(8)]
                    mesh('display frill',vs,[(0,j+1,j+2) for j in range(7)],'#dcb55b',[tone('#dcb55b',.85+.1*(j%3)) for j in range(7)])
        tube('sinuous tail',[(-1.3,0,2.8),(-2.6,0,2.6),(-3.9,.2,2.45),(-5.2,.6,2.7),(-6,1,3)],[.66,.46,.26,.12,.015],c)
        for s in (-1,1):
            step=.35 if s==1 else -.25
            tube('muscular thigh',[(-.55,s*.55,3),(-.95+step,s*.92,1.8)],[.75,.49],c,9)
            tube('angled shin',[(-.95+step,s*.92,1.8),(-.6+step,s*.92,.7),(-.15+step,s*.94,.25)],[.37,.2,.2],c,8)
            for toe in (-1,0,1):
                y=s*.94+toe*.18
                tube('splayed toe',[(-.2+step,y,.25),(.25+step,y+toe*.06,.15),(.75+step,y+toe*.1,.12)],[.15,.12,.045],tone(c,.84),6)
                tube('claw',[(.65+step,y+toe*.1,.15),(.92+step,y+toe*.1,.08)],[.09,0],IVORY,6)
            reach=.45 if rex else 1
            tube('forearm',[(.7,s*.55,3.6),(1.05,s*.85,3.1),(1.05+reach,s*.9,3.2)],[.21,.14,.09],c,7)
            for j in range(2 if rex else 3):tube('finger',[(1.05+reach,s*.9+j*.1,3.2),(1.3+reach,s*.9+j*.1,3.05)],[.065,0],IVORY,6)
        # Angular dorsal markings are mesh details, not floating decals.
        for j in range(5):ell('dorsal scale',(-1.4+j*.42,0,3.4+.14*j),(.2,.35,.18),tone(c,.67),6,3)
    for s in (-1,1):
        ell('eye socket',(eye_x,s*eye_y,eye_z),(.22,.095,.23),tone(c,.57),8,4)
        ell('amber eye',(eye_x+.055,s*(eye_y+.065),eye_z+.015),(.13,.055,.14),'#f5d477',8,4)
        ell('pupil',(eye_x+.095,s*(eye_y+.104),eye_z+.015),(.045,.018,.095),DARK,6,4)
        ell('eye glint',(eye_x+.115,s*(eye_y+.12),eye_z+.07),(.026,.014,.03),WHITE,6,3)
        beam('eyebrow',(eye_x-.16,s*eye_y,eye_z+.2),(eye_x+.19,s*eye_y,eye_z+.19),.095,c)
    finish('SM_'+species)

for species,c in [('brachiosaurus','#379489'),('tyrannosaurus','#e86731'),('triceratops','#cab26b'),('velociraptor','#66876c'),('dilophosaurus','#8c9d48'),('gallimimus','#d59a4f'),('stegosaurus','#779569'),('parasaurolophus','#538d83')]:dinosaur(species,c)

# Two simple plaster volumes with substantial stepped roofs and a recessed entry.
begin()
PLASTER='#deded4'
# Positive X appears at the left of the isometric park view.
for x,y,w,d,h in [(6,-1,9,8,5.8),(-4,1,9,10,10)]:
    box('plain plaster block',(x,y,h/2),(w,d,h),PLASTER)
    box('solid raised roof',(x,y,h+1.25),(w*.68,d*.67,2.5),'#d6d7ce')
    if x>0:
        for wx in (-.25,.25):
            box('deep teal front window',(x+w*wx,y-d/2-.025,3.1),(1.7,.08,1.8),'#38616a')
        for wy in (-.25,.25):
            box('deep teal side window',(x+w/2+.025,y+d*wy,3.1),(.08,1.5,1.8),'#38616a')
    else:
        # Tall portal flanked by two chunky piers, with a sheltered wooden door.
        box('recessed wooden entrance',(x,y-d/2-.035,3),(2.8,.12,6),'#896540')
        for dx in (-2.7,2.7):
            box('entrance pier',(x+dx,y-d/2-.7,3.7),(2.15,1.5,7.4),PLASTER)
            box('small entry window',(x+dx,y-d/2-1.47,4.1),(.85,.08,1.2),'#39676e')
        box('deep portal lintel',(x,y-d/2-.7,7.8),(7.55,1.5,1),'#d0d3ca')
        box('door handle',(x-.55,y-d/2-.13,2.8),(.12,.12,.55),'#465953')
        box('side window',(x-w/2-.025,y,6),(.08,1.8,2.1),'#38616a')
box('low connecting wing',(1,2,2),(3,5,4),'#c8ccc1')
finish('SM_VisitorCentre')

begin()
for side in (-1,1):
    x=side*9
    mesh('tapered stone gate tower',
        [(x-3.25,-2.9,0),(x+3.25,-2.9,0),(x+3.25,2.8,0),(x-3.25,2.8,0),
         (x-1.55,-1.75,19.3),(x+1.55,-1.75,19.3),(x+1.55,1.75,19.3),(x-1.55,1.75,19.3)],
        [(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7),(4,5,6,7),(3,2,1,0)],'#93998e')
    box('tower cap',(x,0,19.35),(3.25,3.65,.4),'#b0b5a7')
    for z,y in [(19.6,0),(9.3,-2.5)]:
        box('torch wall bracket',(x,y,z),(1.05,1.35,.85),'#737d72')
        tube('dark brazier',[(x,y,z+.25),(x,y,z+.7)],[.6,.54],'#626b63',6)
        tube('orange flame',[(x,y,z+.5),(x-.13,y,z+1.6),(x+.14,y,z+3.1)],[.48,.4,0],'#ff8d20',7)
        tube('gold flame',[(x,y-.21,z+.6),(x+.08,y-.18,z+1.6),(x-.08,y-.14,z+2.5)],[.32,.27,0],'#ffc94e',6)
        tube('bright flame heart',[(x,y-.36,z+.62),(x,y-.34,z+1.75)],[.2,0],'#ffe8a0',6)
for side in (-1,1):
    x=side*3.45
    box('solid wooden gate leaf',(x,0,5.8),(6.8,1.15,11.6),'#89603a')
    for z in (3,8.65):
        box('recessed door panel',(x,-.6,z),(5.7,.08,4.7),'#765332')
        mesh('subtle inset wood facets',[(x-2.75,-.67,z-2.25),(x+2.75,-.67,z-2.25),
             (x+2.75,-.67,z+2.25),(x-2.75,-.67,z+2.25)],
             [(0,1,2),(0,2,3)],'#88603a',['#88603a','#90663e'])
    for z in (.35,5.8,11.25):box('thick horizontal door frame',(x,-.82,z),(6.75,.42,.55),'#9b7044')
    for xx in (x-3.12,x+3.12):box('thick vertical door frame',(xx,-.82,5.8),(.55,.42,11.2),'#987046')
    beam('upper carved diagonal',(x+side*2.7,-.87,10.75),(x-side*2.7,-.87,6.35),.37,'#9a6d41',4)
    beam('lower carved diagonal',(x+side*2.7,-.87,.9),(x-side*2.7,-.87,5.25),.37,'#9a6d41',4)
    box('bronze door handle',(side*.42,-1.04,5.8),(.16,.2,.75),'#655139')
outline=[(-7.2,12.4),(-7.2,17.3),(-4.4,17.9),(0,19),(4.4,17.9),(7.2,17.3),(7.2,12.4)]
vs=[(x,-.72,z) for x,z in outline]+[(x,.72,z) for x,z in outline]
mesh('arched park sign',vs,[tuple(reversed(range(7))),tuple(range(7,14))]+[(i,(i+1)%7,(i+1)%7+7,i+7) for i in range(7)],'#354348')
# Layered cast letters: dark extrusion, gold rim and bevelled red face.
for label,y,depth,offset,color in [('letter shadow',-.89,.22,.09,'#202827'),('gold letter outline',-1.13,.16,.065,'#f3cf60'),('red raised letter face',-1.35,.10,0,'#bf422b')]:
    text(label,'git-meta\npark',(0,y,16.1),2.6,color,depth=depth,bevel=.025,offset=offset,font='/System/Library/Fonts/Supplemental/Impact.ttf')
box('broad entrance ramp',(0,-8,.05),(15,11,.7),'#c5c3af')
for x in (-7.7,7.7):
    box('ramp stone block',(x,-11.4,.85),(1.65,3.1,2.6),'#a0a89a')
    box('ramp stone cap',(x,-11.4,2.21),(1.7,3.15,.18),'#bdbfac')
finish('SM_Gate')

begin()
poly=[(8.8*math.cos(math.pi/8+j*math.pi/4),8.8*math.sin(math.pi/8+j*math.pi/4)) for j in range(8)]
slab('thick octagonal concrete plinth',poly,1.45,1.65,'#b8b9ae')
slab('wide pale concrete rim',poly,1.7,.25,'#d5d4c7')
slab('recessed charcoal landing surface',[(x*.81,y*.81) for x,y in poly],1.715,.025,'#777e7b')
for x in (-1.75,1.75):box('H vertical',(x,0,1.75),(.95,5.3,.05),WHITE)
box('H bridge',(0,0,1.75),(4.4,.95,.05),WHITE)
finish('SM_Helipad')

begin()
# Three broad timbers make the reference's compact T-shaped dock.
box('shore crosswalk',(-14,0,-.2),(28,4.8,1.5),'#b58e60')
box('outer pier',(-28,-7,-.2),(3.8,23,1.5),'#ab8257')
box('short boarding finger',(-11,-6,-.2),(3.6,12,1.5),'#b58e60')
for x,y in [(0,0),(-28,3.6),(-28,-17.5),(-11,-11)]:
    box('dock piling',(x,y,-.4),(1.15,1.15,4.5),'#98754f')
    box('flat piling cap',(x,y,1.88),(1.3,1.3,.22),'#c7a677')
# A clean graphic hull, dark windshield, white cabin: no plank or railing clutter.
boat_start=len(parts)
cx,cy=-11,-16
hull=[(-5,-2.5),(-4.3,-3.5),(4.3,-3.5),(5,-2.5),(4.1,2.8),(-4.1,2.8)]
slab('red lower hull',[(cx+x*.88,cy+y*.88) for x,y in hull],.1,.9,'#b64f37')
slab('white boat hull',[(cx+x,cy+y) for x,y in hull],1.15,1.05,'#ecede0')
slab('recessed boat deck',[(cx+x*.8,cy+y*.78) for x,y in hull],1.19,.05,'#9aafb1')
box('white cabin',(cx,cy+.25,2.05),(5.8,3.8,1.9),'#e8eee4')
box('front windshield',(cx,cy-1.69,2.25),(4.7,.12,1.1),'#366978')
box('side glass',(cx+2.94,cy+.25,2.25),(.1,2.7,1.1),'#497c85')
box('cabin roof',(cx,cy+.25,3.12),(6.25,4.1,.32),'#f2f0e2')
for part in parts[boat_start:]:
    for vertex in part.data.vertices:
        vertex.co.x=cx+(vertex.co.x-cx)*1.5;vertex.co.y=cy+(vertex.co.y-cy)*1.5;vertex.co.z*=1.25
finish('SM_Dock')

# A dozen substantial posts follow a gently irregular paddock boundary.
begin()
points=[(-2,-15,0),(-9,-14.5,0),(-14,-10.8,0),(-15,-3,0),(-13,7,0),(-8,13.5,0),
        (0,15,0),(10,13,0),(14.5,7,0),(14,-3,0),(10,-12,0),(2,-15,0)]
for i,p in enumerate(points):
    gatepost=i in (0,len(points)-1)
    box('gateway pier' if gatepost else 'large white fence post',(p[0],p[1],2.05),
        (1.65,1.65,4.1) if gatepost else (1.5,1.35,4.1),'#9aa799' if gatepost else WHITE,.1)
    box('post cap',(p[0],p[1],4.12),(1.72,1.6,.32),'#eeeddb',.08)
    if i==len(points)-1:continue  # The final span is the actual entrance opening.
    q=points[i+1]
    for z in (1.45,3.15):beam('heavy horizontal rail',(p[0],p[1],z),(q[0],q[1],z),.32,'#d7dfca',4)
box('paddock gate leaf',(0,-15,1.65),(3.3,.35,3.1),'#a58653')
for x in (-1.45,0,1.45):box('gate upright',(x,-15.25,1.65),(.19,.2,2.95),'#6e755c')
for z in (.3,3):box('gate frame',(0,-15.25,z),(3.3,.2,.2),'#6e755c')
beam('gate diagonal',(-1.45,-15.4,.45),(1.45,-15.4,2.9),.12,'#6e755c',4)
finish('SM_Enclosure')

exec(compile((ROOT/'scripts/model_raptor_pen.py').read_text(),str(ROOT/'scripts/model_raptor_pen.py'),'exec'))

# Reusable foliage: branching broadleaf tree, sculpted palms and angular boulders.
begin()
tube('branching trunk',[(0,0,0),(.15,0,2),(-.05,0,4.7)],[.45,.32,.13],WOOD,7)
for p in [(-1.1,0,3.8),(1.1,.2,4.4),(.1,1,4.5)]:beam('branch',(0,0,2.5),p,.16,WOOD)
ell('main crown',(0,0,5.2),(2.4,2.1,2.55),'#5c993c',7,4)
ell('side crown',(-1.3,-.3,4.3),(1.5,1.5,1.6),'#78aa45',7,4)
finish('SM_Tree')
for name,broad in [('SM_TreeBroad',True),('SM_TreeCrooked',False)]:
    begin()
    tube('leaning trunk',[(0,0,0),(.4,-.15,2.6),(.1,.15,5.8 if broad else 4.8)],[.52,.38,.18],WOOD,6)
    centers=[((0,0,6.8),(3.1,2.7,3.1)),((2,1,6),(2.4,2.1,2.3)),((-1.8,-.5,5.4),(2.1,2.3,2))] if broad else [((.5,0,5.8),(2.1,2.5,3)),((-1.3,.4,4.1),(1.6,1.8,1.7))]
    for k,(center,size) in enumerate(centers):
        beam('crooked branch',(.1,0,2.8),center,.2,WOOD)
        crown=ell('irregular polygon canopy',center,size,['#639a3a','#78a743','#578c35'][k%3],7,4)
        local_rng=random.Random(421+k+(20 if broad else 0))
        for vertex in crown.data.vertices:
            vertex.co.x+=local_rng.uniform(-.32,.32)
            vertex.co.y+=local_rng.uniform(-.32,.32)
            vertex.co.z+=local_rng.uniform(-.22,.22)
    finish(name)
begin()
tube('curved palm trunk',[(0,0,0),(.25,0,2),(.6,.1,4),(.45,.2,6.4)],[.3,.25,.19,.13],'#9b7e49',7)
for j in range(9):
    a=j*2*math.pi/9; v=Vector((math.cos(a),math.sin(a),0)); w=Vector((-math.sin(a),math.cos(a),0));base=Vector((.45,.2,6.3))
    vs=[base,base+v*1.4+Vector((0,0,.75))+w*.48,base+v*3.1+Vector((0,0,-.7)),base+v*1.4+Vector((0,0,.75))-w*.48,base+v*1.55+Vector((0,0,.95))]
    mesh('folded palm frond',vs,[(0,1,4),(1,2,4),(2,3,4),(3,0,4)],'#4d994b',[tone('#4d994b',t) for t in (1.15,1,.85,.92)])
for a in (0,2,4):ell('coconut',(.45+.23*math.cos(a),.2+.23*math.sin(a),6),(.21,.21,.25),WOOD,6,3)
finish('SM_Palm')
begin()
mesh('angular sea boulder',[(-1.45,-.5,-.35),(-.8,-1.2,-.35),(.6,-1.4,-.35),(1.5,-.5,-.35),
(1.2,.9,-.35),(-.6,1.2,-.35),(-1.4,.1,.7),(-.65,-.9,1.1),(.8,-.7,1.25),(1.05,.7,.95),(-.4,.9,1.35),(-.25,-.1,1.9)],
[(0,1,7,6),(1,2,8,7),(2,3,8),(3,4,9,8),(4,5,10,9),(5,0,6,10),(6,7,11,10),(7,8,11),(8,9,11),(9,10,11)],'#978d7b')
finish('SM_Rock')

exec(compile((ROOT/'scripts/model_beach_bar.py').read_text(),str(ROOT/'scripts/model_beach_bar.py'),'exec'))
exec(compile((ROOT/'scripts/model_terminal.py').read_text(),str(ROOT/'scripts/model_terminal.py'),'exec'))

# Island: hand-shaped shoreline rings, triangulated meadow, a broad ridge volcano and lagoon.
layout=json.loads((ROOT/'examples/git-meta/layout.json').read_text())
habitats=layout['habitats']
islets=[(119,69,18,22),(-132,10,11,16)]
def islet_shape(x,y,rx,ry,factor=1):
    if x>0:
        return [(x+rx*factor*math.cos(j*math.pi/8)*(1+.055*math.sin(j*2+.7)),
                 y+ry*.78*factor*math.sin(j*math.pi/8)*(1+.04*math.cos(j*3))) for j in range(16)]
    return [(x+rx*factor*math.cos(j*math.pi/8)*(1+.08*math.sin(j*2)),y+ry*factor*math.sin(j*math.pi/8)) for j in range(16)]
def lagoon(j,scale=1):
    a=j*2*math.pi/32;return (18*math.cos(a)*(1+.08*math.sin(a*5))*scale,9+18*math.sin(a)*(1+.08*math.cos(a*3))*scale)
def lagoon_ratio(x,y):
    # Exact radial distance relative to the polygon shoreline (1 = water edge).
    dx,dy=x,y-9
    if abs(dx)+abs(dy)<.00001:return 0
    best=1e9
    for j in range(32):
        ax,ay=lagoon(j);bx,by=lagoon((j+1)%32);ay-=9;by-=9
        ex,ey=bx-ax,by-ay;den=dx*ey-dy*ex
        if abs(den)<1e-8:continue
        t=(ax*ey-ay*ex)/den;u=(ax*dy-ay*dx)/den
        if t>0 and -.0001<=u<=1.0001:best=min(best,t)
    return 1/best
# Art-directed, asymmetric coast: broad middle, narrow northern headland,
# eastern coves and a southern entrance. Preserve long straight edges and sharp corners.
outline=[(0,170),(32,167),(49,149),(48,125),(34,94),(70,68),(86,40),(94,17),
(91,3),(104,-16),(96,-40),(76,-72),(47,-98),(16,-112),(-5,-116),
(-31,-106),(-61,-89),(-83,-65),(-96,-36),(-97,-22),(-109,-6),
(-105,24),(-110,53),(-97,85),(-67,99),(-43,121),(-46,147),(-29,165)]
coast=list(reversed(outline))
coast_count=len(coast)
def shore_ring(factor,offset):
    # Radial scaling folds the water back over the concave north-beach neck.
    # Offset neighboring edge lines instead; retain the established southern coast.
    ring=[]
    for j,(x,y) in enumerate(coast):
        a=coast[j-1];b=coast[(j+1)%coast_count]
        dx,dy=x-a[0],y-a[1];length=math.hypot(dx,dy);n0=(dy/length,-dx/length)
        dx,dy=b[0]-x,b[1]-y;length=math.hypot(dx,dy);n1=(dy/length,-dx/length)
        denominator=1+n0[0]*n1[0]+n0[1]*n1[1]
        ox=x+offset*(n0[0]+n1[0])/denominator
        oy=y+offset*(n0[1]+n1[1])/denominator
        blend=max(0,min(1,(y-65)/25));blend=blend*blend*(3-2*blend)
        ring.append((x*factor+(ox-x*factor)*blend,y*factor+(oy-y*factor)*blend))
    return ring
def ground(x,y):
    if y>=110:return 2
    z=2
    for cx,cy,h,r in [(65,-76,6,25),(-79,35,5,24),(26,80,7,22),(9,-24,3.3,18)]:
        z+=h*math.exp(-((x-cx)**2+(y-cy)**2)/(r*r))
    weighted=z;total=1
    for h in habitats:
        px,py,pz=[v/100 for v in h['position']]
        d=math.hypot(x-px,y-py)
        if h['species']=='velociraptor':
            # Flat terrace under every wall, the apron, and the rear watchtower;
            # a broad rounded rectangle blends into the expanded headland.
            dx=max(abs(x-px)-22,0);dy=max(py-23-y,y-(py+26),0)
            d=22+math.hypot(dx,dy)
        if d<=22:return pz
        if d<38:
            weight=((38-d)/(d-22))**2
            weighted+=pz*weight;total+=weight
    z=weighted/total
    for px,py,r in [(0,-51,21),(0,-105,27),(-64,-54,17)]:
        t=max(0,min(1,(math.hypot(x-px,y-py)-r)/12));z=2+(z-2)*t*t*(3-2*t)
    return z
begin()
for factor,offset,z,c in [(1,0,-.8,'#efd28b'),(.953,-4.5,1.15,'#f7df99'),(.9,-9.5,2,'#b1c65e')]:
    if factor==1:prev=[(x,y,-3.05) for x,y in shore_ring(1.018,2.5)]
    else:prev=ring
    ring=[(x,y,z) for x,y in shore_ring(factor,offset)]
    vs=prev+ring;fs=[(j,(j+1)%coast_count,(j+1)%coast_count+coast_count,j+coast_count) for j in range(coast_count)]
    mesh('sculpted beach contour',vs,fs,c,[('#f2d99b' if factor==.9 and (coast[j][1]+coast[(j+1)%coast_count][1])*.5>110 else c) for j in range(coast_count)])
inner_shore=shore_ring(.9,-9.5)
coords=[Vector(p) for p in inner_shore]
for y in range(-112,116,9):
    for x in range(-95,99,9):
        xx=x+rng.uniform(-2,2);yy=y+rng.uniform(-2,2)
        if (xx/90)**2+(yy/108)**2<1:coords.append(Vector((xx,yy)))
# A triangulated sandy terrace, continuous with the existing northern shore.
coords.extend(Vector((x,y)) for y in range(108,153,7) for x in range(-32,36,7))
lake_start=len(coords)
for factor in (1,1.07,1.25):
    coords.extend(Vector(lagoon(j,factor)) for j in range(32))
lake_edges=[(lake_start+k*32+j,lake_start+k*32+(j+1)%32) for k in range(3) for j in range(32)]
for h in habitats:
    px,py,pz=[v/100 for v in h['position']]
    if h['species']=='velociraptor':
        for margin in (0,7,14):
            first=len(coords)
            ring=[(-22-margin,-23-margin),(0,-23-margin),(22+margin,-23-margin),(22+margin,1.5),(22+margin,26+margin),(0,26+margin),(-22-margin,26+margin),(-22-margin,1.5)]
            coords.extend(Vector((px+x,py+y)) for x,y in ring)
            lake_edges.extend((first+j,first+(j+1)%8) for j in range(8))
        continue
    first=len(coords)
    coords.extend(Vector((px+24*math.cos(j*math.pi/8),py+24*math.sin(j*math.pi/8))) for j in range(16))
    lake_edges.extend((first+j,first+(j+1)%16) for j in range(16))
v,e,f,*_=delaunay_2d_cdt(coords,lake_edges,[list(range(coast_count))],1,.001)
boundary={(round(x,4),round(y,4)) for x,y in inner_shore}
verts=[]
for x,y in v:
    z=ground(x,y);ratio=lagoon_ratio(x,y)
    if ratio<.9999:z=-.5
    elif ratio<=1.07:z=.54+(.66)*max(0,(ratio-1)/.07)
    elif ratio<1.25:z=1.2+(max(1.3,z)-1.2)*(ratio-1.07)/.18
    if (round(x,4),round(y,4)) in boundary:z=2
    verts.append((x,y,z))
cs=[]
for face in f:
    x=sum(v[k].x for k in face)/len(face);y=sum(v[k].y for k in face)/len(face)
    cs.append(tone('#f1d89a' if y>108+3*math.sin(x*.12) else '#e5d39c' if 1<=lagoon_ratio(x,y)<=1.07 else '#9db756',rng.uniform(.96,1.035)))
mesh('triangulated meadow',verts,f,'#a6be58',cs)
(OUT/'park-ground.json').write_text(json.dumps({'vertices':verts,'triangles':f}))
terrain_bvh=BVHTree.FromPolygons(verts,f)
def surface(x,y):
    hit=terrain_bvh.ray_cast(Vector((x,y,100)),Vector((0,0,-1)))[0]
    return hit.z if hit else 2
# A deliberately asymmetrical volcano with alternating radial ridges.
vs=[];n=13
for k,(r,z) in enumerate([(39,2),(29,19),(17,42),(10,58)]):
    for j in range(n):
        a=2*math.pi*j/n;rr=r*(1+(.13 if j%2 else -.06))
        vs.append((rr*math.cos(a)-2,66+rr*math.sin(a)*.95,z+(rng.uniform(-2,2) if k<3 else 0)))
fs=[];cs=[]
for k in range(3):
    for j in range(n):
        a=k*n+j;b=k*n+(j+1)%n
        fs.extend([(a,b,b+n),(a,b+n,a+n)])
        c='#709440' if k<2 else '#8d8270'
        cs.extend([tone(c,.85+.14*(j%3)),tone(c,.9+.12*(j%3))])
fs.append(tuple(3*n+j for j in range(n)));cs.append('#c0a98b')
mesh('volcano ridges and summit',vs,fs,'#82994b',cs)
# Rocky waterfall gorge at the volcano's foot, split into irregular pillars.
for x,y,z,s in [(-9,28,5,(4.5,5,6)),(-6,29,10,(4,4.5,8)),(-3.8,33,13,(3.8,3.8,7)),(4.8,32,12,(4.5,4,8)),(8,29,7,(4.5,4.5,8)),(10,24,3,(3.7,4,4)),(-7,22,2,(3.5,3,3))]:
    ell('water-worn gorge rock',(x,y,z),s,tone('#939187',rng.uniform(.92,1.07)),6,3)
for x,y,z,size in [(-10,31,6,(3.1,4.7,5.4)),(-6,36,12,(3.6,3.2,6.8)),
(3,36,15,(2.8,3.4,5.3)),(9,34,9,(3.1,5.1,7.6)),(-9,23,1.8,(3.2,2.7,2.8)),
(8,21,1.6,(2.9,2.2,2.4)),(12,27,3,(2.8,3.7,3.9)),(-12,28,2.3,(2.4,3.1,3.3))]:
    ell('uneven gorge outcrop',(x,y,z),size,tone('#939187',rng.uniform(.8,1.12)),7,3)
# Meandering cream paths route around the mountain instead of climbing its faces.
road_coords=[];road_faces=[]
def road(points,width=1.8):
    for a,b in zip(points,points[1:]):
        va,vb=Vector(a),Vector(b);steps=max(1,math.ceil((vb-va).length/2))
        n=Vector((-(vb-va).y,(vb-va).x)).normalized()*width/2
        for k in range(steps):
            aa=va.lerp(vb,k/steps);bb=va.lerp(vb,(k+1)/steps)
            base=len(road_coords);road_coords.extend([aa+n,aa-n,bb-n,bb+n]);road_faces.append(tuple(range(base,base+4)))
# A single entrance spine branches to the visitor centre, paddock gates and helipad.
road([(0,-99),(0,-78),(-6.6,-67),(-6.6,-60)],2.3)
road([(0,-78),(28,-73),(46,-67),(49,-64)],2)
road([(28,-73),(25,-61),(25,-25),(31,-7),(54,2)],2)
road([(0,-78),(-27,-75),(-43,-66),(-64,-63)],2)
road([(-27,-75),(-29,-43),(-29,-20),(-40,-20),(-60,-25)],2)
road([(-29,-20),(-30,9),(-32,22),(-43,23),(-57,27)],2)
# Split paths wherever a terrain triangle changes plane, so rising ground
# cannot cut through the road surface. The union has no coplanar overlaps.
terrain_offset=len(road_coords)
road_coords.extend(Vector((x,y)) for x,y,z in verts)
terrain_edges=set()
for face in f:
    for a,b in zip(face,face[1:]+face[:1]):terrain_edges.add(tuple(sorted((a+terrain_offset,b+terrain_offset))))
rv,_,rf,_,_,origin_faces=delaunay_2d_cdt(road_coords,list(terrain_edges),road_faces,0,.0001)
union_faces=[face for face,ids in zip(rf,origin_faces) if ids]
mesh('connected path network',[(p.x,p.y,surface(p.x,p.y)+.14) for p in rv],union_faces,'#e5dbac')
# The sandy bank belongs to the terrain itself; there is no raised overlay rim.
finish('SM_Island')
begin()
for x,y,rx,ry in islets:
    outline=islet_shape(x,y,rx,ry)
    n=len(outline)
    lower=[(x+(px-x)*1.045,y+(py-y)*1.045,-1.8) for px,py in outline]
    upper=[(px,py,.7) for px,py in outline]
    mesh('islet beach bank',lower+upper,[(j,(j+1)%n,(j+1)%n+n,j+n) for j in range(n)],'#e9d192')
    mesh('unbroken sandy islet',upper,[tuple(range(n))],'#f1dca0')
    # A broad grass crown sits inside the rounded beach.
    body_y=y
    outer=[(x+rx*.76*math.cos(j*math.pi/7),body_y+ry*.57*math.sin(j*math.pi/7),.73) for j in range(14)]
    inner=[(x+rx*.59*math.cos(j*math.pi/7),body_y+ry*.43*math.sin(j*math.pi/7),2.7) for j in range(14)]
    mesh('low islet grassy rise',outer+inner,[(j,(j+1)%14,(j+1)%14+14,j+14) for j in range(14)],'#99b35b')
    mesh('islet grass crown',inner,[tuple(range(14))],'#91aa55')
    ell('islet rock',(x+rx*.25,body_y+ry*.12,2.9),(rx*.14,ry*.13,1.8),'#a09c87',7,3)
finish('SM_Islets')
begin()
for factor,offset,z,c in [(1.23,23,-2.8,'#429fc1'),(1.15,15,-2.76,'#50b3cc'),(1.075,7.5,-2.72,'#75d0d9')]:
    slab('shallow water band',shore_ring(factor,offset),z,.015,c)
poly=[lagoon(j) for j in range(32)]
slab('turquoise lagoon',poly,.52,.1,'#48adc4')
for x,y,rx,ry in islets:
    for factor,z,c in [(1.48,-2.7,'#50b3cc'),(1.23,-2.65,'#75d0d9')]:
        slab('islet shallow shelf',islet_shape(x,y,rx,ry,factor),z,.02,c)
# Inlaid shallow-water halo around the offshore boulder.
for radius,z,c in [(14,-2.64,'#429fbd'),(11,-2.60,'#58b6c9'),(8,-2.56,'#78cbd2')]:
    slab('boulder shallows',[(96+radius*math.cos(j*2*math.pi/9)*(1+.07*math.sin(j*2)),-101+radius*.85*math.sin(j*2*math.pi/9)) for j in range(9)],z,.02,c)
# Broad, folded cascade with a bright lip, multiple chutes, and a foamy plunge pool.
for a,b,c in [(-5,-1.6,'#86d5e0'),(-1.6,1.7,'#b5e9ec'),(1.7,5,'#68c9dc')]:
    vs=[(a,32,17),(b,32,17),(b*.85,29,15),(a*.85,29,15),
        (a*1.5,27,7),(b*1.5,27,7),(b*2.1,22,.5),(a*2.1,22,.5)]
    mesh('folded waterfall chute',vs,[(0,1,2,3),(3,2,5,4),(4,5,6,7)],c)
# A broad apron joins the cascade continuously to the lagoon under the rocks.
slab('water beneath gorge rocks',[(-10,26),(10,26),(10,22),(7,19),(-7,19),(-10,22)],.57,.08,'#7dccd8')
finish('SM_Water')
begin()
ell('rounded splash lobe',(0,0,0),(1,1,.85),'#c0ece8',9,5)
finish('SM_WaterSplash')

# Oversized, deliberately simple dung heap for the triceratops paddock.
# Broad overlapping lobes give it a soft, heaped silhouette in the faceted park style.
begin()
ell('dung heap broad base',(0,0,1.1),(4.8,3.7,1.6),'#62412b',12,6)
for p,size,c in [((-2.6,-1.4,1.5),(2.2,2,1.5),'#704a2d'),
                 ((2.5,-1.1,1.3),(2.3,2,1.5),'#795033'),
                 ((.4,.4,2.8),(3.3,2.7,1.7),'#785031'),
                 ((-1,-.2,4),(2,1.9,1.5),'#845b38')]:
    ell('rounded dung mound',p,size,c,11,6)
tube('curled heap crest',[(-1,.1,4.4),(-.7,.2,5.1),(.2,.4,5.7),(.9,.5,5.9)],[1.2,.95,.55,.08],'#8a603c',9)
ell('loose dung clump',(4,-2.7,.4),(.95,.7,.6),'#70472c',9,5)
ell('loose dung clump',(-3.8,2.4,.3),(.8,.65,.5),'#68472e',9,5)
finish('SM_DungPile')

# Assemble the review scene from the same independent assets used by Unreal.
instance('SM_Island');instance('SM_Water');instance('SM_Islets');
instance('SM_BeachBar',(0,131,2),180,2.8)
for j in range(10):
    a=j*math.tau/10
    splash=instance('SM_WaterSplash',(6.8*math.cos(a),21.8+2.5*math.sin(a),.95),j*19,1.35+.3*math.sin(j*2.1))
    base=splash.scale.copy()
    for frame in (1,9,17,25,33,41,49):
        pulse=math.sin((frame-1)*math.tau/48+j*1.7)
        splash.scale=base*(1+.22*pulse);splash.keyframe_insert(data_path='scale',frame=frame)
    splash.scale=base
instance('SM_Rock',(96,-101,-1.5),15,4.2)
instance('SM_VisitorCentre',(0,-51,2),0,1.65);instance('SM_Gate',(0,-105,2),0,1.8);instance('SM_Helipad',(-64,-54,2),0,1.5);instance('SM_Dock',(-13.5,-125,-1),0,1.15)
for h in habitats:
    p=[v/100 for v in h['position']]
    if h['species']=='velociraptor':
        instance('SM_RaptorPen',p,0,1.2)
        for y in (-10.35,10.35):
            for x in (-15.6,-5.2,5.2,15.6):instance('SM_RaptorBeacon',(p[0]+x*1.2,p[1]+y*1.2,p[2]+10.55*1.2),0,1.2)
        for dx,dy,yaw in [(-6,-2,155),(6,3,220),(5,-6,120)]:instance('SM_velociraptor',(p[0]+dx,p[1]+dy,p[2]+.7),yaw,1.1)
    else:
        instance('SM_Enclosure',p,0,1.4)
        instance('SM_'+h['species'],p,155,2.7)
        if h['species']=='triceratops':instance('SM_DungPile',(p[0]+10,p[1]-7,surface(p[0]+10,p[1]-7)),0,1)
# Composed trees frame each focal point. Large crowns are spaced like the reference.
forest=[(78,50),(85,22),(83,-4),(72,-16),(82,-38),(65,-73),(66,-77),
(39,-89),(20,-88),(23,-105),(-23,-102),(-37,-93),(-55,-84),(-73,-78),
(-81,-37),(-88,-14),(-84,31),(-67,44),(-55,68),(-36,84),(34,85),
(31,40),(26,11),(27,-16),(25,-57),(19,-69),(-27,-37),(-27,-11),(-27,29),
(65,55),(-44,-76),(62,-85)]
trees=0
for i,(x,y) in enumerate(forest):
    if any(abs(x-h['position'][0]/100)<25 and abs(y-h['position'][1]/100)<25 for h in habitats):continue
    if i in (3,8,10,15,20,27):name='SM_Palm';scale=rng.uniform(1.6,2.1)
    elif i%5==0:name='SM_TreeBroad';scale=rng.uniform(1.8,2.35)
    elif i%3==0:name='SM_Tree';scale=rng.uniform(.85,1.25)
    else:name='SM_TreeCrooked';scale=rng.uniform(1.45,2.15)
    instance(name,(x,y,surface(x,y)),rng.uniform(0,360),scale);trees+=1
for x,y,rx,ry in islets:
    instance('SM_Palm' if rx<9 else 'SM_Tree',(x-rx*.2,y-ry*.15,2.5),25,1.7);trees+=1
# Ocean is a separate scene object (Unreal supplies its infinite ocean).
begin();box('Ocean',(0,0,-3.8),(2000,2000,1),'#3595bd');parts.clear()
# Lighting/camera for a real isometric art review.
def aim(o,target):o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
camd=bpy.data.cameras.new('Isometric review');cam=bpy.data.objects.new('Isometric review',camd);scene.collection.objects.link(cam);cam.location=(12,-280,235);aim(cam,(0,0,0));camd.type='ORTHO';camd.ortho_scale=285;scene.camera=cam
barcamd=bpy.data.cameras.new('North beach • bar review');barcam=bpy.data.objects.new('North beach • bar review',barcamd);scene.collection.objects.link(barcam)
barcam.location=(29,170,29);aim(barcam,(0,131,7));barcamd.type='ORTHO';barcamd.ortho_scale=43
ld=bpy.data.lights.new('Large softbox sun','AREA');ld.energy=500000;ld.shape='DISK';ld.size=140
lo=bpy.data.objects.new('Large softbox sun',ld);scene.collection.objects.link(lo);lo.location=(100,-130,230);aim(lo,(0,0,0))
sun=bpy.data.lights.new('Warm key','SUN');sun.energy=2;sun.angle=.1;so=bpy.data.objects.new('Warm key',sun);scene.collection.objects.link(so);so.rotation_euler=(.4,.5,.4)
# Match the handedness of the native Unreal camera in side-by-side reviews.
tree=bpy.data.node_groups.new('Park • Reference preview','CompositorNodeTree')
tree.interface.new_socket(name='Image',in_out='OUTPUT',socket_type='NodeSocketColor')
rl=tree.nodes.new('CompositorNodeRLayers');rl.scene=scene
flip=tree.nodes.new('CompositorNodeFlip');flip.inputs['Flip X'].default_value=True
out=tree.nodes.new('NodeGroupOutput')
tree.links.new(rl.outputs['Image'],flip.inputs['Image']);tree.links.new(flip.outputs['Image'],out.inputs['Image'])
scene.compositing_node_group=tree
# Export each source collection, preserving edited components in the blend file.
report=[]
export_names=set(filter(None,os.environ.get('PARK_EXPORT_ASSETS','').split(',')))
for name,col in assets.items():
    report.append({'name':name,'objects':len(col.objects),'polygons':sum(len(o.data.polygons) for o in col.objects)})
    if export_names and name not in export_names:continue
    scene.collection.children.link(col)
    # Bake one export mesh with explicit handedness conversion. Unreal's FBX
    # importer flips Y; pre-reflection preserves layout coordinates in centimetres.
    deps=bpy.context.evaluated_depsgraph_get(); vs=[]; fs=[]; corner_colors=[]
    for o in col.objects:
        ev=o.evaluated_get(deps); m=ev.to_mesh()
        base=len(vs); transform=o.matrix_world
        for vertex in m.vertices:
            v=transform @ vertex.co;vs.append((v.x,-v.y,v.z))
        colors=m.color_attributes.get('Color')
        for face in m.polygons:
            loops=list(reversed(face.loop_indices))
            fs.append(tuple(base+m.loops[k].vertex_index for k in loops))
            corner_colors.extend([tuple(colors.data[k].color) if colors else (1,1,1,1) for k in loops])
        ev.to_mesh_clear()
    export=mesh(name+' • FBX',vs,fs,WHITE)
    attr=export.data.color_attributes.get('Color')
    for i,c in enumerate(corner_colors):attr.data[i].color=c
    bpy.ops.object.select_all(action='DESELECT');export.select_set(True);bpy.context.view_layer.objects.active=export
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,mesh_smooth_type='FACE',colors_type='LINEAR',use_triangles=True)
    bpy.data.objects.remove(export,do_unlink=True);parts.clear()
    scene.collection.children.unlink(col)
(OUT/'park-assets.json').write_text(json.dumps({'version':1,'units':'metres','treeCount':trees,'assets':report,'placements':placements},indent=2)+'\n')
scene.render.filepath=str(ROOT/'assets/blender/park-review.png')
exec(compile((ROOT/'scripts/review_terminal.py').read_text(),str(ROOT/'scripts/review_terminal.py'),'exec'))
# Initialize both view layers before Blender's partial library writer copies them.
bpy.context.window.scene=terminal_review;bpy.context.view_layer.update()
bpy.context.window.scene=scene;bpy.context.view_layer.update()
bpy.data.libraries.write(str(ROOT/'assets/blender/park.blend'),{scene,terminal_review,*assets.values()},fake_user=True)
result={'blend':bpy.data.filepath,'assets':report,'trees':trees,'placements':len(placements)}
