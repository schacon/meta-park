#include "SlideGameMode.h"
#include "IslandScene.h"
#include "ParkTerminal.h"
#include "Camera/CameraActor.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void ASlideGameMode::TestLogin() {
 auto Check=[](bool OK,const TCHAR* What){
  UE_LOG(LogTemp,Display,TEXT("LoginTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));
  if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);return OK;
 };
 auto Enter=[](){FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Enter,FModifierKeysState(),0,false,0,0));};
 auto Capture=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name,true,false);};
 if(LoginTestStep==0&&LockTime>.7f) {
  HandleParkKey(EKeys::One);HandleParkKey(EKeys::F);TickPresentationClock(60);
  if(!Check(bLocked&&!bFreeFlight&&MapPhase==EMapPhase::Overview&&TimerElapsed==0&&WorkstationMinutes==35&&RemainingVisitors()==3000&&RemainingStaff()==350,TEXT("starts locked at workstation 35 and blocks map input and timer")))return;
  Capture(TEXT("login-screen.png"));LoginTestStep=1;
 } else if(LoginTestStep==1&&LockTime>1.1f) {
  for(const TCHAR* Invalid:{TEXT(""),TEXT("0"),TEXT("5"),TEXT("181"),TEXT("-30"),TEXT("35.5"),TEXT("abc"),TEXT("99999999999")}) {
   WorkstationField->SetText(FText::FromString(Invalid));StartLogin();
   if(!Check(!bLoginTyping&&!LoginError.IsEmpty(),TEXT("invalid workstation stays at login")))return;
  }
  WorkstationField->SetText(FText::FromString(TEXT("45")));
  FSlateApplication::Get().SetKeyboardFocus(WorkstationField,EFocusCause::SetDirectly);Enter();
  if(!Check(bLoginTyping&&WorkstationMinutes==45&&LoginError.IsEmpty(),TEXT("Enter from edited workstation field begins login")))return;
  LoginTestStep=2;
 } else if(LoginTestStep==2&&LoginTime>.45f) {
  const float Before=LoginTime;Enter();
  if(!Check(bLocked&&LoginName().Len()>0&&LoginName().Len()<8&&LoginPassword().IsEmpty()&&LoginTime==Before&&TimerElapsed==0,TEXT("username types first and repeat Enter does not restart animation")))return;
  LoginTestStep=3;
 } else if(LoginTestStep==3&&LoginTime>1.85f) {
  if(!Check(bLocked&&LoginName()==TEXT("s.chacon")&&LoginPassword()==TEXT("********")&&TimerElapsed==0,TEXT("masked password completes before unlocking")))return;
  Capture(TEXT("login-typing.png"));LoginTestStep=4;
 } else if(LoginTestStep==4&&!bLocked) {
  TickPresentationClock(60);
  bool Good=MapPhase==EMapPhase::Overview&&Index==0&&PendingIndex==-1&&!bTourStarted&&!bTimerStarted&&TimerElapsed==0&&TimerDuration==2700;
  Good&=RemainingVisitors()==4000&&RemainingStaff()==450;
  Good&=VisitorDepartures.Remaining(2399.99f)>0&&VisitorDepartures.Remaining(2400)==0&&StaffDepartures.Remaining(2699.99f)>0&&StaffDepartures.Remaining(2700)==0;
  if(!Check(Good,TEXT("45 minute session preserves five minute wrap-up and waits for first slide")))return;
  GoTo(0);LoginTestStep=5;
 } else if(LoginTestStep==5&&MapPhase==EMapPhase::Slide&&Travel>.3f) {
  if(!Check(bTimerStarted&&TimerElapsed>0&&TimerElapsed<1,TEXT("clock starts when first slide is visible")))return;
  HandleParkKey(EKeys::Right);LoginTestStep=6;
 } else if(LoginTestStep==6&&Terminal.IsValid()&&Terminal->IsRaised()) {
  TickPresentationClock(100);bTimerPaused=true;ParkMenu->SetIsOpen(true);Capture(TEXT("logout-menu.png"));LoginTestStep=11;Travel=0;
 } else if(LoginTestStep==11&&Travel>.3f) {
  LogoutButton->SimulateClick();
  if(!Check(bLocked&&!bTimerStarted&&!bTimerPaused&&TimerElapsed==0&&Terminal->IsHidden()&&Camera->GetActorLocation().Equals(MapEye,1)&&PendingIndex==-1,TEXT("logout clears timer, pause, terminal, camera and navigation")))return;
  Capture(TEXT("login-after-logout.png"));LoginTestStep=7;
 } else if(LoginTestStep==7&&LockTime>.4f) {
  if(!Check(WorkstationText==TEXT("45")&&LoginName().IsEmpty()&&LoginPassword().IsEmpty(),TEXT("logout retains workstation and clears credentials")))return;
  WorkstationField->SetText(FText::FromString(TEXT("35")));LoginButtonWidget->SimulateClick();
  if(!Check(bLoginTyping,TEXT("Login button begins a new session")))return;LoginTestStep=8;
 } else if(LoginTestStep==8&&!bLocked) {
  if(!Check(TimerDuration==2100&&RemainingVisitors()==3000&&RemainingStaff()==350&&TimerElapsed==0,TEXT("second login creates fresh default countdowns")))return;
  HandleParkKey(EKeys::F);HandleParkKey(EKeys::Eight);LogoutToLogin();
  if(!Check(bLocked&&!bFreeFlight&&!bFreeTravelling&&MapPhase==EMapPhase::Overview&&CardExpansion==0&&TimerElapsed==0,TEXT("logout also interrupts free flight")))return;
  StartLogin();LoginTestStep=9;
 } else if(LoginTestStep==9&&!bLocked) {GoTo(GateSlide);LoginTestStep=10;}
 else if(LoginTestStep==10&&IslandScene::GateOpenFraction()>.3f) {
  LogoutToLogin();
  if(!Check(IslandScene::GateOpenFraction()==0&&Panels[GateSlide].Reveal==0&&PendingIndex==-1&&TimerElapsed==0,TEXT("logout closes an opening gate and hides its slide")))return;
  UE_LOG(LogTemp,Display,TEXT("LoginTest: PASS startup, editable duration, Slate Enter, typing, repeated login and complete session reset"));
  FPlatformMisc::RequestExit(false);
 }
 if(Elapsed>50){Check(false,TEXT("interaction completed before timeout"));}
}
