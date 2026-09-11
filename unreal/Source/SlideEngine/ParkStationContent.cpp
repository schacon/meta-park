#include "SlideGameMode.h"
#include "ParkScalar.h"
#include "ParkSpeedGraph.h"
#include "ParkExchange.h"
#include "ParkSlideImage.h"
#include "ParkTerminal.h"
#include "ParkFSV.h"
#include "ParkColdStorage.h"
#include "ParkWoodSign.h"
#include "ParkEmployeeBadge.h"
#include "ParkRaptors.h"
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
  bool TitleOnly=false;Step->TryGetBoolField(TEXT("titleOnly"),TitleOnly);
  const TSharedPtr<FJsonObject>* Image;const bool ImageOnly=Step->TryGetObjectField(TEXT("image"),Image);
  if(ImageOnly) {
   Body->AddSlot().FillHeight(1)[MakeParkSlideImage(*Image)];
  } else if(!TitleOnly) {
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
    .ColorAndOpacity(IsHeading?Accent:FLinearColor(.004,.004,.004)).WrapTextAt(1344)];
  }
  // Dense authored pages shrink inside the sign instead of running off its edge.
  Body->AddSlot().FillHeight(1)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly).HAlign(HAlign_Left).VAlign(VAlign_Top)[SNew(SBox).WidthOverride(1344)[TextBody]]];
  } else {
   Body->AddSlot().FillHeight(1).HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
    [SNew(STextBlock).Text(FText::FromString(Step->GetStringField(TEXT("title")))).Font(FCoreStyle::GetDefaultFontStyle("Bold",160)).ColorAndOpacity(FLinearColor(.001,.001,.001)).WrapTextAt(1240).Justification(ETextJustify::Center)]];
  }
  auto Content=SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(ImageOnly?FLinearColor::White:FLinearColor(.66,.67,.63)).Padding(ImageOnly?12:48)[Body];
  FString Kind;Step->TryGetStringField(TEXT("kind"),Kind);
  const bool Component=Kind==TEXT("component");
  auto Tile=SNew(SBox)
   .WidthOverride(1440).HeightOverride(900)[Content];
  Tile->SetVisibility(TAttribute<EVisibility>::CreateLambda([this,StationIndex,Page,Component]{
   const int32 Current=StationSignPage(StationIndex,Index==StationIndex?PageIndex:0);
   const bool Outgoing=Index==StationIndex&&PageSwipe<1&&Page==StationSignPage(StationIndex,PreviousPage);
   return !Component&&(Page==Current||Outgoing)?EVisibility::Visible:EVisibility::Hidden;
  }));
  Tile->SetRenderTransform(TAttribute<TOptional<FSlateRenderTransform>>::CreateLambda([this,StationIndex,Page]()->TOptional<FSlateRenderTransform>{
   float Offset=0;
   if(Index==StationIndex&&PageSwipe<1) {
    const float Ease=PageSwipe*PageSwipe*(3-2*PageSwipe);
    Offset=(Page==StationSignPage(StationIndex,PageIndex)?1-Ease:-Ease)*1440*PageDirection;
   }
   return FSlateRenderTransform(FVector2D(Offset,0));
  }));
  Pages->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top)[Tile];
 }
 return Pages;
}
bool ASlideGameMode::IsComponentPage() const {
 FString Kind;
 if(!StationSteps.IsValidIndex(Index)||!StationSteps[Index].IsValidIndex(PageIndex))return false;
 const auto Step=StationSteps[Index][PageIndex];const TSharedPtr<FJsonObject>* Component;
 return Step->TryGetStringField(TEXT("kind"),Kind)&&Kind==TEXT("component")&&(!Step->TryGetObjectField(TEXT("component"),Component)||((*Component)->GetStringField(TEXT("type"))!=TEXT("CommandLine")&&(*Component)->GetStringField(TEXT("type"))!=TEXT("FSV")&&(*Component)->GetStringField(TEXT("type"))!=TEXT("Serializer")&&(*Component)->GetStringField(TEXT("type"))!=TEXT("Scalar")));
}
void ASlideGameMode::ResetStationPage() {
 PageIndex=0;RevealedPage=0;PreviousPage=INDEX_NONE;PageSwipe=1;ActiveComponentPage=INDEX_NONE;
 PageColdStorage.Empty();ColdStorage.Reset();EmployeeBadge.Reset();WoodSign.Reset();WoodSignKey=MAX_uint64;RaptorView.Reset();RaptorKey=MAX_uint64;
 SpeedGraph.Reset();
 PageExchanges.Empty();Exchange.Reset();
 PageSerializers.Empty();Serializer.Reset();PageScalars.Empty();Scalar.Reset();
 PageCasts.Empty();CastPlayer.Reset();PageFSVs.Empty();FSV.Reset();
 ActiveTerminals.Empty();ComponentStartTimes.Empty();for(auto& Entry:PageTerminals)Entry.Value->Hide();
}
bool ASlideGameMode::AdvancePage(int32 Direction,bool SkipComponent) {
 if(!SkipComponent&&Exchange.IsValid()&&Exchange->Advance(Direction)){ViewedExchangeSteps.Add((uint64(Index)<<48)|(uint64(PageIndex)<<24)|uint64(Exchange->Selected));return true;}
 if(!SkipComponent&&bCommandDesktop&&bDesktopScalar&&Scalar.IsValid()&&Scalar->Advance(Direction)){ViewedScalarSteps.Add((uint64(Index)<<48)|(uint64(PageIndex)<<24)|uint64(Scalar->Selected));return true;}
 if(!SkipComponent&&bCommandDesktop&&bDesktopFSV&&FSV.IsValid()&&FSV->Advance(Direction)) {
  if(FSV->Selected>=0)ViewedFSVSystems.Add((uint64(Index)<<48)|(uint64(PageIndex)<<24)|uint64(FSV->Selected));
  return true;
 }
 if(!SkipComponent&&ColdStorage.IsValid()&&ColdStorage->Advance(Direction)) {
  if(ColdStorage->Selected>=0)ViewedColdSpecimens.Add((uint64(Index)<<48)|(uint64(PageIndex)<<24)|uint64(ColdStorage->Selected));
  return true;
 }
 if(!SkipComponent&&RaptorView.IsValid()&&RaptorView->Advance(Direction)) {
  if(RaptorView->Selected>=0)ViewedRaptors.Add((uint64(Index)<<48)|(uint64(PageIndex)<<24)|uint64(RaptorView->Selected));
  return true;
 }
 // Ignore repeated presses during the short swipe so pages cannot be skipped accidentally.
 if(PageSwipe<1)return true;
 const int32 Next=PageIndex+Direction;
 if(!StationSteps.IsValidIndex(Index)||!StationSteps[Index].IsValidIndex(Next))return false;
 PreviousPage=PageIndex;PageIndex=Next;PageDirection=Direction;RevealedPage=FMath::Max(RevealedPage,Next);
 PageSwipe=StationSignPage(Index,PreviousPage)!=StationSignPage(Index,PageIndex)?0:1;
 ActiveComponentPage=INDEX_NONE;
 return true;
}
int32 ASlideGameMode::StationSignPage(int32 Station,int32 Page) const {
 if(!StationSteps.IsValidIndex(Station))return INDEX_NONE;
 // Keep the most recent Markdown slide behind native component demonstrations.
 for(int32 Candidate=FMath::Min(Page,StationSteps[Station].Num()-1);Candidate>=0;Candidate--) {
  FString Kind;StationSteps[Station][Candidate]->TryGetStringField(TEXT("kind"),Kind);
  if(Kind!=TEXT("component"))return Candidate;
 }
 return INDEX_NONE;
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
 // Native hardware stays large in the foreground; ordinary pages remain on the sign behind it.
 for(auto& Entry:PageTerminals) {
  const int32 Station=int32(Entry.Key>>32),Page=int32(uint32(Entry.Key));
  const bool Visible=!bFreeFlight&&Station==Index&&Page<=RevealedPage&&(Showing||MapPhase==EMapPhase::Retract);
  if(!Showing)Entry.Value->Hide();
  Entry.Value->Update(Delta,Camera,Visible);
 }
 TSharedPtr<FJsonObject> DesktopComponent;
 if(Showing&&StationSteps.IsValidIndex(Index)&&StationSteps[Index].IsValidIndex(PageIndex)) {
  const TSharedPtr<FJsonObject>* Component;
  if(StationSteps[Index][PageIndex]->TryGetObjectField(TEXT("component"),Component)&&((*Component)->GetStringField(TEXT("type"))==TEXT("CommandLine")||(*Component)->GetStringField(TEXT("type"))==TEXT("FSV")||(*Component)->GetStringField(TEXT("type"))==TEXT("Serializer")||(*Component)->GetStringField(TEXT("type"))==TEXT("Scalar")))DesktopComponent=*Component;
 }
 TickCommandDesktop(Delta,DesktopComponent);
 TSharedPtr<FJsonObject> Prop;
 bool TitleOnly=false;
 if(Showing&&StationSteps.IsValidIndex(Index)&&StationSteps[Index].IsValidIndex(PageIndex)) {
  const auto Step=StationSteps[Index][PageIndex];Step->TryGetBoolField(TEXT("titleOnly"),TitleOnly);
  const TSharedPtr<FJsonObject>* C;if(Step->TryGetObjectField(TEXT("component"),C))Prop=*C;
 }
 const uint64 Key=(uint64(Index)<<32)|uint32(PageIndex);
 const FString Type=Prop.IsValid()?Prop->GetStringField(TEXT("type")):TEXT("");
 Exchange.Reset();
 if(Type==TEXT("Exchange")) {
  if(!PageExchanges.Contains(Key))PageExchanges.Add(Key,MakeShared<FParkExchange>(GetWorld(),Prop));
  Exchange=PageExchanges[Key];ViewedExchangeSteps.Add((uint64(Index)<<48)|(uint64(PageIndex)<<24)|uint64(Exchange->Selected));
 }
 for(auto& Entry:PageExchanges)Entry.Value->Update(Delta,Panels[Index].RaisedPosition,CameraStops[Index],Showing&&DesktopMinimize==0&&Exchange==Entry.Value);
 if(Type==TEXT("SpeedGraph")) {
  if(!SpeedGraph.IsValid())SpeedGraph=MakeShared<FParkSpeedGraph>(GetWorld(),Prop);
  SpeedGraph->Update(Delta,Camera,Panels[Index].RaisedPosition,CameraStops[Index],Showing&&DesktopMinimize==0);
 } else SpeedGraph.Reset();
 ColdStorage.Reset();
 if(Type==TEXT("ColdStorage")) {
  if(!PageColdStorage.Contains(Key))PageColdStorage.Add(Key,MakeShared<FParkColdStorage>(GetWorld(),Prop));
  ColdStorage=PageColdStorage[Key];
 }
 for(auto& Entry:PageColdStorage)Entry.Value->Update(Delta,Camera,Showing&&ColdStorage==Entry.Value);
 const bool Warning=Type==TEXT("RaptorWarning");
 const int32 PreviousSign=StationSignPage(Index,PageIndex);
 bool KeepDirection=false;
 if(Type==TEXT("ColdStorage")&&PreviousSign>=0)StationSteps[Index][PreviousSign]->TryGetBoolField(TEXT("titleOnly"),KeepDirection);
 const bool Badge=Showing&&Index==0&&PreviousSign==0;
 if(Badge) {
  if(!EmployeeBadge.IsValid())EmployeeBadge=MakeShared<FParkEmployeeBadge>(GetWorld(),StationSteps[0][0],LoginLogo);
  EmployeeBadge->Update(Camera,Panels[Index].RaisedPosition);
 } else EmployeeBadge.Reset();
 const uint64 SignKey=(uint64(Index)<<32)|uint32(KeepDirection?PreviousSign:PageIndex);
 if(!Badge&&(TitleOnly||Warning||KeepDirection)) {
  if(WoodSignKey!=SignKey){WoodSign=MakeShared<FParkWoodSign>(GetWorld(),Warning?Prop->GetStringField(TEXT("title")):StationSteps[Index][KeepDirection?PreviousSign:PageIndex]->GetStringField(TEXT("title")),Warning,MapPins[Index].Code==TEXT("HELI"));WoodSignKey=SignKey;}
 } else {WoodSign.Reset();WoodSignKey=MAX_uint64;}
 if(WoodSign.IsValid())WoodSign->Update(Elapsed,Camera,Index==GateSlide?GateScreenPosition(CardExpansion):Panels[Index].RaisedPosition,CameraStops[Index],CardExpansion);
 if(Type==TEXT("Raptors")) {
  if(RaptorKey!=Key){RaptorView=MakeShared<FParkRaptors>(GetWorld(),Prop,Camera,CameraLook,CameraFrameWidth);RaptorKey=Key;}
  RaptorView->Update(Delta,Camera,CameraLook,CameraFrameWidth);
 } else if(RaptorView.IsValid()) {
  if(Showing)RaptorView->Restore(Camera,CameraLook,CameraFrameWidth);
  RaptorView.Reset();RaptorKey=MAX_uint64;
 }
}
bool ASlideGameMode::UsesPhysicalProp() const {
 if(!StationSteps.IsValidIndex(Index)||!StationSteps[Index].IsValidIndex(PageIndex))return false;
 const auto Step=StationSteps[Index][PageIndex];bool TitleOnly=false;
 const int32 SignPage=StationSignPage(Index,PageIndex);if(SignPage>=0)StationSteps[Index][SignPage]->TryGetBoolField(TEXT("titleOnly"),TitleOnly);
 const TSharedPtr<FJsonObject>* C;
 return (Index==0&&SignPage==0)||TitleOnly||(Step->TryGetObjectField(TEXT("component"),C)&&((*C)->GetStringField(TEXT("type"))==TEXT("RaptorWarning")||(*C)->GetStringField(TEXT("type"))==TEXT("Raptors")||(*C)->GetStringField(TEXT("type"))==TEXT("SpeedGraph")||(*C)->GetStringField(TEXT("type"))==TEXT("Exchange")));

}

