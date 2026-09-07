"""Original park diorama assets. Execute through Blender MCP; source coordinates are metres.
Each asset is a named collection, exported independently with flat normals + vertex colours.
The existing Blender scene is preserved. The assembled scene is saved as park.blend.
"""
import bpy, math, random, json
from pathlib import Path
from mathutils import Vector, Matrix
from mathutils.geometry import delaunay_2d_cdt
ROOT = Path('/Users/schacon/projects/slide-engine')
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
        for k in p.loop_indices: colors.data[k].color=c
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
def text(name,value,p,size,c,rot=(math.pi/2,0,math.pi)):
    curve=bpy.data.curves.new(name,'FONT');curve.body=value;curve.align_x='CENTER';curve.size=size;curve.extrude=.025
    o=bpy.data.objects.new(name,curve);scene.collection.objects.link(o);o.location=p;o.rotation_euler=rot
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.convert(target='MESH')
    o=bpy.context.object; o.data.materials.append(mat)
    colors=o.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
    for d in colors.data:d.color=lin(c)
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

# Architecture kit: chamfered masonry, inset glazing, lintels and stepped roof masses.
begin()
for x,y,w,d,h in [(-6,0,9,8,5),(4,1,9,10,8),(0,7,7,5,4.5)]:
    box('foundation',(x,y,.3),(w+.8,d+.8,.6),'#b8b8a3',.15)
    box('plaster wall',(x,y,h/2+.6),(w,d,h),WHITE,.15)
    box('roof cornice',(x,y,h+.65),(w+.5,d+.5,.45),'#c7c9bd',.12)
    box('raised roof',(x,y,h+1.1),(w*.65,d*.65,.75),'#d9d9cc',.15)
    for wx in (-.28, .28):
        xx=x+w*wx
        box('window recess',(xx,y-d/2-.015,h*.55),(1.65,.12,1.85),DARK)
        box('blue glazing',(xx,y-d/2-.09,h*.55),(1.36,.08,1.52),GLASS)
        box('window sill',(xx,y-d/2-.19,h*.55-.85),(1.9,.35,.16),'#b6b6a6')
        box('mullion',(xx,y-d/2-.15,h*.55),(.1,.12,1.55),WHITE)
    for wy in (-.25,.25):box('side window',(x+w/2+.02,y+wy*d,h*.55),(.1,1.6,1.7),GLASS)
box('entrance canopy',(3,-4.7,3.8),(4,2,.35),'#b7c1b5',.1)
box('front doors',(3,-4.05,1.9),(2.4,.16,3),WOOD)
for x in (1.3,4.7):box('porch pillar',(x,-5.1,1.8),(.27,.27,3.6),WHITE)
for i in range(3):box('entry step',(3,-5.5-i*.4,.3-i*.09),(4.4,1,.18),'#c9c8b4')
text('visitor centre sign','VISITOR CENTRE',(3,-4.14,5.6),.57,DARK)
finish('SM_VisitorCentre')

begin()
for s in (-1,1):
    # Tapered, buttressed gate towers with stone courses.
    tube('tapered gate pier',[(s*9,0,0),(s*9,0,14),(s*9,0,17)],[3.2,2.2,1.7],'#93978a',4)
    box('tower foot',(s*9,0,.55),(6.4,6.4,1.1),'#898e81',.18)
    for z in (4,8,12):box('stone course',(s*9,-2.16,z),(4.4,.14,.14),'#7b8177')
    box('torch cradle',(s*9,0,17.5),(1.1,1.1,.65),DARK,.15)
    tube('outer flame',[(s*9,0,17.8),(s*9+.18,0,18.7),(s*9-.12,0,20)],[.48,.37,0],'#ffa529',7)
    tube('inner flame',[(s*9,-.2,17.9),(s*9,-.2,18.9)],[.25,0],'#ffe18a',6)
for s in (-1,1):
    box('wooden gate leaf',(s*3.5,0,5.3),(6.8,1,10.6),'#875a34',.1)
    for j in range(7):box('individual plank',(s*3.5-2.8+j*.92,-.56,5.3),(.76,.12,10.3),tone('#a77843',.85+.04*(j%3)),.04)
    for z in (1,9.5):box('gate crossbar',(s*3.5,-.75,z),(6.5,.3,.5),'#6e4c31')
    beam('diagonal brace',(s*.5,-.9,1.1),(s*6.5,-.9,9.2),.23,'#654a33',4)
