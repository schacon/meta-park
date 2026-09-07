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
material.set_editor_property('used_with_instanced_static_meshes',True)
material.set_editor_property('two_sided',True)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
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
    for slot in range(len(mesh.static_materials)):mesh.set_material(slot,material)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    box=mesh.get_bounding_box()
    report.append(dict(asset=name,min=str(box.min),max=str(box.max)))
(root/'unreal/Content/Slides/park-assets.json').write_text(json.dumps(manifest,indent=2)+'\n')
(root/'assets/exports/import-report.json').write_text(json.dumps(report,indent=2)+'\n')
unreal.log('ParkAssets: imported '+str(len(report))+' authored Blender static meshes')
