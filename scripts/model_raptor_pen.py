"""Compact raptor holding pen; called by build_park_assets.py with its mesh helpers.
Eight shared corner/intermediate buttresses: four on each long side, two on each
short side. Source units are metres; native placement uses scale 1.2.
"""
begin()
CONCRETE='#b8b9ad'; CAP='#d0d0c3'; STEEL='#45565d'; WIRE='#aebbb3'
# A contained grass yard on a shallow concrete foundation.
box('pen foundation',(0,0,.22),(33,24,.44),'#9a9e91')
box('enclosed grass court',(0,0,.52),(29.8,20.8,.25),'#91b451')
for x in (-15.6,15.6):
    box('solid short perimeter wall',(x,0,2.3),(1.25,23,4.15),CONCRETE)
    box('short wall coping',(x,0,4.45),(1.6,23.4,.35),CAP)
box('rear concrete perimeter',(0,11,2.3),(31.2,1.25,4.15),CONCRETE)
box('rear coping',(0,11,4.45),(32,1.6,.35),CAP)
# The actual entry opening faces the park path (south).
for x in (-9.2,9.2):
    box('front gate wall',(x,-11,2.3),(12.8,1.25,4.15),CONCRETE)
    box('front coping',(x,-11,4.45),(13,1.6,.35),CAP)
box('steel containment gate',(0,-11.18,2.35),(5.45,.45,4.2),'#536269')
for x in (-2.8,2.8):box('gate concrete jamb',(x,-11,2.4),(.6,1.55,4.6),'#989e95')
for z in (.6,2.25,4.15):box('gate reinforcing bar',(0,-11.47,z),(5.25,.18,.24),STEEL)
box('gate center seam',(0,-11.45,2.35),(.13,.18,3.95),STEEL)
box('gate latch',(1.4,-11.64,2.25),(.65,.18,.3),'#c4bc83')
# Chunky inward-leaning piers. Corners are shared: 4 long / 2 short.
for y in (-11,11):
    for x in (-15.6,-5.2,5.2,15.6):
        topy=y-math.copysign(.65,y)
        mesh('tapered electric fence buttress',
             [(x+a*.85,y+b*.8,4.5) for a,b in [(-1,-1),(1,-1),(1,1),(-1,1)]]+
             [(x+a*.42,topy+b*.43,10.4) for a,b in [(-1,-1),(1,-1),(1,1),(-1,1)]],
             [(0,3,2,1),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7),(4,5,6,7)],STEEL)
        box('post cap',(x,topy,10.42),(1,.98,.18),'#69767a')
for z in (5.8,7.6,9.4):
    inset=(z-4.5)/5.9*.65
    for y in (-11,11):beam('long electric rail',(-15.6,y-math.copysign(inset,y),z),(15.6,y-math.copysign(inset,y),z),.09,WIRE,5)
    for x in (-15.6,15.6):beam('short electric rail',(x,-11+inset,z),(x,11-inset,z),.09,WIRE,5)
# Restrained masonry patches, rather than individual brickwork.
for x,z,w in [(-12,1.1,1.5),(-7,2.6,2),(7,1.1,1.4),(12,3,1.8)]:
    box('cast concrete patch',(x,-11.64,z),(w,.035,.65),'#a0a79f')
# A narrow front service walk with one simple flight of steps at the entry.
box('service walkway',(-8.8,-13,.75),(13.3,2.4,.7),'#a5a798')
for x in (-15,-11,-7,-3):box('walk railing post',(x,-14,1.95),(.12,.12,2),STEEL)
for z in (1.5,2.65):beam('walk handrail',(-15,-14,z),(-3,-14,z),.07,STEEL,4)
for j in range(3):box('entry step',(0,-14.5-j*.8,.6-j*.15),(5.8,.85,.3),CAP)
box('path apron',(0,-16.6,.13),(6,2.5,.26),'#d2c8a0')
# Modest watchtower at the back corner, with an open lookout and hipped roof.
tx,ty=-11,15.4
box('watchtower concrete shaft',(tx,ty,6.2),(6.8,6.8,12.4),CONCRETE)
box('watchtower upper ledge',(tx,ty,12.4),(7.8,7.8,.65),CAP)
box('watchtower doorway',(tx,ty-3.43,2.2),(2.2,.1,4.1),STEEL)
for z in (6.4,9.4):
    box('tower front slit',(tx,ty-3.44,z),(1.45,.08,1.05),'#627b80')
    box('tower side slit',(tx+3.44,ty,z),(.08,1.45,1.05),'#627b80')
box('lookout dark cabin',(tx,ty+1.0,14.25),(4.7,3.8,3.3),'#3f5965')
for x in (tx-3.25,tx+3.25):
    for y in (ty-3.25,ty+3.25):box('lookout upright',(x,y,14.6),(.3,.3,4.5),STEEL)
for z in (13.1,14.1):
    for y in (ty-3.3,ty+3.3):beam('balcony horizontal',(tx-3.3,y,z),(tx+3.3,y,z),.075,STEEL,4)
    for x in (tx-3.3,tx+3.3):beam('balcony short rail',(x,ty-3.3,z),(x,ty+3.3,z),.075,STEEL,4)
mesh('simple hipped watchtower roof',[(tx-4.4,ty-4.4,16.9),(tx+4.4,ty-4.4,16.9),(tx+4.4,ty+4.4,16.9),(tx-4.4,ty+4.4,16.9),(tx,ty,19.3)],[(0,1,4),(1,2,4),(2,3,4),(3,0,4),(3,2,1,0)],'#8c939f')
for x in (tx-3.25,tx+3.25):box('amber tower beacon',(x,ty-3.3,16.2),(.35,.4,.65),'#e7a03a')
# Compact gatehouse at the opposite front corner.
box('gatehouse',(12.3,-12.2,3.5),(5,5.2,7),CONCRETE)
box('gatehouse flat roof',(12.3,-12.2,7.1),(5.7,5.9,.4),CAP)
box('service door',(12.3,-14.85,2.1),(1.9,.1,3.9),'#929b94')
box('gatehouse high window',(12.3,-14.86,5.5),(2.6,.08,.95),'#58747b')
box('gatehouse side window',(14.86,-12.2,5.5),(.08,2.5,.95),'#58747b')
# Three small folded-leaf plants inside the pen.
for x,y in [(-10,5),(10,6),(-9,-6)]:
    tube('fern trunk',[(x,y,.7),(x,y,1.65)],[.25,.16],'#7c7650',6)
    for j in range(5):
        a=j*math.tau/5;v=Vector((math.cos(a),math.sin(a),0));w=Vector((-v.y,v.x,0));b=Vector((x,y,1.5))
        mesh('broad fern leaf',[b,b+v*1.4+w*.55+Vector((0,0,1)),b+v*3+Vector((0,0,.3)),b+v*1.4-w*.55+Vector((0,0,1)),b+v*1.45+Vector((0,0,1.35))],[(0,1,4),(1,2,4),(2,3,4),(3,0,4)],'#689840')
finish('SM_RaptorPen')

begin()
tube('warning light pedestal',[(0,0,0),(0,0,.16)],[.38,.38],'#35454b',8)
tube('beacon red lens',[(0,0,.17),(0,0,.7)],[.29,.23],'#fa291b',8)
finish('SM_RaptorBeacon')
