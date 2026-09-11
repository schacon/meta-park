#include "ParkSpeedGraph.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Dom/JsonObject.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"

// Vector silhouettes keep their outlines crisp on the physical screen. Only
// the neck varies; the bodies stay identical so dataset size is in the labels.
class SParkSpeedGraph : public SLeafWidget {
 const FParkSpeedGraph* State=nullptr;
public:
 SLATE_BEGIN_ARGS(SParkSpeedGraph){}SLATE_ARGUMENT(const FParkSpeedGraph*,State)SLATE_END_ARGS()
 void Construct(const FArguments& A){State=A._State;}
 virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(1440,900);}
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool)const override {
  if(!State||State->Reveal<=0)return Layer;
  const float Height=900*State->Reveal;
  const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");
  auto Color=[](const TCHAR* Hex){return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));};
  const auto Ink=Color(TEXT("173f37")),Soft=Color(TEXT("40675d")),Paper=Color(TEXT("fcf9ed"));
  auto Rect=[&](float X,float Y,float W,float H,FLinearColor C){FSlateDrawElement::MakeBox(Out,Layer++,G.ToPaintGeometry(FVector2D(W,H),FSlateLayoutTransform(FVector2D(X,Y))),Brush,ESlateDrawEffect::None,C);};
  auto Text=[&](float X,float Y,const FString& Value,int32 Size,FLinearColor C){FSlateDrawElement::MakeText(Out,Layer++,G.ToPaintGeometry(FVector2D(1400-X,90),FSlateLayoutTransform(FVector2D(X,Y))),Value,FCoreStyle::GetDefaultFontStyle("Bold",Size),ESlateDrawEffect::None,C);};
  Out.PushClip(FSlateClippingZone(G.ToPaintGeometry(FVector2D(1440,Height),FSlateLayoutTransform())));
  Rect(0,0,1440,900,Paper);Text(72,42,State->Title,48,Ink);Text(74,115,State->Subtitle,21,Soft);
  const auto Resource=FSlateApplication::Get().GetRenderer()->GetResourceHandle(*Brush);
  auto Polygon=[&](const TArray<FVector2D>& Points,FLinearColor C){TArray<FSlateVertex> Vertices;TArray<SlateIndex> Indices;for(const auto P:Points)Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(),FVector2f(P),FVector2f(.5,.5),C.ToFColor(true)));for(int32 I=1;I<Points.Num()-1;I++){Indices.Add(0);Indices.Add(I);Indices.Add(I+1);}FSlateDrawElement::MakeCustomVerts(Out,Layer++,Resource,Vertices,Indices,nullptr,0,0);};
  auto Ellipse=[&](FVector2D P,FVector2D R,FLinearColor C){TArray<FVector2D> Points;for(int32 I=0;I<48;I++){float A=I*2*PI/48;Points.Add(P+FVector2D(FMath::Cos(A)*R.X,FMath::Sin(A)*R.Y));}Polygon(Points,C);};
  auto Curve=[&](FVector2D A,FVector2D B,FVector2D C,FVector2D D,float StartWidth,float EndWidth,FLinearColor Tint){
   // One continuous ribbon shares its edge vertices; separate quads leave
   // pinholes when the masked world widget is sampled at an oblique angle.
   TArray<FSlateVertex> V;TArray<SlateIndex> Indices;
   for(int32 I=0;I<=48;I++){
    const float T=I/48.f,U=1-T,W=FMath::Lerp(StartWidth,EndWidth,T);
    const auto P=U*U*U*A+3*U*U*T*B+3*U*T*T*C+T*T*T*D;
    const auto Tangent=(3*U*U*(B-A)+6*U*T*(C-B)+3*T*T*(D-C)).GetSafeNormal();
    const FVector2D N(-Tangent.Y,Tangent.X);
    for(float Side:{-1.f,1.f})V.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(),FVector2f(P+N*(W*Side)),FVector2f(.5,.5),Tint.ToFColor(true)));
    if(I>0){const int32 K=(I-1)*2;Indices.Append({SlateIndex(K),SlateIndex(K+1),SlateIndex(K+2),SlateIndex(K+1),SlateIndex(K+3),SlateIndex(K+2)});}
   }
   FSlateDrawElement::MakeCustomVerts(Out,Layer++,Resource,V,Indices,nullptr,0,0);
  };
  auto Line=[&](FVector2D A,FVector2D B,FLinearColor Tint,float Width=1.f){FSlateDrawElement::MakeLines(Out,Layer++,G.ToPaintGeometry(),{A,B},ESlateDrawEffect::None,Tint,true,Width);};
  const float ZeroY=452,PixelsPerSecond=.43f;
  // The common neck base sits below zero; height above this baseline uses
  // the same linear seconds scale for all four dinosaurs.
  for(int32 Seconds=0;Seconds<=500;Seconds+=100){const float Y=ZeroY-Seconds*PixelsPerSecond;Line({65,Y},{1385,Y},Color(TEXT("8ca295")),1.5f);Text(12,Y-14,FString::FromInt(Seconds),20,Ink);}
  Text(17,195,TEXT("s"),18,Soft);
  for(int32 I=0;I<State->Rows.Num();I++) {
   const auto& Row=State->Rows[I];const float X=90+I*335,Ground=600;
   // An illustrative common neck base keeps even a short wait recognizable as
   // a sauropod. The added neck height is linear; exact times remain prominent.
   const float Neck=76+Row.Seconds*PixelsPerSecond;
   const float HeadY=Ground-72-Neck;
   const auto Tint=I==3?Color(TEXT("397a67")):Color(TEXT("24564b"));
   const auto Far=Color(TEXT("6e8c7b"));
   Ellipse({X+144,Ground+7},{100,7},Color(TEXT("d8dfce")));
   // Far legs, broad torso, tapered tail, then near legs and the sweeping neck.
   Rect(X+109,Ground-47,21,49,Far);Rect(X+170,Ground-47,21,49,Far);
   Curve({X+91,Ground-52},{X+54,Ground-37},{X+14,Ground-40},{X+1,Ground-77},18,1,Tint);
   Ellipse({X+137,Ground-60},{78,39},Tint);
   Rect(X+93,Ground-56,23,61,Tint);Ellipse({X+107,Ground+2},{18,7},Tint);
   Rect(X+173,Ground-58,22,63,Tint);Ellipse({X+187,Ground+2},{18,7},Tint);
   auto NeckCurve=[&](FLinearColor NeckColor){Curve({X+184,Ground-68},{X+229,Ground-102},{X+208,HeadY+35},{X+244,HeadY},24,10,NeckColor);};
   NeckCurve(Tint);
   const FLinearColor PhaseColors[]={Color(TEXT("3c549a")),Color(TEXT("8298be")),Color(TEXT("a87925")),Color(TEXT("287b62")),Color(TEXT("6b7181"))};
   float Sum=0;for(const auto& Phase:Row.Phases)if(Phase.InNeck)Sum+=Phase.Seconds;
   float Cumulative=0;
   for(int32 P=0;P<Row.Phases.Num();P++) {
    const auto& Phase=Row.Phases[P];const auto PhaseColor=PhaseColors[P%5];
    if(Phase.InNeck&&Phase.Seconds>0&&Sum>0) {
     const float StartY=ZeroY-Cumulative*Row.Seconds/Sum*PixelsPerSecond;Cumulative+=Phase.Seconds;
     const float EndY=ZeroY-Cumulative*Row.Seconds/Sum*PixelsPerSecond;
     Out.PushClip(FSlateClippingZone(G.ToPaintGeometry(FVector2D(335,StartY-EndY),FSlateLayoutTransform(FVector2D(X,EndY)))));NeckCurve(PhaseColor);Out.PopClip();
     // Find where this cumulative time crosses the curved neck, then add a
     // horizontal phase tick. Small phases retain their relative width even at subpixel size.
     float Lo=0,Hi=1;FVector2D At;
     for(int32 J=0;J<18;J++){const float T=(Lo+Hi)*.5f,U=1-T;At=U*U*U*FVector2D(X+184,Ground-68)+3*U*U*T*FVector2D(X+229,Ground-102)+3*U*T*T*FVector2D(X+208,HeadY+35)+T*T*T*FVector2D(X+244,HeadY);if(At.Y>EndY)Lo=T;else Hi=T;}
     if(P==3){for(float Offset=-48;Offset<48;Offset+=12)Line({At.X+Offset,EndY},{At.X+Offset+7,EndY},Ink,2);}
     else Line({At.X-22,EndY},{At.X+30,EndY},PhaseColor,2);
    }
    const float Y=700+P*29;Rect(X+42,Y+8,17,4,PhaseColor);
    const FString Value=Phase.Seconds==0?TEXT("—"):FString::Printf(TEXT("%g s"),Phase.Seconds);
    Text(X+68,Y,Phase.Name,21,Ink);Text(X+238,Y,Value,21,Ink);
   }
   Ellipse({X+253,HeadY-2},{29,14},Tint);Ellipse({X+275,HeadY+2},{17,9},Tint);
   Ellipse({X+261,HeadY-6},{3,3},Paper);
   Line({X+55,HeadY-2},{X+231,HeadY-2},Tint,2);
   Text(X+62,HeadY-40,Row.Time,25,Ink);
   Text(X+50,642,Row.Label,30,Ink);
  }
  Text(74,863,TEXT("Phase values are rounded. The shallow run has no repack phase."),17,Soft);
  Out.PopClip();return Layer;
 }
};
FParkSpeedGraph::FParkSpeedGraph(UWorld* World,TSharedPtr<FJsonObject> Component) {
 Title=Component->GetStringField(TEXT("title"));Subtitle=Component->GetStringField(TEXT("subtitle"));
 for(auto Item:Component->GetArrayField(TEXT("rows"))){auto O=Item->AsObject();FSpeedDinosaur Row;Row.Label=O->GetStringField(TEXT("label"));Row.Time=O->GetStringField(TEXT("time"));Row.Seconds=O->GetNumberField(TEXT("seconds"));for(auto V:O->GetArrayField(TEXT("phases"))){auto P=V->AsObject();Row.Phases.Add({P->GetStringField(TEXT("name")),float(P->GetNumberField(TEXT("seconds"))),P->GetBoolField(TEXT("inNeck"))});}Rows.Add(Row);}
 auto* A=World->SpawnActor<AActor>();Actor=A;A->SetActorHiddenInGame(true);
 auto* Root=NewObject<USceneComponent>(A);A->AddInstanceComponent(Root);A->SetRootComponent(Root);Root->SetMobility(EComponentMobility::Movable);Root->RegisterComponent();
 auto Box=[&](FVector P,FVector Size,FColor Tint){auto* C=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetRelativeLocation(P);C->SetRelativeScale3D(Size/100);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_IslandLit.M_IslandLit")),C);M->SetVectorParameterValue(TEXT("Tint"),FLinearColor::FromSRGBColor(Tint));C->SetMaterial(0,M);C->RegisterComponent();return C;};
 Box(FVector(0,0,478),FVector(70,1520,65),FColor(57,66,62));
 Box(FVector(39,0,481),FVector(10,1438,13),FColor(165,179,162));
 Roller=Box(FVector(15,0,450),FVector(32,1480,28),FColor(50,63,56));
 Handle=Box(FVector(16,0,422),FVector(16,80,34),FColor(50,63,56));
 auto* Sheet=NewObject<UWidgetComponent>(A);A->AddInstanceComponent(Sheet);Sheet->SetupAttachment(Root);Sheet->SetWidgetSpace(EWidgetSpace::World);Sheet->SetDrawSize(FVector2D(1440,900));Sheet->SetBlendMode(EWidgetBlendMode::Masked);Sheet->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_PropLettering.M_PropLettering")));Sheet->SetTwoSided(true);Sheet->SetCollisionEnabled(ECollisionEnabled::NoCollision);Sheet->SetCastShadow(false);Sheet->SetTickWhenOffscreen(true);Sheet->SetRelativeLocation(FVector(12,0,0));Sheet->RegisterComponent();Sheet->SetSlateWidget(SNew(SParkSpeedGraph).State(this));
}
FParkSpeedGraph::~FParkSpeedGraph(){if(Actor.IsValid())Actor->Destroy();}
void FParkSpeedGraph::Update(float Delta,ACameraActor*,FVector Position,FVector Eye,bool Visible) {
 if(!Actor.IsValid())return;
 if(Visible)Reveal=FMath::Min(1.f,Reveal+Delta/1.6f);else Reveal=0;
 Actor->SetActorHiddenInGame(!Visible);Actor->SetActorLocationAndRotation(Position,(Eye-Position).Rotation());
 Roller->SetRelativeLocation(FVector(15,0,450-Reveal*900));Handle->SetRelativeLocation(FVector(16,0,422-Reveal*900));
}
