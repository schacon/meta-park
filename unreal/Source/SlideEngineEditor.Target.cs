using UnrealBuildTool;
public class SlideEngineEditorTarget : TargetRules {
 public SlideEngineEditorTarget(TargetInfo Target) : base(Target) { Type = TargetType.Editor; DefaultBuildSettings = BuildSettingsVersion.V7; IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8; ExtraModuleNames.Add("SlideEngine"); }
}
