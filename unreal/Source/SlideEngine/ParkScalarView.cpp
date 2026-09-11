#include "ParkScalar.h"
#include "ParkScalarScene.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"
#include "InputCoreTypes.h"

class SParkScalarView : public SLeafWidget {
 TFunction<TSharedPtr<FParkScalar>()> State;TFunction<void()> Restore;
 struct FHit {FSlateRect Rect;int32 Kind,Index;};mutable TArray<FHit> Hits;int32 Drag=-1;bool Orbiting=false,Drawing=false;FVector2D LastPointer;
 void Slide(const FGeometry& G,const FPointerEvent& E){auto S=State();if(!S||Drag<0)return;const float V=1-FMath::Clamp(float((G.AbsoluteToLocal(E.GetScreenSpacePosition()).Y-413)/86),0.f,1.f);if(Drag==0)S->Tilt=V;else if(Drag==1)S->Height=V;else S->Speed=V;}
 void EndHighlight(){auto S=State();if(Drawing&&S&&!S->Highlights.IsEmpty())S->Highlights.Last().Released=true;Drawing=false;}
 void HighlightPoint(FVector2D P){auto S=State();if(!S||S->Highlights.IsEmpty())return;P.X=FMath::Clamp(P.X,272.,1792.);P.Y=FMath::Clamp(P.Y,231.,962.);auto& Points=S->Highlights.Last().Points;if(Points.IsEmpty()||FVector2D::Distance(Points.Last(),P)>2){if(Points.Num()>=4096)Points.RemoveAt(0);Points.Add(P);}}
public:
 SLATE_BEGIN_ARGS(SParkScalarView){}SLATE_ARGUMENT(TFunction<TSharedPtr<FParkScalar>()>,State)SLATE_ARGUMENT(TFunction<void()>,Restore)SLATE_END_ARGS()
 void Construct(const FArguments& A){State=A._State;Restore=A._Restore;SetClipping(EWidgetClipping::ClipToBoundsAlways);}
 virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(1800,1000);}
 virtual FReply OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)override {
  if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();auto S=State();if(!S)return FReply::Unhandled();const auto P=G.AbsoluteToLocal(E.GetScreenSpacePosition());
  for(const auto& H:Hits)if(H.Rect.ContainsPoint(P)){switch(H.Kind){case 0:S->Reset();break;case 1:S->Advance(-1);break;case 2:S->Advance(1);break;case 3:S->TogglePlayback();break;case 4:S->Tilt=1;break;case 5:S->Tilt=.12;S->OrbitYaw=0;break;case 6:S->Playing=false;S->Select(H.Index);break;case 7:Restore();break;case 8:S->ShowHelp=!S->ShowHelp;break;case 10:S->ToggleCamera();break;case 9:Drag=H.Index;Slide(G,E);return FReply::Handled().CaptureMouse(SharedThis(this));}return FReply::Handled();}
  if(P.X>=268&&P.X<=1796&&P.Y>=227&&P.Y<=966&&!S->ShowHelp){if(E.IsShiftDown()){EndHighlight();Orbiting=false;Drawing=true;auto& H=S->Highlights.AddDefaulted_GetRef();H.ColorIndex=S->NextHighlightColor++;HighlightPoint(P);}else{Orbiting=true;LastPointer=P;}return FReply::Handled().CaptureMouse(SharedThis(this));}return FReply::Handled();
 }
 virtual FReply OnMouseButtonDoubleClick(const FGeometry& G,const FPointerEvent& E)override {
  // A quick second stroke may be delivered as a double-click instead of a press.
  return OnMouseButtonDown(G,E);
 }
 virtual FReply OnMouseMove(const FGeometry& G,const FPointerEvent& E)override {if(Drawing){HighlightPoint(G.AbsoluteToLocal(E.GetScreenSpacePosition()));return FReply::Handled();}if(Orbiting){auto S=State();const auto P=G.AbsoluteToLocal(E.GetScreenSpacePosition());if(S)S->Orbit(P-LastPointer);LastPointer=P;return FReply::Handled();}if(Drag<0)return FReply::Unhandled();Slide(G,E);return FReply::Handled();}
 virtual FReply OnMouseButtonUp(const FGeometry& G,const FPointerEvent& E)override {if((Drag<0&&!Orbiting&&!Drawing)||E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();if(Drawing)HighlightPoint(G.AbsoluteToLocal(E.GetScreenSpacePosition()));EndHighlight();Drag=-1;Orbiting=false;return FReply::Handled().ReleaseMouseCapture();}
 virtual void OnMouseCaptureLost(const FCaptureLostEvent&)override {EndHighlight();Drag=-1;Orbiting=false;}
 virtual FReply OnMouseWheel(const FGeometry& G,const FPointerEvent& E)override {auto S=State();const auto P=G.AbsoluteToLocal(E.GetScreenSpacePosition());if(!S||P.X<268||P.Y<227||P.Y>966)return FReply::Unhandled();S->Zoom(E.GetWheelDelta());return FReply::Handled();}
 virtual FReply OnTouchGesture(const FGeometry& G,const FPointerEvent& E)override {
  auto S=State();const auto P=G.AbsoluteToLocal(E.GetScreenSpacePosition());
  if(!S||S->ShowHelp||P.X<268||P.X>1796||P.Y<227||P.Y>966)return FReply::Unhandled();
  if(E.GetGestureType()==EGestureEvent::Magnify){S->Zoom(float(E.GetGestureDelta().X)*8.f);return FReply::Handled();}
  if(E.GetGestureType()!=EGestureEvent::Scroll)return FReply::Unhandled();
  const FVector2D Delta=G.AbsoluteToLocal(E.GetScreenSpacePosition()+FVector2D(E.GetGestureDelta()))-P;
  S->Pan(Delta,1528);return FReply::Handled();
 }
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool)const override {
  auto S=State();if(!S||!S->Steps.IsValidIndex(S->Selected))return Layer;Hits.Reset();const auto& Step=S->Steps[S->Selected];
  auto C=[](const TCHAR* Hex){return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));};
  const auto Gray=C(TEXT("c2c2c2")),Edge=C(TEXT("585858")),White=C(TEXT("ffffff")),Ink=C(TEXT("171717")),Purple=C(TEXT("763bad")),Teal=C(TEXT("208b79")),Gold=C(TEXT("d09a31")),Yellow=C(TEXT("ffe38a"));
  const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
  auto Rect=[&](float X,float Y,float W,float H,FLinearColor Color){FSlateDrawElement::MakeBox(Out,Layer++,G.ToPaintGeometry(FVector2D(W,H),FSlateLayoutTransform(FVector2D(X,Y))),Brush,ESlateDrawEffect::None,Color);};
  auto Line=[&](TArray<FVector2D> P,FLinearColor Color,float Width=1){FSlateDrawElement::MakeLines(Out,Layer++,G.ToPaintGeometry(),P,ESlateDrawEffect::None,Color,true,Width);};
  auto Text=[&](float X,float Y,FString V,int32 Size,FLinearColor Color,float Max=1500,bool Mono=false){auto Font=Mono?FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("Fonts/RobotoMono-Regular.ttf"),Size):FCoreStyle::GetDefaultFontStyle("Regular",Size);if(Measure->Measure(V,Font).X>Max){while(V.Len()&&Measure->Measure(V+TEXT("…"),Font).X>Max)V.LeftChopInline(1);V+=TEXT("…");}FSlateDrawElement::MakeText(Out,Layer++,G.ToPaintGeometry(FVector2D(Max,60),FSlateLayoutTransform(FVector2D(X,Y))),V,Font,ESlateDrawEffect::None,Color);};
  auto Hit=[&](float X,float Y,float W,float H,int32 Kind,int32 Index=-1){Hits.Add({FSlateRect(X,Y,X+W,Y+H),Kind,Index});};
  auto Bevel=[&](float X,float Y,float W,float H){Rect(X,Y,W,H,Edge);Rect(X+3,Y+3,W-6,H-6,Gray);Line({{X,Y+H},{X,Y},{X+W,Y}},White,2);};
  auto Button=[&](float X,float Y,float W,float H,const FString& Label,int32 Kind,int32 Index=-1){Bevel(X,Y,W,H);Text(X+12,Y+8,Label,17,Ink,(X==17&&Kind<4)?W-72:W-22);if(X==17&&Kind<4){const TCHAR* Keys[]={TEXT("R"),TEXT("B"),TEXT("Space"),TEXT("P")};Text(X+W-56,Y+9,Keys[Kind],13,Edge,52);}Hit(X,Y,W,H,Kind,Index);};
  Bevel(0,0,1800,1000);Rect(4,4,1792,29,C(TEXT("5d6878")));Text(620,6,S->Title,19,White,1100);Button(8,6,22,22,TEXT(""),7);Button(1768,6,22,22,TEXT(""),7);
  Text(16,43,TEXT("Session"),19,Ink);Hit(12,36,83,36,7);Text(120,43,TEXT("Reset"),19,Ink);Hit(113,36,80,36,0);Text(220,43,TEXT("Bird's eye"),19,Ink);Hit(213,36,115,36,4);Text(350,43,TEXT("Front view"),19,Ink);Hit(344,36,125,36,5);Text(486,43,TEXT("Step"),19,Ink);Hit(480,36,85,36,2);Button(615,36,340,36,S->MovingCamera?TEXT("Camera: moving camera   [C]"):TEXT("Camera: full scene      [C]"),10);Text(1735,43,TEXT("Help"),19,Ink);Hit(1728,36,65,36,8);
  Line({{4,77},{1796,77}},Edge,2);Line({{265,77},{265,996}},Edge,3);
  Button(17,91,230,39,TEXT("reset"),0);Button(17,139,230,39,TEXT("step back"),1);Button(17,187,230,39,TEXT("step forward"),2);Button(17,235,230,39,S->Playing?TEXT("pause"):TEXT("play"),3);Button(17,283,230,39,TEXT("bird's eye"),4);Button(17,331,230,39,TEXT("front view"),5);
  const TCHAR* SliderLabels[]={TEXT("Tilt"),TEXT("Zoom"),TEXT("Speed")};const float Values[]={S->Tilt,S->Height,S->Speed};for(int32 I=0;I<3;I++){float X=32+I*73;Text(X,373,SliderLabels[I],16,Ink,70);Rect(X+11,401,25,110,Edge);Rect(X+14,404,19,104,C(TEXT("939393")));Bevel(X+9,401+(1-Values[I])*86,29,24);Hit(X,385,54,128,9,I);}
  Text(17,533,TEXT("Steps"),20,Ink);Bevel(17,567,230,238);for(int32 I=0;I<S->Steps.Num();I++){const float Row=230.f/S->Steps.Num(),Y=572+I*Row;if(I==S->Selected)Rect(21,Y,222,Row,Purple);Text(28,Y+1,FString::Printf(TEXT("%d  %s"),I+1,*S->Steps[I].Label),14,I==S->Selected?White:I<S->Selected?Ink:Edge,215,true);Hit(21,Y,222,Row,6,I);}
  Text(17,811,TEXT("Legend"),20,Ink);const TCHAR* Labels[]={TEXT("commit"),TEXT("tree"),TEXT("blob")};const FLinearColor Colors[]={Purple,Teal,Gold};for(int32 I=0;I<3;I++){Rect(18,849+I*28,18,18,Edge);Rect(20,851+I*28,14,14,Colors[I]);Text(47,846+I*28,Labels[I],17,Ink,190,true);}Line({{18,938},{36,938}},Yellow,3);Text(47,925,TEXT("new this step"),15,Ink,192,true);Line({{18,973},{24,973}},Edge,2);Line({{30,973},{36,973}},Edge,2);Text(47,960,TEXT("parent / missing"),14,Ink,195,true);
  Rect(268,77,1528,150,C(TEXT("a43838")));TArray<FString> Commands;S->VisibleCommands().ParseIntoArrayLines(Commands,false);for(int32 I=0;I<Commands.Num();I++)Text(286,86+I*44,Commands[I],32,White,1490,true);
  if(S->Scene.IsValid())FSlateDrawElement::MakeBox(Out,Layer++,G.ToPaintGeometry(FVector2D(1528,739),FSlateLayoutTransform(FVector2D(268,227))),&S->Scene->Brush,ESlateDrawEffect::NoGamma);
  for(const auto& H:S->Highlights) {
   if(H.Points.IsEmpty())continue;
   const float Alpha=H.Released?1-FMath::Clamp((H.Age-.25f)/1.75f,0.f,1.f):1;
   TArray<FVector2D> Points=H.Points;
   if(Points.Num()==1)for(int32 I=0;I<=16;I++){const float Angle=I*2*PI/16;Points.Add(H.Points[0]+FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*3);}
   Line(Points,FLinearColor(0,0,0,.3f*Alpha),9);
   const FColor Palette[]={FColor(255,227,138),FColor(91,231,255),FColor(255,119,192),FColor(181,255,107),FColor(255,167,91),FColor(196,158,255)};
   auto Color=FLinearColor::FromSRGBColor(Palette[H.ColorIndex%UE_ARRAY_COUNT(Palette)]);Color.A=Alpha;Line(Points,Color,4);
  }
  int32 CommitCount=0,BlobCount=0,PointerCount=0;for(const auto& Space:Step.Spaces){CommitCount+=Space.Commits;BlobCount+=Space.Blobs.Num();for(int32 I=Space.FirstCommit;I<Space.FirstCommit+Space.Commits;I++)PointerCount+=1+(I>Space.FirstCommit?1:0)+S->Commits[I].Entries.Num();}
  Rect(268,966,1528,30,Gray);Text(280,968,FString::Printf(TEXT("objects:  %d commit   %d tree   %d blob      pointers: %d"),CommitCount,CommitCount,BlobCount,PointerCount),17,Ink,1270,true);Rect(1680,970,104,23,S->Playing?Teal:C(TEXT("a43838")));Text(1687,970,S->Playing?TEXT("Playing"):TEXT("Paused"),17,White,94);
  if(S->ShowHelp){Bevel(550,250,1040,235);Text(575,265,TEXT("Scalar — partial metadata history"),25,Ink,990);Text(575,310,TEXT("Space / arrows: step    R: reset    B: back    P: play / pause"),20,Ink,990);Text(575,345,TEXT("Drag: orbit. Two-finger swipe: pan. Pinch / wheel: zoom. C: camera mode."),19,Ink,990);Text(575,386,TEXT("A–E and C1–C4 are readable object aliases; all five keys belong to path:demo."),18,Ink,990);Text(575,431,TEXT("Shift-drag draws an outline that fades after release. Dashed boxes are promised blobs."),17,Ink,990);}
  return Layer;
 }
};
TSharedRef<SWidget> MakeParkScalarView(TFunction<TSharedPtr<FParkScalar>()> State,TFunction<void()> Restore){return SNew(SParkScalarView).State(State).Restore(Restore);}
