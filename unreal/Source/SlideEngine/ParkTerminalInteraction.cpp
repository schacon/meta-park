#include "SlideGameMode.h"
#include "ParkTerminal.h"
#include "Camera/CameraActor.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void ASlideGameMode::TestTerminal() {
 auto Check=[](bool OK,const TCHAR* What){
  UE_LOG(LogTemp,Display,TEXT("TerminalTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));
  if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);return OK;
 };
 if(SmokeStep==0&&Elapsed>1){GoTo(0);SmokeStep=1;}
 else if(SmokeStep==1&&MapPhase==EMapPhase::Slide){HandleParkKey(EKeys::Right);SmokeStep=2;}
 else if(SmokeStep==2&&Terminal.IsValid()&&Terminal->IsRaised()&&Terminal->Command().Len()>=4) {
  if(!Check(Index==0&&MapPhase==EMapPhase::Slide&&Terminal->Command().Len()<12&&!Terminal->OutputReady()&&Terminal->ScreenFillsViewport(),TEXT("Right advances to MDX computer, raises a close-framed display and types incrementally")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/terminal-typing.png"),true,false);SmokeStep=3;
 } else if(SmokeStep==3&&Terminal->OutputReady()) {
  if(!Check(Terminal->Command()==TEXT("git meta set")&&Camera->GetActorLocation().Equals(ParkFocusEye(),1.f),TEXT("command completes before OK with camera unchanged")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/terminal-ok.png"),true,false);SmokeStep=4;Travel=0;
 } else if(SmokeStep==4&&Travel>.25f){HandleParkKey(EKeys::Left);SmokeStep=5;}
 else if(SmokeStep==5&&Terminal->IsHidden()) {
  if(!Check(Index==0&&MapPhase==EMapPhase::Slide,TEXT("Left returns to the first MDX page and retracts the computer")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/terminal-hidden.png"),true,false);SmokeStep=6;Travel=0;
 } else if(SmokeStep==6&&Travel>.25f){HandleParkKey(EKeys::Right);SmokeStep=7;}
 else if(SmokeStep==7&&Terminal.IsValid()&&Terminal->IsRaised()) {
  if(!Check(Terminal->Command().IsEmpty()&&!Terminal->OutputReady(),TEXT("reopening restarts typing")))return;
  HandleParkKey(EKeys::Two);SmokeStep=8;
 } else if(SmokeStep==8&&Index==1&&MapPhase==EMapPhase::Slide) {
  if(!Check(Terminal->IsHidden(),TEXT("leaving the first area hides the computer")))return;
  HandleParkKey(EKeys::Right);SmokeStep=9;
 } else if(SmokeStep==9&&MapPhase==EMapPhase::Overview) {
  if(!Check(Terminal->IsHidden(),TEXT("Right at the end returns to overview")))return;
  FPlatformMisc::RequestExit(false);
 }
}
