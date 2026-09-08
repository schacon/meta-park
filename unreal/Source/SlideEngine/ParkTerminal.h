#pragma once
#include "CoreMinimal.h"
class AStaticMeshActor;
class ACameraActor;
class UWorld;
class FParkTerminal {
 TWeakObjectPtr<AStaticMeshActor> Actor;
 FBox Bounds;
 bool Raised=false;
 float Reveal=0,Clock=0;
public:
 explicit FParkTerminal(UWorld* World);
 void Toggle();
 void Update(float Delta,ACameraActor* Camera,bool InFirstArea);
 FString Command() const;
 bool OutputReady() const{return Clock>=1.65f;}
 bool IsHidden() const{return Reveal==0;}
 bool IsRaised() const{return Reveal==1&&Raised;}
 bool ScreenFillsViewport() const;
};
