#pragma once
#include "CoreMinimal.h"
class UWorld;class AActor;class ACameraActor;class USceneComponent;
class FParkWoodSign {
 TWeakObjectPtr<AActor> Actor;
 USceneComponent* Board=nullptr;
 bool Warning=false;
public:
 FParkWoodSign(UWorld*,const FString& Title,bool IsWarning);
 ~FParkWoodSign();
 void Update(float Seconds,ACameraActor*,FVector Position,FVector Eye,float Reveal);
};
