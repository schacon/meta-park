#include "SlideGameMode.h"
#include "ParkTerminal.h"
#include "Camera/CameraActor.h"
#include "Components/SceneComponent.h"
#include "Dom/JsonObject.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void ASlideGameMode::TestStationContent() {
 auto Check=[](bool OK,const TCHAR* What){
  UE_LOG(LogTemp,Display,TEXT("StationContentTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));
  if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);return OK;
 };
 if(SmokeStep==0&&Elapsed>1){GoTo(0);SmokeStep=1;}
 else if(SmokeStep==1&&MapPhase==EMapPhase::Slide&&Travel>.2f) {
  if(!Check(StationSteps[0].Num()==4&&PageIndex==0,TEXT("fixture loaded four ordered MDX pages")))return;
  HandleParkKey(EKeys::Right);SmokeStep=2;
 } else if(SmokeStep==2&&PageSwipe>.25f&&PageSwipe<.8f) {
  if(!Check(PageIndex==1&&PreviousPage==0&&MapPhase==EMapPhase::Slide&&Camera->GetActorLocation().Equals(ParkFocusEye(),1)&&Panels[0].Root->GetActorLocation().Equals(Panels[0].RaisedPosition,1),TEXT("Markdown swipe keeps sign and camera fixed")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/station-swipe.png"),true,false);
  HandleParkKey(EKeys::Right);
  if(!Check(PageIndex==1,TEXT("key repeat during swipe cannot skip a page")))return;
  SmokeStep=3;
 } else if(SmokeStep==3&&PageSwipe==1) {
  HandleParkKey(EKeys::Left);SmokeStep=4;
 } else if(SmokeStep==4&&PageSwipe==1) {
  if(!Check(PageIndex==0&&PageDirection==-1,TEXT("backward swipe restores preceding page")))return;
  HandleParkKey(EKeys::Right);SmokeStep=5;
 } else if(SmokeStep==5&&PageSwipe==1) {HandleParkKey(EKeys::Right);SmokeStep=6;}
 else if(SmokeStep==6&&Terminal.IsValid()&&Terminal->OutputReady()) {
  if(!Check(PageIndex==2&&Terminal->Command()==TEXT("git meta get owner")&&!Panels[0].Root->GetRootComponent()->IsVisible(),TEXT("authored component prompt plays with physical sign hidden")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/station-custom-computer.png"),true,false);
  HandleParkKey(EKeys::Right);SmokeStep=7;
 } else if(SmokeStep==7&&Terminal->IsHidden()) {
  if(!Check(PageIndex==3&&Panels[0].Root->GetRootComponent()->IsVisible()&&MapPhase==EMapPhase::Slide,TEXT("next Markdown page restores sign as terminal retracts")))return;
  HandleParkKey(EKeys::Right);SmokeStep=8;
 } else if(SmokeStep==8&&MapPhase==EMapPhase::Overview&&Travel>.6f) {
  if(!Check(Index==0&&PendingIndex==-1,TEXT("end of station waits in overview")))return;
  HandleParkKey(EKeys::Right);SmokeStep=9;
 } else if(SmokeStep==9&&MapPhase==EMapPhase::Slide) {
  if(!Check(Index==1&&PageIndex==0,TEXT("second press enters next station at its first page")))return;
  HandleParkKey(EKeys::One);SmokeStep=10;
 } else if(SmokeStep==10&&Index==0&&MapPhase==EMapPhase::Slide) {
  if(!Check(PageIndex==0&&Terminal->IsHidden(),TEXT("number selection resets a station sequence")))return;
  FPlatformMisc::RequestExit(false);
 }
}
