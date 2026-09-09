#include "ParkCastPlayer.h"
#include "Dom/JsonObject.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"

namespace {
FLinearColor CastColor(const FString& Value){return FLinearColor::FromSRGBColor(FColor::FromHex(Value));}
FSlateFontInfo CastFont(bool Bold){return FSlateFontInfo(FPaths::ProjectContentDir()/(Bold?TEXT("Fonts/RobotoMono-Bold.ttf"):TEXT("Fonts/RobotoMono-Regular.ttf")),24);}
}
FParkCastPlayer::FParkCastPlayer(TSharedPtr<FJsonObject> Recording) {
 Frames=Recording->GetArrayField(TEXT("frames"));Duration=Recording->GetNumberField(TEXT("duration"));
 Title=Recording->GetStringField(TEXT("title"));Foreground=CastColor(Recording->GetStringField(TEXT("fg")));Background=CastColor(Recording->GetStringField(TEXT("bg")));
 Seek(0);
}
void FParkCastPlayer::Tick(float Delta){if(!Paused){Time=FMath::Min(Duration,Time+Delta);ApplyThroughTime();}}
void FParkCastPlayer::Seek(float Seconds){Time=FMath::Clamp(Seconds,0.f,Duration);NextFrame=0;Lines.Empty();ApplyThroughTime();}
void FParkCastPlayer::ApplyThroughTime() {
 while(Frames.IsValidIndex(NextFrame)&&Frames[NextFrame]->AsObject()->GetNumberField(TEXT("time"))<=Time) {
  const auto Frame=Frames[NextFrame++]->AsObject();Columns=Frame->GetIntegerField(TEXT("cols"));Rows=Frame->GetIntegerField(TEXT("rows"));Lines.SetNum(Rows);
  for(const auto& Change:Frame->GetArrayField(TEXT("lines"))) {
   const auto& Row=Change->AsArray();auto& Line=Lines[int32(Row[0]->AsNumber())];Line.Reset();
   for(const auto& Value:Row[1]->AsArray()) {
    const auto& Run=Value->AsArray();Line.Add({int32(Run[0]->AsNumber()),int32(Run[1]->AsNumber()),int32(Run[5]->AsNumber()),Run[2]->AsString(),CastColor(Run[3]->AsString()),CastColor(Run[4]->AsString())});
   }
  }
  const auto& Cursor=Frame->GetArrayField(TEXT("cursor"));CursorX=Cursor[0]->AsNumber();CursorY=Cursor[1]->AsNumber();CursorVisible=Cursor[2]->AsBool();
 }
}
FString FParkCastPlayer::PlainText() const {
 FString Result;
 for(const auto& Line:Lines){int32 Column=0;for(const auto& Run:Line){Result+=FString::ChrN(FMath::Max(0,Run.Column-Column),TEXT(' '));Result+=Run.Text;Column=Run.Column+Run.Width;}Result+=TEXT("\n");}return Result;
}
class SParkCastView : public SLeafWidget {
public:
 SLATE_BEGIN_ARGS(SParkCastView){}SLATE_ARGUMENT(TFunction<TSharedPtr<FParkCastPlayer>()>,Player)SLATE_END_ARGS()
 TFunction<TSharedPtr<FParkCastPlayer>()> Player;
 void Construct(const FArguments& Args){Player=Args._Player;}
 virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(800,480);}
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override {
  const auto Cast=Player();if(!Cast.IsValid())return Layer;
  const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");
  FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(),Brush,ESlateDrawEffect::None,Cast->Background);
  const auto Normal=CastFont(false),Bold=CastFont(true);
  const FVector2D Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(TEXT("M"),Normal);
  const FVector2D Cell(Measure.X,Measure.Y+3),Size=Cell*FVector2D(Cast->Columns,Cast->Rows);
  const float Scale=FMath::Max(.01f,FMath::Min(float(G.GetLocalSize().X/Size.X),float(G.GetLocalSize().Y/Size.Y)));
  auto Box=[&](FVector2D At,FVector2D Extent,FLinearColor Color,int L){FSlateDrawElement::MakeBox(Out,L,G.ToPaintGeometry(Extent,FSlateLayoutTransform(Scale,At*Scale)),Brush,ESlateDrawEffect::None,Color);};
  for(int32 Row=0;Row<Cast->Lines.Num();Row++)for(const auto& Run:Cast->Lines[Row]) {
   const FVector2D At(Run.Column*Cell.X,Row*Cell.Y),Extent(Run.Width*Cell.X,Cell.Y);
   Box(At,Extent,Run.Background,Layer+1);
   if((Run.Flags&16)&&FMath::Fmod(FPlatformTime::Seconds(),1.)>.5)continue;
   const FLinearColor Ink=Run.Flags&2?Run.Foreground*.55f:Run.Foreground;
   FSlateDrawElement::MakeText(Out,Layer+2,G.ToPaintGeometry(Extent,FSlateLayoutTransform(Scale,At*Scale)),Run.Text,Run.Flags&1?Bold:Normal,ESlateDrawEffect::None,Ink);
   if(Run.Flags&4)Box(At+FVector2D(0,Cell.Y-4),FVector2D(Extent.X,2),Ink,Layer+3);
   if(Run.Flags&8)Box(At+FVector2D(0,Cell.Y*.5),FVector2D(Extent.X,2),Ink,Layer+3);
  }
  if(Cast->CursorVisible&&FMath::Fmod(FPlatformTime::Seconds(),1.)<.5)Box(FVector2D(Cast->CursorX*Cell.X,(Cast->CursorY+1)*Cell.Y-3),FVector2D(Cell.X,2),Cast->Foreground,Layer+4);
  return Layer+4;
 }
};
TSharedRef<SWidget> MakeParkCastView(TFunction<TSharedPtr<FParkCastPlayer>()> Player){return SNew(SParkCastView).Player(Player);}
