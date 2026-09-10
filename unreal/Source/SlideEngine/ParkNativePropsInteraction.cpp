#include "SlideGameMode.h"
#include "ParkColdStorage.h"
#include "ParkRaptors.h"
#include "IslandScene.h"
#include "Dom/JsonObject.h"
#include "Camera/CameraActor.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "TimerManager.h"
#include "Engine/World.h"
void ASlideGameMode::TestNativeProps() {
 auto Check=[](bool OK,const TCHAR* What){UE_LOG(LogTemp,Display,TEXT("NativePropsTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);return OK;};
 auto Capture=[this](const TCHAR* Name){FTimerHandle Handle;const FString Path=FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name;GetWorld()->GetTimerManager().SetTimer(Handle,[Path]{FScreenshotRequest::RequestScreenshot(Path,true,false);},.35f,false);};
 if(SmokeStep==0&&Elapsed>1) {
  if(!Check(GateSlide==0&&MapPins[2].Code==TEXT("ENC-04"),TEXT("gate first and raptor pen third")))return;
  GoTo(0);SmokeStep=1;
 } else if(SmokeStep==1&&MapPhase==EMapPhase::Slide) {
  if(!Check(WoodSign.IsValid()&&UsesPhysicalProp(),TEXT("h1 is a physical wooden direction sign")))return;
  Capture(TEXT("props-directions.png"));Travel=0;SmokeStep=2;
 } else if(SmokeStep==2&&Travel>.5f){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=3;}
 else if(SmokeStep==3&&Travel>1&&ColdStorage.IsValid()) {
  if(!Check(ColdStorage->Specimens.Num()==4&&ColdStorage->Selected==-1&&WoodSign.IsValid(),TEXT("cryogenic rack has four authored vials")))return;
  Capture(TEXT("props-cold-overview.png"));Travel=0;SmokeStep=4;
 } else if(SmokeStep==4&&Travel>.5f){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=5;}
 else if(SmokeStep==5&&ColdStorage->Ready()) {
  if(!Check(ColdStorage->Selected==0&&ColdStorage->Species()==TEXT("granularity")&&!ColdStorage->Description().IsEmpty(),TEXT("Right extracts first vial and shows authored description")))return;
  Capture(TEXT("props-cold-vial.png"));Travel=0;SmokeStep=6;
 } else if(SmokeStep==6&&Travel>.5f){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=7;}
 else if(SmokeStep==7&&ColdStorage->Ready()) {
  if(!Check(ColdStorage->Selected==1&&ColdStorage->Species()==TEXT("performance"),TEXT("next vial returns rotates and extracts")))return;
  HandleParkKey(EKeys::Left);SmokeStep=8;
 } else if(SmokeStep==8&&ColdStorage->Ready()) {
  if(!Check(ColdStorage->Selected==0,TEXT("Left retrieves previous vial")))return;
  HandleParkKey(EKeys::Right);SmokeStep=9;
 } else if(SmokeStep==9&&ColdStorage->Ready()) {
  if(ColdStorage->Selected<3)HandleParkKey(EKeys::Right);
  else {if(!Check(ViewedColdSpecimens.Num()==4,TEXT("all vials count toward slide progress")))return;HandleParkKey(EKeys::Right);SmokeStep=10;}
 } else if(SmokeStep==10&&MapPhase==EMapPhase::Overview) {GoTo(2);SmokeStep=11;}
 else if(SmokeStep==11&&MapPhase==EMapPhase::Slide) {
  if(!Check(WoodSign.IsValid()&&UsesPhysicalProp()&&IslandScene::RaptorPositions().Num()==4,TEXT("warning replaces screen and pen has four walking raptors")))return;
  Capture(TEXT("props-raptor-warning.png"));Travel=0;SmokeStep=12;
 } else if(SmokeStep==12&&Travel>.5f){HandleParkKey(EKeys::Right);SmokeStep=13;}
 else if(SmokeStep==13&&RaptorView.IsValid()&&RaptorView->Ready()) {
  if(!Check(Camera->GetActorLocation().Z>9000&&RaptorView->Selected==-1,TEXT("Raptors flies overhead and opens keeper clipboard")))return;
  Capture(TEXT("props-raptors-overview.png"));Travel=0;SmokeStep=14;
 } else if(SmokeStep==14&&Travel>.5f){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=15;}
 else if(SmokeStep==15&&RaptorView->Ready()) {
  if(!Check(RaptorView->Selected==0&&RaptorView->Label()==TEXT("commit message / trailers")&&RaptorView->Workers()==2,TEXT("selection flips clipboard and displays problems and worker tally")))return;
  Capture(TEXT("props-raptor-clipboard.png"));Travel=0;SmokeStep=16;
 } else if(SmokeStep==16&&Travel>.5f){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=17;}
 else if(SmokeStep==17&&Travel>.20f){Capture(TEXT("props-clipboard-flip.png"));SmokeStep=18;}
 else if(SmokeStep==18&&RaptorView->Ready()) {
  if(!Check(RaptorView->Selected==1&&RaptorView->Workers()==4,TEXT("second incident sheet replaces first after flipping")))return;
  HandleParkKey(EKeys::Left);SmokeStep=19;
 } else if(SmokeStep==19&&RaptorView->Ready()) {
  if(!Check(RaptorView->Selected==0,TEXT("Left flips to previous raptor")))return;
  HandleParkKey(EKeys::Right);SmokeStep=20;
 } else if(SmokeStep==20&&RaptorView->Ready()) {
  if(RaptorView->Selected<3)HandleParkKey(EKeys::Right);
  else {if(!Check(ViewedRaptors.Num()==4,TEXT("four raptor records count toward progress")))return;HandleParkKey(EKeys::Right);SmokeStep=21;}
 } else if(SmokeStep==21&&MapPhase==EMapPhase::Overview) {
  if(!Check(!RaptorView.IsValid()&&!WoodSign.IsValid()&&!ColdStorage.IsValid(),TEXT("leaving station removes all native props")))return;
  if(!Check(IslandScene::ValidateWandering()&&IslandScene::ValidateArticulatedGaits(),TEXT("four raptors stay inside the pen without intersecting")))return;
  ResetPresentationSession();
  if(!Check(ViewedRaptors.IsEmpty()&&ViewedColdSpecimens.IsEmpty()&&PowerPercent()==100,TEXT("logout resets component progress")))return;
  FPlatformMisc::RequestExit(false);
 }
}
