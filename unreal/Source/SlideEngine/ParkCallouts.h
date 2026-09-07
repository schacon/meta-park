#pragma once
#include "SlideGameMode.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "GameFramework/PlayerController.h"
#include "Input/Reply.h"

using FGetParkPins=TFunction<TArray<FParkMapPin>()>;
using FChooseParkPin=TFunction<void(int32)>;

// Screen-space cards stay upright and readable while their leader lines follow world landmarks.
class SParkCallouts : public SLeafWidget {
public:
 SLATE_BEGIN_ARGS(SParkCallouts) {}
  SLATE_ARGUMENT(APlayerController*,Controller)
  SLATE_ARGUMENT(FGetParkPins,Pins)
  SLATE_ARGUMENT(FChooseParkPin,Choose)
 SLATE_END_ARGS()
 void Construct(const FArguments& Args) { Controller=Args._Controller; Pins=Args._Pins; Choose=Args._Choose; }
 virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1600,1000); }
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override {
  HitAreas.Reset(); if(!Controller.IsValid())return Layer;
  int32 Width,Height;Controller->GetViewportSize(Width,Height);if(!Width||!Height)return Layer;
  const FVector2D Scale=G.GetLocalSize()/FVector2D(Width,Height);
  const FLinearColor Ink(.018,.028,.025),Paper(.66,.67,.63),Green(.15,.61,.25);
  const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");
  auto Box=[&](FVector2D P,FVector2D S,FLinearColor Color,int32 Z){FSlateDrawElement::MakeBox(Out,Layer+Z,G.ToPaintGeometry(S,FSlateLayoutTransform(P)),Brush,ESlateDrawEffect::None,Color);};
  auto Text=[&](FString Value,FVector2D P,int32 Size,int32 Z){FSlateDrawElement::MakeText(Out,Layer+Z,G.ToPaintGeometry(FVector2D(220,24),FSlateLayoutTransform(P)),Value,FCoreStyle::GetDefaultFontStyle("Mono",Size),ESlateDrawEffect::None,Ink);};
  for(const auto& Pin:Pins()) {
   FVector2D Screen;if(!Controller->ProjectWorldLocationToScreen(Pin.Position,Screen))continue;
   const FVector2D Anchor=Screen*Scale; if(Anchor.X<370||Anchor.X>G.GetLocalSize().X-30||Anchor.Y<100||Anchor.Y>G.GetLocalSize().Y-20)continue;
   const FVector2D Size(208,79);
   FVector2D P=Anchor-FVector2D(Size.X*.5,145);
   P.X=FMath::Clamp(P.X,365.,G.GetLocalSize().X-Size.X-20);P.Y=FMath::Max(20.,P.Y);
   FSlateDrawElement::MakeLines(Out,Layer+1,G.ToPaintGeometry(),{FVector2D(Anchor.X,P.Y+Size.Y),Anchor},ESlateDrawEffect::None,Ink,true,2);
   Box(P+FVector2D(5,6),Size,Ink,2);Box(P,Size,Ink,3);Box(P+FVector2D(3,3),Size-FVector2D(6,6),Paper,4);
   Text(Pin.Code,P+FVector2D(12,8),14,5);
   Text(Pin.SlideIndex>=0?FString::Printf(TEXT("%02d"),Pin.SlideIndex+1):TEXT("—"),P+FVector2D(173,8),14,5);
   Text(Pin.Name,P+FVector2D(12,29),14,5);
   const bool Live=Pin.Status==TEXT("OK")||Pin.Status==TEXT("OPEN");
   Box(P+FVector2D(12,54),FVector2D(Pin.Status.Len()*10+12,19),Live?Green:FLinearColor(.39,.41,.37),5);
   Text(Pin.Status,P+FVector2D(18,53),13,6);
   TArray<FVector2D> Circle;for(int I=0;I<=20;I++){float A=I*2*PI/20;Circle.Add(Anchor+FVector2D(FMath::Cos(A),FMath::Sin(A))*8);}
   Box(Anchor-FVector2D(5,5),FVector2D(10,10),Ink,7);
   FSlateDrawElement::MakeLines(Out,Layer+8,G.ToPaintGeometry(),Circle,ESlateDrawEffect::None,FLinearColor(.96,.96,.87),true,2.5);
   if(Pin.SlideIndex>=0)HitAreas.Add({FSlateRect(P.X,P.Y,P.X+Size.X,P.Y+Size.Y),Pin.SlideIndex});
  }
  return Layer+9;
 }
 virtual FReply OnMouseButtonDown(const FGeometry& G,const FPointerEvent& Event) override {
  const FVector2D P=G.AbsoluteToLocal(Event.GetScreenSpacePosition());
  for(const auto& H:HitAreas)if(H.Key.ContainsPoint(P)){Choose(H.Value);return FReply::Handled();}
  return FReply::Unhandled();
 }
private:
 TWeakObjectPtr<APlayerController> Controller;
 FGetParkPins Pins;FChooseParkPin Choose;
 mutable TArray<TPair<FSlateRect,int32>> HitAreas;
};
