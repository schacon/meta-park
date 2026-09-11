#include "SlideGameMode.h"
#include "ParkExchange.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Engine/World.h"
#include "TimerManager.h"
void ASlideGameMode::TestExchange() {
 auto Check=[](bool OK,const TCHAR* What){UE_LOG(LogTemp,Display,TEXT("ExchangeTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);return OK;};
 auto Capture=[this](const TCHAR* Name){FTimerHandle H;const auto Path=FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name;GetWorld()->GetTimerManager().SetTimer(H,[Path]{FScreenshotRequest::RequestScreenshot(Path,true,false);},.15f,false);};
 if(SmokeStep==0&&Elapsed>1){GoTo(5);SmokeStep=1;}
 else if(SmokeStep==1&&MapPhase==EMapPhase::Slide){HandleParkKey(EKeys::Right);SmokeStep=2;}
 else if(SmokeStep==2&&Exchange.IsValid()&&Exchange->Age>4){
  if(!Check(!bCommandDesktop&&DesktopMinimize==0&&Exchange->Visible&&UsesPhysicalProp(),TEXT("keeper clipboards open in the park")))return;
  if(!Check(Exchange->Steps.Num()==7&&Exchange->Records.Num()==3&&Exchange->Steps[0].Log.Num()==1&&Exchange->Values(0,1).Num()==2&&Exchange->Values(0,2).Num()==1&&Exchange->Values(2,0).IsEmpty(),TEXT("Keeper 1 writes one name two foods and one feeding time")))return;
  Capture(TEXT("keepers-start.png"));Travel=0;SmokeStep=3;
 } else if(SmokeStep==3&&Travel>.5){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=4;}
 else if(SmokeStep==4&&Travel>3.5){
  const auto& S=Exchange->Steps[Exchange->Selected];const int32 P=Exchange->Selected;
  if(P==1&&!Check(Exchange->Values(2,0)[0].Text==TEXT("Rex")&&Exchange->Values(2,1).Num()==2&&S.Log.Num()==1,TEXT("Keeper 2 copies the complete initial snapshot")))return;
  if(P==2&&!Check(Exchange->Values(0,0)[0].Text==TEXT("Chomper")&&Exchange->Values(2,0)[0].Text==TEXT("Tiny")&&Exchange->Values(1,0)[0].Text==TEXT("Rex")&&Exchange->MovingValues==6,TEXT("both keepers edit simultaneously while the record stays B")))return;
  if(P==3&&!Check(S.Log.Num()==2&&S.Record==TEXT("R")&&Exchange->Values(1,0)[0].Text==TEXT("Tiny"),TEXT("Keeper 2 appends full edition R after B")))return;
  if(P==4&&!Check(S.Log.Num()==2&&S.Record==TEXT("R")&&Exchange->Values(0,0)[0].Text==TEXT("Chomper")&&Exchange->Values(1,0)[0].Text==TEXT("Tiny"),TEXT("blocked push preserves both the record and local edits")))return;
  if(P==5&&!Check(S.Log.Num()==2&&Exchange->Values(0,0)[0].Text==TEXT("Chomper")&&Exchange->Values(0,1).Num()==4&&Exchange->Values(0,2).Num()==3&&Exchange->Values(0,2)[1].Text==TEXT("12:00")&&Exchange->Values(1,0)[0].Text==TEXT("Tiny"),TEXT("local merge keeps the name unions foods and orders both appends without publishing")))return;
  if(P==6&&!Check(S.Log.Num()==3&&S.Record==TEXT("M")&&Exchange->Records[2].Parent==TEXT("R")&&Exchange->Values(1,0)[0].Text==TEXT("Chomper")&&Exchange->Values(1,1).Num()==4&&Exchange->Values(1,2).Num()==3&&Exchange->Values(2,0)[0].Text==TEXT("Tiny"),TEXT("retry appends full M and leaves Keeper 2 at R")))return;
  const auto Name=FString::Printf(TEXT("keepers-stage-%d.png"),P);Capture(*Name);Travel=0;SmokeStep=P<6?3:5;
 } else if(SmokeStep==5&&Travel>.5){
  HandleParkKey(EKeys::Left);if(!Check(Exchange->Selected==5,TEXT("back revisits local merge")))return;
  HandleParkKey(EKeys::R);if(!Check(Exchange->Selected==0&&Exchange->Age==0,TEXT("R restarts the keeper story")))return;
  Exchange->Select(6);HandleParkKey(EKeys::Right);Travel=0;SmokeStep=6;
 } else if(SmokeStep==6&&Travel>1){if(!Check(PageIndex==2&&!Exchange.IsValid()&&!bCommandDesktop,TEXT("next page restores the park")))return;HandleParkKey(EKeys::Left);Travel=0;SmokeStep=7;}
 else if(SmokeStep==7&&Travel>1){if(!Check(Exchange.IsValid()&&Exchange->Selected==6&&Exchange->Visible,TEXT("returning preserves the selected stage")))return;LogoutToLogin();if(!Check(!Exchange.IsValid()&&PageExchanges.IsEmpty()&&ViewedExchangeSteps.IsEmpty(),TEXT("logout clears keeper records and progress")))return;UE_LOG(LogTemp,Display,TEXT("ExchangeTest: PASS"));FPlatformMisc::RequestExitWithStatus(false,0);}
}
