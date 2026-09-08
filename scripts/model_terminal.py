"""Chunky 1990s workstation for the first area's interactive command demo."""
begin()
CASE='#b6b4ab'; SIDE='#969890'; BEZEL='#c3c0b5'; BLACK='#303b3e'; KEY='#c2c3b9'
# Positive source X appears on the left in the native front view.
cx=.65
box('monitor weighted foot',(cx,.22,.09),(1.7,1.05,.18),SIDE,.035)
box('monitor pedestal',(cx,.2,.29),(1.1,.64,.3),SIDE,.045)
crt_start=len(parts)
# A tapered CRT housing, not a flat display.
mesh('deep tapered CRT case',[(cx+x,y,z) for x,y,z in [(-1.1,-.42,.5),(1.1,-.42,.5),(1.1,-.42,2.28),(-1.1,-.42,2.28),(-.85,.9,.68),(.85,.9,.68),(.85,.9,2.1),(-.85,.9,2.1)]],[(0,4,5,1),(1,5,6,2),(2,6,7,3),(3,7,4,0),(4,7,6,5)],CASE)
for x in (cx-1.02,cx+1.02):box('thick CRT side bezel',(x,-.46,1.39),(.17,.2,1.8),BEZEL,.025)
box('thick top bezel',(cx,-.46,2.18),(2.1,.2,.22),BEZEL,.025)
box('deep lower bezel',(cx,-.46,.66),(2.1,.2,.34),BEZEL,.025)
box('recessed black screen surround',(cx,-.435,1.44),(1.84,.07,1.33),BLACK,.02)
box('blue CRT glass',(cx,-.48,1.445),(1.7,.018,1.16),'#1830a5',.016)
# Native animated widget overlays this rectangle: centre (.65,-.508,1.445).
for j in range(4):box('monitor square control',(cx-.14+j*.16,-.573,.615),(.08,.036,.073),SIDE,.01)
box('monitor green power LED',(cx-.84,-.578,.62),(.055,.025,.055),'#8ded49')
for y in (.1,.22,.34,.46,.58,.7):
    box('monitor side vent',(cx+1.05-(y+.42)*.19,y,1.52),(.018,.045,.46),'#737a76')
# A slightly crooked paper reminder, with its lower edge curled off the glass.
nx,nz=cx-.73,.92
note_start=len(parts)
note=mesh('yellow sticky note',[(nx-.19,-.6,nz+.2),(nx+.19,-.6,nz+.2),(nx+.2,-.62,nz-.12),(nx+.14,-.665,nz-.2),(nx-.17,-.65,nz-.2),(nx-.19,-.62,nz-.12)],[(0,1,2,5),(5,2,3,4)],'#f7dc6b',['#f7dc6b','#e4c14f'])
note_font='/System/Library/Fonts/Supplemental/Comic Sans MS.ttf'
text('sticky note pencil message','magic\nword?',(nx,-.625,nz+.085),.069,'#675936',depth=.0007,font=note_font if Path(note_font).exists() else None)
tilt=Matrix.Translation((nx,-.62,nz))@Matrix.Rotation(math.radians(7),4,'Y')@Matrix.Translation((-nx,.62,-nz))
for o in parts[note_start:]:o.matrix_world=tilt@o.matrix_world
# Enlarge the CRT 15% around its stand, leaving the tower and keyboard intact.
crt_transform=Matrix.Translation((cx,0,.5))@Matrix.Scale(1.15,4)@Matrix.Translation((-cx,0,-.5))
for o in parts[crt_start:]:o.matrix_world=crt_transform@o.matrix_world
# Tower with deep vertical vents, disc slot, buttons and a lit status lamp.
x=-1.15
box('tower chassis',(x,.22,1.13),(.92,1.43,2.2),CASE,.055)
box('tower side inset',(x-.476,.25,1.17),(.02,1.22,1.95),SIDE,.012)
for j in range(12):box('vertical tower vent',(x-.35+j*.056,-.508,1.17),(.021,.02,1.84),'#666d69')
box('front tower panel',(x+.2,-.529,1.16),(.32,.07,2.03),SIDE,.02)
box('tower disk slot',(x+.2,-.57,.43),(.055,.016,.39),'#374247')
for z in (1.74,1.91):box('tower square switch',(x+.2,-.581,z),(.095,.035,.095),'#cecdc1',.012)
box('tower green status lamp',(x+.22,-.586,1.52),(.075,.02,.06),'#8fea4c')
for x2 in (x-.31,x+.31):box('tower rubber foot',(x2,.05,.035),(.15,.8,.07),'#656961')
# Sloping keyboard slab with separate keys, arrows and numeric keypad.
box('keyboard base',(.48,-1.22,.12),(2.65,.83,.23),CASE,.055)
box('recessed keyboard key bed',(.48,-1.22,.245),(2.42,.65,.027),'#616b6b')
for row in range(5):
    for col in range(12):
        if row==4 and 2<=col<=7:continue
        x2=-.08+col*.143;y=-1.49+row*.132
        box('individual bevelled key',(x2,y,.285+row*.009),(.123,.109,.082),tone(KEY,1-.035*((col+row)%3)),.014)
box('long spacebar',(.56,-1.49,.285),(.84,.109,.082),KEY,.014)
for row in range(4):
    for col in range(3):box('numeric keypad key',(-.62+col*.14,-1.44+row*.15,.3),(.12,.125,.09),KEY,.014)
for x2,y in [(-.16,-1.49),(-.3,-1.49),(-.16,-1.34)]:box('arrow cluster key',(x2,y,.3),(.12,.12,.09),KEY,.014)
# Mouse with two buttons and gently bent polygonal cables.
box('faceted mouse',(-1.3,-1.24,.135),(.46,.64,.25),CASE,.09)
for x2 in (-1.415,-1.185):box('separate mouse button',(x2,-1.05,.26),(.205,.235,.04),BEZEL,.025)
tube('mouse cable',[(-1.3,-.96,.1),(-1.45,-.7,.055),(-.95,-.53,.05),(-.5,-.73,.05)],[.023]*4,'#5c6561',6)
tube('keyboard cable',[(1.35,-.81,.07),(1.95,-.55,.05),(2.02,.08,.05),(1.37,.36,.1)],[.025]*4,'#5c6561',6)
finish('SM_Terminal')
