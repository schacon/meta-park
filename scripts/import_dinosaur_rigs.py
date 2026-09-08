"""Import the Blender articulated body/limb kit and its rest-joint metadata."""
import unreal,json
from pathlib import Path
rig_root=Path(unreal.Paths.project_dir()).resolve().parent
rig_source=rig_root/'assets/exports'
rig_manifest=json.loads((rig_source/'dinosaur-rigs.json').read_text())
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
paint=unreal.load_asset('/Game/Models/Park/M_ParkPaint')
for rig_name in rig_manifest['assets']:
    task=unreal.AssetImportTask();task.filename=str(rig_source/(rig_name+'.fbx'))
    task.destination_path='/Game/Models/Park';task.destination_name=rig_name
    task.automated=True;task.replace_existing=True;task.save=True
    opts=unreal.FbxImportUI();opts.import_mesh=True;opts.import_as_skeletal=False
    opts.import_materials=False;opts.import_textures=False;opts.automated_import_should_detect_type=False
    opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    data=opts.static_mesh_import_data;data.combine_meshes=True;data.generate_lightmap_u_vs=False;data.auto_generate_collision=False
    data.vertex_color_import_option=unreal.VertexColorImportOption.REPLACE
    data.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    data.convert_scene=True;data.convert_scene_unit=True;data.force_front_x_axis=False;task.options=opts
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    loaded=unreal.load_asset('/Game/Models/Park/'+rig_name)
    if not isinstance(loaded,unreal.StaticMesh):raise RuntimeError('Missing articulated part '+rig_name)
    for i in range(len(loaded.static_materials)):loaded.set_material(i,paint)
    unreal.EditorAssetLibrary.save_loaded_asset(loaded)
(rig_root/'unreal/Content/Slides/dinosaur-rigs.json').write_text(json.dumps(rig_manifest,indent=2)+'\n')
unreal.log('DinosaurRigs: imported '+str(len(rig_manifest['assets']))+' articulated mesh parts')
