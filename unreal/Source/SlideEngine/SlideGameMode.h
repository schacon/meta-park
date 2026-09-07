#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Styling/SlateBrush.h"
#include "SlideGameMode.generated.h"
class ACameraActor;
class SWidget;
class UTextureRenderTarget2D;
class ASceneCapture2D;
struct FParkMapPin { FVector Position; FString Code, Name, Status; int32 SlideIndex=-1; bool bHero=false; };
struct FSlideView { FVector Position; FRotator Rotation; float Duration; };
struct FSlideMotion { TWeakObjectPtr<AActor> Actor; FVector Origin; FRotator Rotation; FString Kind; float Speed; float Amplitude; };
struct FSlidePanel { TWeakObjectPtr<AActor> Root; FVector RaisedPosition; float Reveal = 0; TArray<TWeakObjectPtr<AActor>> Models; };
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
 TArray<FSlideView> Views;
 TArray<FParkMapPin> MapPins;
 bool bAllHabitats=false;
 FString ParkSection=TEXT("Park Status");
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
 enum class EMapPhase { Overview, ZoomIn, Arrived, Expand, Loading, Slide, Retract, ZoomOut };
 EMapPhase MapPhase=EMapPhase::Overview;
 TArray<TSharedPtr<SWidget>> SlideContents;
 TArray<FVector> CardTargets, SignAnchors, CameraStops, CameraTargets, SwoopA, SwoopB;
 TArray<float> SignYaws;
 FVector MapCenter=FVector(2850,-750,0), MapEye=MapCenter+FVector(3000,-28000,23000).GetSafeNormal()*200000;
 FVector CameraLook=MapCenter, FromLook;
 float CameraFrameWidth=40000, FromFrameWidth=40000, FromDistance=200000;
 float CardExpansion=0, FromExpansion=0, FromOrthoWidth=23500, MapLegDuration=.8f;
 int32 PendingIndex=-1, SmokeStep=0;
 FVector4 CardStartRect=FVector4(400,200,208,79);
 void BeginMapLeg(bool ZoomIn);
 float ParkFocusWidth() const;
 FVector ParkFocusEye() const;
 void CaptureCardBounds();
 bool bArrivalCaptured=false, bLoadingCaptured=false;
 void TickParkNavigation(float Delta);
 void HandleParkKey(const FKey& Key);
 FVector4 ExpandedCardBounds() const;
 void Overview();
 void GoTo(int32 Next, bool Instant = false);
 void CreateDesktopHUD();
 void SetMouseMode(bool Fly);
};
