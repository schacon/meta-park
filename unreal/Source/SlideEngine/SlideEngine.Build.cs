using UnrealBuildTool;
public class SlideEngine : ModuleRules {
 public SlideEngine(ReadOnlyTargetRules Target) : base(Target) {
  PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
  PublicDependencyModuleNames.AddRange(new string[] {"Core","CoreUObject","Engine","InputCore","Json","UMG","Slate","SlateCore","ProceduralMeshComponent"});
 }
}
