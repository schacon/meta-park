#include "SlideGameMode.h"
#include "ParkTerminal.h"
#include "Dom/JsonObject.h"
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
   [SNew(STextBlock).Text(FText::FromString(Heading)).Font(FCoreStyle::GetDefaultFontStyle("Mono",24)).ColorAndOpacity(FLinearColor(.015,.025,.02))]];
  Body->AddSlot().AutoHeight().Padding(0,0,0,28)[SNew(STextBlock).Text(FText::FromString(Step->GetStringField(TEXT("title")))).Font(FCoreStyle::GetDefaultFontStyle("Bold",48*TypeScale)).ColorAndOpacity(FLinearColor(.001,.001,.001)).AutoWrapText(true)];
  auto TextBody=SNew(SVerticalBox);
  for(const auto& Value:Step->GetArrayField(TEXT("blocks"))) {
   const auto Block=Value->AsObject();const FString Kind=Block->GetStringField(TEXT("kind"));
   FString Text=Block->GetStringField(TEXT("text"));if(Kind==TEXT("li"))Text=TEXT("•  ")+Text;
   const bool IsHeading=Kind.StartsWith(TEXT("h")),Code=Kind==TEXT("pre");
   TextBody->AddSlot().AutoHeight().Padding(0,0,0,18)[SNew(STextBlock).Text(FText::FromString(Text))
    .Font(FCoreStyle::GetDefaultFontStyle(Code?"Mono":IsHeading?"Bold":"Regular",(Code?26:IsHeading?30:32)*TypeScale))
    .ColorAndOpacity(IsHeading?Accent:FLinearColor(.004,.004,.004)).WrapTextAt(1320)];
  }
  // Dense authored pages shrink inside the sign instead of running off its edge.
  Body->AddSlot().FillHeight(1)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly).HAlign(HAlign_Left).VAlign(VAlign_Top)[SNew(SBox).WidthOverride(1320)[TextBody]]];
  Body->AddSlot().AutoHeight().Padding(0,20,0,10)[SNew(STextBlock).Text(FText::FromString(TEXT("← → previous / next page    •    end of station: overview    •    Esc overview    •    N notes"))).Font(FCoreStyle::GetDefaultFontStyle("Regular",15)).ColorAndOpacity(Accent)];
  auto Content=SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.66,.67,.63)).Padding(48)[Body];
  Content->SetVisibility(TAttribute<EVisibility>::CreateLambda([this,StationIndex,Page]{
   const int32 Current=Index==StationIndex?PageIndex:0;
   return Page==Current||(Index==StationIndex&&PageSwipe<1&&Page==PreviousPage)?EVisibility::Visible:EVisibility::Hidden;
  }));
  Content->SetRenderTransform(TAttribute<TOptional<FSlateRenderTransform>>::CreateLambda([this,StationIndex,Page]()->TOptional<FSlateRenderTransform>{
   if(Index!=StationIndex||PageSwipe>=1)return FSlateRenderTransform();
   const float Ease=PageSwipe*PageSwipe*(3-2*PageSwipe);
   return FSlateRenderTransform(FVector2D((Page==PageIndex?1-Ease:-Ease)*PageDirection*1440,0));
  }));
  Pages->AddSlot()[Content];
 }
 return Pages;
}
bool ASlideGameMode::IsComponentPage() const {
 FString Kind;
 return StationSteps.IsValidIndex(Index)&&StationSteps[Index].IsValidIndex(PageIndex)&&StationSteps[Index][PageIndex]->TryGetStringField(TEXT("kind"),Kind)&&Kind==TEXT("component");
}
void ASlideGameMode::ResetStationPage() {
 PageIndex=0;PreviousPage=INDEX_NONE;PageSwipe=1;ActiveComponentPage=INDEX_NONE;
 if(Terminal.IsValid())Terminal->Hide();
}
bool ASlideGameMode::AdvancePage(int32 Direction) {
 // Ignore repeated presses during the short swipe so pages cannot be skipped accidentally.
 if(PageSwipe<1)return true;
 const int32 Next=PageIndex+Direction;
 if(!StationSteps.IsValidIndex(Index)||!StationSteps[Index].IsValidIndex(Next))return false;
 const bool WasComponent=IsComponentPage();
 PreviousPage=PageIndex;PageIndex=Next;PageDirection=Direction;
 PageSwipe=WasComponent||IsComponentPage()?1:0;
 ActiveComponentPage=INDEX_NONE;
 if(Terminal.IsValid())Terminal->Hide();
 Notes[Index]=StationSteps[Index][PageIndex]->GetStringField(TEXT("notes"));
 return true;
}
void ASlideGameMode::TickStationPage(float Delta) {
 PageSwipe=FMath::Min(1.f,PageSwipe+Delta/.28f);
 if(bFreeFlight||MapPhase!=EMapPhase::Slide){ActiveComponentPage=INDEX_NONE;return;}
 if(ActiveComponentPage==PageIndex)return;
 ActiveComponentPage=PageIndex;
 const auto Step=StationSteps[Index][PageIndex];
 Notes[Index]=Step->GetStringField(TEXT("notes"));
 const TSharedPtr<FJsonObject>* Component;
 if(Step->TryGetObjectField(TEXT("component"),Component)&&(*Component)->GetStringField(TEXT("type"))==TEXT("Computer")) {
  if(!Terminal.IsValid())Terminal=MakeShared<FParkTerminal>(GetWorld());
  Terminal->Show((*Component)->GetStringField(TEXT("prompt")),(*Component)->GetStringField(TEXT("output")));
 }
}
