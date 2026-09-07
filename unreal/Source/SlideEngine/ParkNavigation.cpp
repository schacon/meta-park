#include "SlideGameMode.h"
#include "IslandScene.h"
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

float ASlideGameMode::ParkFocusWidth() const { return 3100.f*FVector::Distance(CameraStops[Index],CameraTargets[Index])/FVector::Distance(CameraStops[Index],Panels[Index].RaisedPosition); }
FVector ASlideGameMode::ParkFocusEye() const { return CameraStops[Index]; }
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
 if(MapPhase==EMapPhase::Retract)MapLegDuration=.16f;
 if(ZoomIn) { bTourStarted=true; PendingIndex=-1; CardExpansion=0; }
 SetMouseMode(false);
}
void ASlideGameMode::HandleParkKey(const FKey& Key) {
 static const FKey Numbers[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven};
 static const FKey Numpad[]={EKeys::NumPadOne,EKeys::NumPadTwo,EKeys::NumPadThree,EKeys::NumPadFour,EKeys::NumPadFive,EKeys::NumPadSix,EKeys::NumPadSeven};
 for(int32 I=0;I<7&&I<Views.Num();I++)if(Key==Numbers[I]||Key==Numpad[I]){GoTo(I);return;}
 const int32 Current=PendingIndex>=0?PendingIndex:Index;
 if(Key==EKeys::Right||Key==EKeys::SpaceBar||Key==EKeys::PageDown)GoTo(!bTourStarted?0:Current+1);
 else if(Key==EKeys::Left||Key==EKeys::PageUp)GoTo(Current-1);
 else if(Key==EKeys::Escape||Key==EKeys::O)Overview();
 else if(Key==EKeys::Home)GoTo(0);
 else if(Key==EKeys::P)bTimerPaused=!bTimerPaused;
 else if(Key==EKeys::N&&GEngine)GEngine->AddOnScreenDebugMessage(42,20,FColor::Cyan,Notes[Index].IsEmpty()?TEXT("No speaker notes for this slide."):Notes[Index]);
}
void ASlideGameMode::TickParkNavigation(float Delta) {
 const bool Smoke=FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest"));
 auto* PC=GetWorld()->GetFirstPlayerController();
 // Slate geometry already includes display scaling. Reserve exactly the same
 // fraction of the viewport for the sidebar and title bar, on every frame.
 if(DesktopHUD.IsValid())if(auto* Player=PC->GetLocalPlayer()) {
  const FVector2D Size=DesktopHUD->GetCachedGeometry().GetLocalSize();
  if(Size.X>0&&Size.Y>0) {
   Player->Origin=FVector2D(FMath::Min(350.f,float(Size.X)*.3f)/Size.X,FMath::Min(72.f,float(Size.Y)*.15f)/Size.Y);
   Player->Size=FVector2D(1,1)-Player->Origin;
  }
 }
 if(!Smoke) {
  const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,
   EKeys::NumPadOne,EKeys::NumPadTwo,EKeys::NumPadThree,EKeys::NumPadFour,EKeys::NumPadFive,EKeys::NumPadSix,EKeys::NumPadSeven,
   EKeys::Right,EKeys::Left,EKeys::SpaceBar,EKeys::PageDown,EKeys::PageUp,EKeys::Escape,EKeys::O,EKeys::Home,EKeys::P,EKeys::N};
  for(const FKey& Key:Keys)if(PC->WasInputKeyJustPressed(Key))HandleParkKey(Key);
 }
 Travel+=Delta;
 if(MapPhase==EMapPhase::ZoomIn||MapPhase==EMapPhase::ZoomOut) {
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
  CameraFrameWidth=FMath::Exp(FMath::Lerp(FMath::Loge(FromFrameWidth),FMath::Loge(In?ParkFocusWidth():40000.f),Ease));
  Camera->SetActorLocationAndRotation(Position,Rotation);
  Camera->GetCameraComponent()->SetFieldOfView(FMath::RadiansToDegrees(2*FMath::Atan(CameraFrameWidth/(2*Distance))));
  CardExpansion=0; // The flight is unobstructed; slide contents never travel across the map.
  if(T>=1) {
   MapPhase=In?EMapPhase::Arrived:EMapPhase::Overview; bOverview=!In; Travel=0;
   if(!In&&PendingIndex>=0&&Smoke)UE_LOG(LogTemp,Display,TEXT("SlideSmoke: overview between slides=PASS"));
  }
 } else if(MapPhase==EMapPhase::Arrived&&Travel>=.12f) {
  MapPhase=EMapPhase::Expand;Travel=0;
 } else if(MapPhase==EMapPhase::Expand) {
  const float T=FMath::Clamp(Travel/.24f,0.f,1.f);CardExpansion=T*T*(3-2*T);
  if(T>=1){MapPhase=EMapPhase::Loading;Travel=0;}
 } else if(MapPhase==EMapPhase::Loading&&Travel>=.32f) {
  MapPhase=EMapPhase::Slide;Travel=0;
 } else if(MapPhase==EMapPhase::Retract) {
  const float T=FMath::Clamp(Travel/MapLegDuration,0.f,1.f);CardExpansion=FromExpansion*(1-T*T*(3-2*T));
  if(T>=1){CardExpansion=0;BeginMapLeg(false);}
 }
 if(MapPhase==EMapPhase::Overview&&PendingIndex>=0&&Travel>.12f) {
  const int32 Next=PendingIndex;PendingIndex=-1;GoTo(Next);
 }
 for(int32 I=0;I<Panels.Num();I++) {
  auto& Panel=Panels[I];if(!Panel.Root.IsValid())continue;
  const float Rise=I==Index?CardExpansion:0;
  Panel.Reveal=Rise;
  Panel.Root->SetActorLocation(FMath::Lerp(SignAnchors[I]-FVector(0,0,500),Panel.RaisedPosition,Rise));
  Panel.Root->GetRootComponent()->SetVisibility(Rise>.001f,true);
  for(const auto& Model:Panel.Models)if(Model.IsValid())Model->SetActorHiddenInGame(I!=Index||MapPhase!=EMapPhase::Slide);
 }
 for(auto& M:Motions)if(M.Actor.IsValid()) {
  if(M.Kind==TEXT("spin"))M.Actor->SetActorRelativeRotation(M.Rotation+FRotator(0,Elapsed*M.Speed,0));
  else M.Actor->SetActorRelativeLocation(M.Origin+FVector(0,0,FMath::Sin(Elapsed*M.Speed)*M.Amplitude));
 }
 if(!Smoke)return;
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
  const bool Seven=Views.Num()==7&&MapPins.Num()==7&&CardTargets.Num()==7&&CardExpansion==0;
  UE_LOG(LogTemp,Display,TEXT("SlideSmoke: seven numbered map cards=%s"),Seven?TEXT("PASS"):TEXT("FAIL"));
  if(!Seven){FPlatformMisc::RequestExitWithStatus(false,1);return;}
  HandleParkKey(EKeys::One);SmokeStep=1;
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
 else if(SmokeStep>=3&&SmokeStep<=9&&MapPhase==EMapPhase::Slide&&Travel>.7f) {
  const int32 Expected[]={0,1,6,5,2,3,4};
  const int32 Step=SmokeStep-3;
  const bool Good=Index==Expected[Step]&&CardExpansion==1&&Camera->GetActorLocation().Equals(ParkFocusEye(),1.f)&&Camera->GetActorRotation().Equals((CameraTargets[Index]-ParkFocusEye()).Rotation(),.1f)&&Panels[Index].Reveal==1;
  bool Fits=true;
  const auto* Player=PC->GetLocalPlayer();
  const FIntPoint Screen=Player->ViewportClient->Viewport->GetSizeXY();
  const FVector2D Min=Player->Origin*FVector2D(Screen.X,Screen.Y)+FVector2D(4,4);
  const FVector2D Max=(Player->Origin+Player->Size)*FVector2D(Screen.X,Screen.Y)-FVector2D(4,4);
  auto CheckPoint=[&](FVector Point){FVector2D Pixel;Fits&=PC->ProjectWorldLocationToScreen(Point,Pixel)&&Pixel.X>=Min.X&&Pixel.X<=Max.X&&Pixel.Y>=Min.Y&&Pixel.Y<=Max.Y;};
  for(float Y:{-740.f,740.f})for(float Z:{-470.f,470.f})CheckPoint(Panels[Index].Root->GetActorTransform().TransformPosition(FVector(0,Y,Z)));
  for(const auto& Model:Panels[Index].Models)if(Model.IsValid()) {
   FVector Center,Extent;Model->GetActorBounds(false,Center,Extent,true);
   for(float X:{-1.f,1.f})for(float Y:{-1.f,1.f})for(float Z:{-1.f,1.f})CheckPoint(Center+Extent*FVector(X,Y,Z));
  }
  if(!Fits){UE_LOG(LogTemp,Error,TEXT("SlideSmoke: slide/model clipped on card %d at %dx%d"),Index+1,Screen.X,Screen.Y);FPlatformMisc::RequestExitWithStatus(false,1);return;}
  if(!bSmokeCaptured)UE_LOG(LogTemp,Display,TEXT("SlideSmoke: slide/model fits card %d at %dx%d=PASS"),Index+1,Screen.X,Screen.Y);
  if(!bSmokeCaptured)UE_LOG(LogTemp,Display,TEXT("SlideSmoke: card %d expanded at stable camera=%s"),Index+1,Good?TEXT("PASS"):TEXT("FAIL"));
  if(!Good){FPlatformMisc::RequestExitWithStatus(false,1);return;}
  if(!bSmokeCaptured){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/slide-%02d.png"),Index+1),true,false);bSmokeCaptured=true;}
  if(Travel<1.f)return;
  const FKey NextKeys[]={EKeys::Right,EKeys::Seven,EKeys::Left,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Escape};
  HandleParkKey(NextKeys[Step]);SmokeStep++;bSmokeCaptured=false;
 }
 else if(SmokeStep==10&&MapPhase==EMapPhase::Overview&&Travel>.5f) {
  if(!Camera->GetActorLocation().Equals(MapEye,1.f)||PendingIndex!=-1||CardExpansion!=0){FPlatformMisc::RequestExitWithStatus(false,1);return;}
  UE_LOG(LogTemp,Display,TEXT("SlideSmoke: PASS seven cards, arrival before expansion, delayed content, number/arrow/Escape navigation, ground-level perspective and physical signs"));
  FPlatformMisc::RequestExit(false);
 }
}
