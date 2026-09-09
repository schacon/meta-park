#include "SlideGameMode.h"
#include "Camera/CameraActor.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"

void ASlideGameMode::TestProgress() {
 auto Check=[](bool OK,const TCHAR* What){
  UE_LOG(LogTemp,Display,TEXT("ProgressTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));
  if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);return OK;
 };
 auto Capture=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name,true,false);};
 if(SmokeStep==0&&Elapsed>1) {
  if(!Check(PowerPercent()==100&&SecurityPercent()==100,TEXT("overview starts with both bars full")))return;
  const float SavedElapsed=Elapsed;
  Elapsed=0;auto Before=StatusPins();Elapsed=.5f;auto After=StatusPins();Elapsed=SavedElapsed;
  if(!Check(Before[0].StatusStage==1&&Before[1].StatusStage==0&&Before[0].StatusColor!=After[0].StatusColor,TEXT("next station blinks red and later stations are blue")))return;
  Capture(TEXT("progress-full.png"));SmokeStep=1;Travel=0;
 } else if(SmokeStep==1&&Travel>.3f){GoTo(0);SmokeStep=2;}
 else if(SmokeStep==2&&MapPhase==EMapPhase::Slide&&Travel>.2f) {
  int32 Total=0;for(const auto& Steps:StationSteps)Total+=Steps.Num();
  if(!Check(Total>Views.Num()&&ViewedPages.Num()==1&&PowerPercent()==FMath::CeilToInt(100.f*(Total-1)/Total),TEXT("all station subpages, including hidden station, contribute to total")))return;
  const auto Pins=StatusPins();
  if(!Check(Pins[0].StatusStage==-1&&Pins[1].StatusStage==1&&Pins[2].StatusStage==0,TEXT("visiting a group advances funny status colors")))return;
  for(const auto& Sample:TArray<TPair<const TCHAR*,const TCHAR*>>{{TEXT("17.5"),TEXT("OPERATIONAL")},{TEXT("17.85"),TEXT("Nominal")},{TEXT("31.5"),TEXT("Nominal")},{TEXT("31.85"),TEXT("Critical")},{TEXT("33.25"),TEXT("Critical")},{TEXT("33.6"),TEXT("UTTER CHAOS")}}) {
   if(!Check(SetElapsedMinutes(Sample.Key)&&ParkStatusLabel()==Sample.Value,TEXT("Time editor updates exact Security status thresholds")))return;
  }
  const float BeforeInvalid=TimerElapsed;
  for(const TCHAR* Invalid:{TEXT(""),TEXT("-1"),TEXT("36"),TEXT("nonsense"),TEXT("1abc"),TEXT("nan")})if(!Check(!SetElapsedMinutes(Invalid)&&TimerElapsed==BeforeInvalid,TEXT("invalid time leaves the clock unchanged")))return;
  const int32 Before=PowerPercent();ActiveComponentPage=INDEX_NONE;TickStationPage(0);
  if(!Check(PowerPercent()==Before,TEXT("revisiting a page does not drain Power twice")))return;
  const float SavedDuration=TimerDuration;TimerDuration=2700;TimerElapsed=1350;bTimerPaused=true;TickPresentationClock(60);
  if(!Check(SecurityPercent()==50&&TimerElapsed==1350,TEXT("Security uses configured duration and respects pause")))return;
  bTimerPaused=false;TickPresentationClock(270);
  if(!Check(SecurityPercent()==40,TEXT("Security drains with elapsed presentation time")))return;
  TimerDuration=SavedDuration;TimerElapsed=TimerDuration*.5f;bTimerPaused=true;
  Capture(TEXT("progress-partial.png"));SmokeStep=3;Travel=0;
 } else if(SmokeStep==3&&Travel>.3f) {
  ParkMenu->SetIsOpen(true);TimeMenu->SetIsOpen(true);SmokeStep=30;Travel=0;
 } else if(SmokeStep==30&&Travel>.3f) {
  ElapsedTimeField->SetText(FText::FromString(TEXT("34")));Capture(TEXT("time-editor.png"));SmokeStep=31;Travel=0;
 } else if(SmokeStep==31&&Travel>.3f) {
  ApplyTimeButton->SimulateClick();
  if(!Check(TimerElapsed==2040&&SecurityPercent()==3&&ParkStatusLabel()==TEXT("UTTER CHAOS")&&!bEditingTime,TEXT("Time menu accepts typed minutes and applies them to the live HUD")))return;
  Capture(TEXT("progress-chaos.png"));SmokeStep=32;Travel=0;
 } else if(SmokeStep==32&&Travel>.3f) {
  for(int32 Station=0;Station<StationSteps.Num();Station++)for(int32 Page=0;Page<StationSteps[Station].Num();Page++) {
   Index=Station;PageIndex=Page;ActiveComponentPage=INDEX_NONE;TickStationPage(0);
  }
  TimerElapsed=TimerDuration;bTimerPaused=false;TickPresentationClock(10);
  if(!Check(PowerPercent()==0&&SecurityPercent()==0,TEXT("all pages and expired timer empty both bars at zero")))return;
  Index=0;ResetStationPage();TickStationPage(0);bTimerPaused=true;
  Capture(TEXT("progress-empty.png"));SmokeStep=4;Travel=0;
 } else if(SmokeStep==4&&Travel>.3f) {
  LogoutToLogin();
  if(!Check(PowerPercent()==100&&SecurityPercent()==100&&ViewedPages.IsEmpty()&&VisitedStations.IsEmpty()&&ParkStatusLabel()==TEXT("OPERATIONAL"),TEXT("logout resets bars and station statuses")))return;
  FPlatformMisc::RequestExit(false);
 }
}
