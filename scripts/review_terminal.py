"""Create a persistent, isolated workstation review scene in the park blend file."""
terminal_review=bpy.data.scenes.new('Workstation • model review')
terminal_review.render.engine='BLENDER_EEVEE';terminal_review.view_settings.view_transform='Standard'
terminal_review.world=bpy.data.worlds.new('Workstation • soft daylight');terminal_review.world.use_nodes=True
terminal_review.world.node_tree.nodes['Background'].inputs[0].default_value=(.7,.72,.75,1)
terminal_review.world.node_tree.nodes['Background'].inputs[1].default_value=.65
obj=bpy.data.objects.new('Workstation • assembled',None);obj.instance_type='COLLECTION';obj.instance_collection=assets['SM_Terminal'];terminal_review.collection.objects.link(obj)
cd=bpy.data.cameras.new('Workstation • review camera');co=bpy.data.objects.new(cd.name,cd);terminal_review.collection.objects.link(co)
co.location=(-4.6,-7,4.2);co.rotation_euler=(Vector((.2,-.35,1.1))-co.location).to_track_quat('-Z','Y').to_euler();cd.type='ORTHO';cd.ortho_scale=5.6;terminal_review.camera=co
for name,p,power,size in [('key',(-3,-5,7),800,5),('fill',(4,-1,4),350,4)]:
    d=bpy.data.lights.new('Workstation '+name,'AREA');d.energy=power;d.shape='DISK';d.size=size
    o=bpy.data.objects.new(d.name,d);terminal_review.collection.objects.link(o);o.location=p;o.rotation_euler=(Vector((0,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
floor=box('Workstation review ground',(0,0,-.13),(200,200,.2),'#ded8c9');scene.collection.objects.unlink(floor);terminal_review.collection.objects.link(floor);parts.clear()
terminal_review.render.resolution_x=1280;terminal_review.render.resolution_y=960;terminal_review.render.resolution_percentage=100
terminal_review.compositing_node_group=scene.compositing_node_group.copy()
for node in terminal_review.compositing_node_group.nodes:
    if node.type=='R_LAYERS':node.scene=terminal_review
terminal_review.render.filepath=str(ROOT/'assets/blender/terminal-review.png')
