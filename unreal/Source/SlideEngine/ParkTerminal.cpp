#include "ParkTerminal.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"

namespace {
// Physical display rectangle: frame around the CRT, allowing peripherals to crop.
const FVector DisplayCenter(65,-58.42,158.675);
const FBox DisplayBounds(DisplayCenter-FVector(97.75,0,66.7),DisplayCenter+FVector(97.75,0,66.7));
}

FParkTerminal::FParkTerminal(UWorld* World) {
 auto* Model=World->SpawnActor<AStaticMeshActor>();Actor=Model;
 auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Models/Park/SM_Terminal.SM_Terminal"));
 check(Mesh);Bounds=Mesh->GetBoundingBox();
 auto* Body=Model->GetStaticMeshComponent();Body->SetMobility(EComponentMobility::Movable);
 Body->SetStaticMesh(Mesh);Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);Body->SetCastShadow(false);
 auto* Screen=NewObject<UWidgetComponent>(Model);Model->AddInstanceComponent(Screen);Screen->SetupAttachment(Body);
 Screen->SetWidgetSpace(EWidgetSpace::World);Screen->SetDrawSize(FVector2D(1024,720));Screen->SetTwoSided(true);
 Screen->SetBlendMode(EWidgetBlendMode::Opaque);Screen->SetCollisionEnabled(ECollisionEnabled::NoCollision);Screen->SetCastShadow(false);
 Screen->SetRelativeLocation(DisplayCenter);Screen->SetRelativeRotation(FRotator(0,-90,0));
 Screen->SetRelativeScale3D(FVector(1,195.5f/1024,133.4f/720));Screen->RegisterComponent();
 auto Font=[](int32 Size){return FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("Fonts/RobotoMono-Regular.ttf"),Size);};
 Screen->SetSlateWidget(SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
  .BorderBackgroundColor(FLinearColor(.007,.015,.32)).Padding(52)
  [SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,76)[SNew(STextBlock).Text(FText::FromString(TEXT("git-meta / system terminal\nREADY"))).Font(Font(28)).ColorAndOpacity(FLinearColor(.66,.78,1))]
   +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,38)[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(Command()+(FMath::Fmod(Clock,.7f)<.35f?TEXT("_"):TEXT(" ")));}).Font(Font(82)).ColorAndOpacity(FLinearColor(.91,.94,1))]
   +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(OutputReady()?TEXT("OK"):TEXT(""));}).Font(Font(96)).ColorAndOpacity(FLinearColor(.72,1,.74))]
  ]);
 Model->SetActorHiddenInGame(true);
}
void FParkTerminal::Toggle(){Raised=!Raised;if(Raised)Clock=0;}
FString FParkTerminal::Command() const {
 return FString(TEXT("git meta set")).Left(FMath::Clamp(FMath::FloorToInt((Clock-.15f)/.09f),0,12));
}
void FParkTerminal::Update(float Delta,ACameraActor* Camera,bool InFirstArea) {
 if(!Actor.IsValid())return;
 if(!InFirstArea){Raised=false;Reveal=0;Clock=0;Actor->SetActorHiddenInGame(true);return;}
 Reveal=FMath::Clamp(Reveal+(Raised?1.f:-1.f)*Delta/.5f,0.f,1.f);
 Actor->SetActorHiddenInGame(Reveal==0);if(Reveal==0)return;
 if(Reveal==1&&Raised)Clock+=Delta;
 auto* PC=Camera->GetWorld()->GetFirstPlayerController();auto* Player=PC->GetLocalPlayer();
 int32 W,H;PC->GetViewportSize(W,H);
 const float Aspect=(W*Player->Size.X)/FMath::Max(1.f,H*Player->Size.Y);
 const float TX=FMath::Tan(FMath::DegreesToRadians(Camera->GetCameraComponent()->FieldOfView*.5f))*.59f,TY=TX/Aspect;
 constexpr float Distance=900;
 const FQuat LocalRotation=FRotator(0,-84,0).Quaternion();
 // Center and fit the display itself. The closer keyboard, tower, and mouse
 // intentionally extend beyond the viewport to keep attention on the command.
 float Scale=1000;
 for(float X:{float(DisplayBounds.Min.X),float(DisplayBounds.Max.X)})for(float Y:{float(DisplayBounds.Min.Y),float(DisplayBounds.Max.Y)})for(float Z:{float(DisplayBounds.Min.Z),float(DisplayBounds.Max.Z)}) {
  const FVector P=LocalRotation.RotateVector(FVector(X,Y,Z)-DisplayCenter);
  for(FVector2D Pair:{FVector2D(FMath::Abs(P.Y),TX),FVector2D(FMath::Abs(P.Z),TY)}) {
   const float Denominator=Pair.X-P.X*Pair.Y;
   if(Denominator>0)Scale=FMath::Min(Scale,float(Distance*Pair.Y/Denominator));
  }
 }
 const float Ease=Reveal*Reveal*(3-2*Reveal);
 const FQuat Rotation=Camera->GetActorQuat()*LocalRotation;
 const FVector Drop=Camera->GetActorUpVector()*((1-Ease)*(Distance*TY*2+Bounds.GetSize().Z*Scale));
 Actor->SetActorTransform(FTransform(Rotation,Camera->GetActorLocation()+Camera->GetActorForwardVector()*Distance-Rotation.RotateVector((DisplayCenter+FVector(-30,0,-35))*Scale)-Drop,FVector(Scale)));
}
bool FParkTerminal::ScreenFillsViewport() const {
 if(!Actor.IsValid())return false;
 auto* PC=Actor->GetWorld()->GetFirstPlayerController();auto* Player=PC->GetLocalPlayer();int32 W,H;PC->GetViewportSize(W,H);
 FVector2D Min(W,H),Max(0,0);
 for(float X:{float(DisplayBounds.Min.X),float(DisplayBounds.Max.X)})for(float Y:{float(DisplayBounds.Min.Y),float(DisplayBounds.Max.Y)})for(float Z:{float(DisplayBounds.Min.Z),float(DisplayBounds.Max.Z)}) {
  FVector2D P;if(!PC->ProjectWorldLocationToScreen(Actor->GetActorTransform().TransformPosition(FVector(X,Y,Z)),P))return false;
  if(P.X<W*Player->Origin.X||P.X>W||P.Y<H*Player->Origin.Y||P.Y>H)return false;
  Min.X=FMath::Min(Min.X,P.X);Min.Y=FMath::Min(Min.Y,P.Y);
  Max.X=FMath::Max(Max.X,P.X);Max.Y=FMath::Max(Max.Y,P.Y);
 }
 // At least one dimension should fill most of the usable viewport.
 return FMath::Max((Max.X-Min.X)/(W*Player->Size.X),(Max.Y-Min.Y)/(H*Player->Size.Y))>.56f;
}
