"""Import the login emblem as an uncompressed UI texture for packaged builds."""
import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve().parent
task=unreal.AssetImportTask()
task.filename=str(root/'assets/ui/git-meta-park-logo.png')
task.destination_path='/Game/Models/UI'
task.destination_name='T_ParkLogo'
task.automated=True;task.replace_existing=True;task.save=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture=unreal.load_asset('/Game/Models/UI/T_ParkLogo')
assert isinstance(texture,unreal.Texture2D)
texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
