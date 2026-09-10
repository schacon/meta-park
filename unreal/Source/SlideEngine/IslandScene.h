#pragma once
#include "CoreMinimal.h"
class UWorld;
class FJsonObject;
class AStaticMeshActor;
namespace IslandScene {
 struct FStats { int32 Trees = 0, Dinosaurs = 0; };
 void Animate(float Seconds);
 TArray<FVector> RaptorPositions();
 bool ValidateWandering();
 bool ValidateArticulatedGaits();
 void ResetGate();
 void RegisterGateLeaf(AStaticMeshActor* Actor,float Direction);
 void TickGate(float Delta,bool Open);
 float GateOpenFraction();
 bool ValidateGatePose();
 float GroundHeight(float X,float Y);
 FStats Build(UWorld* World, const TArray<TSharedPtr<FJsonValue>>& Habitats, int32 Seed);
}
