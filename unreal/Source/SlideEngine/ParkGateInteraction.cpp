#include "SlideGameMode.h"
#include "IslandScene.h"
#include "Camera/CameraActor.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void ASlideGameMode::TestGate() {
 auto Check=[](bool OK,const TCHAR* What){
  UE_LOG(LogTemp,Display,TEXT("GateTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));
  if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);return OK;
 };
 const float Open=IslandScene::GateOpenFraction();
 if(!IslandScene::ValidateGatePose()){Check(false,TEXT("leaves rotate symmetrically around fixed hinges"));return;}
 if(SmokeStep==0&&Elapsed>1) {
  if(!Check(Open==0&&GateSlide>=0,TEXT("gate starts closed")))return;
  GoTo(GateSlide);SmokeStep=1;
 } else if(SmokeStep==1&&Open>.45f&&Open<1) {
  if(!Check(MapPhase==EMapPhase::Arrived&&Panels[GateSlide].Root->GetActorLocation().Equals(Panels[GateSlide].RaisedPosition,.01f),TEXT("doors reveal a stationary sign after arrival")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/gate-opening.png"),true,false);SmokeStep=2;
 } else if(SmokeStep==2&&MapPhase==EMapPhase::Expand&&CardExpansion>.1f&&CardExpansion<.8f) {
  if(!Check(Open==1&&Camera->GetActorLocation().Equals(ParkFocusEye(),1)&&Panels[GateSlide].Root->GetActorLocation().Equals(GateScreenPosition(CardExpansion),.1f),TEXT("screen flies through fully open doors while camera stays fixed")))return;
  // The frame clears both pillars and the arch as it crosses the doorway.
  const FVector Crossing=GateScreenPosition((-10500-Panels[GateSlide].RaisedPosition.Y)/(GateScreenPosition(1).Y-Panels[GateSlide].RaisedPosition.Y));
  if(!Check(Crossing.Z-564>200&&Crossing.Z+564<2288&&888<1050,TEXT("flying screen clears the gate opening")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/gate-screen-flying.png"),true,false);SmokeStep=20;
 } else if(SmokeStep==20&&MapPhase==EMapPhase::Slide&&Travel>.3f) {
  auto* PC=GetWorld()->GetFirstPlayerController();auto* Player=PC->GetLocalPlayer();int32 W,H;PC->GetViewportSize(W,H);
  bool Fits=Open==1&&Panels[GateSlide].Root->GetActorLocation().Equals(GateScreenPosition(1),.1f);
  FVector2D Min(W,H),Max(0,0);
  const FVector Eye=Camera->GetActorLocation();
  for(float Y:{-740.f,740.f})for(float Z:{-470.f,470.f}) {
   const FVector P=Panels[GateSlide].Root->GetActorTransform().TransformPosition(FVector(0,Y,Z));
   FVector2D Screen;Fits&=PC->ProjectWorldLocationToScreen(P,Screen);
   Fits&=Screen.X>W*Player->Origin.X&&Screen.X<W&&Screen.Y>H*Player->Origin.Y&&Screen.Y<H;
   Min.X=FMath::Min(Min.X,Screen.X);Min.Y=FMath::Min(Min.Y,Screen.Y);
   Max.X=FMath::Max(Max.X,Screen.X);Max.Y=FMath::Max(Max.Y,Screen.Y);
   Fits&=P.Y<-10500; // The entire panel has passed through to the camera side.
  }
  // The pullback must also retain the raised lettering above the opening.
  for(float X:{-1100.f,1100.f})for(float Z:{2480.f,3460.f}) {
   FVector2D Screen;Fits&=PC->ProjectWorldLocationToScreen(FVector(X,-10750,Z),Screen);
   Fits&=Screen.X>W*Player->Origin.X&&Screen.X<W&&Screen.Y>H*Player->Origin.Y&&Screen.Y<H;
  }
  Fits&=(Max.X-Min.X)/(W*Player->Size.X)>.55f||(Max.Y-Min.Y)/(H*Player->Size.Y)>.7f;
  if(!Check(Fits,TEXT("screen settles close to camera, fits viewport and retains gate lettering framing")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/gate-open.png"),true,false);SmokeStep=3;Travel=0;
 } else if(SmokeStep==3&&Travel>.4f){HandleParkKey(EKeys::Escape);SmokeStep=4;}
 else if(SmokeStep==4&&MapPhase==EMapPhase::Retract&&CardExpansion<.85f&&CardExpansion>.1f) {
  if(!Check(Open==1&&Panels[GateSlide].Root->GetActorLocation().Equals(GateScreenPosition(CardExpansion),.1f),TEXT("screen returns behind the gate before doors close")))return;
  SmokeStep=40;
 }
 else if(SmokeStep==40&&MapPhase==EMapPhase::Overview&&Open==0) {
  if(!Check(Panels[GateSlide].Reveal==0,TEXT("leaving closes the doors and hides the sign")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/gate-closed-overview.png"),true,false);
  GoTo(GateSlide);SmokeStep=5;
 } else if(SmokeStep==5&&Open>.3f) {HandleParkKey(EKeys::One);SmokeStep=6;}
 else if(SmokeStep==6&&Index==0&&MapPhase==EMapPhase::Slide) {
  if(!Check(Open==0,TEXT("number navigation also closes an interrupted opening")))return;
  GoTo(GateSlide);SmokeStep=7;
 } else if(SmokeStep==7&&Index==GateSlide&&MapPhase==EMapPhase::Slide) {HandleParkKey(EKeys::F);SmokeStep=8;}
 else if(SmokeStep==8&&Open==0) {
  if(!Check(bFreeFlight&&Panels[GateSlide].Reveal==0,TEXT("free flight closes the gate and hides its slide")))return;
  FPlatformMisc::RequestExit(false);
 }
}
