#include "ParkCamera.h"
#include "Camera/CameraTypes.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "UnrealClient.h"
void AParkCamera::CalcCamera(float DeltaTime,FMinimalViewInfo& OutResult) {
 Super::CalcCamera(DeltaTime,OutResult);
 auto* PC=GetWorld()->GetFirstPlayerController();
 auto* Player=PC?PC->GetLocalPlayer():nullptr;
 if(!Player||!Player->ViewportClient||!Player->ViewportClient->Viewport)return;
 const FIntPoint Size=Player->ViewportClient->Viewport->GetSizeXY();
 const float Aspect=(Size.X*Player->Size.X)/FMath::Max(1.f,Size.Y*Player->Size.Y);
 // Contain a 16:9 composition inside the available scene rectangle. Preserve
 // horizontal coverage in narrow windows and vertical coverage in wide ones.
 const float Tangent=FMath::Tan(FMath::DegreesToRadians(OutResult.FOV*.5f));
 OutResult.FOV=FMath::RadiansToDegrees(2*FMath::Atan(Tangent*FMath::Max(1.f,Aspect/(16.f/9.f))));
 OutResult.AspectRatioAxisConstraint=AspectRatio_MaintainXFOV;
 OutResult.OffCenterProjectionOffset=FVector2D::ZeroVector;
}
