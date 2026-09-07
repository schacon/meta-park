#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Styling/SlateBrush.h"
#include "SlideGameMode.generated.h"
class ACameraActor;
class SWidget;
class UTextureRenderTarget2D;
class ASceneCapture2D;
struct FSlideView { FVector Position; FRotator Rotation; float Duration; };
struct FSlideMotion { TWeakObjectPtr<AActor> Actor; FVector Origin; FRotator Rotation; FString Kind; float Speed; float Amplitude; };
struct FSlidePanel { TWeakObjectPtr<AActor> Root; FVector RaisedPosition; float Reveal = 0; };
UCLASS()
class SLIDEENGINE_API ASlideGameMode : public AGameModeBase {
 GENERATED_BODY()
public:
 ASlideGameMode();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 UPROPERTY() TObjectPtr<ACameraActor> Camera;
 UPROPERTY() TObjectPtr<AActor> MascotActor;
 UPROPERTY() TObjectPtr<ASceneCapture2D> MascotCapture;
 UPROPERTY() TObjectPtr<UTextureRenderTarget2D> MascotTexture;
 FSlateBrush MascotBrush;
 TArray<FSlideView> Views;
 TArray<FSlideMotion> Motions;
 TArray<FSlidePanel> Panels;
 TArray<FString> Notes, Titles, HabitatNames;
 TSharedPtr<SWidget> DesktopHUD;
 int32 Index = 0, TreeCount = 0, DinoCount = 0, HabitatCount = 0, RouteSeed = 0;
 float Travel = 0, Elapsed = 0, TimerElapsed = 0, TimerDuration = 1200;
 FVector FromPosition;
 FQuat FromRotation;
 bool bFlying = false, bOverview = false, bTourStarted = false, bTimerPaused = false;
 bool bIsland = false, bFlightTested = false, bSmokeCaptured = false, bInitialCaptured = false;
 float FlightLift = 0, SmokeOverviewStart = -1;
 void Overview();
 void GoTo(int32 Next, bool Instant = false);
 void CreateDesktopHUD();
 void SetMouseMode(bool Fly);
};
