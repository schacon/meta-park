"""Secluded north-beach bar; editable painted meshes using the park kit helpers."""
bar_rng=random.Random(25508)
begin()
TIMBER='#8b653d'; EDGE='#bc9256'; STRAW='#d6ad60'; CREAM='#f4e7c2'; SKIN='#deb184'
# Timber counter, individual boards, shelves, and an open serving window.
box('bar low platform',(0,1.6,.1),(6.4,3.4,.2),TIMBER,.04)
for j in range(13):box('bar front vertical plank',(-2.9+j*.48,.12,.84),(.46,.16,1.4),tone(TIMBER,.9+.07*(j%3)),.015)
for x in (-2.95,2.95):
    box('bar side panel',(x,1.5,.84),(.16,2.8,1.4),TIMBER)
    for y in (.15,2.95):box('square roof pillar',(x,y,1.55),(.22,.22,3.1),TIMBER,.02)
box('wide polished counter',(0,-.06,1.62),(6.65,.85,.18),EDGE,.035)
box('rear shelf backing',(0,2.88,1.75),(5.9,.13,1.9),'#594630')
for z in (1.1,1.9,2.65):box('bottle shelf',(0,2.65,z),(5.8,.5,.12),EDGE,.015)
box('upper serving beam',(0,.14,3.12),(6.2,.22,.25),TIMBER)
for x in (-1.85,0,1.85):
    box('square bar stool seat',(x,-.85,.96),(.65,.65,.13),EDGE,.02)
    for dx in (-.23,.23):
        for dy in (-.23,.23):beam('stool leg',(x+dx,-.85+dy,.05),(x+dx*.8,-.85+dy*.8,.9),.075,TIMBER,4)
    for y in (-1.08,-.62):beam('stool cross rung',(x-.23,y,.36),(x+.23,y,.36),.04,TIMBER,4)
# A continuous roof with irregular, overlapping sculpted thatch leaves.
roof=[(3.55*math.cos(j*math.tau/12),1.5+2.25*math.sin(j*math.tau/12),3.15) for j in range(12)]
mesh('thatch roof underlayer',roof+[(0,1.5,4.25)],[(j,(j+1)%12,12) for j in range(12)],'#b78d4e')
for ring,(radius,inner,width,count) in enumerate([(3.65,2.25,.45,38),(2.75,1.2,.43,30),(1.65,.12,.38,22)]):
    for j in range(count):
        a=j*math.tau/count+ring*.13;v=Vector((math.cos(a),math.sin(a)*.64,0));w=Vector((-math.sin(a),math.cos(a),0))
        r=radius+bar_rng.uniform(-.15,.12);b=Vector((0,1.5,4.34))
        def p(t,s=0,h=0):return b+v*t+w*s+Vector((0,0,-t*.32+h))
        mesh('overlapping pointed thatch', [p(inner,-width*.45),p(inner,width*.45),p(r-.35,width*.65),p(r,0,-.08),p(r-.35,-width*.65),p((inner+r)*.5,0,.13)],
            [(0,1,5),(1,2,5),(2,3,5),(3,4,5),(4,0,5)],tone(STRAW,bar_rng.uniform(.88,1.12)))
# Faceted bottles with paper labels, clearly distinct from the timber.
def bottle(x,y,z,c):
    tube('glass bottle',[(x,y,z),(x,y,z+.32),(x,y,z+.41),(x,y,z+.57)],[.11,.11,.05,.05],c,8)
    tube('bottle neck cap',[(x,y,z+.56),(x,y,z+.62)],[.055,.055],'#a58c50',8)
    box('bottle paper label',(x,y-.105,z+.2),(.15,.014,.19),'#d6c99a')
for j in range(9):bottle(-2.5+j*.61,2.54,1.97,['#667b32','#a47a43','#d6c4a3'][j%3])
for x,c in [(-2.15,'#687d32'),(-1.55,'#949640'),(.4,'#b39a67')]:bottle(x,-.04,1.72,c)
# Round planked table and a solid crossed trestle.
tube('round table edge',[(0,-2.7,1.18),(0,-2.7,1.34)],[1.5,1.5],EDGE,16)
for j in range(-3,4):
    x=j*.39;length=2*math.sqrt(max(0,1.45**2-x*x))
    box('table plank seam',(x,-2.7,1.345),(.014,length,.009),'#a47b43')
