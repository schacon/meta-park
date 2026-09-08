#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Styling/SlateBrush.h"
#include "ParkDepartures.h"
#include "SlideGameMode.generated.h"
class ACameraActor;
class SWidget;
class UTextureRenderTarget2D;
class ASceneCapture2D;
class FParkTerminal;
struct FParkMapPin { FVector Position; FString Code, Name, Status; int32 SlideIndex=-1; bool bHero=false; };
struct FSlideView { FVector Position; FRotator Rotation; float Duration; };
struct FSlideMotion { TWeakObjectPtr<AActor> Actor; FVector Origin; FRotator Rotation; FString Kind; float Speed; float Amplitude; };
struct FSlidePanel { TWeakObjectPtr<AActor> Root; FVector RaisedPosition; float Reveal = 0; TArray<TWeakObjectPtr<AActor>> Models; TArray<int32> ModelSteps; };
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
 int32 GateSlide=INDEX_NONE;
 void TestGate();
 TSharedPtr<FParkTerminal> Terminal;

 void TestTerminal();
 void TestStationContent();
 TSet<int32> HiddenSlides;
 int32 NextVisibleSlide(int32 Current,int32 Direction) const;
 bool bAllHabitats=false;
 FString ParkSection=TEXT("Park Status");
 TArray<FSlideMotion> Motions;
 TArray<FSlidePanel> Panels;
 TArray<FString> Notes, Titles, HabitatNames;
 UPROPERTY() TObjectPtr<class UTexture2D> LoginLogo;
 FSlateBrush LoginLogoBrush;
 TSharedPtr<SWidget> DesktopHUD, LoginHUD;
 TSharedPtr<class SEditableTextBox> WorkstationField;
 TSharedPtr<class SButton> LoginButtonWidget, LogoutButton;
 TSharedPtr<class SComboButton> ParkMenu;
 TSet<FString> MinimizedLoginWindows;
 bool bLocked=false, bLoginTyping=false;
 int32 WorkstationMinutes=35, LoginTestStep=0;
 float LoginTime=0, LockTime=0;
 FString WorkstationText=TEXT("35"), LoginError;
 void CreateLoginHUD();
 void StartLogin();
 void FinishLogin();
 void LogoutToLogin();
 void ResetPresentationSession();
 void TickLogin(float Delta);
 void TestLogin();
 FString LoginName() const;
 FString LoginPassword() const;
 int32 Index = 0, TreeCount = 0, DinoCount = 0, HabitatCount = 0, RouteSeed = 0;
 float Travel = 0, Elapsed = 0, TimerElapsed = 0, TimerDuration = 1200;
 FVector FromPosition;
 FQuat FromRotation;
 bool bFlying = false, bOverview = false, bTourStarted = false, bTimerStarted = false, bTimerPaused = false;
 bool bIsland = false, bFlightTested = false, bSmokeCaptured = false, bInitialCaptured = false;
 float FlightLift = 0, SmokeOverviewStart = -1;
 enum class EMapPhase { Overview, ZoomIn, Arrived, Expand, Loading, Slide, Retract, ZoomOut, Free };
 bool bFreeFlight=false, bFreeTravelling=false;
 float FreeTravel=0;
 FVector FreeEye, FreeLook;
 FVector2D FreeDrag=FVector2D::ZeroVector;
 TArray<TWeakObjectPtr<AActor>> SignBases;
 void ToggleFreeFlight();
 void FlyToArea(int32 Next);
 void TickFreeFlight(float Delta);
 void MoveFreeCamera(float Delta, FVector Move, FVector2D Look, bool Fast);
 void TestFreeFlight();
 EMapPhase MapPhase=EMapPhase::Overview;
 TArray<TArray<TSharedPtr<class FJsonObject>>> StationSteps;
 int32 PageIndex=0, PreviousPage=INDEX_NONE, PageDirection=1, ActiveComponentPage=INDEX_NONE;
 float PageSwipe=1;
 TSharedRef<SWidget> BuildStationContent(TSharedPtr<FJsonObject> Station,int32 StationIndex,const FString& Label,FLinearColor Accent);
 bool AdvancePage(int32 Direction);
 bool IsComponentPage() const;
 void ResetStationPage();
 void TickStationPage(float Delta);
 TArray<FVector> CardTargets, SignAnchors, CameraStops, CameraTargets, SwoopA, SwoopB;
 TArray<float> SignYaws, FocusFrameWidths;
 FVector MapCenter=FVector(-350,-750,0), MapEye=MapCenter+FVector(3000,-28000,23000).GetSafeNormal()*200000;
 FVector CameraLook=MapCenter, FromLook;
 static constexpr float OverviewFrameWidth=36000.f;
 float CameraFrameWidth=OverviewFrameWidth, FromFrameWidth=OverviewFrameWidth, FromDistance=200000;
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
 void TickPresentationClock(float Delta);
 FParkDepartures VisitorDepartures, StaffDepartures;
 int32 RemainingVisitors() const;
 int32 RemainingStaff() const;
 int32 DinoPopulation() const;
 FSlateColor PopulationColor(int32 Remaining) const;
 FVector4 ExpandedCardBounds() const;
 void Overview();
 void GoTo(int32 Next, bool Instant = false);
 void CreateDesktopHUD();
 void SetMouseMode(bool Fly);
};
