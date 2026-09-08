#pragma once
#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "ParkViewportClient.generated.h"
UCLASS()
class SLIDEENGINE_API UParkViewportClient : public UGameViewportClient {
 GENERATED_BODY()
public:
 FVector2D SceneOrigin=FVector2D::ZeroVector;
 virtual void LayoutPlayers() override;
};