int32 ASlideGameMode::PowerPercent() const {
 int32 Total=0;for(const auto& Steps:StationSteps)for(const auto& Step:Steps) {
  Total++;const TSharedPtr<FJsonObject>* Component;
  if(Step->TryGetObjectField(TEXT("component"),Component)&&((*Component)->GetStringField(TEXT("type"))==TEXT("Scalar")||(*Component)->GetStringField(TEXT("type"))==TEXT("Exchange")))Total+=(*Component)->GetArrayField(TEXT("steps")).Num();
  if(Step->TryGetObjectField(TEXT("component"),Component)&&(*Component)->GetStringField(TEXT("type"))==TEXT("FSV"))Total+=(*Component)->GetArrayField(TEXT("systems")).Num();
  if(Step->TryGetObjectField(TEXT("component"),Component)&&(*Component)->GetStringField(TEXT("type"))==TEXT("ColdStorage"))Total+=4;
  if(Step->TryGetObjectField(TEXT("component"),Component)&&(*Component)->GetStringField(TEXT("type"))==TEXT("Raptors"))Total+=4;
 }
 return Total>0?FMath::Clamp(FMath::CeilToInt(100.f*(Total-ViewedPages.Num()-ViewedExchangeSteps.Num()-ViewedScalarSteps.Num()-ViewedFSVSystems.Num()-ViewedColdSpecimens.Num()-ViewedRaptors.Num())/Total),0,100):100;
}
int32 ASlideGameMode::SecurityPercent() const {
 return TimerDuration>0?FMath::Clamp(FMath::CeilToInt(100.f*(TimerDuration-TimerElapsed)/TimerDuration),0,100):100;
}
