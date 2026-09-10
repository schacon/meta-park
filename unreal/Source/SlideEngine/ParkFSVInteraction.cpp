#include "SlideGameMode.h"
#include "ParkFSV.h"
#include "Dom/JsonObject.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
void ASlideGameMode::TestFSV() {
 auto Check=[](bool OK,const TCHAR* What){UE_LOG(LogTemp,Display,TEXT("FSVTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);return OK;};
 auto Key=[](FKey K){FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(K,FModifierKeysState(),0,false,0,0));FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(K,FModifierKeysState(),0,false,0,0));};
 auto Capture=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name,true,false);};
 if(SmokeStep==0&&Elapsed>1) {
  if(!Check(GateSlide==0,TEXT("station 01 is the main gate")))return;GoTo(1);SmokeStep=1;
 } else if(SmokeStep==1&&MapPhase==EMapPhase::Slide) {
  bool TitleOnly=false;StationSteps[Index][0]->TryGetBoolField(TEXT("titleOnly"),TitleOnly);
  if(!Check(TitleOnly,TEXT("single h1 uses the full sign layout")))return;
  Capture(TEXT("fsv-title-only.png"));Travel=0;SmokeStep=2;
 } else if(SmokeStep==2&&Travel>.4f){HandleParkKey(EKeys::Right);SmokeStep=3;}
 else if(SmokeStep==3&&DesktopMinimize==1&&FSV.IsValid()) {
  if(!Check(bDesktopFSV&&!bLocked&&FSV->Selected==-1&&FSV->Systems.Num()==7&&FSV->Title==TEXT("New Metadata Use Cases"),TEXT("FSV minimizes to the desktop with authored title and seven flat systems")))return;
  Capture(TEXT("fsv-overview.png"));Travel=0;SmokeStep=4;
 } else if(SmokeStep==4&&Travel>.4f){Key(EKeys::Right);Travel=0;SmokeStep=5;}
 else if(SmokeStep==5&&Travel>.65f) {
  if(!Check(FSV->Selected==0&&FSV->Heights[0]==1&&FSV->Systems[0].Meta==TEXT("identity, signoffs, attestations")&&PageIndex==1,TEXT("Right raises trust and reveals its authored metadata without leaving FSV")))return;
  Capture(TEXT("fsv-trust.png"));Travel=0;SmokeStep=50;
 } else if(SmokeStep==50&&Travel>.3f){Key(EKeys::Right);Travel=0;SmokeStep=6;}
 else if(SmokeStep==6&&Travel>.65f) {
  if(!Check(FSV->Selected==1&&FSV->Heights[1]==1&&FSV->Heights[0]==0,TEXT("next system rises while the previous tower lowers")))return;
  Capture(TEXT("fsv-provenance.png"));Travel=0;SmokeStep=60;
 } else if(SmokeStep==60&&Travel>.3f) {Key(EKeys::Left);Key(EKeys::Left);
  if(!Check(FSV->Selected==-1&&PageIndex==1,TEXT("Left steps backward to the flat overview")))return;
  Key(EKeys::Left);SmokeStep=7;
 } else if(SmokeStep==7&&DesktopMinimize==0) {
  if(!Check(PageIndex==0&&!bCommandDesktop&&TimerElapsed>0,TEXT("Left from FSV overview restores the previous slide and preserves the timer")))return;
  HandleParkKey(EKeys::Right);SmokeStep=8;
 } else if(SmokeStep==8&&DesktopMinimize==1) {
  for(int32 I=0;I<FSV->Systems.Num();I++)Key(EKeys::Right);
  if(!Check(FSV->Selected==6&&PageIndex==1,TEXT("all seven systems are individual arrow steps")))return;
  Key(EKeys::Right);SmokeStep=9;
 } else if(SmokeStep==9&&MapPhase==EMapPhase::Overview&&DesktopMinimize==0) {
  if(!Check(!bCommandDesktop&&FSV->Selected==6,TEXT("Right after the last system returns to the island overview")))return;
  FPlatformMisc::RequestExit(false);
 }
}