for x in (-.75,.75):
    beam('table crossed leg',(x,-3.55,.05),(-x,-2,1.17),.13,TIMBER,4)
beam('table lower brace',(-.73,-2.73,.36),(.73,-2.73,.36),.1,TIMBER,4)
# Two simple slatted chairs, angled toward the table.
def chair(cx,cy,yaw):
    first=len(parts)
    box('chair seat',(0,0,.8),(.9,.86,.13),EDGE,.025)
    for x in (-.35,.35):
        for y in (-.32,.32):box('chair leg',(x,y,.4),(.12,.12,.8),TIMBER)
        box('chair tall back upright',(x,.35,1.18),(.13,.13,1.5),TIMBER)
    for z in (1.1,1.42,1.74):box('chair back slat',(0,.36,z),(.82,.1,.18),EDGE)
    tr=Matrix.Translation((cx,cy,0))@Matrix.Rotation(math.radians(yaw),4,'Z')
    for o in parts[first:]:o.matrix_world=tr@o.matrix_world
chair(-1.8,-2.5,25);chair(1.8,-2.5,-25)
# Seated character sculptures: separate clothing, limbs, facial planes and glasses.
def person(cx,cy,yaw,floral):
    first=len(parts);shirt='#659b9c' if floral else '#a64c39';trousers='#6990a0' if floral else '#c4b48e'
    width=.58 if floral else .47
    tube('floral shirt torso' if floral else 'red polo torso',[(0,0,.91),(0,0,1.22),(0,.02,1.68)],[(width*.86,.3),(width,.38),(width*.83,.28)],shirt,10)
    tube('neck',[(0,0,1.67),(0,0,1.87)],[.15,.16],SKIN,8)
    ell('angular jaw and cheeks',(0,-.015,2.02),(.29,.24,.36),SKIN,10,5)
    ell('hair cap',(0,.025,2.24),(.295,.245,.19),'#51432e',9,3)
    for x in (-.25,.25):ell('ear',(x,-.015,2.04),(.06,.06,.095),SKIN,7,3)
    mesh('projecting angular nose',[(-.07,-.23,2.1),(.07,-.23,2.1),(.06,-.31,1.99),(-.06,-.31,1.99),(0,-.37,2.015)],[(0,1,4),(1,2,4),(2,3,4),(3,0,4)],tone(SKIN,.95))
    for x in (-.13,.13):
        box('dark glasses frame',(x,-.245,2.1),(.225,.045,.145),'#302e28',.015)
        if floral:
            box('clear spectacle lens',(x,-.271,2.1),(.168,.008,.095),'#c0bb9b')
            box('eye in glasses',(x,-.279,2.11),(.038,.005,.038),'#322d25')
    beam('glasses bridge',(-.045,-.273,2.1),(.045,-.273,2.1),.016,'#302e28',4)
    for x in (-.25,.25):beam('glasses temple',(x,-.23,2.1),(x,.12,2.12),.019,'#302e28',4)
    box('mouth',(0,-.244,1.88 if floral else 1.91),(.14,.022,.085 if floral else .02),'#623e2e')
    if floral:box('upper teeth',(0,-.26,1.91),(.11,.014,.025),'#f0dbb6')
    # Open collars and buttons are individual painted facets.
    for side in (-1,1):
        mesh('shirt folded collar',[(side*.04,-.295,1.71),(side*.24,-.24,1.64),(side*.12,-.34,1.47)],[(0,1,2)],tone(shirt,.8))
    for z in (1.15,1.32,1.5):ell('shirt button',(.02,-.38,z),(.023,.014,.023),'#dccbac',6,3)
    for side in (-1,1):
        x=side*.27
        tube('seated thigh',[(x,0,.88),(x,-.48,.83)],[.25,.23],trousers,8)
        if floral:
            tube('bare shin',[(x,-.48,.7),(x,-.56,.27)],[.16,.12],SKIN,8)
            tube('white sock',[(x,-.56,.16),(x,-.56,.36)],[.13,.13],'#ded7bb',8)
        else:tube('bent trouser leg',[(x,-.48,.75),(x,-.56,.15)],[.19,.13],trousers,8)
        box('cream sneaker' if floral else 'brown loafer',(x,-.69,.12),(.33,.49,.22),'#d7cfb4' if floral else '#62472f',.045)
        if floral:
            box('rubber sneaker sole',(x,-.69,.035),(.35,.51,.065),'#e5dec7')
            for y in (-.75,-.67,-.59):beam('shoelace',(x-.09,y,.24),(x+.09,y,.24),.012,'#f3e9ce',4)
    # One relaxed arm on the tabletop; the floral-shirt visitor points left.
    for side in (-1,1):
        shoulder=(side*width*.9,0,1.58)
        if floral and side==-1:
            elbow=(-.93,-.13,1.79);wrist=(-1.45,-.24,1.95)
            sleeve=(-.7,-.06,1.68)
        else:
            elbow=(side*(width+.12),-.34,1.21);wrist=(side*.26,-.8,1.33)
            sleeve=(side*(width+.08),-.16,1.4)
        tube('short shirt sleeve',[shoulder,sleeve],[.24,.2],shirt,8)
        tube('articulated bare arm',[sleeve,elbow,wrist],[.145,.14,.105],SKIN,8)
        ell('resting hand' if not(floral and side==-1) else 'pointing hand',wrist,(.14,.15,.1),SKIN,8,3)
        if floral and side==-1:
            tube('extended pointing finger',[wrist,(-1.75,-.27,2.02)],[.055,.04],SKIN,6)
            tube('raised pointing thumb',[(-1.42,-.23,1.99),(-1.48,-.21,2.13)],[.048,.035],SKIN,6)
    if floral:
        # Scatter five-petal flowers over the front and visible sides of the shirt.
        for j in range(32):
            a=bar_rng.uniform(math.pi,math.tau);z=bar_rng.uniform(1.02,1.62)
            x=width*math.cos(a);y=.38*math.sin(a)-.008
            col=['#dfb0bf','#e4dec3','#91bda7'][j%3]
            for k in range(5):
                b=k*math.tau/5
                ell('Hawaiian shirt flower petal',(x+.045*math.cos(b),y-.01,z+.045*math.sin(b)),(.037,.019,.042),col,5,3)
            ell('flower center',(x,y-.034,z),(.023,.012,.023),'#d1b969',5,3)
    else:
        tube('straw hat brim',[(0,0,2.35),(0,0,2.4)],[.5,.5],'#e4bd6e',10)
        tube('straw hat crown',[(0,0,2.4),(0,0,2.65)],[.29,.25],'#eed18c',8)
        tube('hat orange band',[(0,0,2.42),(0,0,2.5)],[.292,.281],'#c47f43',8)
    if floral:
        # Broaden the seated silhouette, with the greatest volume through the
        # belly and waist. Deform clothing, flowers and limbs together so the
        # pattern stays on the shirt and joints remain connected.
        for o in parts[first:]:
            for v in o.data.vertices:
                z=v.co.z
                belly=max(0,1-abs(z-1.2)/.55)
                v.co.x*=1.16+.30*belly
                v.co.y*=1.12+.52*belly
    tr=Matrix.Translation((cx,cy,0))@Matrix.Rotation(math.radians(yaw),4,'Z')
    for o in parts[first:]:o.matrix_world=tr@o.matrix_world
