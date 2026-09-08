#include "SlideGameMode.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Components/SceneComponent.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Framework/Application/SlateApplication.h"

void ASlideGameMode::ToggleFreeFlight() {
 FSlateApplication::Get().ReleaseAllPointerCapture();
 if(bFreeFlight) {
  bFreeFlight=false;bFreeTravelling=false;FreeDrag=FVector2D::ZeroVector;
  Overview();return;
 }
 bFreeFlight=true;bFreeTravelling=true;bFlying=false;bOverview=false;
 MapPhase=EMapPhase::Free;PendingIndex=-1;CardExpansion=0;FreeTravel=0;
 FromPosition=Camera->GetActorLocation();FromRotation=Camera->GetActorQuat();
 FromLook=CameraLook;FromFrameWidth=CameraFrameWidth;
 // Open the lens smoothly while preserving the current composition. The map
 // uses a distant, narrow lens; free flight needs a normal perspective lens.
 FreeLook=CameraLook;
 FreeEye=FreeLook-Camera->GetActorForwardVector()*(CameraFrameWidth/(2*FMath::Tan(PI/6)));
 FreeDrag=FVector2D::ZeroVector;SetMouseMode(false);
}

void ASlideGameMode::FlyToArea(int32 Next) {
 Index=(Next%Views.Num()+Views.Num())%Views.Num();
 PendingIndex=-1;MapPhase=EMapPhase::Free;CardExpansion=0;bOverview=false;
 FromPosition=Camera->GetActorLocation();FromRotation=Camera->GetActorQuat();
 FromLook=CameraLook;FromFrameWidth=CameraFrameWidth;
 FreeEye=CameraStops[Index];FreeLook=CardTargets[Index]+FVector(0,0,100);
 FreeTravel=0;bFreeTravelling=true;FreeDrag=FVector2D::ZeroVector;
}

void ASlideGameMode::MoveFreeCamera(float Delta,FVector Move,FVector2D Look,bool Fast) {
 if(Move.IsNearlyZero()&&Look.IsNearlyZero())return;
 bFreeTravelling=false;
 const float Lens=CameraFrameWidth/(2*FMath::Max(1.f,FVector::Distance(Camera->GetActorLocation(),CameraLook)));
 FRotator Rotation=Camera->GetActorRotation();
 Rotation.Yaw+=Look.X*.18f;Rotation.Pitch=FMath::Clamp(Rotation.Pitch-Look.Y*.18f,-85.f,85.f);Rotation.Roll=0;
 Camera->SetActorRotation(Rotation);
 const FVector Direction=Camera->GetActorForwardVector()*Move.X+Camera->GetActorRightVector()*Move.Y+FVector::UpVector*Move.Z;
 Camera->AddActorWorldOffset(Direction.GetClampedToMaxSize(1)*Delta*(Fast?9000.f:3000.f));
 CameraLook=Camera->GetActorLocation()+Camera->GetActorForwardVector()*10000;
 CameraFrameWidth=20000*Lens;
}

void ASlideGameMode::TickFreeFlight(float Delta) {
 auto* PC=GetWorld()->GetFirstPlayerController();
 const FVector Move(float(PC->IsInputKeyDown(EKeys::W)||PC->IsInputKeyDown(EKeys::Up))-float(PC->IsInputKeyDown(EKeys::S)||PC->IsInputKeyDown(EKeys::Down)),
  float(PC->IsInputKeyDown(EKeys::D)||PC->IsInputKeyDown(EKeys::Right))-float(PC->IsInputKeyDown(EKeys::A)||PC->IsInputKeyDown(EKeys::Left)),
  float(PC->IsInputKeyDown(EKeys::E))-float(PC->IsInputKeyDown(EKeys::Q)));
 MoveFreeCamera(Delta,Move,FreeDrag,PC->IsInputKeyDown(EKeys::LeftShift)||PC->IsInputKeyDown(EKeys::RightShift));
 FreeDrag=FVector2D::ZeroVector;
 if(!bFreeTravelling)return;
 FreeTravel+=Delta;
 const float T=FMath::Clamp(FreeTravel/.9f,0.f,1.f),Ease=T*T*(3-2*T);
 const FVector Position=FMath::Lerp(FromPosition,FreeEye,Ease);
 const FQuat TargetRotation=(FreeLook-FreeEye).Rotation().Quaternion();
 Camera->SetActorLocationAndRotation(Position,FQuat::Slerp(FromRotation,TargetRotation,Ease));
 // Use a forward look point throughout the move, avoiding a flip when a
 // destination is behind the camera or the flight crosses its old target.
 const float Distance=FMath::Lerp(FVector::Distance(FromPosition,FromLook),FVector::Distance(FreeEye,FreeLook),Ease);
 CameraLook=Position+Camera->GetActorForwardVector()*Distance;
 CameraFrameWidth=FMath::Lerp(FromFrameWidth,2*FVector::Distance(FreeEye,FreeLook)*FMath::Tan(PI/6),Ease);
 if(T>=1)bFreeTravelling=false;
}

