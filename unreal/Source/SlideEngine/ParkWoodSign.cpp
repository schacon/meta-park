#include "ParkWoodSign.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

FParkWoodSign::FParkWoodSign(UWorld* World,const FString& Title,bool IsWarning):Warning(IsWarning) {
 auto* A=World->SpawnActor<AActor>();Actor=A;A->SetActorHiddenInGame(true);
 auto* Root=NewObject<USceneComponent>(A);A->AddInstanceComponent(Root);A->SetRootComponent(Root);Root->SetMobility(EComponentMobility::Movable);Root->RegisterComponent();
 auto Material=[&](UPrimitiveComponent* C,FColor Color){auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_IslandLit.M_IslandLit")),C);M->SetVectorParameterValue(TEXT("Tint"),FLinearColor::FromSRGBColor(Color));return M;};
 auto Box=[&](USceneComponent* Parent,FVector P,FVector Size,FColor Color,FRotator Rotation=FRotator::ZeroRotator){
  auto* C=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(C);C->SetupAttachment(Parent);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetRelativeLocation(P);C->SetRelativeScale3D(Size/100);C->SetRelativeRotation(Rotation);C->SetMaterial(0,Material(C,Color));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();return C;
 };
 const FColor Dark(66,42,24),Wood(109,70,37),Light(133,87,44);
 if(Warning) {
  Box(Root,FVector(-45,-860,-190),FVector(85,85,1470),Dark);
  Box(Root,FVector(-45,0,515),FVector(90,1790,100),Wood);
  for(float Y:{-550.f,550.f})Box(Root,FVector(0,Y,417),FVector(16,16,140),FColor(64,70,61));
 } else for(float Y:{-550.f,500.f})Box(Root,FVector(-55,Y,-420),FVector(85,95,1460),Dark);
 Board=NewObject<USceneComponent>(A);A->AddInstanceComponent(Board);Board->SetupAttachment(Root);Board->SetMobility(EComponentMobility::Movable);Board->SetRelativeLocation(FVector(0,0,360));Board->RegisterComponent();
 // Three thick planks; the direction sign tapers to an actual arrow silhouette.
 for(int32 I=0;I<3;I++) {
  const float Z=-360+(1-I)*218;
  Box(Board,FVector(0,Warning?0:-75,Z),FVector(65,Warning?1560:1410,210),I%2?Wood:Light);
 }
 if(!Warning) {
  auto* M=NewObject<UProceduralMeshComponent>(A);A->AddInstanceComponent(M);M->SetupAttachment(Board);M->SetMobility(EComponentMobility::Movable);
  TArray<FVector> V={FVector(-32,630,-690),FVector(-32,965,-360),FVector(-32,630,-30),FVector(32,630,-690),FVector(32,965,-360),FVector(32,630,-30)};
  TArray<int32> T={0,2,1,3,4,5,0,1,4,0,4,3,1,2,5,1,5,4,2,0,3,2,3,5};
  M->CreateMeshSection(0,V,T,TArray<FVector>(),TArray<FVector2D>(),TArray<FColor>(),TArray<FProcMeshTangent>(),false);M->SetMaterial(0,Material(M,Light));M->RegisterComponent();
 }
 // Uneven grain and worn plank edges remain real geometry under the lettering.
 FRandomStream R(1993);
 for(int32 I=0;I<34;I++)Box(Board,FVector(33,R.FRandRange(-630,500),R.FRandRange(-650,-65)),FVector(1,R.FRandRange(90,330),R.FRandRange(1,3)),FColor(83,53,29));
 for(float Y:{-700.f,Warning?700.f:540.f})for(float Z:{-620.f,-100.f})Box(Board,FVector(36,Y,Z),FVector(7,14,14),FColor(47,50,42));
 if(Warning)for(int32 I=0;I<3;I++) {
  Box(Board,FVector(36,475+I*60,-475),FVector(3,14,255),FColor(42,22,12),FRotator(0,0,-24));
  Box(Board,FVector(38,482+I*60,-475),FVector(2,4,251),FColor(206,155,85),FRotator(0,0,-24));
 }
 auto* W=NewObject<UWidgetComponent>(A);A->AddInstanceComponent(W);W->SetupAttachment(Board);W->SetWidgetSpace(EWidgetSpace::World);W->SetDrawSize(FVector2D(1280,520));W->SetBlendMode(EWidgetBlendMode::Masked);W->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_PropLettering.M_PropLettering")));W->SetTwoSided(true);W->SetCollisionEnabled(ECollisionEnabled::NoCollision);W->SetCastShadow(false);W->SetRelativeLocation(FVector(39,Warning?-85:-100,-360));W->SetRelativeScale3D(FVector(1));W->RegisterComponent();
 auto Body=SNew(SVerticalBox);
 Body->AddSlot().AutoHeight().Padding(0,10,0,20)[SNew(STextBlock).Text(FText::FromString(Warning?TEXT("DANGER  /  RAPTOR PEN"):TEXT("GIT-META PARK  /  THIS WAY"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",32)).ColorAndOpacity(FLinearColor(1,.78,.36)).Justification(ETextJustify::Center)];
 Body->AddSlot().FillHeight(1)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(Warning?1080:1200)[SNew(STextBlock).Text(FText::FromString(Title)).Font(FCoreStyle::GetDefaultFontStyle("Bold",110)).WrapTextAt(Warning?1080:1200).Justification(ETextJustify::Center).ColorAndOpacity(FLinearColor(1,.91,.66)).ShadowOffset(FVector2D(3,4)).ShadowColorAndOpacity(FLinearColor(.12,.055,.02))]]];
 W->SetSlateWidget(Body);
}
FParkWoodSign::~FParkWoodSign(){if(Actor.IsValid())Actor->Destroy();}
void FParkWoodSign::Update(float Seconds,ACameraActor* Camera,FVector Position,FVector Eye,float Reveal) {
 if(!Actor.IsValid())return;
 // Start the complete warning sign outside the left edge, including its post.
 // Use its own clock because the slide panel can already be fully expanded.
 const FRotator Facing(0,(Eye-Position).Rotation().Yaw,0);
 if(Warning) {
  if(Reveal<=0)EntranceStart=-1;
  else if(EntranceStart<0)EntranceStart=Seconds;
  const float Progress=EntranceStart<0?0:FMath::Clamp((Seconds-EntranceStart)/.85f,0.f,1.f);
  const float Ease=1-FMath::Pow(1-Progress,3.f);
  const FVector ToSign=Position-Camera->GetActorLocation(),Right=Camera->GetActorRightVector();
  const float HalfWidth=FMath::Max(1.f,FVector::DotProduct(ToSign,Camera->GetActorForwardVector()))*FMath::Tan(FMath::DegreesToRadians(Camera->GetCameraComponent()->FieldOfView*.5f));
  Position-=Right*(HalfWidth+FVector::DotProduct(ToSign,Right)+1100.f)*(1-Ease);
 }
 // Keep the final upright orientation throughout the entrance.
 Actor->SetActorLocationAndRotation(Position,Facing);
 Actor->SetActorScale3D(FVector(.92f));Actor->SetActorHiddenInGame(Reveal<=0);
 Board->SetRelativeRotation(FRotator(Warning?FMath::Sin(Seconds*.9f)*1.2f:0,0,Warning?FMath::Sin(Seconds*1.35f)*2.f:0));
}
