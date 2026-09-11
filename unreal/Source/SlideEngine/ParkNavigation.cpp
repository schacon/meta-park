#include "SlideGameMode.h"
#include "ParkScalar.h"
#include "ParkExchange.h"
#include "IslandScene.h"
#include "ParkViewportClient.h"
#include "ParkCamera.h"
#include "ParkTerminal.h"
#include "ParkCastPlayer.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SWidget.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

FVector ASlideGameMode::GateScreenPosition(float Progress) const {
 const FVector Behind=Panels[GateSlide].RaisedPosition;
 // Travel through the doorway, then settle nearer the fixed camera for reading.
 const FVector Reading=FMath::Lerp(CameraStops[GateSlide],Behind,.4f)+FVector(0,0,260);
 return FMath::Lerp(Behind,Reading,Progress);
}
float ASlideGameMode::ParkFocusWidth() const { return FocusFrameWidths[Index]; }
FVector ASlideGameMode::ParkFocusEye() const { return CameraStops[Index]; }
int32 ASlideGameMode::NextVisibleSlide(int32 Current,int32 Direction) const {
 if(Direction>0&&Current==Views.Num()-2&&HiddenSlides.Contains(Current+1))return Current+1;
 for(int32 Step=0;Step<Views.Num();Step++) {
  Current=(Current+Direction+Views.Num())%Views.Num();
  if(!HiddenSlides.Contains(Current))return Current;
 }
 return Current;
}
void ASlideGameMode::CaptureCardBounds() {
 auto* PC=GetWorld()->GetFirstPlayerController(); FVector2D Screen;
 int32 W,H;PC->GetViewportSize(W,H);
 if(DesktopHUD.IsValid()&&W>0&&H>0&&PC->ProjectWorldLocationToScreen(CardTargets[Index],Screen)) {
  const FVector2D Size=DesktopHUD->GetCachedGeometry().GetLocalSize();
  const FVector2D P=Screen*Size/FVector2D(W,H)-FVector2D(104,145);
  CardStartRect=FVector4(FMath::Clamp(P.X,365.,Size.X-228),FMath::Max(20.,P.Y),208,79);
 }
}
void ASlideGameMode::BeginMapLeg(bool ZoomIn) {
 auto* Component=Camera->GetCameraComponent();
 FromPosition=Camera->GetActorLocation(); FromRotation=Camera->GetActorQuat();
 FromLook=CameraLook; FromFrameWidth=CameraFrameWidth; FromDistance=FVector::Distance(FromPosition,FromLook);
 FromExpansion=CardExpansion; Travel=0; bFlying=false; bOverview=false;
 MapLegDuration=Views[Index].Duration*(ZoomIn?1.f:.75f);
 MapPhase=ZoomIn?EMapPhase::ZoomIn:CardExpansion>0?EMapPhase::Retract:EMapPhase::ZoomOut;
 if(MapPhase==EMapPhase::Retract)MapLegDuration=Index==GateSlide?.5f:.16f;
 if(!ZoomIn)for(auto& Entry:PageTerminals)if(!Entry.Value->IsHidden()){Entry.Value->Hide();MapPhase=EMapPhase::Retract;MapLegDuration=.5f;}
 if(ZoomIn) { ResetStationPage(); bTourStarted=true; PendingIndex=-1; CardExpansion=0; }
 SetMouseMode(false);
}
void ASlideGameMode::HandleParkKey(const FKey& Key) {
 if(bLocked){if(Key==EKeys::Enter)StartLogin();return;}
 if(bConfirmFinish){if(Key==EKeys::Enter||Key==EKeys::Y)ShowQuestions();else if(Key==EKeys::Escape||Key==EKeys::N)bConfirmFinish=false;return;}
 if(bQuestions){if(Key==EKeys::Escape)LogoutToLogin();return;}
 if(DesktopMinimize>0&&DesktopMinimize<1)return;
 if(bCommandDesktop&&bDesktopScalar&&Scalar.IsValid()){if(Key==EKeys::C){Scalar->ToggleCamera();return;}if(Key==EKeys::R){Scalar->Reset();return;}if(Key==EKeys::B){Scalar->Advance(-1);return;}if(Key==EKeys::P){Scalar->TogglePlayback();return;}}
 if(Exchange.IsValid()&&Key==EKeys::R){Exchange->Select(0);return;}
 if(bCommandDesktop&&!bDesktopScalar&&!bDesktopFSV&&!bDesktopSerializer&&Key==EKeys::SpaceBar){if(CastPlayer.IsValid())CastPlayer->TogglePlayback();return;}
 if(Key==EKeys::F){ToggleFreeFlight();return;}
 static const FKey Numbers[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight};
 static const FKey Numpad[]={EKeys::NumPadOne,EKeys::NumPadTwo,EKeys::NumPadThree,EKeys::NumPadFour,EKeys::NumPadFive,EKeys::NumPadSix,EKeys::NumPadSeven,EKeys::NumPadEight};
 for(int32 I=0;I<8&&I<Views.Num();I++)if(Key==Numbers[I]||Key==Numpad[I]){GoTo(I);return;}
 if(bFreeFlight) {
  if(Key==EKeys::Escape||Key==EKeys::O)ToggleFreeFlight();
  else if(Key==EKeys::P)bTimerPaused=!bTimerPaused;
  return;
 }
 const int32 Current=PendingIndex>=0?PendingIndex:Index;
 const bool Forward=Key==EKeys::Right||Key==EKeys::SpaceBar||Key==EKeys::PageDown;
 const bool Backward=Key==EKeys::Left||Key==EKeys::PageUp;
 if(Forward||Backward) {
  if(MapPhase==EMapPhase::Slide&&AdvancePage(Forward?1:-1))return;
  if(Forward&&Current==Views.Num()-1&&(MapPhase==EMapPhase::Slide||MapPhase==EMapPhase::Overview)){bConfirmFinish=true;SetMouseMode(false);return;}
  const int32 Next=Forward?(!bTourStarted?0:NextVisibleSlide(Current,1)):NextVisibleSlide(Current,-1);
  if(MapPhase==EMapPhase::Overview)GoTo(Next);
  // A second explicit press during the return can choose the next stop,
  // without restarting the flight. The first press only returns to the map.
  else if(MapPhase==EMapPhase::Retract||MapPhase==EMapPhase::ZoomOut)PendingIndex=(Next%Views.Num()+Views.Num())%Views.Num();
  else Overview();
 }
 else if(Key==EKeys::Escape||Key==EKeys::O)Overview();
 else if(Key==EKeys::Home)GoTo(0);
 else if(Key==EKeys::P)bTimerPaused=!bTimerPaused;
}
void ASlideGameMode::TickParkNavigation(float Delta) {
 IslandScene::Animate(Elapsed);
 const bool Smoke=FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest"));
 int32 WalkReview=0;if(Smoke)FParse::Value(FCommandLine::Get(),TEXT("DinoWalkReview="),WalkReview);
 auto* PC=GetWorld()->GetFirstPlayerController();
 // Slate geometry already includes display scaling. Reserve exactly the same
 // fraction of the viewport for the sidebar and title bar, on every frame.
 if(DesktopHUD.IsValid())if(auto* Player=PC->GetLocalPlayer()) {
  const FVector2D Size=DesktopHUD->GetCachedGeometry().GetLocalSize();
  if(Size.X>0&&Size.Y>0) {
   Player->Origin=FVector2D(FMath::Min(280.f,float(Size.X)*.26f)/Size.X,FMath::Min(72.f,float(Size.Y)*.15f)/Size.Y);
   Player->Size=FVector2D(1,1)-Player->Origin;
   if(auto* Viewport=Cast<UParkViewportClient>(Player->ViewportClient))Viewport->SceneOrigin=Player->Origin;
  }
 }
 if(!Smoke&&!bEditingTime) {
  const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight,
   EKeys::NumPadOne,EKeys::NumPadTwo,EKeys::NumPadThree,EKeys::NumPadFour,EKeys::NumPadFive,EKeys::NumPadSix,EKeys::NumPadSeven,EKeys::NumPadEight,
   EKeys::Right,EKeys::Left,EKeys::SpaceBar,EKeys::PageDown,EKeys::PageUp,EKeys::Escape,EKeys::O,EKeys::Home,EKeys::P,EKeys::F,EKeys::Enter,EKeys::Y,EKeys::N};
  for(const FKey& Key:Keys)if(PC->WasInputKeyJustPressed(Key))HandleParkKey(Key);
 }
 Travel+=Delta;
 if(bFreeFlight)TickFreeFlight(Delta);
 else if(MapPhase==EMapPhase::ZoomIn||MapPhase==EMapPhase::ZoomOut) {
  const bool In=MapPhase==EMapPhase::ZoomIn;
  const float T=FMath::Clamp(Travel/MapLegDuration,0.f,1.f), Ease=T*T*(3-2*T);
  const FVector Look=In?CameraTargets[Index]:MapCenter;
  const FVector Eye=In?ParkFocusEye():MapEye;
  const FVector A=In?SwoopA[Index]:SwoopB[Index], B=In?SwoopB[Index]:SwoopA[Index];
  const float U=1-Ease;
  const FVector Position=U*U*U*FromPosition+3*U*U*Ease*A+3*U*Ease*Ease*B+Ease*Ease*Ease*Eye;
  CameraLook=FMath::Lerp(FromLook,Look,Ease);
  FRotator Rotation=(CameraLook-Position).Rotation();Rotation.Roll=FMath::Sin(PI*Ease)*(Index%2?2.f:-2.f);
  const float Distance=FVector::Distance(Position,CameraLook);
  CameraFrameWidth=FMath::Exp(FMath::Lerp(FMath::Loge(FromFrameWidth),FMath::Loge(In?ParkFocusWidth():OverviewFrameWidth),Ease));
  Camera->SetActorLocationAndRotation(Position,Rotation);
  Camera->GetCameraComponent()->SetFieldOfView(FMath::RadiansToDegrees(2*FMath::Atan(CameraFrameWidth/(2*Distance))));
  CardExpansion=0; // The flight is unobstructed; slide contents never travel across the map.
  if(T>=1) {
   MapPhase=In?EMapPhase::Arrived:EMapPhase::Overview; bOverview=!In; Travel=0;
   if(!In&&PendingIndex>=0&&Smoke)UE_LOG(LogTemp,Display,TEXT("SlideSmoke: overview between slides=PASS"));
  }
 } else if(MapPhase==EMapPhase::Arrived&&Travel>=.12f&&WalkReview==0) {
  if(Index!=GateSlide){MapPhase=EMapPhase::Expand;Travel=0;}
  else if(IslandScene::GateOpenFraction()>=1){MapPhase=EMapPhase::Expand;Travel=0;}
 } else if(MapPhase==EMapPhase::Expand) {
  const float T=FMath::Clamp(Travel/(Index==GateSlide?.7f:.24f),0.f,1.f);CardExpansion=T*T*(3-2*T);
  if(T>=1){MapPhase=Index==GateSlide?EMapPhase::Slide:EMapPhase::Loading;Travel=0;if(Index==GateSlide)bTimerStarted=true;}
 } else if(MapPhase==EMapPhase::Loading&&Travel>=.32f) {
  MapPhase=EMapPhase::Slide;Travel=0;bTimerStarted=true;
 } else if(MapPhase==EMapPhase::Retract) {
  const float T=FMath::Clamp(Travel/MapLegDuration,0.f,1.f);CardExpansion=FromExpansion*(1-T*T*(3-2*T));
  if(T>=1){CardExpansion=0;BeginMapLeg(false);}
 }
 if(MapPhase==EMapPhase::Overview&&PendingIndex>=0&&Travel>.12f) {
  const int32 Next=PendingIndex;PendingIndex=-1;GoTo(Next);
 }
 Cast<AParkCamera>(Camera)->FrameScene(CameraFrameWidth,FVector::Distance(Camera->GetActorLocation(),CameraLook));
 TickStationPage(Delta);

 const bool AtGate=!bFreeFlight&&Index==GateSlide&&(MapPhase==EMapPhase::Arrived||MapPhase==EMapPhase::Expand||MapPhase==EMapPhase::Loading||MapPhase==EMapPhase::Slide||CardExpansion>0);
 IslandScene::TickGate(Delta,AtGate);
 for(const auto& Base:SignBases)if(Base.IsValid())Base->SetActorHiddenInGame(bFreeFlight);
 for(int32 I=0;I<Panels.Num();I++) {
  auto& Panel=Panels[I];if(!Panel.Root.IsValid())continue;
  const bool GateVisible=I==GateSlide&&!bFreeFlight&&I==Index&&IslandScene::GateOpenFraction()>0;
  const float Rise=I==GateSlide?(GateVisible?1.f:0.f):(!bFreeFlight&&I==Index?CardExpansion:0);
  Panel.Reveal=Rise;
  Panel.Root->SetActorLocation(I==GateSlide?GateScreenPosition(CardExpansion):FMath::Lerp(SignAnchors[I]-FVector(0,0,500),Panel.RaisedPosition,Rise));
  Panel.Root->GetRootComponent()->SetVisibility(Rise>.001f&&!(I==Index&&(UsesPhysicalProp()||(RevealedPage==0&&IsComponentPage()))),true);
  for(int32 M=0;M<Panel.Models.Num();M++)if(Panel.Models[M].IsValid()) {
   const bool Visible=!bFreeFlight&&I==Index&&MapPhase==EMapPhase::Slide&&Panel.ModelSteps[M]<=RevealedPage;
   Panel.Models[M]->SetActorHiddenInGame(!Visible);
   Panel.Models[M]->GetRootComponent()->SetVisibility(Visible,true);
  }
 }
 for(auto& M:Motions)if(M.Actor.IsValid()) {
  if(M.Kind==TEXT("spin"))M.Actor->SetActorRelativeRotation(M.Rotation+FRotator(0,Elapsed*M.Speed,0));
  else M.Actor->SetActorRelativeLocation(M.Origin+FVector(0,0,FMath::Sin(Elapsed*M.Speed)*M.Amplitude));
 }
 if(!Smoke)return;
 if(FParse::Param(FCommandLine::Get(),TEXT("WanderTest"))){if(Elapsed>1)FPlatformMisc::RequestExitWithStatus(false,IslandScene::ValidateWandering()&&IslandScene::ValidateArticulatedGaits()?0:1);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("ProgressTest"))){TestProgress();return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("LoginTest"))){TestLogin();return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("GateTest"))){TestGate();return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("StationContentTest"))){TestStationContent();return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("EndingTest"))){TestEnding();return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("NativePropsTest"))){TestNativeProps();return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("ExchangeTest"))){TestExchange();return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("ScalarTest"))){TestScalar();return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("SerializerTest"))){TestSerializer();return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("FSVTest"))){TestFSV();return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("CastTest"))){TestCast();return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("TerminalTest"))){TestTerminal();return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("FreeFlightTest"))){TestFreeFlight();return;}
 if(WalkReview>0) {
  if(SmokeStep==0&&Elapsed>2){GoTo(FMath::Clamp(WalkReview-1,0,3));SmokeStep=1;}
  if(MapPhase==EMapPhase::Arrived) {
   const int32 Frame=FMath::FloorToInt(Travel*8);
   if(Frame>=SmokeStep-1) {
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/walk-%d-%03d.png"),WalkReview,Frame),true,false);
    SmokeStep=Frame+2;
   }
   if(Travel>16)FPlatformMisc::RequestExit(false);
  }
  return;
 }

 // Waiting on the map, an interrupted approach, and the first loading prompt
 // must not consume presentation time before any slide content is visible.
 if(!bTimerStarted&&(TimerElapsed!=0||(RemainingVisitors()!=3000||RemainingStaff()!=350))) {
  UE_LOG(LogTemp,Error,TEXT("SlideSmoke: countdown started before first viewed slide"));FPlatformMisc::RequestExitWithStatus(false,1);return;
 }
 const bool StableProjection=Camera->GetCameraComponent()->ProjectionMode==ECameraProjectionMode::Perspective;
 if(!StableProjection){UE_LOG(LogTemp,Error,TEXT("SlideSmoke: projection changed"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
 const bool AtArea=Camera->GetActorLocation().Equals(ParkFocusEye(),1.f);
 if((MapPhase==EMapPhase::ZoomIn||MapPhase==EMapPhase::ZoomOut)&&CardExpansion!=0) {
  UE_LOG(LogTemp,Error,TEXT("SlideSmoke: card obstructed flight"));FPlatformMisc::RequestExitWithStatus(false,1);return;
 }
 if((MapPhase==EMapPhase::Expand||MapPhase==EMapPhase::Loading||MapPhase==EMapPhase::Slide)&&!AtArea) {
  UE_LOG(LogTemp,Error,TEXT("SlideSmoke: card opened before arrival"));FPlatformMisc::RequestExitWithStatus(false,1);return;
 }
 if(MapPhase==EMapPhase::Arrived&&Travel==0) {
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/arrival-%02d.png"),Index+1),true,false);
 }
 if(MapPhase==EMapPhase::Loading&&!bLoadingCaptured) {
  bLoadingCaptured=true;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/card-loading.png"),true,false);
 }
 if(!bInitialCaptured&&Elapsed>1) {
  bInitialCaptured=true;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/park-overview.png"),true,false);
 }
 if(SmokeStep==0&&Elapsed>2) {
  if(!IslandScene::ValidateWandering()||!IslandScene::ValidateArticulatedGaits()){FPlatformMisc::RequestExitWithStatus(false,1);return;}
  const bool Seven=Views.Num()==8&&MapPins.Num()==7&&CardTargets.Num()==8&&HiddenSlides.Contains(7)&&CardExpansion==0;
  UE_LOG(LogTemp,Display,TEXT("SlideSmoke: seven numbered map cards=%s"),Seven?TEXT("PASS"):TEXT("FAIL"));
  if(!Seven){FPlatformMisc::RequestExitWithStatus(false,1);return;}
  HandleParkKey(EKeys::Right);SmokeStep=1;
 }
 // Exercise interruption while zooming, then Escape must cancel the queued destination.
 else if(SmokeStep==1&&Travel>.2f) {FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/card-zoom.png"),true,false);HandleParkKey(EKeys::Seven);HandleParkKey(EKeys::Escape);SmokeStep=2;}
 else if(SmokeStep==2&&MapPhase==EMapPhase::ZoomOut&&Travel>MapLegDuration*.9f&&!bFlightTested) {
  bFlightTested=true;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/card-return.png"),true,false);
 }
 else if(SmokeStep==2&&MapPhase==EMapPhase::Overview) {
  const bool Cancelled=PendingIndex==-1&&Camera->GetActorLocation().Equals(MapEye,1.f)&&CardExpansion==0;
  UE_LOG(LogTemp,Display,TEXT("SlideSmoke: interrupted zoom and Escape=%s"),Cancelled?TEXT("PASS"):TEXT("FAIL"));
  if(!Cancelled){FPlatformMisc::RequestExitWithStatus(false,1);return;}
  HandleParkKey(EKeys::One);SmokeStep=3;
 }
 // Arrow departure must remain neutral until a second explicit press.
 else if((SmokeStep==4||SmokeStep==6)&&MapPhase==EMapPhase::Overview&&Travel>.8f) {
  const int32 Previous=SmokeStep==4?0:6;
  const bool Held=PendingIndex==-1&&Index==Previous&&CardExpansion==0&&Camera->GetActorLocation().Equals(MapEye,1.f)&&FMath::IsNearlyEqual(CameraFrameWidth,OverviewFrameWidth,1.f);
  UE_LOG(LogTemp,Display,TEXT("SlideSmoke: %s waits in overview for second press=%s"),SmokeStep==4?TEXT("forward"):TEXT("backward"),Held?TEXT("PASS"):TEXT("FAIL"));
  if(!Held){FPlatformMisc::RequestExitWithStatus(false,1);return;}
  HandleParkKey(SmokeStep==4?EKeys::PageDown:EKeys::PageUp);
 }
 else if(SmokeStep>=3&&SmokeStep<=9&&MapPhase==EMapPhase::Slide&&Travel>.7f) {
  const int32 Expected[]={0,1,6,5,2,3,4};
  const int32 Step=SmokeStep-3;
  if(SmokeStep==3&&!bSmokeCaptured) {
   const float Saved=TimerElapsed;
   bool ClockOK=bTimerStarted&&Saved>0&&Saved<1&&TimerDuration==2100;
   const int32 VisitorsBefore=RemainingVisitors(),StaffBefore=RemainingStaff();
   bTimerPaused=true;TickPresentationClock(10);
   ClockOK&=TimerElapsed==Saved&&RemainingVisitors()==VisitorsBefore&&RemainingStaff()==StaffBefore;
   bTimerPaused=false;
   auto CheckDepartures=[&](const FParkDepartures& Schedule,int32 PerMinute,int32 MaxBatch) {
    bool Good=Schedule.Remaining(0)==Schedule.Initial;
    float Last=0;
    for(const auto& Batch:Schedule.Batches) {
     Good&=Batch.Time>Last&&Batch.Count>=1&&Batch.Count<=MaxBatch;Last=Batch.Time;
    }
    for(int32 Minute=1;Minute<=35;Minute++)Good&=Schedule.Remaining(Minute*60.f)==FMath::Max(0,Schedule.Initial-Minute*PerMinute);
    const float End=Schedule.Initial/PerMinute*60.f;
    Good&=Schedule.Remaining(End-.01f)>0&&Schedule.Remaining(End)==0&&Schedule.Remaining(End+3600)==0;
    return Good;
   };
   ClockOK&=CheckDepartures(VisitorDepartures,100,50)&&CheckDepartures(StaffDepartures,10,10);
   // Check multiple randomized schedules, not just the current launch's seed.
   for(int32 Seed=0;Seed<100;Seed++) {
    FRandomStream Random(Seed);FParkDepartures Visitors,Staff;
    Visitors.Build(3000,100,50,Random);Staff.Build(350,10,10,Random);
    ClockOK&=CheckDepartures(Visitors,100,50)&&CheckDepartures(Staff,10,10);
   }
   TickPresentationClock(1800-Saved);ClockOK&=RemainingVisitors()==0&&RemainingStaff()==50;
   TickPresentationClock(300);ClockOK&=TimerElapsed==2100&&RemainingStaff()==0;
   TickPresentationClock(600);ClockOK&=TimerElapsed==2100&&RemainingVisitors()==0&&RemainingStaff()==0;
   const float SavedElapsed=Elapsed;
   Elapsed=0;const auto Bright=PopulationColor(0).GetSpecifiedColor();
   Elapsed=.5f;const auto Dim=PopulationColor(0).GetSpecifiedColor();
   ClockOK&=Bright.R>Bright.G&&Dim.R>Dim.G&&Bright!=Dim;
   Elapsed=SavedElapsed;TimerElapsed=Saved;
   UE_LOG(LogTemp,Display,TEXT("SlideSmoke: randomized batches, exact minute totals, 30/35-minute endpoints, pause and flashing zero counters=%s"),ClockOK?TEXT("PASS"):TEXT("FAIL"));
   if(!ClockOK){FPlatformMisc::RequestExitWithStatus(false,1);return;}
  }
  const bool Good=Index==Expected[Step]&&CardExpansion==1&&Camera->GetActorLocation().Equals(ParkFocusEye(),1.f)&&Camera->GetActorRotation().Equals((CameraTargets[Index]-ParkFocusEye()).Rotation(),.1f)&&Panels[Index].Reveal==1;
  bool Fits=true;
  const auto* Player=PC->GetLocalPlayer();
  const FIntPoint Screen=Player->ViewportClient->Viewport->GetSizeXY();
  const FVector2D Min=Player->Origin*FVector2D(Screen.X,Screen.Y)+FVector2D(4,4);
  const FVector2D Max=(Player->Origin+Player->Size)*FVector2D(Screen.X,Screen.Y)-FVector2D(4,4);
  auto CheckPoint=[&](FVector Point){FVector2D Pixel;const bool Inside=PC->ProjectWorldLocationToScreen(Point,Pixel)&&Pixel.X>=Min.X&&Pixel.X<=Max.X&&Pixel.Y>=Min.Y&&Pixel.Y<=Max.Y; if(!Inside)UE_LOG(LogTemp,Warning,TEXT("Fit point %s projected %s within %s .. %s"),*Point.ToString(),*Pixel.ToString(),*Min.ToString(),*Max.ToString());Fits&=Inside;};
  for(float Y:{-740.f,740.f})for(float Z:{-470.f,470.f})CheckPoint(Panels[Index].Root->GetActorTransform().TransformPosition(FVector(0,Y,Z)));
  if(!Fits){UE_LOG(LogTemp,Error,TEXT("SlideSmoke: slide clipped on card %d at %dx%d"),Index+1,Screen.X,Screen.Y);FPlatformMisc::RequestExitWithStatus(false,1);return;}
  if(!bSmokeCaptured)UE_LOG(LogTemp,Display,TEXT("SlideSmoke: slide fits card %d at %dx%d=PASS"),Index+1,Screen.X,Screen.Y);
  if(!bSmokeCaptured)UE_LOG(LogTemp,Display,TEXT("SlideSmoke: card %d expanded at stable camera=%s"),Index+1,Good?TEXT("PASS"):TEXT("FAIL"));
  if(!Good){FPlatformMisc::RequestExitWithStatus(false,1);return;}
  if(!bSmokeCaptured){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/slide-%02d.png"),Index+1),true,false);bSmokeCaptured=true;}
  if(Travel<1.f)return;
  const FKey NextKeys[]={EKeys::Right,EKeys::Seven,EKeys::Left,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Escape};
  if(Step==0)PageIndex=StationSteps[0].Num()-1;
  HandleParkKey(NextKeys[Step]);SmokeStep++;bSmokeCaptured=false;
 }
 else if(SmokeStep==10&&MapPhase==EMapPhase::Overview&&Travel>.5f) {
  if(!Camera->GetActorLocation().Equals(MapEye,1.f)||PendingIndex!=-1||CardExpansion!=0){FPlatformMisc::RequestExitWithStatus(false,1);return;}
  UE_LOG(LogTemp,Display,TEXT("SlideSmoke: PASS seven cards, arrival before expansion, delayed content, number/arrow/Escape navigation, ground-level perspective and physical signs"));
  TimerElapsed=TimerDuration;Elapsed=100;Travel=0;SmokeStep=11;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/population-zero-bright.png"),true,false);
 }
 else if(SmokeStep==11&&Travel>.55f) {
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/population-zero-dim.png"),true,false);
  SmokeStep=12;Travel=0;
 }
 else if(SmokeStep==12&&Travel>.2f){HandleParkKey(EKeys::Eight);SmokeStep=13;}
 else if(SmokeStep==13&&MapPhase==EMapPhase::Slide&&Travel>.8f) {
  bool Good=Index==7&&HiddenSlides.Contains(7)&&MapPins.Num()==7&&CardExpansion==1&&Panels[7].Reveal==1;
  Good&=NextVisibleSlide(6,1)==7&&NextVisibleSlide(0,-1)==6;
  Good&=Camera->GetActorLocation().Equals(CameraStops[7],1.f);
  const FVector2D Size=DesktopHUD->GetCachedGeometry().GetLocalSize();int32 W,H;PC->GetViewportSize(W,H);
  for(float Y:{-740.f,740.f})for(float Z:{-470.f,470.f}) {
   FVector2D P;Good&=PC->ProjectWorldLocationToScreen(Panels[7].Root->GetActorTransform().TransformPosition(FVector(0,Y,Z)),P);
   P=P*Size/FVector2D(W,H);Good&=P.X>FMath::Min(280.f,float(Size.X)*.26f)&&P.X<Size.X&&P.Y>72&&P.Y<Size.Y;
  }
  UE_LOG(LogTemp,Display,TEXT("SlideSmoke: hidden eighth beach bar, fitted sign and visible route exclusion=%s"),Good?TEXT("PASS"):TEXT("FAIL"));
  if(!Good){FPlatformMisc::RequestExitWithStatus(false,1);return;}
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/beach-bar-slide.png"),true,false);
  SmokeStep=14;Travel=0;
 } else if(SmokeStep==14&&Travel>.2f){HandleParkKey(EKeys::F);HandleParkKey(EKeys::Eight);SmokeStep=15;}
 else if(SmokeStep==15&&!bFreeTravelling) {
  const bool Good=bFreeFlight&&Index==7&&CardExpansion==0&&Panels[7].Reveal==0&&Camera->GetActorLocation().Equals(CameraStops[7],1.f);
  UE_LOG(LogTemp,Display,TEXT("SlideSmoke: free flight key 8 visits hidden bar without a sign=%s"),Good?TEXT("PASS"):TEXT("FAIL"));
  if(!Good){FPlatformMisc::RequestExitWithStatus(false,1);return;}
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/beach-bar-free-flight.png"),true,false);
  SmokeStep=16;Travel=0;
 } else if(SmokeStep==16&&Travel>.2f){HandleParkKey(EKeys::F);SmokeStep=17;}
 else if(SmokeStep==17&&MapPhase==EMapPhase::Overview&&Travel>.3f)FPlatformMisc::RequestExit(false);
}
