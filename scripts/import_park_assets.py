"""Import the Blender kit as cooked static meshes with authored vertex colours."""
import unreal, json
from pathlib import Path
root = Path(unreal.Paths.project_dir()).resolve().parent
source = root/'assets/exports'
manifest = json.loads((source/'park-assets.json').read_text())
unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
material_path='/Game/Models/Park/M_ParkPaint'
material=unreal.load_asset(material_path)
if not material:
    material=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_ParkPaint','/Game/Models/Park',unreal.Material,unreal.MaterialFactoryNew())
unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
color=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionVertexColor,-600,0)
rough=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionConstant,-300,200)
rough.set_editor_property('r',.82)
strength=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionMultiply,-400,400)
strength.set_editor_property('const_b',3)
glow=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionMultiply,-150,400)
assert unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
assert unreal.MaterialEditingLibrary.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
assert unreal.MaterialEditingLibrary.connect_material_expressions(color,'A',strength,'A')
assert unreal.MaterialEditingLibrary.connect_material_expressions(color,'',glow,'A')
assert unreal.MaterialEditingLibrary.connect_material_expressions(strength,'',glow,'B')
assert unreal.MaterialEditingLibrary.connect_material_property(glow,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
# Lightweight shader loops animate only authored flames and the waterfall region.
def expression(owner, cls):
    return unreal.MaterialEditingLibrary.create_material_expression(owner,cls)
def custom(owner, code, inputs, scalar=False):
    node=expression(owner,unreal.MaterialExpressionCustom)
    node.set_editor_property('code',code)
    node.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT1 if scalar else unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    pins=[]
    for name in inputs:
        pin=unreal.CustomInput();pin.set_editor_property('input_name',name);pins.append(pin)
    node.set_editor_property('inputs',pins)
    for name,(source_node,output) in inputs.items():
        assert unreal.MaterialEditingLibrary.connect_material_expressions(source_node,output,node,name)
    return node
world=expression(material,unreal.MaterialExpressionWorldPosition)
time=expression(material,unreal.MaterialExpressionTime)
flicker=custom(material,"return 2.8 + .32*sin(T*8.0+P.x*.017) + .16*sin(T*13.0+P.y*.023);",{'T':(time,''),'P':(world,'')},True)
assert unreal.MaterialEditingLibrary.connect_material_expressions(flicker,'',strength,'B')
flame_offset=custom(material,"float p=T*6.0+P.z*.011+P.x*.007; return Mask*float3(15*sin(p),9*cos(p*1.17),12*sin(p*.83));",{'T':(time,''),'P':(world,''),'Mask':(color,'A')})
assert unreal.MaterialEditingLibrary.connect_material_property(flame_offset,'',unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
water_path='/Game/Models/Park/M_ParkWater'
water=unreal.load_asset(water_path)
if not water:
    water=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_ParkWater','/Game/Models/Park',unreal.Material,unreal.MaterialFactoryNew())
unreal.MaterialEditingLibrary.delete_all_material_expressions(water)
w_color=expression(water,unreal.MaterialExpressionVertexColor)
w_time=expression(water,unreal.MaterialExpressionTime)
w_world=expression(water,unreal.MaterialExpressionWorldPosition)
mask='float chute=step(abs(P.x),1100.0)*step(2100.0,P.y)*step(P.y,3400.0)*smoothstep(100.0,250.0,P.z); '
flow=custom(water,mask+'return C*(1.0+chute*(.055*sin(P.z*.025+T*6.0+P.x*.008)));',{'C':(w_color,''),'P':(w_world,''),'T':(w_time,'')})
assert unreal.MaterialEditingLibrary.connect_material_property(flow,'',unreal.MaterialProperty.MP_BASE_COLOR)
ripple=custom(water,mask+'float foam=step(abs(P.x),600.0)*step(1900.0,P.y)*step(P.y,2550.0)*step(P.z,140.0)*step(60.0,P.z); return chute*float3(9*sin(T*3.5+P.z*.016),4*cos(T*3.1+P.z*.013),0)+foam*float3(3*sin(T*2+P.x*.02),4*cos(T*2.2+P.y*.02),3*sin(T*3+P.x*.015));',{'P':(w_world,''),'T':(w_time,'')})
assert unreal.MaterialEditingLibrary.connect_material_property(ripple,'',unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
w_rough=expression(water,unreal.MaterialExpressionConstant);w_rough.set_editor_property('r',.65)
assert unreal.MaterialEditingLibrary.connect_material_property(w_rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
water.set_editor_property('two_sided',True)
unreal.MaterialEditingLibrary.recompile_material(water)
unreal.EditorAssetLibrary.save_loaded_asset(water)
material.set_editor_property('used_with_instanced_static_meshes',True)
material.set_editor_property('two_sided',True)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
# Red warning lenses blink without moving their supporting poles.
beacon=unreal.load_asset('/Game/Models/Park/M_ParkBeacon')
if not beacon:beacon=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_ParkBeacon','/Game/Models/Park',unreal.Material,unreal.MaterialFactoryNew())
unreal.MaterialEditingLibrary.delete_all_material_expressions(beacon)
bc=expression(beacon,unreal.MaterialExpressionVertexColor)
bt=expression(beacon,unreal.MaterialExpressionTime)
bp=expression(beacon,unreal.MaterialExpressionObjectPositionWS)
blink=custom(beacon,'float phase=frac(T*.8+P.x*.00007); float pulse=1-smoothstep(.22,.30,phase); return C*Mask*(.12+8*pulse);',{'T':(bt,''),'P':(bp,''),'C':(bc,''),'Mask':(bc,'A')})
assert unreal.MaterialEditingLibrary.connect_material_property(bc,'',unreal.MaterialProperty.MP_BASE_COLOR)
assert unreal.MaterialEditingLibrary.connect_material_property(blink,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(beacon)
unreal.EditorAssetLibrary.save_loaded_asset(beacon)
exec(compile((root/'scripts/import_dinosaur_rigs.py').read_text(),str(root/'scripts/import_dinosaur_rigs.py'),'exec'))
report=[]
for entry in manifest['assets']:
    name=entry['name']
    task=unreal.AssetImportTask()
    task.filename=str(source/(name+'.fbx'))
    task.destination_path='/Game/Models/Park'
    task.destination_name=name
    task.automated=True
    task.replace_existing=True
    task.save=True
    opts=unreal.FbxImportUI()
    opts.import_mesh=True
    opts.import_as_skeletal=False
    opts.import_materials=False
    opts.import_textures=False
    opts.automated_import_should_detect_type=False
    opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    data=opts.static_mesh_import_data
    data.combine_meshes=True
    data.generate_lightmap_u_vs=False
    data.auto_generate_collision=False
    data.vertex_color_import_option=unreal.VertexColorImportOption.REPLACE
    data.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    data.convert_scene=True
    data.convert_scene_unit=True
    data.force_front_x_axis=False
    task.options=opts
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    path='/Game/Models/Park/'+name
    mesh=unreal.load_asset(path)
    if not isinstance(mesh,unreal.StaticMesh):raise RuntimeError('Missing mesh '+path+' '+str(task.imported_object_paths))
    for slot in range(len(mesh.static_materials)):mesh.set_material(slot,beacon if name=='SM_RaptorBeacon' else water if name in ('SM_Water','SM_WaterSplash') else material)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    box=mesh.get_bounding_box()
    report.append(dict(asset=name,min=str(box.min),max=str(box.max)))
(root/'unreal/Content/Slides/park-ground.json').write_text((source/'park-ground.json').read_text())
(root/'unreal/Content/Slides/park-assets.json').write_text(json.dumps(manifest,indent=2)+'\n')
(root/'assets/exports/import-report.json').write_text(json.dumps(report,indent=2)+'\n')
unreal.log('ParkAssets: imported '+str(len(report))+' authored Blender static meshes')