box('arched sign lintel',(0,0,12.8),(14.5,1.6,3.6),'#d5c58f',.4)
text('park lettering','JURASSIC\nPARK',(0,-.85,13.05),1.38,'#db6329')
for i in range(4):box('causeway step',(0,-4-i*1.3,.3-i*.14),(15,1.5,.3),'#c5c0a7')
finish('SM_Gate')

begin()
poly=[(8*math.cos(j*math.pi/4),8*math.sin(j*math.pi/4)) for j in range(8)]
slab('octagonal landing foundation',poly,.55,.65,'#c5c8b9')
slab('tarmac inset',[(x*.88,y*.88) for x,y in poly],.6,.1,'#777e78')
for x in (-2.2,2.2):box('H vertical',(x,0,.67),(1.15,6,.1),WHITE)
box('H bridge',(0,0,.67),(4.8,1.15,.1),WHITE)
for j in range(8):a=j*math.pi/4;ell('perimeter lamp',(7.5*math.cos(a),7.5*math.sin(a),.72),(.18,.18,.16),'#f4dc8c',6,3)
finish('SM_Helipad')

begin()
for y in range(0,27):box('dock plank',(0,-y,.15),(4,.85,.3),tone('#ab8054',.85+.055*(y%4)),.035)
for x in range(-12,1):box('berth plank',(x,-23,.15),(.85,5,.3),tone('#ab8054',.9+.03*(x%3)),.03)
for y in (0,-8,-16,-25):
    for x in (-2.2,2.2):box('mooring piling',(x,y,-.2),(.6,.6,3.7),WOOD,.09);box('piling cap',(x,y,1.72),(.75,.75,.24),'#cfb185',.04)
# Hull loft along X with upswept bow, contrasting waterline, cabin and windscreen.
tube('boat hull',[(-12,-28,.5),(-10.5,-28,-.2),(-5,-28,-.2),(-3,-28,.7)],[(.3,.35),(.8,1.8),(.9,1.8),(.12,.4)],WHITE,8)
box('red gunwale',(-7.5,-28,.65),(7.8,3.65,.32),'#ab5239',.15)
box('open deck',(-7.5,-28,.9),(7.4,3.3,.25),WHITE,.12)
box('cabin',(-7,-28,1.75),(3.8,2.8,1.7),'#d3ded7',.3)
box('cabin glass',(-7,-29.42,1.95),(2.9,.08,.95),GLASS,.08)
box('cabin roof',(-7,-28,2.7),(4.1,3.1,.25),WHITE,.12)
beam('radio mast',(-8,-28,2.8),(-8,-28,4.1),.035,DARK)
finish('SM_Dock')

# Shared enclosure: square with rounded corners, caps, twin rails and a service gateway.
begin()
points=[]
for cx,cy,start in [(10,10,0),(-10,10,90),(-10,-10,180),(10,-10,270)]:
    for j in range(7):a=math.radians(start+j*15);points.append((cx+4*math.cos(a),cy+4*math.sin(a),0))
for i,p in enumerate(points):
    q=points[(i+1)%len(points)]
    box('concrete fence post',(p[0],p[1],1.65),(.48,.48,3.3),WHITE,.08)
    box('post cap',(p[0],p[1],3.34),(.65,.65,.22),'#f0efd9',.08)
    for z in (1.2,2.65):beam('paddock rail',(p[0],p[1],z),(q[0],q[1],z),.12,'#d6ddc6',4)
for x in (-1.5,1.5):box('gate pier',(x,-14,1.85),(.9,.9,3.7),'#879388',.09)
box('service gate',(0,-14,1.5),(2.7,.35,3),'#967247',.08)
finish('SM_Enclosure')

# Reusable foliage: branching broadleaf tree, sculpted palms and angular boulders.
begin()
tube('branching trunk',[(0,0,0),(.15,0,2),(-.05,0,4.7)],[.45,.32,.13],WOOD,7)
for p in [(-1.1,0,3.8),(1.1,.2,4.4),(.1,1,4.5)]:beam('branch',(0,0,2.5),p,.16,WOOD)
ell('main crown',(0,0,5.2),(2.4,2.1,2.55),'#5c993c',7,4)
ell('side crown',(-1.3,-.3,4.3),(1.5,1.5,1.6),'#78aa45',7,4)
finish('SM_Tree')
begin()
tube('curved palm trunk',[(0,0,0),(.25,0,2),(.6,.1,4),(.45,.2,6.4)],[.3,.25,.19,.13],'#9b7e49',7)
for j in range(9):
    a=j*2*math.pi/9; v=Vector((math.cos(a),math.sin(a),0)); w=Vector((-math.sin(a),math.cos(a),0));base=Vector((.45,.2,6.3))
    vs=[base,base+v*1.4+Vector((0,0,.75))+w*.48,base+v*3.1+Vector((0,0,-.7)),base+v*1.4+Vector((0,0,.75))-w*.48,base+v*1.55+Vector((0,0,.95))]
    mesh('folded palm frond',vs,[(0,1,4),(1,2,4),(2,3,4),(3,0,4)],'#4d994b',[tone('#4d994b',t) for t in (1.15,1,.85,.92)])
