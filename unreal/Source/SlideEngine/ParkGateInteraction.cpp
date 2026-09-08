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
 } else if(SmokeStep==2&&MapPhase==EMapPhase::Slide) {
  auto* PC=GetWorld()->GetFirstPlayerController();auto* Player=PC->GetLocalPlayer();int32 W,H;PC->GetViewportSize(W,H);
  bool Fits=Open==1;
  const FVector Eye=Camera->GetActorLocation();
  for(float Y:{-740.f,740.f})for(float Z:{-470.f,470.f}) {
   const FVector P=Panels[GateSlide].Root->GetActorTransform().TransformPosition(FVector(0,Y,Z));
   FVector2D Screen;Fits&=PC->ProjectWorldLocationToScreen(P,Screen);
   Fits&=Screen.X>W*Player->Origin.X&&Screen.X<W&&Screen.Y>H*Player->Origin.Y&&Screen.Y<H;
   // The sight line passes through the actual opening below the arch.
   const FVector Portal=FMath::Lerp(Eye,P,(-10500-Eye.Y)/(P.Y-Eye.Y));
   Fits&=P.Y>-10500&&FMath::Abs(Portal.X)<1050&&Portal.Z>200&&Portal.Z<2288;
  }
  if(!Check(Fits,TEXT("open doorway frames the sign behind the gate")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/gate-open.png"),true,false);SmokeStep=3;Travel=0;
 } else if(SmokeStep==3&&Travel>.4f){HandleParkKey(EKeys::Escape);SmokeStep=4;}
 else if(SmokeStep==4&&MapPhase==EMapPhase::Overview&&Open==0) {
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
