#include "SlideGameMode.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "TimerManager.h"
#include "Engine/World.h"
void ASlideGameMode::TestEnding() {
 auto Check=[](bool OK,const TCHAR* What){UE_LOG(LogTemp,Display,TEXT("EndingTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);return OK;};
 auto Capture=[this](const TCHAR* Name){FTimerHandle Handle;const FString Path=FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name;GetWorld()->GetTimerManager().SetTimer(Handle,[Path]{FScreenshotRequest::RequestScreenshot(Path,true,false);},.35f,false);};
 if(SmokeStep==0&&Elapsed>1){
  if(!Check(MapPins[1].Code==TEXT("HELI")&&MapPins[6].Code==TEXT("ENC-01"),TEXT("helipad second and brontosaurus seventh")))return;
  GoTo(6);SmokeStep=1;
 } else if(SmokeStep==1&&MapPhase==EMapPhase::Slide){HandleParkKey(EKeys::Right);SmokeStep=2;}
 else if(SmokeStep==2&&MapPhase==EMapPhase::Overview){
  if(!Check(Index==6&&PendingIndex==-1,TEXT("seventh station returns to overview first")))return;
  HandleParkKey(EKeys::Right);SmokeStep=3;
 } else if(SmokeStep==3&&MapPhase==EMapPhase::Slide){
  if(!Check(Index==7&&HiddenSlides.Contains(7),TEXT("forward enters hidden eighth station")))return;
  HandleParkKey(EKeys::Right);
  if(!Check(bConfirmFinish&&Index==7,TEXT("last forward prompts logout without wrapping")))return;
  Capture(TEXT("ending-logout.png"));Travel=0;SmokeStep=4;
 } else if(SmokeStep==4&&Travel>.8f){
  HandleParkKey(EKeys::N);if(!Check(!bConfirmFinish&&Index==7,TEXT("No stays at final station")))return;
  HandleParkKey(EKeys::Right);HandleParkKey(EKeys::Enter);SmokeStep=5;
 } else if(SmokeStep==5&&DesktopMinimize==1){
  if(!Check(bQuestions&&bCommandDesktop&&!bDesktopFSV&&!bLocked&&bTimerPaused,TEXT("Yes minimizes to questions terminal and pauses timer")))return;
  HandleParkKey(EKeys::One);if(!Check(Index==7,TEXT("questions ignores presentation navigation")))return;
  Capture(TEXT("ending-questions.png"));Travel=0;SmokeStep=6;
 } else if(SmokeStep==6&&Travel>1){
  RestoreParkWindow();SmokeStep=7;
 } else if(SmokeStep==7&&DesktopMinimize==0){
  if(!Check(!bQuestions&&!bCommandDesktop&&Index==7,TEXT("dock restores final station")))return;
  HandleParkKey(EKeys::Right);HandleParkKey(EKeys::Y);SmokeStep=8;
 } else if(SmokeStep==8&&DesktopMinimize==1){
  HandleParkKey(EKeys::Escape);
  if(!Check(bLocked&&!bQuestions&&!bConfirmFinish&&TimerElapsed==0&&PowerPercent()==100,TEXT("Escape returns to login and resets session")))return;
  UE_LOG(LogTemp,Display,TEXT("EndingTest: PASS"));FPlatformMisc::RequestExitWithStatus(false,0);
 }
}
