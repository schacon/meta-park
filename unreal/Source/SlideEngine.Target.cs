using UnrealBuildTool;
public class SlideEngineTarget : TargetRules {
 public SlideEngineTarget(TargetInfo Target) : base(Target) { Type = TargetType.Game; DefaultBuildSettings = BuildSettingsVersion.V7; IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8; ExtraModuleNames.Add("SlideEngine"); }
}
