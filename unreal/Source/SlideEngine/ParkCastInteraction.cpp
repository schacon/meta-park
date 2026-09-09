#include "SlideGameMode.h"
#include "ParkCastPlayer.h"
#include "Dom/JsonObject.h"
#include "Widgets/Input/SButton.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
void ASlideGameMode::TestCast() {
 auto Check=[](bool OK,const TCHAR* What){UE_LOG(LogTemp,Display,TEXT("CastTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);return OK;};
 if(SmokeStep==0&&Elapsed>1){GoTo(0);SmokeStep=1;}
 else if(SmokeStep==1&&MapPhase==EMapPhase::Slide) {
  int32 Page=INDEX_NONE;for(int32 I=0;I<StationSteps[0].Num();I++){const TSharedPtr<FJsonObject>* Component;if(StationSteps[0][I]->TryGetObjectField(TEXT("component"),Component)&&(*Component)->GetStringField(TEXT("type"))==TEXT("CommandLine")){Page=I;break;}}
  if(!Check(Page>=0,TEXT("authored first station has a recording")))return;
  PageIndex=Page;RevealedPage=Page;ActiveComponentPage=INDEX_NONE;SmokeStep=2;
 } else if(SmokeStep==2&&DesktopMinimize==1&&CastPlayer.IsValid()&&CastPlayer->Time>.5f) {
  if(!Check(!CastPlayer->PlainText().TrimStartAndEnd().IsEmpty()&&CastPlayer->Columns==StationSteps[Index][PageIndex]->GetObjectField(TEXT("component"))->GetObjectField(TEXT("cast"))->GetIntegerField(TEXT("cols"))&&CastPlayer->Rows==StationSteps[Index][PageIndex]->GetObjectField(TEXT("component"))->GetObjectField(TEXT("cast"))->GetIntegerField(TEXT("rows")),TEXT("current authored cast plays with recorded terminal dimensions")))return;
  CastPauseButton->SimulateClick();Travel=CastPlayer->Time;SmokeStep=3;
 } else if(SmokeStep==3) {
  const float Before=CastPlayer->Time;CastPlayer->Tick(3);
  if(!Check(CastPlayer->Paused&&CastPlayer->Time==Before,TEXT("Pause button freezes recording time")))return;
  CastPlayer->Seek(12);TSet<FLinearColor> Colors;for(const auto& Line:CastPlayer->Lines)for(const auto& Run:Line)Colors.Add(Run.Foreground);
  if(!Check(Colors.Num()>3&&CastPlayer->Time==12&&!CastPlayer->PlainText().Contains(TEXT("\x1b")),TEXT("seek rebuilds ANSI-colored terminal cells without escape-code text")))return;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/cast-vim.png"),true,false);SmokeStep=4;Travel=0;
 } else if(SmokeStep==4&&Travel>.3f) {
  CastPlayer->Seek(CastPlayer->Duration);const FString Final=CastPlayer->PlainText();
  CastPlayer->Seek(0);CastPlayer->Seek(CastPlayer->Duration);
  if(!Check(Final==CastPlayer->PlainText(),TEXT("backward and forward seeking reconstructs the same terminal screen")))return;
  CastReplayButton->SimulateClick();
  if(!Check(CastPlayer->Time==0&&!CastPlayer->Paused,TEXT("Replay button restarts the recording")))return;
  HandleParkKey(EKeys::Escape);SmokeStep=5;
 } else if(SmokeStep==5&&MapPhase==EMapPhase::Overview&&DesktopMinimize==0) {
  if(!Check(!bCommandDesktop&&!bLocked&&TimerElapsed>0,TEXT("Escape maximizes the map without resetting its timer")))return;
  FPlatformMisc::RequestExit(false);
 }
}