person(-1.8,-2.5,25,False);person(1.8,-2.5,-25,True)
# The unmistakable little shaving-foam can: mesh, stripes and raised type.
x,y=.02,-2.72
for z1,z2,c in [(1.35,1.42,'#d9ddd3'),(1.42,1.78,'#326584'),(1.78,1.86,'#d9ddd3')]:tube('shaving can',[(x,y,z1),(x,y,z2)],[.14,.14],c,12)
for z in (1.45,1.53):tube('red shaving can stripe',[(x,y,z),(x,y,z+.035)],[.142,.142],'#ca5140',12)
tube('foam can cap',[(x,y,1.86),(x,y,1.98)],[.1,.06],'#eeeee0',10)
text('can label','Barbasol',(x,y-.146,1.67),.062,'#f6ebd3',rot=(math.pi/2,0,math.pi),depth=.001)
# Cream eight-panel parasols, positioned to frame rather than cover the guests.
def umbrella(x,y,r,z):
    beam('parasol timber pole',(x,y,0),(x,y,z+.15),.06,TIMBER,8)
    for j in range(8):
        a=j*math.tau/8;b=(j+1)*math.tau/8
        mesh('cream parasol panel',[(x,y,z),(x+r*math.cos(a),y+r*math.sin(a),z-.72),(x+r*math.cos(b),y+r*math.sin(b),z-.72)],[(0,1,2)],tone(CREAM,.95+.035*(j%3)))
        beam('parasol rim',(x+r*math.cos(a),y+r*math.sin(a),z-.72),(x+r*math.cos(b),y+r*math.sin(b),z-.72),.035,'#dacaa5',4)
    box('parasol finial',(x,y,z+.07),(.13,.13,.2),EDGE)
