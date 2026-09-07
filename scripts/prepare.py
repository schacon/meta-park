import unreal
path = '/Game/Maps/Presentation'
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not editor.new_level(path):
        raise RuntimeError('Could not create presentation map')
    if not editor.save_current_level():
        raise RuntimeError('Could not save presentation map')
unreal.log('Presentation map ready: ' + path)

# A cooked parameterized material provides the flat SGI palette on native meshes.
material_path = '/Game/Models/M_Retro'
if not unreal.EditorAssetLibrary.does_asset_exist(material_path):
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset('M_Retro', '/Game/Models', unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property('two_sided', True)
    material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    color = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVectorParameter, 0, 0)
    color.set_editor_property('parameter_name', 'Tint')
    color.set_editor_property('default_value', unreal.LinearColor(0.3, 0.5, 0.8, 1))
    unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(material_path)

# Lit surfaces preserve the workstation palette while receiving sun and sky shadows.
def create_lit(name, vertex_color=False, water=False):
    path = '/Game/Models/' + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, '/Game/Models', unreal.Material, unreal.MaterialFactoryNew())
    color_type = unreal.MaterialExpressionVertexColor if vertex_color else unreal.MaterialExpressionVectorParameter
    color = unreal.MaterialEditingLibrary.create_material_expression(material, color_type, -400, 0)
    if not vertex_color:
        color.set_editor_property('parameter_name', 'Tint')
        color.set_editor_property('default_value', unreal.LinearColor(0.25,0.4,0.28,1))
    unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 200)
    roughness.set_editor_property('r', 0.26 if water else 0.82)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(path)

create_lit('M_IslandLit')
create_lit('M_IslandTerrain', vertex_color=True)
create_lit('M_IslandWater', water=True)

# Real-time skylight captures this emissive dome; it must not shadow the island.
sky = unreal.load_asset('/Game/Models/M_Retro')
sky.set_editor_property('is_sky', True)
unreal.MaterialEditingLibrary.recompile_material(sky)
unreal.EditorAssetLibrary.save_asset('/Game/Models/M_Retro')

forest_material = unreal.load_asset('/Game/Models/M_IslandLit')
forest_material.set_editor_property('used_with_instanced_static_meshes', True)
unreal.MaterialEditingLibrary.recompile_material(forest_material)
unreal.EditorAssetLibrary.save_asset('/Game/Models/M_IslandLit')
