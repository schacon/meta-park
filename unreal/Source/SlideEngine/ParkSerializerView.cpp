#include "ParkSerializer.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"
#include "InputCoreTypes.h"
#include "Layout/Clipping.h"

class SParkSerializerView : public SLeafWidget {
 static constexpr int32 TableVisibleRows=8,TreeVisibleRows=14,DataFontSize=18;
 static constexpr float TableRowHeight=40,TreeRowHeight=34;
 TFunction<TSharedPtr<FParkSerializer>()> State;TFunction<void()> Restore;
 struct FHit{FSlateRect Rect;int32 Kind,Index;};mutable TArray<FHit> Hits;mutable float ValueScrollMax=0;
public:
 SLATE_BEGIN_ARGS(SParkSerializerView){}SLATE_ARGUMENT(TFunction<TSharedPtr<FParkSerializer>()>,State)SLATE_ARGUMENT(TFunction<void()>,Restore)SLATE_END_ARGS()
 void Construct(const FArguments& A){State=A._State;Restore=A._Restore;SetClipping(EWidgetClipping::ClipToBoundsAlways);}
 virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(1800,1000);}
 virtual FReply OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)override {
  if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();auto S=State();if(!S)return FReply::Unhandled();const FVector2D P=G.AbsoluteToLocal(E.GetScreenSpacePosition());
  for(const auto& H:Hits){if(H.Rect.ContainsPoint(P)){
   switch(H.Kind){case 0:S->Serialize();break;case 1:S->AddMetadata();break;case 2:S->SelectRow(H.Index);break;case 3:S->SelectNode(H.Index);break;case 4:if(S->Expanded.Contains(H.Index))S->Expanded.Remove(H.Index);else S->Expanded.Add(H.Index);break;case 5:S->SelectCommit(H.Index);break;case 6:Restore();break;case 7:S->ExpandAll();break;case 8:S->FoldAll();break;}return FReply::Handled();
  }}
  return FReply::Handled();
 }
 virtual FReply OnMouseWheel(const FGeometry& G,const FPointerEvent& E)override {
  auto S=State();if(!S)return FReply::Unhandled();const auto P=G.AbsoluteToLocal(E.GetScreenSpacePosition());
  if(P.X>766&&P.X<1784&&P.Y>855&&P.Y<952){S->ValueScroll=FMath::Clamp(S->ValueScroll-E.GetWheelDelta()*37.f,0.f,ValueScrollMax);return FReply::Handled();}
  if(P.X<730&&P.Y>210&&P.Y<574)S->TableScroll=FMath::Clamp(S->TableScroll-FMath::RoundToInt(E.GetWheelDelta()*3),0,FMath::Max(0,S->TableRows().Num()-TableVisibleRows));
  if(P.X>960&&P.Y<650)S->Scroll=FMath::Clamp(S->Scroll-FMath::RoundToInt(E.GetWheelDelta()*3),0,FMath::Max(0,S->VisibleNodes().Num()-TreeVisibleRows));return FReply::Handled();
 }
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool)const override {
  auto S=State();if(!S)return Layer;Hits.Reset();
  auto Color=[](const TCHAR* Hex){return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));};
  const auto Gray=Color(TEXT("bfc3cc")),Light=Color(TEXT("f2f2e9")),Ink=Color(TEXT("171d26")),Blue=Color(TEXT("395e93")),White=Color(TEXT("ffffff")),Edge=Color(TEXT("626b7b"));
  const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");const auto Font=FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("Fonts/RobotoMono-Regular.ttf"),20);
  const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
  auto Rect=[&](float X,float Y,float W,float H,FLinearColor C){FSlateDrawElement::MakeBox(Out,Layer++,G.ToPaintGeometry(FVector2D(W,H),FSlateLayoutTransform(FVector2D(X,Y))),Brush,ESlateDrawEffect::None,C);};
  auto Line=[&](TArray<FVector2D> P,FLinearColor C,float Width=1){FSlateDrawElement::MakeLines(Out,Layer++,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,Width);};
  auto Text=[&](float X,float Y,FString V,int32 Size=20,FLinearColor C=FLinearColor::Black,float Max=1800){V.ReplaceInline(TEXT("\r"),TEXT(""));V.ReplaceInline(TEXT("\n"),TEXT(" / "));auto F=Font;F.Size=Size;if(Measure->Measure(V,F).X>Max){while(V.Len()>0&&Measure->Measure(V+TEXT("…"),F).X>Max)V.LeftChopInline(1);V+=TEXT("…");}FSlateDrawElement::MakeText(Out,Layer++,G.ToPaintGeometry(FVector2D(Max,80),FSlateLayoutTransform(FVector2D(X,Y))),V,F,ESlateDrawEffect::None,C);};
  auto Hit=[&](float X,float Y,float W,float H,int32 K,int32 I=-1){Hits.Add({FSlateRect(X,Y,X+W,Y+H),K,I});};
  auto Bevel=[&](float X,float Y,float W,float H,FLinearColor C){Rect(X,Y,W,H,Edge);Rect(X+3,Y+3,W-6,H-6,C);Line({FVector2D(X,Y+H),FVector2D(X,Y),FVector2D(X+W,Y)},White,2);};
  auto Button=[&](float X,float Y,float W,float H,const TCHAR* Label,int32 K,int32 I=-1){Bevel(X,Y,W,H,Gray);if(W<50)Line({FVector2D(X+8,Y+H*.5f),FVector2D(X+W-8,Y+H*.5f)},Ink,2);else Text(X+12,Y+H*.5f-12,Label,17,Ink,W-24);Hit(X,Y,W,H,K,I);};
  auto Window=[&](float X,float Y,float W,float H,const FString& Title){Rect(X+6,Y+7,W,H,FLinearColor(0,0,0,.4));Bevel(X,Y,W,H,Gray);Rect(X+4,Y+4,W-8,40,Blue);Text(X+16,Y+8,Title,W<800?17:23,White,W-76);Button(X+W-42,Y+9,28,28,TEXT("−"),6);Line({FVector2D(X+3,Y+46),FVector2D(X+W-3,Y+46)},Edge,2);};
  Window(0,0,730,705,TEXT("SQLite Browser — .git/git-meta.sqlite"));
  Text(18,55,TEXT("File   Edit   View   Query   Help"),21,Ink);
  Text(18,105,TEXT("Table:"),19,Ink);Bevel(108,96,275,40,Light);Text(121,104,TEXT("metadata"),19,Ink);Line({FVector2D(357,110),FVector2D(365,119),FVector2D(373,110)},Blue,3);
  Bevel(18,151,690,44,Light);Text(30,157,TEXT("select * from metadata;"),19,Ink,666);
  const float Cols[]={18,210,414,524,708};
  Rect(18,214,690,39,Color(TEXT("aab4c5")));const TCHAR* Headers[]={TEXT("target"),TEXT("key"),TEXT("type"),TEXT("value")};for(int32 C=0;C<4;C++)Text(Cols[C]+8,222,Headers[C],19,Ink);
  const auto Table=S->TableRows();int32 N=0;
  for(int32 T=FMath::Clamp(S->TableScroll,0,FMath::Max(0,Table.Num()-TableVisibleRows));T<Table.Num()&&N<TableVisibleRows;T++){const int32 I=Table[T];const auto& R=S->Rows[I];float Y=253+N*TableRowHeight;Rect(18,Y,690,TableRowHeight,S->SelectedRow==I?Blue:(!S->ViewingDraft&&S->SelectedCommit==1&&R.Hidden)?Color(TEXT("b5e3b6")):N%2?Color(TEXT("e7e8df")):Light);const auto C=S->SelectedRow==I?White:Ink;
   const FString Values[]={R.ShortTarget,R.Key,R.Type,R.Value};for(int32 J=0;J<4;J++)Text(Cols[J]+8,Y+7,Values[J],DataFontSize,C,Cols[J+1]-Cols[J]-14);Hit(18,Y,690,TableRowHeight,2,I);N++;}
  for(float X:Cols)Line({FVector2D(X,214),FVector2D(X,253+N*TableRowHeight)},Edge,.7);
  if(Table.Num()>TableVisibleRows){const float Track=TableVisibleRows*TableRowHeight,Thumb=Track*TableVisibleRows/Table.Num();Rect(714,253,8,Track,Gray);Rect(714,253+(Track-Thumb)*S->TableScroll/(Table.Num()-TableVisibleRows),8,Thumb,Edge);}
  Text(20,582,FString::Printf(TEXT("%d metadata values"),Table.Num()),18,Ink);
  if(S->CanAddMetadata())Button(448,574,260,37,TEXT("Add metadata"),1);
  else Text(454,582,S->AddedMetadata&&S->Committed<2?TEXT("Metadata added"):TEXT(""),17,Edge,250);
  if(S->CanSerialize())Button(18,630,690,55,TEXT("SERIALIZE"),0);
  else {Bevel(18,630,690,55,Gray);Text(32,645,S->Serializing?TEXT("SERIALIZING…"):TEXT("UP TO DATE"),20,Edge,654);}
  Window(750,0,1050,1000,S->Title);
  float MenuX=768;auto MenuFont=Font;MenuFont.Size=21;
  const FVector2D MenuCursor=G.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
  for(const TCHAR* Label:{TEXT("File"),TEXT("View"),TEXT("Navigate"),TEXT("Help"),TEXT("Expand"),TEXT("Fold")}) {
   const float Width=Measure->Measure(Label,MenuFont).X;
   const int32 Action=FCString::Strcmp(Label,TEXT("Expand"))==0?7:FCString::Strcmp(Label,TEXT("Fold"))==0?8:-1;
   if(Action>=0){if(FSlateRect(MenuX-7,51,MenuX+Width+7,91).ContainsPoint(MenuCursor))Rect(MenuX-7,51,Width+14,40,Light);Hit(MenuX-7,51,Width+14,40,Action);}
   Text(MenuX,55,Label,21,Ink);MenuX+=Width+34;
  }
  Rect(766,100,210,533,Light);Rect(980,100,804,533,Color(TEXT("e1e5ec")));
  Text(777,110,TEXT("Commit Explorer"),15,Ink,190);
  const int32 CommitCount=S->Committed+(S->Serializing&&S->SelectedCommit==S->Committed?1:0);
  for(int32 Position=0;Position<CommitCount;Position++) {
   const int32 I=CommitCount-1-Position;const float Y=163+Position*110;const auto& C=S->Snapshots[I];
   if(S->SelectedCommit==I)Rect(770,Y-8,203,91,Color(TEXT("a8c5e9")));
   if(Position+1<CommitCount)Line({FVector2D(785,Y+14),FVector2D(785,Y+124)},Edge,3);
   Rect(778,Y+6,14,14,I==1?Color(TEXT("30a75c")):Blue);
   Text(803,Y,C.Id.Left(7),21,Ink,160);Text(803,Y+39,C.Message,13,Ink,167);Hit(770,Y-8,203,91,5,I);
  }
  Rect(980,100,804,35,Color(TEXT("aab4c5")));Text(996,106,TEXT("Name"),19,Ink);Text(1520,106,TEXT("Type"),19,Ink);Text(1630,106,TEXT("Object"),19,Ink);
  if(!S->Started){Text(1020,310,TEXT("No serialized objects"),26,Edge,730);Text(1020,359,TEXT("Click SERIALIZE to write the Git tree."),20,Edge,730);}
  const auto Nodes=S->VisibleNodes();const int32 Scroll=FMath::Clamp(S->Scroll,0,FMath::Max(0,Nodes.Num()-TreeVisibleRows));
  for(int32 K=Scroll;K<FMath::Min(Scroll+TreeVisibleRows,Nodes.Num());K++) {
   const int32 I=Nodes[K];const auto& V=S->Nodes[I];const float Y=137+(K-Scroll)*TreeRowHeight;const bool Selected=S->SelectedNode==I;Rect(982,Y,800,TreeRowHeight,Selected?Blue:S->IsNewNode(I)?Color(TEXT("b5e3b6")):K%2?Color(TEXT("d2d9e4")):Color(TEXT("e1e5ec")));const auto C=Selected?White:Ink;
   const float X=991+V.Depth*17;
   if(V.Type==TEXT("tree")){Text(X,Y+4,S->Expanded.Contains(I)?TEXT("-"):TEXT("+"),DataFontSize,C);Hit(X,Y,19,TreeRowHeight,4,I);Rect(X+20,Y+11,17,14,Color(TEXT("dcae37")));Rect(X+20,Y+7,9,5,Color(TEXT("dcae37")));}else {Rect(X+20,Y+7,14,19,White);}
   Text(X+42,Y+5,V.Name,DataFontSize,C,1500-X-42);Text(1518,Y+5,V.Type,DataFontSize,C,95);Text(1625,Y+5,V.Object.Left(10)+TEXT("…"),DataFontSize,C,153);Hit(X+20,Y,1780-X-20,TreeRowHeight,3,I);
  }
  if(Nodes.Num()>TreeVisibleRows){Rect(1785,138,8,474,Gray);const float Height=474.f*TreeVisibleRows/Nodes.Num();Rect(1785,138+(474-Height)*Scroll/FMath::Max(1,Nodes.Num()-TreeVisibleRows),8,Height,Edge);}
  Rect(980,614,804,30,Color(TEXT("d2d9e4")));
  Out.PushClip(FSlateClippingZone(G.ToPaintGeometry(FVector2D(804,30),FSlateLayoutTransform(FVector2D(980,614)))));
  Text(988,617,S->Started?FString::Printf(TEXT("%d/%d values written    %d tree entries"),S->Written,S->Snapshots[S->SelectedCommit].Rows.Num(),Nodes.Num()):TEXT("Awaiting serialization"),17,Ink,788);
  Out.PopClip();
  Bevel(766,650,1018,197,Color(TEXT("e2e6ee")));
  Bevel(766,855,1018,97,Light);
  const FSerializerNode* Node=S->Nodes.IsValidIndex(S->SelectedNode)?&S->Nodes[S->SelectedNode]:nullptr;
  const FSerializerRow* Row=S->Rows.IsValidIndex(S->SelectedRow)?&S->Rows[S->SelectedRow]:nullptr;
  if(Node||Row) {
   const float LabelX=783,ValueX=914,Width=810;
   constexpr int32 MetadataSize=16;
   const auto TargetColor=Color(TEXT("235997")),KeyColor=Color(TEXT("287142"));
   const FString KeyPath=Row?Row->Key.Replace(TEXT(":"),TEXT("/")):TEXT("");
   const int32 TargetLength=Row?Row->Path.Len()-KeyPath.Len()-1:0;
   auto PathColor=[&](int32 Offset){return Offset<TargetLength?TargetColor:Offset>TargetLength&&Offset<TargetLength+1+KeyPath.Len()?KeyColor:Ink;};
   auto Wrap=[&](float Y,const FString& Value,int32 Size,float LineHeight,FLinearColor C=FLinearColor::Black,bool Draw=true,bool ColorPath=false){
    auto F=Font;F.Size=Size;const float FontScale=G.GetAccumulatedLayoutTransform().GetScale();
    auto WidthOf=[&](const FString& TextValue){return Measure->Measure(TextValue,F,FontScale).X/FontScale;};
    TArray<FString> Lines;Value.ParseIntoArray(Lines,TEXT("\n"),false);int32 Offset=0;
    for(FString Part:Lines){do {
     int32 Count=Part.Len();while(Count>1&&WidthOf(Part.Left(Count))>Width)Count--;
     if(Draw) {
      if(ColorPath&&Row){
       for(int32 Start=0;Start<Count;){const auto Tint=PathColor(Offset+Start);int32 End=Start+1;while(End<Count&&PathColor(Offset+End)==Tint)End++;
        const float X=WidthOf(Part.Left(Start));Text(ValueX+X,Y,Part.Mid(Start,End-Start),Size,Tint,Width-X);Start=End;}
      } else Text(ValueX,Y,Part.Left(Count),Size,C,Width);
     }
     Y+=LineHeight;Offset+=Count;Part.RightChopInline(Count);
    }while(!Part.IsEmpty());Offset++;}
    return Y;
   };
   float Y=666;
   auto Field=[&](const TCHAR* Label,const FString& Value,FLinearColor Tint=FLinearColor::Black,bool ColorPath=false){Text(LabelX,Y,Label,MetadataSize,Tint,122);Y=Wrap(Y,Value,MetadataSize,22,Tint,true,ColorPath)+7;};
   Field(TEXT("SHA/type"),(Node?Node->Object:TEXT("Not serialized"))+TEXT("  /  ")+(Node?Node->Type:Row->Type));
   Field(TEXT("Path"),Node?(Node->Path.IsEmpty()?S->Ref+TEXT("^{tree}"):Node->Path):Row->Path,Ink,true);
   Field(TEXT("Target"),Row?Row->Target:TEXT("—"),TargetColor);
   Field(TEXT("Key"),Row?Row->Key:TEXT("—"),KeyColor);
   Text(LabelX,873,TEXT("Value"),MetadataSize,Ink,122);
   const FString Value=Node&&Node->Type==TEXT("blob")?Node->Value:Row?Row->Value:TEXT("—");
   const float ContentHeight=Wrap(0,Value,28,37,Ink,false),ViewportHeight=72;
   ValueScrollMax=FMath::Max(0.f,ContentHeight-ViewportHeight);S->ValueScroll=FMath::Clamp(S->ValueScroll,0.f,ValueScrollMax);
   Out.PushClip(FSlateClippingZone(G.ToPaintGeometry(FVector2D(840,ViewportHeight),FSlateLayoutTransform(FVector2D(ValueX,870)))));
   Wrap(870-S->ValueScroll,Value,28,37,Ink);
   Out.PopClip();
   if(ValueScrollMax>0){const float Track=77,Thumb=FMath::Max(12.f,Track*ViewportHeight/ContentHeight);Rect(1765,865,8,Track,Gray);Rect(1765,865+(Track-Thumb)*S->ValueScroll/ValueScrollMax,8,Thumb,Edge);}
  } else {Text(793,746,TEXT("Select a value or Git object to inspect its metadata."),21,Edge,940);}
  Text(770,968,S->Ref,17,Ink,710);if(S->Ready())Text(1410,969,TEXT("tree ")+S->Tree.Left(16),16,Ink,365);
  // xeyes tracks the real desktop pointer, independent of the table selection.
  const FVector2D Cursor=G.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());const auto Resource=FSlateApplication::Get().GetRenderer()->GetResourceHandle(*Brush);
  auto Ellipse=[&](FVector2D Center,FVector2D Radius,FLinearColor C){TArray<FSlateVertex> V;TArray<SlateIndex> Indices;for(int32 I=0;I<48;I++){const float A=I*2*PI/48;const FVector2D P=Center+FVector2D(FMath::Cos(A)*Radius.X,FMath::Sin(A)*Radius.Y);V.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(),FVector2f(P),FVector2f(.5,.5),C.ToFColor(true)));}for(int32 I=1;I<47;I++){Indices.Add(0);Indices.Add(I);Indices.Add(I+1);}FSlateDrawElement::MakeCustomVerts(Out,Layer++,Resource,V,Indices,nullptr,0,0);};
  for(float X:{290.f,455.f}){const FVector2D Center(X,833);Ellipse(Center+FVector2D(5,7),FVector2D(78,104),FLinearColor(0,0,0,.3));Ellipse(Center,FVector2D(78,104),Ink);Ellipse(Center,FVector2D(70,96),Light);FVector2D D=Cursor-Center;if(D.Size()>1)D.Normalize();Ellipse(Center+FVector2D(D.X*31,D.Y*47),FVector2D(29,35),Ink);}
  Text(334,953,TEXT("xeyes"),23,White);
  return Layer;
 }
};
TSharedRef<SWidget> MakeParkSerializerView(TFunction<TSharedPtr<FParkSerializer>()> State,TFunction<void()> Restore){return SNew(SParkSerializerView).State(State).Restore(Restore);}
