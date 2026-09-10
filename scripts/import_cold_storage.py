"""Import only the cryogenic prop, leaving the existing island assets intact."""
import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve().parent
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
path='/Game/Models/Park/M_ColdMetal'
m=unreal.load_asset(path)
if not m:m=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_ColdMetal','/Game/Models/Park',unreal.Material,unreal.MaterialFactoryNew())
unreal.MaterialEditingLibrary.delete_all_material_expressions(m)
c=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionVertexColor,0,0)
unreal.MaterialEditingLibrary.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
for prop,value in [(unreal.MaterialProperty.MP_METALLIC,.5),(unreal.MaterialProperty.MP_ROUGHNESS,.3)]:
    n=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant,0,200);n.set_editor_property('r',value);unreal.MaterialEditingLibrary.connect_material_property(n,'',prop)
em=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionMultiply,200,0);em.set_editor_property('const_b',.18);unreal.MaterialEditingLibrary.connect_material_expressions(c,'',em,'A');unreal.MaterialEditingLibrary.connect_material_property(em,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
for name in ['SM_ColdBody','SM_ColdRack','SM_ColdLid','SM_ColdVial']:
    t=unreal.AssetImportTask();t.filename=str(root/'assets/exports'/f'{name}.fbx');t.destination_path='/Game/Models/Park';t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True
    o=unreal.FbxImportUI();o.import_mesh=True;o.import_as_skeletal=False;o.import_materials=False;o.import_textures=False;o.automated_import_should_detect_type=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    d=o.static_mesh_import_data;d.combine_meshes=True;d.generate_lightmap_u_vs=False;d.auto_generate_collision=False;d.vertex_color_import_option=unreal.VertexColorImportOption.REPLACE;d.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS;d.convert_scene=True;d.convert_scene_unit=True;t.options=o
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t]);mesh=unreal.load_asset('/Game/Models/Park/'+name)
    if not mesh:raise RuntimeError('Missing '+name)
    for i in range(len(mesh.static_materials)):mesh.set_material(i,m)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    print(name,mesh.get_bounding_box())
