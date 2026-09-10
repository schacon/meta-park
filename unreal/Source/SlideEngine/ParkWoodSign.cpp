#include "ParkWoodSign.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
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

FParkWoodSign::FParkWoodSign(UWorld* World,const FString& Title,bool IsWarning,bool IsHelipad):Warning(IsWarning),Helipad(IsHelipad) {
 auto* A=World->SpawnActor<AActor>();Actor=A;A->SetActorHiddenInGame(true);
 auto* Root=NewObject<USceneComponent>(A);A->AddInstanceComponent(Root);A->SetRootComponent(Root);Root->SetMobility(EComponentMobility::Movable);Root->RegisterComponent();
 auto Material=[&](UPrimitiveComponent* C,FColor Color){auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_IslandLit.M_IslandLit")),C);M->SetVectorParameterValue(TEXT("Tint"),FLinearColor::FromSRGBColor(Color));return M;};
 auto Box=[&](USceneComponent* Parent,FVector P,FVector Size,FColor Color,FRotator Rotation=FRotator::ZeroRotator){
  auto* C=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(C);C->SetupAttachment(Parent);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetRelativeLocation(P);C->SetRelativeScale3D(Size/100);C->SetRelativeRotation(Rotation);C->SetMaterial(0,Material(C,Color));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();return C;
 };
 if(IsHelipad) {
  // Enamel aviation placard on galvanized posts, with a raised helicopter pictogram.
  const FColor Metal(155,169,177),Navy(22,58,91),White(237,243,235);
  for(float Y:{-610.f,610.f}) {
   Box(Root,FVector(-45,Y,-565),FVector(70,70,1270),Metal);
   Box(Root,FVector(-45,Y,-1190),FVector(150,150,35),Metal);
  }
  Board=NewObject<USceneComponent>(A);A->AddInstanceComponent(Board);Board->SetupAttachment(Root);Board->SetMobility(EComponentMobility::Movable);Board->RegisterComponent();
  Box(Board,FVector::ZeroVector,FVector(65,1660,790),Metal);
  Box(Board,FVector(36,0,0),FVector(12,1610,740),Navy);
  for(float Z:{-346.f,346.f})Box(Board,FVector(44,0,Z),FVector(5,1560,10),White);
  for(float Y:{-775.f,775.f})Box(Board,FVector(44,Y,0),FVector(5,10,695),White);
  for(float Y:{-801.f,801.f})for(float Z:{-371.f,371.f})Box(Board,FVector(44,Y,Z),FVector(10,13,13),FColor(80,98,110));
  Box(Board,FVector(45,325,0),FVector(5,6,575),White);
  auto Icon=[&](float Y,float Z,float Width,float Height,FColor Color){return Box(Board,FVector(51,Y,Z),FVector(9,Width,Height),Color);};
  // Cabin and nose, tail boom and rotor, main rotor and landing skids.
  auto* Cabin=Icon(537,25,176,108,White);Cabin->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
  Icon(648,55,115,23,White);Icon(704,80,18,85,White);
  Icon(537,103,14,52,White);Icon(537,134,304,12,White);
  Icon(503,43,47,49,Navy)->SetRelativeLocation(FVector(58,503,43));
  for(float Y:{495.f,576.f})Icon(Y,-54,12,66,White);
  Icon(537,-88,218,13,White);
  Icon(435,-78,12,28,White);
  auto* W=NewObject<UWidgetComponent>(A);A->AddInstanceComponent(W);W->SetupAttachment(Board);W->SetWidgetSpace(EWidgetSpace::World);W->SetDrawSize(FVector2D(1020,590));W->SetBlendMode(EWidgetBlendMode::Masked);W->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_PropLettering.M_PropLettering")));W->SetTwoSided(true);W->SetCollisionEnabled(ECollisionEnabled::NoCollision);W->SetCastShadow(false);W->SetRelativeLocation(FVector(55,-225,0));W->RegisterComponent();
  auto Body=SNew(SVerticalBox);
  Body->AddSlot().AutoHeight().Padding(0,22,0,24)[SNew(STextBlock).Text(FText::FromString(TEXT("HELIPAD  /  PARK AVIATION"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",30)).ColorAndOpacity(FLinearColor(.95,.98,1)).Justification(ETextJustify::Center)];
  Body->AddSlot().FillHeight(1)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(970)[SNew(STextBlock).Text(FText::FromString(Title)).Font(FCoreStyle::GetDefaultFontStyle("Bold",92)).WrapTextAt(970).Justification(ETextJustify::Center).ColorAndOpacity(FLinearColor(.95,.98,1))]]];
  Body->AddSlot().AutoHeight().Padding(0,20,0,22)[SNew(STextBlock).Text(FText::FromString(TEXT("ARRIVALS  •  DEPARTURES"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",25)).ColorAndOpacity(FLinearColor(.95,.98,1)).Justification(ETextJustify::Center)];
  W->SetSlateWidget(Body);return;
 }
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
 // Start the warning sign off the left edge and the helipad sign below the frame.
 // Use its own clock because the slide panel can already be fully expanded.
 const FRotator Facing(0,(Eye-Position).Rotation().Yaw,0);
 if(Warning||Helipad) {
  if(Reveal<=0)EntranceStart=-1;
  else if(EntranceStart<0)EntranceStart=Seconds;
  const float Progress=EntranceStart<0?0:FMath::Clamp((Seconds-EntranceStart)/(Helipad?1.05f:.85f),0.f,1.f);
  const float Ease=1-FMath::Pow(1-Progress,3.f);
  const FVector ToSign=Position-Camera->GetActorLocation(),Right=Camera->GetActorRightVector();
  const float HalfWidth=FMath::Max(1.f,FVector::DotProduct(ToSign,Camera->GetActorForwardVector()))*FMath::Tan(FMath::DegreesToRadians(Camera->GetCameraComponent()->FieldOfView*.5f));
  if(Helipad) {
   float Aspect=16.f/9.f;
   if(auto* PC=Camera->GetWorld()->GetFirstPlayerController()) {
    int32 Width,Height;PC->GetViewportSize(Width,Height);
    if(auto* Player=PC->GetLocalPlayer())Aspect=(Width*Player->Size.X)/FMath::Max(1.f,Height*Player->Size.Y);
   }
   const float BelowFrame=HalfWidth/FMath::Max(.1f,Aspect)+FVector::DotProduct(ToSign,Camera->GetActorUpVector())+500.f;
   Position.Z-=BelowFrame*(1-Ease);
  } else Position-=Right*(HalfWidth+FVector::DotProduct(ToSign,Right)+1100.f)*(1-Ease);
 }
 // Keep the final upright orientation throughout the entrance.
 Actor->SetActorLocationAndRotation(Position,Facing);
 Actor->SetActorScale3D(FVector(.92f));Actor->SetActorHiddenInGame(Reveal<=0);
 Board->SetRelativeRotation(FRotator(Warning?FMath::Sin(Seconds*.9f)*1.2f:0,0,Warning?FMath::Sin(Seconds*1.35f)*2.f:0));
}
