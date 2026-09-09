#include "SlideGameMode.h"
#include "ParkTerminal.h"
#include "Camera/CameraActor.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"
#include "UnrealClient.h"

void ASlideGameMode::TestTerminal() {
 auto Check=[](bool OK,const TCHAR* What){
  UE_LOG(LogTemp,Display,TEXT("TerminalTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));
  if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);return OK;
 };
 auto Capture=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name,true,false);};
 if(SmokeStep==0&&Elapsed>1){GoTo(0);SmokeStep=1;}
 else if(SmokeStep==1&&MapPhase==EMapPhase::Slide){HandleParkKey(EKeys::Right);SmokeStep=2;}
 else if(SmokeStep==2&&Terminal.IsValid()&&Terminal->IsRaised()&&Terminal->Command().Len()>=4) {
  if(!Check(Index==0&&MapPhase==EMapPhase::Slide&&Terminal->Command().Len()<12&&!Terminal->OutputReady()&&Terminal->ScreenFillsViewport(.56f),TEXT("large foreground computer types the authored command incrementally")))return;
  Capture(TEXT("terminal-typing.png"));SmokeStep=3;
 } else if(SmokeStep==3&&Terminal->OutputReady()) {
  if(!Check(Terminal->Command()==TEXT("git meta set")&&Camera->GetActorLocation().Equals(ParkFocusEye(),1.f),TEXT("command completes before OK with camera unchanged")))return;
  Capture(TEXT("terminal-ok.png"));SmokeStep=4;Travel=0;
 } else if(SmokeStep==4&&Travel>.3f){HandleParkKey(EKeys::Right);SmokeStep=50;Travel=0;}
 else if(SmokeStep==50&&DesktopMinimize>.2f&&DesktopMinimize<.9f) {
  if(!Check(ParkSnapshot!=nullptr&&bCommandDesktop&&!bLocked&&PageIndex==2&&TimerElapsed>0,TEXT("map snapshot minimizes without logging out or resetting presentation")))return;
  Capture(TEXT("desktop-minimizing.png"));SmokeStep=51;
 } else if(SmokeStep==51&&DesktopMinimize==1&&DesktopCommand().Len()>=3) {
  if(!Check(DesktopCommand().Len()<DesktopPrompt.Len()&&!DesktopOutputReady()&&LoginHUD->GetVisibility()==EVisibility::Visible&&DesktopHUD->GetVisibility()==EVisibility::Hidden,TEXT("desktop terminal types only after map reaches dock")))return;
  Capture(TEXT("desktop-command-typing.png"));SmokeStep=52;
 } else if(SmokeStep==52&&DesktopOutputReady()) {
  if(!Check(DesktopCommand()==TEXT("git meta set")&&DesktopOutput==TEXT("OK")&&RevealedPage==2&&Camera->GetActorLocation().Equals(ParkFocusEye(),1.f),TEXT("CommandLine uses MDX contents and preserves the scene")))return;
  Capture(TEXT("desktop-command-ok.png"));SmokeStep=53;Travel=0;
 } else if(SmokeStep==53&&Travel>.3f) {
  MapDockButton->SimulateClick();SmokeStep=54;
 } else if(SmokeStep==54&&DesktopMinimize>.2f&&DesktopMinimize<.9f) {
  if(!Check(!bCommandDesktop&&PageIndex==1,TEXT("dock restores the preceding presentation step")))return;
  Capture(TEXT("desktop-maximizing.png"));SmokeStep=55;
 } else if(SmokeStep==55&&DesktopMinimize==0) {
  if(!Check(LoginHUD->GetVisibility()==EVisibility::Collapsed&&DesktopHUD->GetVisibility()==EVisibility::Visible&&Terminal->IsRaised()&&Terminal->OutputReady(),TEXT("maximized map retains completed foreground computer")))return;
  HandleParkKey(EKeys::Right);SmokeStep=56;
 } else if(SmokeStep==56&&DesktopMinimize==1) {
  if(!Check(DesktopOutputReady(),TEXT("revisiting CommandLine keeps its completed output")))return;
  FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Right,FModifierKeysState(),0,false,0,0));
  FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(EKeys::Right,FModifierKeysState(),0,false,0,0));SmokeStep=57;
 } else if(SmokeStep==57&&MapPhase==EMapPhase::Overview&&DesktopMinimize==0) {
  if(!Check(Terminal->IsHidden()&&!bLocked&&TimerElapsed>0&&LoginHUD->GetVisibility()==EVisibility::Collapsed,TEXT("Slate Right from CommandLine restores the map and returns to overview")))return;
  HandleParkKey(EKeys::One);SmokeStep=58;
 } else if(SmokeStep==58&&MapPhase==EMapPhase::Slide) {
  HandleParkKey(EKeys::Right);SmokeStep=59;
 } else if(SmokeStep==59&&PageSwipe==1) {
  HandleParkKey(EKeys::Right);SmokeStep=60;
 } else if(SmokeStep==60&&DesktopMinimize==1) {
  LogoutToLogin();
  if(!Check(bLocked&&!bCommandDesktop&&DesktopMinimize==0&&TimerElapsed==0&&PageIndex==0&&LoginHUD->GetVisibility()==EVisibility::Visible,TEXT("actual logout removes command desktop and resets the session")))return;
  FPlatformMisc::RequestExit(false);
 }
}
