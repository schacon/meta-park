#include "ParkRaptors.h"
#include "IslandScene.h"
#include "ParkCamera.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

namespace {
class SWorkerTally:public SLeafWidget {
public:
 SLATE_BEGIN_ARGS(SWorkerTally){} SLATE_ATTRIBUTE(int32,Count) SLATE_END_ARGS()
 TAttribute<int32> Count;
 void Construct(const FArguments& A){Count=A._Count;}
 virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(560,72);}
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool)const override {
  const FLinearColor Ink(.22,.20,.15),Red(.60,.025,.015);
  auto Line=[&](TArray<FVector2D> P,FLinearColor C,float Width){FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,Width);};
  for(int32 I=0;I<Count.Get(0);I++) {
   const float X=24+I*62;
   TArray<FVector2D> Head;for(int32 K=0;K<=12;K++){float A=K*2*PI/12;Head.Add(FVector2D(X+7*FMath::Cos(A),13+7*FMath::Sin(A)));}Line(Head,Ink,3);
   Line({FVector2D(X,22),FVector2D(X,45)},Ink,7);
   Line({FVector2D(X-13,37),FVector2D(X,25),FVector2D(X+13,37)},Ink,4);
   Line({FVector2D(X-12,63),FVector2D(X,43),FVector2D(X+12,63)},Ink,5);
   Line({FVector2D(X-22,66),FVector2D(X+23,2)},Red,4);
  }
  return Layer;
 }
};
}
FParkRaptors::FParkRaptors(UWorld* World,TSharedPtr<FJsonObject> Component,ACameraActor* Camera,FVector Look,float Width) {
 OriginalEye=Camera->GetActorLocation();OriginalRotation=Camera->GetActorQuat();OriginalLook=Look;OriginalWidth=Width;
 const int32 Counts[]={2,4,3,6};
 for(const auto& V:Component->GetArrayField(TEXT("raptors"))) {auto J=V->AsObject();FRaptorIssues R;R.Label=J->GetStringField(TEXT("label"));for(const auto& P:J->GetArrayField(TEXT("problems")))R.Problems.Add(P->AsString());R.Workers=Counts[Raptors.Num()%4];double K;if(J->TryGetNumberField(TEXT("workers"),K))R.Workers=int32(K);Raptors.Add(R);}
 auto* A=World->SpawnActor<AActor>();Actor=A;auto* Root=NewObject<USceneComponent>(A);A->AddInstanceComponent(Root);A->SetRootComponent(Root);Root->RegisterComponent();
 auto Node=[&](USceneComponent* Parent){auto* N=NewObject<USceneComponent>(A);A->AddInstanceComponent(N);N->SetupAttachment(Parent);N->SetMobility(EComponentMobility::Movable);N->RegisterComponent();return N;};
 auto Box=[&](USceneComponent* Parent,FVector P,FVector Size,FColor Color,bool Unlit=false){auto* C=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(C);C->SetupAttachment(Parent);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetRelativeLocation(P);C->SetRelativeScale3D(Size/100);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,Unlit?TEXT("/Game/Models/M_Retro.M_Retro"):TEXT("/Game/Models/M_IslandLit.M_IslandLit")),C);M->SetVectorParameterValue(TEXT("Tint"),FLinearColor::FromSRGBColor(Color));C->SetMaterial(0,M);C->RegisterComponent();return C;};
 auto Widget=[&](USceneComponent* Parent,FVector At,FVector2D Size,float Scale){auto* W=NewObject<UWidgetComponent>(A);A->AddInstanceComponent(W);W->SetupAttachment(Parent);W->SetWidgetSpace(EWidgetSpace::World);W->SetDrawSize(Size);W->SetTwoSided(true);W->SetBlendMode(EWidgetBlendMode::Opaque);W->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_PropPaper.M_PropPaper")));W->SetRelativeLocation(At);W->SetRelativeScale3D(FVector(Scale));W->SetCollisionEnabled(ECollisionEnabled::NoCollision);W->SetCastShadow(false);W->RegisterComponent();return W;};
 for(int32 I=0;I<Raptors.Num();I++) {
  auto* W=Widget(Root,FVector::ZeroVector,FVector2D(900,240),1.2f);Labels.Add(W);
  W->SetSlateWidget(SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor_Lambda([this,I]{return Selected==I?FLinearColor(.75,.18,.035):FLinearColor(.035,.12,.11);}).Padding(12)
   [SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%02d  %s"),I+1,*Raptors[I].Label))).Font(FCoreStyle::GetDefaultFontStyle("Bold",64)).AutoWrapText(true).Justification(ETextJustify::Center).ColorAndOpacity(FLinearColor(1,.94,.72))]);
 }
 Clipboard=Node(Root);
 Box(Clipboard,FVector(0,0,0),FVector(20,480,610),FColor(109,66,29));
 for(int32 I=0;I<3;I++)Box(Clipboard,FVector(-13-I*2,I*2,-I*2),FVector(2,438,564),FColor(218,209,175),true);
 auto* UnderPage=Widget(Clipboard,FVector(-21,0,0),FVector2D(700,900),.626f);UnderPage->SetRelativeRotation(FRotator(0,180,0));
 UnderPage->SetSlateWidget(BuildRecord([this]{return Displayed;}));
 Paper=Node(Clipboard);Paper->SetRelativeLocation(FVector(-27,0,282));
 Box(Paper,FVector(2,0,-282),FVector(2,438,563),FColor(235,230,214),true);
 auto* Page=Widget(Paper,FVector(0,0,-282),FVector2D(700,900),.626f);Page->SetRelativeRotation(FRotator(0,180,0));
 Page->SetSlateWidget(BuildRecord([this]{return PreviousDisplayed;}));
 Paper->SetVisibility(false,true);
 Box(Clipboard,FVector(-29,0,284),FVector(16,155,56),FColor(128,139,126));
 Box(Clipboard,FVector(-39,0,298),FVector(12,72,40),FColor(77,85,77));
 for(int32 I=0;I<40;I++) {float R=I*2*PI/40;auto* B=Box(Root,FVector::ZeroVector,FVector(13,58,6),FColor(255,214,89),true);B->SetRelativeRotation(FRotator(0,FMath::RadiansToDegrees(R),0));Ring.Add(B);}
 Spotlight=NewObject<UPointLightComponent>(A);A->AddInstanceComponent(Spotlight);Spotlight->SetupAttachment(Root);Spotlight->SetMobility(EComponentMobility::Movable);Spotlight->SetIntensity(18000);Spotlight->SetAttenuationRadius(1100);Spotlight->SetLightColor(FLinearColor(1,.83,.36));Spotlight->SetCastShadows(false);Spotlight->RegisterComponent();
}
TSharedRef<SWidget> FParkRaptors::BuildRecord(TFunction<int32()> Record) {
 const FLinearColor Ink(.025,.018,.01),Red(.38,.02,.01);
 auto Text=[&](TAttribute<FText> Value,int32 Size,FLinearColor Color){return SNew(STextBlock).Text(Value).Font(FCoreStyle::GetDefaultFontStyle("Mono",Size)).ColorAndOpacity(Color).AutoWrapText(true);};
 return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.91,.85,.66)).Padding(FMargin(40,60,40,28))
  [SNew(SOverlay)
   +SOverlay::Slot()[SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center).Visibility_Lambda([Record]{return Record()<0?EVisibility::Visible:EVisibility::Collapsed;})
    [SNew(STextBlock).Text(FText::FromString(TEXT("Incident Reports"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",90)).WrapTextAt(600).Justification(ETextJustify::Center).ColorAndOpacity(Ink)]]
   +SOverlay::Slot()[SNew(SVerticalBox).Visibility_Lambda([Record]{return Record()>=0?EVisibility::Visible:EVisibility::Collapsed;})
    +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,20)[Text(FText::FromString(TEXT("INGEN / ANIMAL CONTROL\nCONFIDENTIAL INCIDENT RECORD")),20,Red)]
    +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,22)[Text(TAttribute<FText>::CreateLambda([Record]{return FText::FromString(FString::Printf(TEXT("SPECIMEN %02d / 04"),Record()+1));}),22,Ink)]
    +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,24)[SNew(STextBlock).Text_Lambda([this,Record]{return FText::FromString(Record()<0?TEXT(""):Raptors[Record()].Label);}).Font(FCoreStyle::GetDefaultFontStyle("Bold",38)).AutoWrapText(true).ColorAndOpacity(Ink)]
    +SVerticalBox::Slot().FillHeight(1)[Text(TAttribute<FText>::CreateLambda([this,Record]{FString S;if(Record()>=0)for(const auto& P:Raptors[Record()].Problems)S+=TEXT("• ")+P+TEXT("\n\n");return FText::FromString(S);}),25,Ink)]
    +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,8)[Text(FText::FromString(TEXT("STAFF LOST / FIELD TALLY")),19,Red)]
    +SVerticalBox::Slot().AutoHeight()[SNew(SWorkerTally).Count_Lambda([this,Record]{return Record()<0?0:Raptors[Record()].Workers;})]
   ]
  ];
}
FParkRaptors::~FParkRaptors(){if(Actor.IsValid())Actor->Destroy();}
bool FParkRaptors::Advance(int32 Direction) {
 if(!Ready())return true;
 int32 Next=Selected+Direction;if(Next<INDEX_NONE||Next>=Raptors.Num())return false;
 PreviousDisplayed=Displayed;Selected=Next;Displayed=Next;Flip=0;
 Paper->SetRelativeRotation(FRotator::ZeroRotator);Paper->SetVisibility(true,true);return true;
}
FString FParkRaptors::Label()const{return Displayed<0?TEXT("Incident Reports"):Raptors[Displayed].Label;}
FString FParkRaptors::Problems()const {
 if(Displayed<0)return TEXT("");
 FString S;for(const auto& P:Raptors[Displayed].Problems)S+=TEXT("• ")+P+TEXT("\n\n");return S;
}
int32 FParkRaptors::Workers()const{return Displayed<0?0:Raptors[Displayed].Workers;}
void FParkRaptors::Restore(ACameraActor* Camera,FVector& Look,float& Width){Camera->SetActorLocationAndRotation(OriginalEye,OriginalRotation);Look=OriginalLook;Width=OriginalWidth;}
void FParkRaptors::Update(float Delta,ACameraActor* Camera,FVector& Look,float& Width) {
 Flight=FMath::Min(1.f,Flight+Delta/1.4f);const float Ease=Flight*Flight*(3-2*Flight);
 // The pen occupies the left side, leaving the keeper's clipboard beside it.
 const FVector Target(-6800,4800,1200),Eye(-6800,2050,10100);
 Look=FMath::Lerp(OriginalLook,Target,Ease);Width=FMath::Lerp(OriginalWidth,7400.f,Ease);
 const FVector Position=FMath::Lerp(OriginalEye,Eye,Ease)+FVector(0,0,FMath::Sin(PI*Ease)*700);
 Camera->SetActorLocationAndRotation(Position,(Look-Position).Rotation());Cast<AParkCamera>(Camera)->FrameScene(Width,FVector::Distance(Position,Look));
 Flip=FMath::Min(1.f,Flip+Delta/.65f);
 // The outgoing sheet travels only upward; the next record is already underneath.
 const float PageEase=Flip*Flip*(3-2*Flip);
 Paper->SetRelativeRotation(FRotator(-180.f*PageEase,0,0));
 Paper->SetVisibility(Flip<1,true);
 const auto Points=IslandScene::RaptorPositions();
 for(int32 I=0;I<Labels.Num();I++)if(Points.IsValidIndex(I)) {
  const FVector P=Points[I]+Camera->GetActorUpVector()*340+FVector(0,0,260);Labels[I]->SetWorldLocationAndRotation(P,Camera->GetActorQuat()*FRotator(0,180,0).Quaternion());
 }
 for(int32 I=0;I<Ring.Num();I++) {
  Ring[I]->SetVisibility(Selected>=0);
  if(Points.IsValidIndex(Selected)){const float R=I*2*PI/Ring.Num();Ring[I]->SetWorldLocation(Points[Selected]+FVector(365*FMath::Cos(R),365*FMath::Sin(R),-155));}
 }
 Spotlight->SetVisibility(Selected>=0);if(Points.IsValidIndex(Selected))Spotlight->SetWorldLocation(Points[Selected]+FVector(0,0,600));
 int32 W,H;auto* PC=Camera->GetWorld()->GetFirstPlayerController();PC->GetViewportSize(W,H);auto* Player=PC->GetLocalPlayer();
 const float TX=FMath::Tan(FMath::DegreesToRadians(Camera->GetCameraComponent()->FieldOfView*.5f)),TY=TX/((W*Player->Size.X)/FMath::Max(1.f,H*Player->Size.Y));
 const float Distance=1100,Scale=FMath::Min(Distance*TX*.76f/480.f,Distance*TY*1.72f/610.f);
 Clipboard->SetWorldLocationAndRotation(Camera->GetActorLocation()+Camera->GetActorForwardVector()*Distance+Camera->GetActorRightVector()*(Distance*TX*.55f),Camera->GetActorQuat());Clipboard->SetWorldScale3D(FVector(Scale));
}
