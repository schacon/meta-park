#include "ParkColdStorage.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/PointLightComponent.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"
#include "Materials/MaterialInterface.h"

FParkColdStorage::FParkColdStorage(UWorld* World,TSharedPtr<FJsonObject> Component) {
 for(const auto& Value:Component->GetArrayField(TEXT("canisters"))){auto S=Value->AsObject();FString Label;S->TryGetStringField(TEXT("label"),Label);Specimens.Add({S->GetStringField(TEXT("species")),S->GetStringField(TEXT("description")),Label});}
 auto* A=World->SpawnActor<AActor>();Actor=A;
 auto* Root=NewObject<USceneComponent>(A);A->AddInstanceComponent(Root);A->SetRootComponent(Root);Root->SetMobility(EComponentMobility::Movable);Root->RegisterComponent();
 auto Mesh=[&](const TCHAR* Name,USceneComponent* Parent,FVector At){
  auto* M=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(M);M->SetupAttachment(Parent);M->SetMobility(EComponentMobility::Movable);
  M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Models/Park/%s.%s"),Name,Name)));check(M->GetStaticMesh());
  M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetRelativeLocation(At);M->RegisterComponent();return M;
 };
 auto* Body=Mesh(TEXT("SM_ColdBody"),Root,FVector(0,-165,-181));Body->SetRelativeScale3D(FVector(1,1,1.7));
 Rack=NewObject<USceneComponent>(A);A->AddInstanceComponent(Rack);Rack->SetupAttachment(Root);Rack->SetMobility(EComponentMobility::Movable);Rack->SetRelativeLocation(FVector(0,-165,252));Rack->RegisterComponent();
 Mesh(TEXT("SM_ColdRack"),Rack,FVector::ZeroVector);
 Lid=Mesh(TEXT("SM_ColdLid"),Root,FVector(0,-165,400));
 auto Font=[](int32 Size){return FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("Fonts/RobotoMono-Regular.ttf"),Size);};
 auto Widget=[&](USceneComponent* Parent,FVector At,FVector2D Size,FVector Scale){
  auto* W=NewObject<UWidgetComponent>(A);A->AddInstanceComponent(W);W->SetupAttachment(Parent);W->SetWidgetSpace(EWidgetSpace::World);W->SetDrawSize(Size);W->SetTwoSided(true);W->SetBlendMode(EWidgetBlendMode::Opaque);W->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_PropPaper.M_PropPaper")));W->SetCollisionEnabled(ECollisionEnabled::NoCollision);W->SetCastShadow(false);W->SetRelativeLocation(At);W->SetRelativeRotation(FRotator(0,180,0));W->SetRelativeScale3D(Scale);W->RegisterComponent();return W;
 };
 for(int32 I=0;I<Specimens.Num();I++) {
  const float R=FMath::DegreesToRadians(I*90.f);auto* V=Mesh(TEXT("SM_ColdVial"),Rack,FVector(-46*FMath::Cos(R),-46*FMath::Sin(R),12));Vials.Add(V);
  auto* Label=Widget(V,FVector(-13,0,52),FVector2D(768,100),FVector(.1));Label->SetRelativeRotation(FRotator(0,180,-90));
  Label->SetSlateWidget(SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.72,.88,.94)).Padding(8)
   [SNew(STextBlock).Text(FText::FromString(Specimens[I].Species)).Font(Font(56)).Justification(ETextJustify::Center).ColorAndOpacity(FLinearColor(.002,.004,.006))]);
 }
 auto* Display=Widget(Root,FVector(-40,190,315),FVector2D(650,560),FVector(.5));
 Display->SetSlateWidget(SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.009,.027,.047)).Padding(32)
  [SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,34)[SNew(STextBlock).Text(FText::FromString(TEXT("COLD STORAGE / 04"))).Font(Font(22)).ColorAndOpacity(FLinearColor(.24,.69,.91))]
   +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,26)[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(Ready()&&!Specimens[Selected].Label.IsEmpty()?Specimens[Selected].Label:Species());}).Font(Font(46)).AutoWrapText(true).ColorAndOpacity(FLinearColor(.85,.95,1))]
   +SVerticalBox::Slot().FillHeight(1)[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(Description());}).Font(Font(26)).AutoWrapText(true).ColorAndOpacity(FLinearColor(.54,.76,.86))]
  ]);
 for(FVector P:{FVector(-180,-370,500),FVector(80,70,410)}) {
  auto* Light=NewObject<UPointLightComponent>(A);A->AddInstanceComponent(Light);Light->SetupAttachment(Root);Light->SetMobility(EComponentMobility::Movable);Light->SetRelativeLocation(P);Light->SetIntensity(3500);Light->SetAttenuationRadius(850);Light->SetLightColor(FLinearColor(.38,.70,1));Light->SetCastShadows(false);Light->RegisterComponent();
 }
 A->SetActorHiddenInGame(true);
}
FParkColdStorage::~FParkColdStorage(){if(Actor.IsValid())Actor->Destroy();}
bool FParkColdStorage::Advance(int32 Direction) {
 if(Phase!=EPhase::Idle)return true;
 const int32 Next=Selected+Direction;if(Next<INDEX_NONE||Next>=Specimens.Num())return false;
 Selected=Next;Phase=EPhase::Return;return true;
}
FString FParkColdStorage::Species() const {return Selected<0?TEXT("Specimens on ice"):Ready()?Specimens[Selected].Species:TEXT("Retrieving...");}
FString FParkColdStorage::Description() const {
 if(Selected>=0)return Ready()?Specimens[Selected].Description:TEXT("");
 FString List;for(const auto& S:Specimens)List+=S.Species+TEXT("\n");return List;
}
void FParkColdStorage::Update(float Delta,ACameraActor* Camera,bool Visible) {
 if(!Actor.IsValid())return;
 Reveal=FMath::Clamp(Reveal+(Visible?1.f:-1.f)*Delta/.5f,0.f,1.f);Actor->SetActorHiddenInGame(Reveal==0);if(Reveal==0)return;
 if(Visible&&Reveal==1) {
  if(Phase==EPhase::Return){Lift=FMath::Max(0.f,Lift-Delta*3);if(Lift==0){Phase=Selected<0?EPhase::Idle:EPhase::Rotate;Moving=Selected;}}
  if(Phase==EPhase::Rotate){Angle=FMath::FixedTurn(Angle,-Selected*90.f,Delta*160);if(FMath::Abs(FMath::FindDeltaAngleDegrees(Angle,-Selected*90.f))<.1f)Phase=EPhase::Extract;}
  if(Phase==EPhase::Extract){Lift=FMath::Min(1.f,Lift+Delta*1.6f);if(Lift==1)Phase=EPhase::Idle;}
 }
 Rack->SetRelativeRotation(FRotator(0,Angle,0));
 const float LiftEase=Lift*Lift*(3-2*Lift);
 for(int32 I=0;I<Vials.Num();I++) {
  const float R=FMath::DegreesToRadians(I*90.f),Out=I==Moving?LiftEase:0;
  Vials[I]->SetRelativeLocation(FVector(-(46+65*Out)*FMath::Cos(R),-(46+65*Out)*FMath::Sin(R),12+118*Out)+FRotator(0,-Angle,0).RotateVector(FVector(0,70*Out,0)));
  const FQuat Rest=FRotator(0,I*90.f,0).Quaternion();
  const FQuat Reading=FRotator(0,-Angle,0).Quaternion()*FRotator(0,0,-74).Quaternion();
  Vials[I]->SetRelativeRotation(FQuat::Slerp(Rest,Reading,Out));
  Vials[I]->SetRelativeScale3D(FVector(1+1.4f*Out));
 }
 Lid->SetRelativeLocation(FVector(0,-165,270+130*Reveal+LiftEase*100));
 auto* PC=Camera->GetWorld()->GetFirstPlayerController();auto* Player=PC->GetLocalPlayer();int32 W,H;PC->GetViewportSize(W,H);
 const float TX=FMath::Tan(FMath::DegreesToRadians(Camera->GetCameraComponent()->FieldOfView*.5f)),TY=TX/((W*Player->Size.X)/FMath::Max(1.f,H*Player->Size.Y));
 constexpr float Distance=850;
 const float Scale=FMath::Min(Distance*TX*1.52f/700.f,Distance*TY*1.62f/580.f);
 const float Ease=Reveal*Reveal*(3-2*Reveal);const auto Rotation=Camera->GetActorQuat();
 const FVector Center(0,25,285);
 const FVector Position=Camera->GetActorLocation()+Camera->GetActorForwardVector()*Distance-Rotation.RotateVector(Center*Scale)-Camera->GetActorUpVector()*((1-Ease)*1100*TY);
 Actor->SetActorTransform(FTransform(Rotation,Position,FVector(Scale)));
}
