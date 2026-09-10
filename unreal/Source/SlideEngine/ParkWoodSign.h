#pragma once
#include "CoreMinimal.h"
class UWorld;class AActor;class ACameraActor;class USceneComponent;
class FParkWoodSign {
 TWeakObjectPtr<AActor> Actor;
 USceneComponent* Board=nullptr;
 bool Warning=false,Helipad=false;
 float EntranceStart=-1;
public:
 FParkWoodSign(UWorld*,const FString& Title,bool IsWarning,bool IsHelipad=false);
 ~FParkWoodSign();
 void Update(float Seconds,ACameraActor*,FVector Position,FVector Eye,float Reveal);
};