umbrella(-4,-1.6,2.05,3.8);umbrella(4,.1,1.35,3.4)
# A chalkboard menu and a small slatted crate.
box('chalk menu timber frame',(3.48,-.03,1.05),(1.15,.16,1.8),EDGE,.02)
box('chalk menu blackboard',(3.48,-.125,1.05),(.95,.025,1.56),'#42473d')
text('chalkboard bar title','BAR',(3.48,-.147,1.42),.34,'#dfddc7',rot=(math.pi/2,0,math.pi),depth=.002)
text('chalkboard menu','COLD\nDRINKS',(3.48,-.15,.96),.13,'#c7d0b4',rot=(math.pi/2,0,math.pi),depth=.001)
for z in (.25,.52,.79):box('crate wooden slat',(4.65,1.4,z),(1,.8,.23),TIMBER,.015)
# Bent palm trunks and broad folded leaves; a few beach plants and rocks.
def palm(x,y,height):
    tube('segmented leaning palm',[(x,y,0),(x+.16,y,.9),(x+.36,y+.08,height*.6),(x+.7,y+.2,height)],[.23,.2,.16,.12],'#8c7145',7)
    top=Vector((x+.7,y+.2,height))
    for j in range(8):
        a=j*math.tau/8;v=Vector((math.cos(a),math.sin(a),0));w=Vector((-v.y,v.x,0))
        mesh('broad folded palm frond',[top,top+v*.8+w*.36+Vector((0,0,.5)),top+v*1.7+w*.42+Vector((0,0,.22)),top+v*2.4-Vector((0,0,.8)),top+v*1.7-w*.42+Vector((0,0,.22)),top+v*.8-w*.36+Vector((0,0,.5)),top+v*1.4+Vector((0,0,.55))],[(0,1,6),(1,2,6),(2,3,6),(3,4,6),(4,5,6),(5,0,6)],tone('#83a640',.9+.05*(j%4)))
palm(4.4,3,5.2)
for x,y in [(-4.5,-2.4),(4.9,.5),(-3.8,2.8)]:
    for j in range(7):
        a=j*math.tau/7;v=Vector((math.cos(a),math.sin(a),0));w=Vector((-v.y,v.x,0));b=Vector((x,y,0))
        mesh('pointed beach plant',[b,b+v*.65+w*.23+Vector((0,0,.6)),b+v*1.3+Vector((0,0,1.1)),b+v*.65-w*.23+Vector((0,0,.6)),b+v*.65+Vector((0,0,.75))],[(0,1,4),(1,2,4),(2,3,4),(3,0,4)],tone('#86ac40',.9+j*.035))
for x,y,s in [(-4.9,-3,.43),(-4.35,-3.1,.3),(5.1,2.2,.5)]:ell('beach weathered rock',(x,y,s*.4),(s,s*.75,s*.6),'#aaa28a',7,3)
finish('SM_BeachBar')
