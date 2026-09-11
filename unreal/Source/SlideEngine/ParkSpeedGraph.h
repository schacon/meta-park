#pragma once
#include "CoreMinimal.h"
class UWorld;class AActor;class ACameraActor;class UStaticMeshComponent;class FJsonObject;
struct FSpeedPhase {FString Name;float Seconds=0;bool InNeck=true;};
struct FSpeedDinosaur {FString Label,Time;float Seconds=0;TArray<FSpeedPhase> Phases;};
class FParkSpeedGraph {
 TWeakObjectPtr<AActor> Actor;
 UStaticMeshComponent* Roller=nullptr;
 UStaticMeshComponent* Handle=nullptr;
public:
 FParkSpeedGraph(UWorld*,TSharedPtr<FJsonObject>);
 ~FParkSpeedGraph();
 TArray<FSpeedDinosaur> Rows;FString Title,Subtitle;
 float Reveal=0;
 void Update(float Delta,ACameraActor*,FVector Position,FVector Eye,bool Visible);
};
