#pragma once
#include "CoreMinimal.h"
class AStaticMeshActor;
class ACameraActor;
class UWorld;
class FParkTerminal {
 TWeakObjectPtr<AStaticMeshActor> Actor;
 FBox Bounds;
 bool Raised=false;
 FString Prompt, Output;
 float Reveal=0,Clock=0;
public:
 explicit FParkTerminal(UWorld* World);
 void Show(const FString& InPrompt,const FString& InOutput);
 void Hide(){Raised=false;}
 void Update(float Delta,ACameraActor* Camera,bool InFirstArea,FVector4 Region=FVector4(0,0,1,1),bool WholeModel=false);
 FString Command() const;
 bool OutputReady() const{return Clock>=.15f+Prompt.Len()*.09f+.4f;}
 bool IsHidden() const{return Reveal==0;}
 bool IsRaised() const{return Reveal==1&&Raised;}
 bool ScreenFillsViewport(float MinCoverage=.56f) const;
};
