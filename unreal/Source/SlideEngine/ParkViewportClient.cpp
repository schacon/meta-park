#include "ParkViewportClient.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
void UParkViewportClient::LayoutPlayers() {
 Super::LayoutPlayers();
 if(auto* Instance=GetGameInstance())for(auto* Player:Instance->GetLocalPlayers()) {
  Player->Origin=SceneOrigin;
  Player->Size=FVector2D(1,1)-SceneOrigin;
 }
}
