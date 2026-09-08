#pragma once
#include "CoreMinimal.h"
class UWorld;
class FJsonObject;
namespace IslandScene {
 struct FStats { int32 Trees = 0, Dinosaurs = 0; };
 void Animate(float Seconds);
 bool ValidateWandering();
 bool ValidateArticulatedGaits();
 float GroundHeight(float X,float Y);
 FStats Build(UWorld* World, const TArray<TSharedPtr<FJsonValue>>& Habitats, int32 Seed);
}
