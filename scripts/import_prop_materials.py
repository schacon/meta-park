"""Unlit lettering for physical presentation props, independent of scene lights."""
import unreal
for name,masked in [('M_PropPaper',False),('M_PropLettering',True)]:
    path='/Game/Models/'+name
    m=unreal.load_asset(path)
    if not m:m=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,'/Game/Models',unreal.Material,unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(m)
    m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property('two_sided',True)
    m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED if masked else unreal.BlendMode.BLEND_OPAQUE)
    t=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionTextureSampleParameter2D,0,0)
    t.set_editor_property('parameter_name','SlateUI');t.set_editor_property('texture',unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture'))
    unreal.MaterialEditingLibrary.connect_material_property(t,'RGB',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if masked:unreal.MaterialEditingLibrary.connect_material_property(t,'A',unreal.MaterialProperty.MP_OPACITY_MASK)
    unreal.MaterialEditingLibrary.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
