#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
class UWorld;class AActor;class ACameraActor;class UTexture2D;class FJsonObject;
class FParkEmployeeBadge {
 TWeakObjectPtr<AActor> Actor;
 FSlateBrush LogoBrush;
public:
 FParkEmployeeBadge(UWorld*,TSharedPtr<FJsonObject> Step,UTexture2D* Logo);
 ~FParkEmployeeBadge();
 void Update(ACameraActor*,FVector Position);
};