for a in (0,2,4):ell('coconut',(.45+.23*math.cos(a),.2+.23*math.sin(a),6),(.21,.21,.25),WOOD,6,3)
finish('SM_Palm')
begin();ell('weathered boulder',(0,0,.75),(1.6,1.3,1.7),'#939389',7,3);finish('SM_Rock')

# Island: hand-shaped shoreline rings, triangulated meadow, a broad ridge volcano and lagoon.
layout=json.loads((ROOT/'examples/git-meta/layout.json').read_text())
habitats=layout['habitats']
coast=[]
for j in range(64):
    a=j*2*math.pi/64;f=1+.055*math.sin(a*5)+.025*math.sin(a*9+.7)
    coast.append((105*math.cos(a)*f,128*math.sin(a)*f))
def ground(x,y):
    # Flat paddocks stay flush; softly faceted meadow is only 2m high.
    return 2+.4*math.sin(x*.065)*math.sin(y*.08)
begin()
for factor,z,c in [(1,-.8,'#efd28b'),(.953,1.15,'#f7df99'),(.9,2,'#b1c65e')]:
    if factor==1:prev=[(x,y,-1.8) for x,y in coast];fac=1.035
    else:prev=ring
    ring=[(x*factor,y*factor,z) for x,y in coast]
    vs=prev+ring;fs=[(j,(j+1)%64,(j+1)%64+64,j+64) for j in range(64)]
    mesh('sculpted beach contour',vs,fs,c)
coords=[Vector((x*.9,y*.9)) for x,y in coast]
for y in range(-112,116,9):
    for x in range(-95,99,9):
        xx=x+rng.uniform(-2,2);yy=y+rng.uniform(-2,2)
        if (xx/90)**2+(yy/113)**2<1:coords.append(Vector((xx,yy)))
v,e,f,*_=delaunay_2d_cdt(coords,[],[list(range(64))],1,.001)
verts=[]
for x,y in v:
    z=ground(x,y);lake=((x/17)**2+((y-9)/18)**2)
    if lake<1.1:z=-1
    verts.append((x,y,z))
cs=[]
for face in f:
    x=sum(v[k].x for k in face)/len(face);y=sum(v[k].y for k in face)/len(face)
    cs.append(tone('#a6be58',rng.uniform(.93,1.045)))
mesh('triangulated meadow',verts,f,'#a6be58',cs)
# A deliberately asymmetrical volcano with alternating radial ridges.
vs=[];n=13
for k,(r,z) in enumerate([(49,2),(32,21),(19,44),(11,58)]):
    for j in range(n):
        a=2*math.pi*j/n;rr=r*(1+(.13 if j%2 else -.06))
        vs.append((rr*math.cos(a)-4,70+rr*math.sin(a)*.95,z+(rng.uniform(-2,2) if k<3 else 0)))
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
for x,y,z,s in [(-8,27,6,(5,4,7)),(-4,30,10,(4,4,9)),(5,29,9,(4,4,9)),(9,26,5,(4,4,6))]:ell('gorge rock',(x,y,z),s,'#96958a',7,3)
# Meandering cream paths route around the mountain instead of climbing its faces.
def road(points,width=1.5):
    for a,b in zip(points,points[1:]):
        d=Vector((b[0]-a[0],b[1]-a[1],0));nrm=Vector((-d.y,d.x,0)).normalized()*width/2
        aa=Vector((a[0],a[1],ground(*a)+.06));bb=Vector((b[0],b[1],ground(*b)+.06))
        mesh('park trail',[aa+nrm,aa-nrm,bb-nrm,bb+nrm],[(0,1,2,3)],'#e5dbac')
road([(0,-111),(0,-90),(5,-72),(0,-55),(21,-34),(24,-7),(27,20),(37,38),(41,68)],2)
road([(0,-90),(-26,-75),(-32,-51),(-31,-19),(-27,8),(-33,40),(-39,69)],1.8)
for h in habitats:
    x,y,z=[p/100 for p in h['position']]; road([(x-math.copysign(14,x),y),(math.copysign(33 if y>30 else 27,x),y)])
