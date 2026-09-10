#include "ParkEmployeeBadge.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Dom/JsonObject.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

FParkEmployeeBadge::FParkEmployeeBadge(UWorld* World,TSharedPtr<FJsonObject> Step,UTexture2D* Logo) {
 auto* A=World->SpawnActor<AActor>();Actor=A;A->SetActorHiddenInGame(true);
 auto* Root=NewObject<USceneComponent>(A);A->AddInstanceComponent(Root);A->SetRootComponent(Root);Root->SetMobility(EComponentMobility::Movable);Root->RegisterComponent();
 auto Box=[&](FVector At,FVector Size,FColor Color){
  auto* C=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetRelativeLocation(At);C->SetRelativeScale3D(Size/100);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_IslandLit.M_IslandLit")),C);M->SetVectorParameterValue(TEXT("Tint"),FLinearColor::FromSRGBColor(Color));C->SetMaterial(0,M);C->RegisterComponent();return C;
 };
 // A thick plastic holder, inset credential and metal clip suspended on a fabric strap.
 Box(FVector(-18,0,0),FVector(58,1700,1090),FColor(40,64,58));
 Box(FVector(14,0,0),FVector(18,1650,1040),FColor(226,225,208));
 Box(FVector(-32,0,805),FVector(14,75,510),FColor(24,69,55));
 Box(FVector(8,0,553),FVector(65,190,145),FColor(155,165,151));
 Box(FVector(44,0,590),FVector(15,105,55),FColor(65,77,68));
 auto* W=NewObject<UWidgetComponent>(A);A->AddInstanceComponent(W);W->SetupAttachment(Root);W->SetWidgetSpace(EWidgetSpace::World);W->SetDrawSize(FVector2D(1600,980));W->SetBlendMode(EWidgetBlendMode::Opaque);W->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_PropPaper.M_PropPaper")));W->SetRelativeLocation(FVector(25,0,0));W->SetCollisionEnabled(ECollisionEnabled::NoCollision);W->SetCastShadow(false);W->RegisterComponent();
 const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");const FLinearColor Ink(.018,.038,.029),Green(.035,.22,.14),Paper(.93,.92,.81);
 auto Text=[&](FString Value,int32 Size,bool Bold=false){return SNew(STextBlock).Text(FText::FromString(Value)).Font(FCoreStyle::GetDefaultFontStyle(Bold?"Bold":"Mono",Size)).ColorAndOpacity(Ink).AutoWrapText(true);};
 auto Body=SNew(SVerticalBox);
 for(const auto& V:Step->GetArrayField(TEXT("blocks"))){auto B=V->AsObject();Body->AddSlot().AutoHeight().Padding(0,0,0,18)[Text((B->GetStringField(TEXT("kind"))==TEXT("li"))?TEXT("• ")+B->GetStringField(TEXT("text")):B->GetStringField(TEXT("text")),42)];}
 LogoBrush.SetResourceObject(Logo);LogoBrush.ImageSize=FVector2D(460,310);LogoBrush.DrawAs=ESlateBrushDrawType::Image;
 auto Bars=SNew(SHorizontalBox);FRandomStream Random(1993);
 for(int32 I=0;I<46;I++)Bars->AddSlot().AutoWidth().Padding(2,0)[SNew(SBox).WidthOverride(Random.RandRange(2,9)).HeightOverride(70)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(0)]];
 W->SetSlateWidget(SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Paper).Padding(42)
  [SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Green).Padding(FMargin(24,18))
    [SNew(STextBlock).Text(FText::FromString(TEXT("RESEARCH CENTER  /  STAFF ACCESS"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",40)).ColorAndOpacity(Paper)]]
   +SVerticalBox::Slot().FillHeight(1).Padding(14,42)[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,40,0)[SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,20)[Text(TEXT("EMPLOYEE IDENTIFICATION"),26)]
     +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,30)[Text(Step->GetStringField(TEXT("title")),112,true)]
     +SVerticalBox::Slot().FillHeight(1)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly).HAlign(HAlign_Left).VAlign(VAlign_Top)[SNew(SBox).WidthOverride(900)[Body]]]]
    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(460).HeightOverride(380)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(12)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(&LogoBrush)]]]]]
   +SVerticalBox::Slot().AutoHeight().Padding(14,8)[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().FillWidth(1)[SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight()[Text(TEXT("s.chacon"),48,true)]
     +SVerticalBox::Slot().AutoHeight().Padding(0,8)[Text(TEXT("INGEN  /  GENETICS DIVISION"),24)]]
    +SHorizontalBox::Slot().AutoWidth()[SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight()[Bars]
     +SVerticalBox::Slot().AutoHeight().Padding(0,8)[Text(TEXT("001  /  AUTHORIZED PERSONNEL"),22)]]]]);
}
FParkEmployeeBadge::~FParkEmployeeBadge(){if(Actor.IsValid())Actor->Destroy();}
void FParkEmployeeBadge::Update(ACameraActor* Camera,FVector Position) {
 if(!Actor.IsValid())return;
 Actor->SetActorLocationAndRotation(Position,(Camera->GetActorLocation()-Position).Rotation());
 Actor->SetActorScale3D(FVector(.9f));Actor->SetActorHiddenInGame(false);
}