void ASlideGameMode::TestFreeFlight() {
 auto Check=[](bool OK,const TCHAR* What){
  UE_LOG(LogTemp,Display,TEXT("FreeFlightTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));
  if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);
  return OK;
 };
 if(bFreeFlight) {
  bool Hidden=CardExpansion==0&&MapPhase==EMapPhase::Free;
  for(const auto& Panel:Panels)if(Panel.Root.IsValid())Hidden&=!Panel.Root->GetRootComponent()->IsVisible()&&Panel.Reveal==0;
  for(const auto& Base:SignBases)if(Base.IsValid())Hidden&=Base->IsHidden();
  if(!Hidden){Check(false,TEXT("all signs hidden"));return;}
 }
 if(SmokeStep==0&&Elapsed>1) {
  const FVector Before=Camera->GetActorLocation();
  HandleParkKey(EKeys::F);
  if(!Check(bFreeFlight&&Camera->GetActorLocation().Equals(Before)&&PendingIndex==-1,TEXT("F enters without an initial camera jump")))return;
  SmokeStep=1;
 } else if(SmokeStep==1&&!bFreeTravelling) {
  const FVector Before=Camera->GetActorLocation(),Forward=Camera->GetActorForwardVector();
  MoveFreeCamera(.25f,FVector(1,0,0),FVector2D::ZeroVector,false);
  if(!Check(Camera->GetActorLocation().Equals(Before+Forward*750,.1f),TEXT("W movement follows view direction")))return;
  const FRotator BeforeLook=Camera->GetActorRotation();
  MoveFreeCamera(0,FVector::ZeroVector,FVector2D(100,-100),false);
  if(!Check(FMath::IsNearlyEqual(FRotator::NormalizeAxis(Camera->GetActorRotation().Yaw-BeforeLook.Yaw),18.f,.1f)&&Camera->GetActorRotation().Pitch>BeforeLook.Pitch,TEXT("drag turns and tilts camera")))return;
  const FVector AfterLook=Camera->GetActorLocation();
  MoveFreeCamera(.25f,FVector(0,0,1),FVector2D::ZeroVector,true);
  if(!Check(Camera->GetActorLocation().Equals(AfterLook+FVector(0,0,2250),.1f),TEXT("vertical movement and Shift speed")))return;
  HandleParkKey(EKeys::One);SmokeStep=2;
 } else if(SmokeStep>=2&&SmokeStep<=8&&!bFreeTravelling) {
  const int32 Expected=SmokeStep-2;
  if(!Check(Index==Expected&&Camera->GetActorLocation().Equals(CameraStops[Expected],1.f)&&!bTimerStarted&&TimerElapsed==0,TEXT("number arrives at subject without starting a slide or timer")))return;
  if(SmokeStep==5)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/free-flight-raptors.png"),true,false);
  const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven};
  if(SmokeStep<8){HandleParkKey(Keys[Expected+1]);SmokeStep++;}
  else {HandleParkKey(EKeys::One);MoveFreeCamera(.1f,FVector(0,1,0),FVector2D(5,0),false);
   if(!Check(!bFreeTravelling,TEXT("manual movement interrupts destination flight")))return;
   HandleParkKey(EKeys::F);SmokeStep=9;}
 } else if(SmokeStep==9&&MapPhase==EMapPhase::Overview&&Travel>.3f) {
  if(!Check(!bFreeFlight&&PendingIndex==-1&&Camera->GetActorLocation().Equals(MapEye,1.f)&&FMath::IsNearlyEqual(CameraFrameWidth,OverviewFrameWidth,1.f),TEXT("F returns to fitted overview")))return;
  GoTo(2);SmokeStep=10;
 } else if(SmokeStep==10&&MapPhase==EMapPhase::Slide) {
  HandleParkKey(EKeys::F);SmokeStep=11;
 } else if(SmokeStep==11&&!bFreeTravelling) {
  if(!Check(bFreeFlight&&CardExpansion==0,TEXT("entering from an expanded slide hides its content")))return;
  HandleParkKey(EKeys::Escape);SmokeStep=12;
 } else if(SmokeStep==12&&MapPhase==EMapPhase::Overview&&Travel>.3f) {
  Check(!bFreeFlight&&Camera->GetActorLocation().Equals(MapEye,1.f),TEXT("Escape also exits to overview"));
  FPlatformMisc::RequestExit(false);
 }
}
