#include "SlideGameMode.h"
#include "ParkTerminal.h"
#include "Camera/CameraActor.h"
#include "Components/SceneComponent.h"
#include "Dom/JsonObject.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void ASlideGameMode::TestBrowser() {
 auto Check=[](bool OK,const TCHAR* What){UE_LOG(LogTemp,Display,TEXT("BrowserTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);return OK;};
 if(SmokeStep==0&&Elapsed>1){GoTo(3);SmokeStep=1;}
 else if(SmokeStep==1&&MapPhase==EMapPhase::Slide&&Travel>1&&Terminal.IsValid()&&Terminal->IsRaised()) {
  if(!Check(PageIndex==0&&Terminal->IsBrowser()&&Terminal->Url()==TEXT("https://git-meta.com")&&Terminal->ScreenFillsViewport()&&!Panels[Index].Root->GetRootComponent()->IsVisible(),TEXT("intro shows browser screenshot on the physical computer")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/computer-browser.png"),true,false);
  Travel=0;SmokeStep=2;
 } else if(SmokeStep==2&&Travel>.6f){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=3;}
 else if(SmokeStep==3&&Travel>.6f) {
  if(!Check(PageIndex==1&&bCommandDesktop&&Terminal->IsHidden(),TEXT("next page opens recording and hides browser model")))return;
  HandleParkKey(EKeys::Left);Travel=0;SmokeStep=4;
 } else if(SmokeStep==4&&Travel>1&&Terminal->IsRaised()) {
  if(!Check(PageIndex==0&&!bCommandDesktop&&Terminal->IsBrowser(),TEXT("Left restores browser screenshot")))return;
  GoTo(0);Travel=0;SmokeStep=5;
 } else if(SmokeStep==5&&Index==0&&MapPhase==EMapPhase::Slide) {
  if(!Check(Terminal->IsHidden(),TEXT("leaving the station hides the computer")))return;
  FPlatformMisc::RequestExit(false);
 }
}

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
  SmokeStep=3;Travel=0;
 } else if(SmokeStep==3&&PageSwipe==1&&Travel>.8f) {
  if(!Check(StationSignPage(0,PageIndex)==1,TEXT("next slide replaces the old slide across the entire sign")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/station-replaced.png"),true,false);
  SmokeStep=30;Travel=0;
 } else if(SmokeStep==30&&Travel>.2f) {
  HandleParkKey(EKeys::Left);SmokeStep=4;
 } else if(SmokeStep==4&&PageSwipe==1) {
  if(!Check(PageIndex==0&&PageDirection==-1&&StationSignPage(0,PageIndex)==0,TEXT("backward navigation restores only the previous full-width slide")))return;
  HandleParkKey(EKeys::Right);SmokeStep=5;
 } else if(SmokeStep==5&&PageSwipe==1) {HandleParkKey(EKeys::Right);SmokeStep=6;}
 else if(SmokeStep==6&&Terminal.IsValid()&&Terminal->OutputReady()) {
  if(!Check(PageIndex==2&&Terminal->Command()==TEXT("git meta get owner")&&Panels[0].Root->GetRootComponent()->IsVisible()&&StationSignPage(0,PageIndex)==1&&Terminal->ScreenFillsViewport(.56f),TEXT("authored computer fills the foreground with preceding Markdown retained behind")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/station-custom-computer.png"),true,false);
  HandleParkKey(EKeys::Right);SmokeStep=7;
 } else if(SmokeStep==7&&PageSwipe==1) {
  if(!Check(PageIndex==3&&StationSignPage(0,PageIndex)==3&&Terminal->IsRaised()&&Panels[0].Root->GetRootComponent()->IsVisible()&&MapPhase==EMapPhase::Slide,TEXT("new Markdown replaces the sign after a native component")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/station-four-pages.png"),true,false);
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
