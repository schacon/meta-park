#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
class SWidget;class UWorld;class FParkScalarScene;
struct FScalarCommit {FString Id,Tree,Parent,Delta;TArray<int32> Entries;};
struct FScalarSpace {int32 Commits=0,FirstCommit=0;TArray<int32> Blobs;};
struct FScalarHighlight {TArray<FVector2D> Points;int32 ColorIndex=0;float Age=0;bool Released=false;};
struct FScalarStep {FString Label,Actor,Description,Command,Detail,Action;int32 Room=0,From=-1,Commit=-1;TArray<FScalarSpace> Spaces;};
class FParkScalar {
public:
 explicit FParkScalar(TSharedPtr<FJsonObject> Component,UWorld* World);
 ~FParkScalar();
 TSharedPtr<FParkScalarScene> Scene;
 FString Title;
 TArray<FScalarCommit> Commits;
 TArray<FScalarStep> Steps;
 TArray<FScalarHighlight> Highlights;
 int32 NextHighlightColor=0;
 int32 Selected=0;
 float Clock=0,Age=0,Tilt=.46f,Height=.5f,Speed=.5f,OrbitYaw=-8;
 FVector PanOffset=FVector::ZeroVector;
 bool Playing=false,ShowHelp=false,MovingCamera=true;
 FString VisibleCommands() const;
 void Tick(float Delta);
 void ToggleCamera(){PanOffset=FVector::ZeroVector;MovingCamera=!MovingCamera;Height=.5f;OrbitYaw=-8;Tilt=.46f;}
 void Orbit(FVector2D Delta){OrbitYaw=FMath::UnwindDegrees(OrbitYaw-Delta.X*.3f);Tilt=FMath::Clamp(Tilt+float(Delta.Y)*.003f,0.f,1.f);}
 void Pan(FVector2D Delta,float ViewWidth);
 void Zoom(float Delta){Height=FMath::Clamp(Height+Delta*.07f,0.f,1.f);}
 void Select(int32 Step);
 bool Advance(int32 Direction);
 void Reset(){PanOffset=FVector::ZeroVector;Playing=false;Select(0);}
 void TogglePlayback(){if(Selected==Steps.Num()-1)Select(0);Playing=!Playing;Clock=0;}
};
TSharedRef<SWidget> MakeParkScalarView(TFunction<TSharedPtr<FParkScalar>()> State,TFunction<void()> Restore);