road([(5,-72),(-42,-64),(-71,-63)],2)
finish('SM_Island')
begin()
for factor,z,c in [(1.13,-2.5,'#459fbf'),(1.08,-2,'#55b4cb'),(1.035,-1.5,'#7dd1d5')]:slab('shallow water band',[(x*factor,y*factor) for x,y in coast],z,.2,c)
poly=[(17*math.cos(j*2*math.pi/25)*(1+.05*math.sin(j*3)),9+18*math.sin(j*2*math.pi/25)) for j in range(25)]
slab('turquoise lagoon',poly,.6,.1,'#45adca')
for offset,c in [(0,'#79d2df'),(1,'#a6e5e7')]:
    mesh('falling water ribbon',[(-2+offset,28,17),(2+offset*.5,28,17),(3-offset*.4,23,.8),(-3+offset,23,.8)],[(0,1,2,3)],c)
for j in range(8):ell('waterfall foam',(-3+j*.85,22,.8),(.8,1.3,.2),'#b5e7e2',7,3)
finish('SM_Water')

# Assemble the review scene from the same independent assets used by Unreal.
instance('SM_Island');instance('SM_Water')
instance('SM_VisitorCentre',(0,-56,2));instance('SM_Gate',(0,-110,2));instance('SM_Helipad',(-71,-63,2));instance('SM_Dock',(-12,-128,-1))
for h in habitats:
    p=[v/100 for v in h['position']];instance('SM_Enclosure',p)
    instance('SM_'+h['species'],p,-25,1.75)
# Sparse groves frame the landmarks; trees never intersect fences or main roads.
trees=0
for i in range(330):
    x=rng.uniform(-94,94);y=rng.uniform(-114,116)
    if (x/94)**2+(y/115)**2>.9 or math.hypot(x+4,y-70)<44 or math.hypot(x,y-9)<23:continue
    if any(math.hypot(x-h['position'][0]/100,y-h['position'][1]/100)<20 for h in habitats):continue
    if abs(x)<20 and y<0:continue
    if -84<x<-55 and -78<y<-48:continue
    if any(abs(x-s)<5 for s in (-31,27,33,41)):continue
    instance('SM_Palm' if i%5==0 else 'SM_Tree',(x,y,ground(x,y)),rng.uniform(0,360),rng.uniform(.9,1.5));trees+=1
# Tiny rocky offshore islet.
instance('SM_Rock',(75,75,-.7),0,5);instance('SM_Tree',(75,75,5),25,1.1)
# Ocean is a separate scene object (Unreal supplies its infinite ocean).
begin();box('Ocean',(0,0,-3.8),(2000,2000,1),'#3595bd');parts.clear()
# Lighting/camera for a real isometric art review.
def aim(o,target):o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
camd=bpy.data.cameras.new('Isometric review');cam=bpy.data.objects.new('Isometric review',camd);scene.collection.objects.link(cam);cam.location=(25,-280,240);aim(cam,(0,0,0));camd.type='ORTHO';camd.ortho_scale=290;scene.camera=cam
ld=bpy.data.lights.new('Large softbox sun','AREA');ld.energy=1600000;ld.shape='DISK';ld.size=140
lo=bpy.data.objects.new('Large softbox sun',ld);scene.collection.objects.link(lo);lo.location=(-100,-130,230);aim(lo,(0,0,0))
sun=bpy.data.lights.new('Warm key','SUN');sun.energy=2;sun.angle=.1;so=bpy.data.objects.new('Warm key',sun);scene.collection.objects.link(so);so.rotation_euler=(.4,-.5,-.4)
# Export each source collection, preserving edited components in the blend file.
report=[]
for name,col in assets.items():
    scene.collection.children.link(col)
    bpy.ops.object.select_all(action='DESELECT')
    for o in col.objects:o.select_set(True)
    bpy.context.view_layer.objects.active=next(iter(col.objects))
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,mesh_smooth_type='FACE',colors_type='SRGB',use_triangles=True)
    report.append({'name':name,'objects':len(col.objects),'polygons':sum(len(o.data.polygons) for o in col.objects)})
    scene.collection.children.unlink(col)
(OUT/'park-assets.json').write_text(json.dumps({'version':1,'units':'metres','treeCount':trees+1,'assets':report,'placements':placements},indent=2)+'\n')
scene.render.filepath=str(ROOT/'assets/blender/park-review.png')
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'assets/blender/park.blend'))
result={'blend':bpy.data.filepath,'assets':report,'trees':trees+1,'placements':len(placements)}
