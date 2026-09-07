#pragma once
#include "CoreMinimal.h"
class UWorld;
class FJsonObject;
namespace IslandScene {
 struct FStats { int32 Trees = 0, Dinosaurs = 0; };
 FStats Build(UWorld* World, const TArray<TSharedPtr<FJsonValue>>& Habitats, int32 Seed);
}
