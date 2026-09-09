#include "SlideGameMode.h"
#include "ParkTerminal.h"
#include "Dom/JsonObject.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> ASlideGameMode::BuildStationContent(TSharedPtr<FJsonObject> Station,int32 StationIndex,const FString& Label,FLinearColor Accent) {
 TArray<TSharedPtr<FJsonObject>> Steps;
 const TArray<TSharedPtr<FJsonValue>>* Values;
 if(Station->TryGetArrayField(TEXT("steps"),Values))for(const auto& Value:*Values)Steps.Add(Value->AsObject());
 if(Steps.IsEmpty())Steps.Add(Station); // Legacy single-file decks remain readable.
 StationSteps.Add(Steps);
 auto Pages=SNew(SOverlay).Clipping(EWidgetClipping::ClipToBoundsAlways);
 for(int32 Page=0;Page<Steps.Num();Page++) {
  const auto Step=Steps[Page];
  const int32 TypeScale=HiddenSlides.Contains(StationIndex)?2:1;
  auto Body=SNew(SVerticalBox);
  const FString Heading=bIsland?FString::Printf(TEXT("[%d] %s  /  %s     %02d / %02d"),StationIndex+1,*Station->GetObjectField(TEXT("card"))->GetStringField(TEXT("code")),*Label,Page+1,Steps.Num()):Step->GetStringField(TEXT("title"));
  Body->AddSlot().AutoHeight().Padding(0,0,0,28)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.13,.61,.24)).Padding(12)
   [SNew(STextBlock).Text_Lambda([this,StationIndex,Page,Heading,Station]{return FText::FromString(Index==StationIndex&&RevealedPage>0?FString::Printf(TEXT("[%d] %s  %02d/%02d"),StationIndex+1,*Station->GetObjectField(TEXT("card"))->GetStringField(TEXT("code")),Page+1,StationSteps[StationIndex].Num()):Heading);}).Font(FCoreStyle::GetDefaultFontStyle("Mono",24)).ColorAndOpacity(FLinearColor(.015,.025,.02))]];
  Body->AddSlot().AutoHeight().Padding(0,0,0,28)[SNew(STextBlock).Text(FText::FromString(Step->GetStringField(TEXT("title")))).Font(FCoreStyle::GetDefaultFontStyle("Bold",48*TypeScale)).ColorAndOpacity(FLinearColor(.001,.001,.001)).AutoWrapText(true)];
  auto TextBody=SNew(SVerticalBox);
  for(const auto& Value:Step->GetArrayField(TEXT("blocks"))) {
   const auto Block=Value->AsObject();const FString Kind=Block->GetStringField(TEXT("kind"));
   FString Text=Block->GetStringField(TEXT("text"));if(Kind==TEXT("li"))Text=TEXT("•  ")+Text;
   const bool IsHeading=Kind.StartsWith(TEXT("h")),Code=Kind==TEXT("pre");
   TextBody->AddSlot().AutoHeight().Padding(0,0,0,18)[SNew(STextBlock).Text(FText::FromString(Text))
    .Font(FCoreStyle::GetDefaultFontStyle(Code?"Mono":IsHeading?"Bold":"Regular",(Code?26:IsHeading?30:32)*TypeScale))
    .ColorAndOpacity(IsHeading?Accent:FLinearColor(.004,.004,.004)).WrapTextAt_Lambda([this,StationIndex,Page]{return PageTile(Page,Index==StationIndex?RevealedPage+1:1).Z-96;})];
  }
  // Dense authored pages shrink inside the sign instead of running off its edge.
  Body->AddSlot().FillHeight(1)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly).HAlign(HAlign_Left).VAlign(VAlign_Top)[SNew(SBox).WidthOverride_Lambda([this,StationIndex,Page]{return FOptionalSize(PageTile(Page,Index==StationIndex?RevealedPage+1:1).Z-96);})[TextBody]]];
  auto Content=SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.66,.67,.63)).Padding(48)[Body];
  FString Kind;Step->TryGetStringField(TEXT("kind"),Kind);
  const bool Component=Kind==TEXT("component");
  const TSharedPtr<FJsonObject>* NativeComponent;
  const bool CommandLine=Step->TryGetObjectField(TEXT("component"),NativeComponent)&&(*NativeComponent)->GetStringField(TEXT("type"))==TEXT("CommandLine");
  if(CommandLine) {
   const FString Prompt=(*NativeComponent)->GetStringField(TEXT("prompt")),Output=(*NativeComponent)->GetStringField(TEXT("output"));
   const uint64 Key=(uint64(StationIndex)<<32)|uint32(Page);
   auto Clock=[this,Key]{const float* Start=ComponentStartTimes.Find(Key);return Start?FMath::Max(0.f,Elapsed-*Start):0.f;};
   Content=SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.008,.015,.012)).Padding(32)
    [SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,24)[SNew(STextBlock).Text(FText::FromString(TEXT("indigo-git / terminal"))).Font(FCoreStyle::GetDefaultFontStyle("Mono",20)).ColorAndOpacity(FLinearColor(.15,.65,.3))]
     +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,24)[SNew(STextBlock).Text_Lambda([Prompt,Clock]{return FText::FromString(TEXT("$ ")+Prompt.Left(FMath::Clamp(FMath::FloorToInt((Clock()-.15f)/.09f),0,Prompt.Len()))+(FMath::Fmod(Clock(),.7f)<.35f?TEXT("_"):TEXT(" ")));}).Font(FCoreStyle::GetDefaultFontStyle("Mono",32)).AutoWrapText(true).ColorAndOpacity(FLinearColor(.88,.95,.88))]
     +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([Prompt,Output,Clock]{return FText::FromString(Clock()>.15f+Prompt.Len()*.09f+.4f?Output:TEXT(""));}).Font(FCoreStyle::GetDefaultFontStyle("Mono",32)).AutoWrapText(true).ColorAndOpacity(FLinearColor(.15,.8,.3))]
    ];
  }
  auto Tile=SNew(SBox)
   .WidthOverride_Lambda([this,StationIndex,Page]{return FOptionalSize(PageTile(Page,Index==StationIndex?RevealedPage+1:1).Z);})
   .HeightOverride_Lambda([this,StationIndex,Page]{return FOptionalSize(PageTile(Page,Index==StationIndex?RevealedPage+1:1).W);})[Content];
  Tile->SetVisibility(TAttribute<EVisibility>::CreateLambda([this,StationIndex,Page,Component,CommandLine]{
   return (!Component||CommandLine)&&Page<=(Index==StationIndex?RevealedPage:0)?EVisibility::Visible:EVisibility::Hidden;
  }));
  Tile->SetRenderTransform(TAttribute<TOptional<FSlateRenderTransform>>::CreateLambda([this,StationIndex,Page]()->TOptional<FSlateRenderTransform>{
   const FVector4 Rect=PageTile(Page,Index==StationIndex?RevealedPage+1:1);
   float Offset=0;
   if(Index==StationIndex&&Page>PreviousRevealedPage&&PageSwipe<1) {
    const float Ease=PageSwipe*PageSwipe*(3-2*PageSwipe);Offset=(1-Ease)*1440;
   }
   return FSlateRenderTransform(FVector2D(Rect.X+Offset,Rect.Y));
  }));
  Pages->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top)[Tile];
 }
 return Pages;
}
bool ASlideGameMode::IsComponentPage() const {
 FString Kind;
 if(!StationSteps.IsValidIndex(Index)||!StationSteps[Index].IsValidIndex(PageIndex))return false;
 const auto Step=StationSteps[Index][PageIndex];const TSharedPtr<FJsonObject>* Component;
 return Step->TryGetStringField(TEXT("kind"),Kind)&&Kind==TEXT("component")&&(!Step->TryGetObjectField(TEXT("component"),Component)||(*Component)->GetStringField(TEXT("type"))!=TEXT("CommandLine"));
}
void ASlideGameMode::ResetStationPage() {
 PageIndex=0;RevealedPage=0;PreviousRevealedPage=0;PreviousPage=INDEX_NONE;PageSwipe=1;ActiveComponentPage=INDEX_NONE;
 ActiveTerminals.Empty();ComponentStartTimes.Empty();for(auto& Entry:PageTerminals)Entry.Value->Hide();
}
bool ASlideGameMode::AdvancePage(int32 Direction) {
 // Ignore repeated presses during the short swipe so pages cannot be skipped accidentally.
 if(PageSwipe<1)return true;
 const int32 Next=PageIndex+Direction;
 if(!StationSteps.IsValidIndex(Index)||!StationSteps[Index].IsValidIndex(Next))return false;
 PreviousRevealedPage=RevealedPage;
 PreviousPage=PageIndex;PageIndex=Next;PageDirection=Direction;RevealedPage=FMath::Max(RevealedPage,Next);
 PageSwipe=RevealedPage>PreviousRevealedPage?0:1;
 ActiveComponentPage=INDEX_NONE;
 return true;
}
FVector4 ASlideGameMode::PageTile(int32 Page,int32 Count) const {
 const int32 Columns=Count<=3?Count:Count<=4?2:FMath::CeilToInt(FMath::Sqrt(float(Count)*1.6f));
 const int32 Rows=FMath::DivideAndRoundUp(Count,Columns);
 const float W=1440.f/Columns,H=900.f/Rows;
 return FVector4((Page%Columns)*W,(Page/Columns)*H,W,H);
}
void ASlideGameMode::TickStationPage(float Delta) {
 PageSwipe=FMath::Min(1.f,PageSwipe+Delta/.28f);
 const bool Showing=!bFreeFlight&&MapPhase==EMapPhase::Slide;
 if(Showing&&ActiveComponentPage!=PageIndex) {
  ActiveComponentPage=PageIndex;
  ViewedPages.Add((uint64(Index)<<32)|uint32(PageIndex));VisitedStations.Add(Index);
 }
 if(!Showing)ActiveComponentPage=INDEX_NONE;
 if(Showing)for(int32 Page=0;Page<=RevealedPage;Page++) {
  const auto Step=StationSteps[Index][Page];const TSharedPtr<FJsonObject>* Component;
  const uint64 PageKey=(uint64(Index)<<32)|uint32(Page);
  if(!ComponentStartTimes.Contains(PageKey))ComponentStartTimes.Add(PageKey,Elapsed);
  if(!Step->TryGetObjectField(TEXT("component"),Component)||(*Component)->GetStringField(TEXT("type"))!=TEXT("Computer"))continue;
  const uint64 Key=(uint64(Index)<<32)|uint32(Page);
  if(!PageTerminals.Contains(Key))PageTerminals.Add(Key,MakeShared<FParkTerminal>(GetWorld()));
  Terminal=PageTerminals[Key];
  if(!ActiveTerminals.Contains(Key)) {
   Terminal->Show((*Component)->GetStringField(TEXT("prompt")),(*Component)->GetStringField(TEXT("output")));ActiveTerminals.Add(Key);
  }
 }
 auto* PC=GetWorld()->GetFirstPlayerController();const auto* Player=PC->GetLocalPlayer();int32 W,H;PC->GetViewportSize(W,H);
 for(auto& Entry:PageTerminals) {
  const int32 Station=int32(Entry.Key>>32),Page=int32(uint32(Entry.Key));
  const bool Visible=!bFreeFlight&&Station==Index&&Page<=RevealedPage&&(Showing||MapPhase==EMapPhase::Retract);
  FVector4 Region(0,0,1,1);
  if(Visible&&RevealedPage>0) {
   const FVector4 Rect=PageTile(Page,RevealedPage+1);FVector2D Min(1,1),Max(0,0);
   for(float X:{float(Rect.X),float(Rect.X+Rect.Z)})for(float Y:{float(Rect.Y),float(Rect.Y+Rect.W)}) {
    FVector2D Pixel;
    if(PC->ProjectWorldLocationToScreen(Panels[Index].Root->GetActorTransform().TransformPosition(FVector(0,720-X,450-Y)),Pixel)) {
     const FVector2D P=(Pixel/FVector2D(W,H)-Player->Origin)/Player->Size;
     Min.X=FMath::Min(Min.X,P.X);Min.Y=FMath::Min(Min.Y,P.Y);Max.X=FMath::Max(Max.X,P.X);Max.Y=FMath::Max(Max.Y,P.Y);
    }
   }
   Region=FVector4(Min.X,Min.Y,Max.X-Min.X,Max.Y-Min.Y);
  }
  if(!Showing)Entry.Value->Hide();
  Entry.Value->Update(Delta,Camera,Visible,Region,RevealedPage>0);
 }
}

int32 ASlideGameMode::PowerPercent() const {
 int32 Total=0;for(const auto& Steps:StationSteps)Total+=Steps.Num();
 return Total>0?FMath::Clamp(FMath::CeilToInt(100.f*(Total-ViewedPages.Num())/Total),0,100):100;
}
int32 ASlideGameMode::SecurityPercent() const {
 return TimerDuration>0?FMath::Clamp(FMath::CeilToInt(100.f*(TimerDuration-TimerElapsed)/TimerDuration),0,100):100;
}
