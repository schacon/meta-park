#include "ParkFSV.h"
#include "Dom/JsonObject.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"
#include "InputCoreTypes.h"

FParkFSV::FParkFSV(TSharedPtr<FJsonObject> Component) {
 Title=Component->GetStringField(TEXT("title"));
 for(const auto& Value:Component->GetArrayField(TEXT("systems"))) {
  const auto System=Value->AsObject();Systems.Add({System->GetStringField(TEXT("label")),System->GetStringField(TEXT("meta"))});
 }
 Heights.Init(0,Systems.Num());
}
void FParkFSV::Tick(float Delta){for(int32 I=0;I<Heights.Num();I++)Heights[I]=FMath::FInterpConstantTo(Heights[I],I==Selected?1.f:0.f,Delta,2.f);}
bool FParkFSV::Advance(int32 Direction){const int32 Next=Selected+Direction;if(Next<INDEX_NONE||Next>=Systems.Num())return false;Select(Next);return true;}

class SParkFSVView : public SLeafWidget {
public:
 SLATE_BEGIN_ARGS(SParkFSVView){}SLATE_ARGUMENT(TFunction<TSharedPtr<FParkFSV>()>,State)SLATE_END_ARGS()
 TFunction<TSharedPtr<FParkFSV>()> State;
 mutable TArray<FSlateRect> Hits;
 void Construct(const FArguments& Args){State=Args._State;SetClipping(EWidgetClipping::ClipToBoundsAlways);}
 virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(900,620);}
 virtual FReply OnMouseButtonDown(const FGeometry& G,const FPointerEvent& Event) override {
  if(Event.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();
  const auto FSV=State();if(!FSV)return FReply::Unhandled();const auto P=G.AbsoluteToLocal(Event.GetScreenSpacePosition());
  for(int32 I=0;I<Hits.Num();I++)if(Hits[I].ContainsPoint(P)){FSV->Select(I);return FReply::Handled();}
  return FReply::Unhandled();
 }
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override {
  const auto FSV=State();if(!FSV)return Layer;
  const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");
  const auto Resource=FSlateApplication::Get().GetRenderer()->GetResourceHandle(*Brush);
  const auto Font=FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("Fonts/RobotoMono-Regular.ttf"),17);
  auto Ink=[](const TCHAR* Hex){return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));};
  const auto Cyan=Ink(TEXT("80c8d4")),Blue=Ink(TEXT("579fd1")),Rose=Ink(TEXT("a8637d"));
  auto Rect=[&](FVector2D At,FVector2D Size,FLinearColor Color){FSlateDrawElement::MakeBox(Out,Layer++,G.ToPaintGeometry(Size,FSlateLayoutTransform(At)),Brush,ESlateDrawEffect::None,Color);};
  auto Line=[&](TArray<FVector2D> Points,FLinearColor Color,float Width=1.f){FSlateDrawElement::MakeLines(Out,Layer++,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color,true,Width);};
  auto Text=[&](FVector2D At,const FString& Value,FLinearColor Color,int32 Size=17){auto F=Font;F.Size=Size;FSlateDrawElement::MakeText(Out,Layer++,G.ToPaintGeometry(FVector2D(900,80),FSlateLayoutTransform(At)),Value,F,ESlateDrawEffect::None,Color);};
  auto Poly=[&](TArray<FVector2D> Points,FLinearColor Color){
   TArray<FSlateVertex> Vertices;TArray<SlateIndex> Indices;
   for(const auto& P:Points)Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(),FVector2f(P),FVector2f(.5f,.5f),Color.ToFColor(true)));
   for(int32 I=1;I<Points.Num()-1;I++){Indices.Add(0);Indices.Add(I);Indices.Add(I+1);}
   FSlateDrawElement::MakeCustomVerts(Out,Layer++,Resource,Vertices,Indices,nullptr,0,0);
  };
  Rect(FVector2D::ZeroVector,G.GetLocalSize(),Ink(TEXT("090b14")));
  const float W=G.GetLocalSize().X,H=G.GetLocalSize().Y;
  auto Project=[&](float X,float Y,float Z){const float K=1000.f/(1000+Y);return FVector2D(W*.46f+X*K,H*.77f-Y*.72f*K-Z*K);};
  auto Box=[&](float X,float Y,float SX,float SY,float Bottom,float Top,FLinearColor Color){
   const auto A=Project(X-SX,Y-SY,Top),B=Project(X+SX,Y-SY,Top),C=Project(X+SX,Y+SY,Top),D=Project(X-SX,Y+SY,Top);
   Poly({A,Project(X-SX,Y-SY,Bottom),Project(X+SX,Y-SY,Bottom),B},FLinearColor(Color.R*.65f,Color.G*.65f,Color.B*.65f,1));
   Poly({B,Project(X+SX,Y-SY,Bottom),Project(X+SX,Y+SY,Bottom),C},FLinearColor(Color.R*.43f,Color.G*.43f,Color.B*.43f,1));
   Poly({A,B,C,D},Color);Line({A,B,C,D,A},Ink(TEXT("273d59")));
  };
  const int32 Columns=FMath::CeilToInt(FMath::Sqrt(float(FSV->Systems.Num()))),Rows=FMath::DivideAndRoundUp(FSV->Systems.Num(),Columns);
  const float Spacing=FMath::Min(215.f,(W-110.f)/Columns);
  auto Position=[&](int32 I){return FVector2D((I%Columns-(Columns-1)*.5f)*Spacing,(I/Columns)*240.f);};
  const auto Root=Project(0,Rows*240.f+30,0);
  Text(Root+FVector2D(-55,-30),TEXT("/systems"),Cyan,20);
  for(int32 I=0;I<FSV->Systems.Num();I++){const auto P=Position(I);Line({Root,Project(P.X,P.Y,0)},Cyan*.55f);}
  Hits.SetNum(FSV->Systems.Num());
  for(int32 I=FSV->Systems.Num()-1;I>=0;I--) {
   const auto P=Position(I);const float T=FSV->Heights[I],Ease=T*T*(3-2*T),Height=38+160*Ease;
   Box(P.X,P.Y,Spacing*.46f,72,0,12,Rose);
   if(I==FSV->Selected) {
    TArray<FVector2D> Pool;
    for(int32 J=0;J<32;J++){const float A=J*2*PI/32;Pool.Add(Project(P.X+FMath::Cos(A)*Spacing*.47f,P.Y+FMath::Sin(A)*68,13));}
    Poly(Pool,FLinearColor(1,.98,.65,.68f*Ease));const FVector2D First=Pool[0];Pool.Add(First);Line(Pool,FLinearColor(1,1,.78,.55f*Ease),1.5f);
    // A restrained translucent column catches the selected tile, like the SGI navigator.
    const auto BL=Project(P.X-Spacing*.46f,P.Y,13),BR=Project(P.X+Spacing*.46f,P.Y,13);
    const auto TL=Project(P.X-Spacing*.28f,P.Y,390),TR=Project(P.X+Spacing*.28f,P.Y,390);
    Poly({BL,BR,TR,TL},FLinearColor(1,1,.8,.045f*Ease));
    Line({BL,TL,TR,BR},FLinearColor(1,1,.8,.30f*Ease));
   }
   Box(P.X,P.Y,Spacing*.33f,46,12,Height,I==FSV->Selected?Ink(TEXT("79c5ed")):Blue);
   const auto Top=Project(P.X,P.Y,Height),Base=Project(P.X,P.Y-76,0);
   const auto Label=FSV->Systems[I].Label;
   const float LabelWidth=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label,Font).X;
   Text(Top+FVector2D(-LabelWidth*.5f*16/17,-10),Label,Ink(TEXT("172f48")),16);
   const auto Left=Project(P.X-Spacing*.33f,P.Y-46,Height),Right=Project(P.X+Spacing*.33f,P.Y-46,12);
   Hits[I]=FSlateRect(Left.X,Top.Y-30,Right.X,Base.Y+30);
   if(I==FSV->Selected) {
    const auto A=Top+FVector2D(-Spacing*.30f,-27),B=Top+FVector2D(Spacing*.30f,27);
    for(const auto& Corner:TArray<FVector2D>{A,FVector2D(B.X,A.Y),B,FVector2D(A.X,B.Y)}) {
     const float DX=Corner.X==A.X?12:-12,DY=Corner.Y==A.Y?12:-12;
     Line({Corner+FVector2D(DX,0),Corner,Corner+FVector2D(0,DY)},FLinearColor::White,2);
    }
   }
  }
  if(FSV->Selected>=0) {
   const auto P=Position(FSV->Selected);const auto Top=Project(P.X,P.Y,38+160*FSV->Heights[FSV->Selected]);
   const FVector2D At(W-280,24);Line({Top,At+FVector2D(-16,70),At+FVector2D(0,70)},Cyan,1.5f);
   Rect(At,FVector2D(264,145),Ink(TEXT("142330")));Rect(At,FVector2D(3,145),Cyan);
   Text(At+FVector2D(16,12),FSV->Systems[FSV->Selected].Label,Ink(TEXT("ffffff")),22);
   TArray<FString> Words;FSV->Systems[FSV->Selected].Meta.ParseIntoArray(Words,TEXT(" "),true);FString Row;float Y=52;
   for(const auto& Word:Words){if((Row+Word).Len()>23&&!Row.IsEmpty()){Text(At+FVector2D(16,Y),Row,Cyan,16);Y+=24;Row.Empty();}Row+=Word+TEXT(" ");}
   if(!Row.IsEmpty())Text(At+FVector2D(16,Y),Row,Cyan,16);
  }
  return Layer;
 }
};
TSharedRef<SWidget> MakeParkFSVView(TFunction<TSharedPtr<FParkFSV>()> State){return SNew(SParkFSVView).State(State);}
