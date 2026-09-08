#include "SlideGameMode.h"
#include "ParkCallouts.h"
#include "ParkFlightInput.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"

void ASlideGameMode::SetMouseMode(bool Fly) {
 auto* PC=GetWorld()->GetFirstPlayerController(); if(!PC) return;
 if(FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest"))) Fly=true;
 PC->bShowMouseCursor=!Fly;
 if(Fly) { FInputModeGameOnly Mode; PC->SetInputMode(Mode); }
 else { FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false); Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); PC->SetInputMode(Mode); }
}
void ASlideGameMode::CreateDesktopHUD() {
 const FLinearColor Ink(.018,.022,.018), Gray(.69,.69,.66), Active(.13,.61,.24);
 const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");
 static FButtonStyle MenuStyle=FButtonStyle().SetNormal(FSlateColorBrush(FLinearColor::Transparent)).SetHovered(FSlateColorBrush(FLinearColor(.32,.64,.35))).SetPressed(FSlateColorBrush(FLinearColor(.18,.48,.25)));
 auto Font=[](int Size) {return FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("Fonts/RobotoMono-Bold.ttf"),Size);};
 auto SideFont=[](int Size) {return FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("Fonts/RobotoMono-Regular.ttf"),FMath::RoundToInt(Size*.85f));};
 auto Text=[&](const TCHAR* Value,int Size=20) { return SNew(STextBlock).Text(FText::FromString(Value)).Font(SideFont(Size)).ColorAndOpacity(Ink); };
 auto Side=SNew(SVerticalBox);
 auto Rule=[&](){Side->AddSlot().AutoHeight().Padding(0,8)[SNew(SBox).HeightOverride(2)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink)]];};
 auto Row=[&](const TCHAR* Label,TAttribute<FText> Value) {
  Side->AddSlot().AutoHeight().Padding(0,2)[SNew(SHorizontalBox)
   +SHorizontalBox::Slot().FillWidth(1)[Text(Label)]
   +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(Value).Font(SideFont(20)).ColorAndOpacity(Ink)]];
 };
 Side->AddSlot().AutoHeight().Padding(0,10,0,0)[Text(TEXT("ingen:~$"),23)]; Rule();
 for(const auto& Pin:MapPins) {
  const int32 SlideIndex=Pin.SlideIndex;
  FString Name=Pin.Name;
  if(Name==TEXT("Brontosaurus"))Name=TEXT("Bronto. paddock");
  else if(Name==TEXT("Triceratops"))Name=TEXT("Tri. paddock");
  else if(Name==TEXT("T. rex"))Name+=TEXT(" paddock");
  const FString Label=FString::Printf(TEXT("%d %s"),SlideIndex+1,*Name);
  Side->AddSlot().AutoHeight()[SNew(SBorder).BorderImage(Brush).Padding(0)
   .BorderBackgroundColor_Lambda([this,Active,SlideIndex]{return (PendingIndex>=0?PendingIndex:MapPhase!=EMapPhase::Overview?Index:-1)==SlideIndex?Active:FLinearColor::Transparent;})
   [SNew(SButton).ButtonStyle(&MenuStyle).IsFocusable(false).ContentPadding(FMargin(6,3))
    .OnClicked_Lambda([this,SlideIndex]{GoTo(SlideIndex);return FReply::Handled();})
    [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly).HAlign(HAlign_Left)
     [SNew(STextBlock).Text(FText::FromString(Label)).Font(SideFont(18)).ColorAndOpacity(Ink)]]]];
 }
 Rule();
 Side->AddSlot().AutoHeight().Padding(0,0,0,8)[Text(TEXT("PARK STATUS"),18)];
 Side->AddSlot().AutoHeight().HAlign(HAlign_Left)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Active).Padding(FMargin(10,4))[Text(TEXT("OPERATIONAL"),22)]];
 Rule();
 auto PopulationRow=[&](const TCHAR* Label,bool Visitors) {
  auto Count=[this,Visitors]{return Visitors?RemainingVisitors():RemainingStaff();};
  Side->AddSlot().AutoHeight().Padding(0,2)[SNew(SHorizontalBox)
   +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text(FText::FromString(Label)).Font(SideFont(20))
    .ColorAndOpacity_Lambda([this,Count]{return PopulationColor(Count());})]
   +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text_Lambda([Count]{return FText::AsNumber(Count());}).Font(SideFont(20))
    .ColorAndOpacity_Lambda([this,Count]{return PopulationColor(Count());})]];
 };
 PopulationRow(TEXT("Visitors"),true);PopulationRow(TEXT("Staff"),false);
 Row(TEXT("Dinosaurs"),TAttribute<FText>::CreateLambda([this]{return FText::AsNumber(DinoPopulation());}));
 Rule();
 Side->AddSlot().FillHeight(1).MinHeight(12);
 Side->AddSlot().AutoHeight().HAlign(HAlign_Right).Padding(0,12)[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(bFreeFlight?TEXT("FREE FLIGHT\nDrag to look\nWASD / arrows move\nQ down · E up\nShift faster\n1–7 fly · F overview"):(HiddenSlides.Contains(Index)&&!bOverview?TEXT("BONUS 08"):FString::Printf(TEXT("SLIDE %02d / %02d"),bOverview?0:Index+1,Views.Num()-HiddenSlides.Num())));}).Font(SideFont(17)).ColorAndOpacity(Ink).Justification(ETextJustify::Right)];
 Rule();
 Side->AddSlot().AutoHeight().HAlign(HAlign_Right).Padding(0,6,0,12)[SNew(STextBlock).Text(FText::FromString(TEXT("G I T\nF I N D S\nA  W A Y  _"))).Font(SideFont(22)).ColorAndOpacity(Ink).Justification(ETextJustify::Right)];
 auto Scanlines=SNew(SVerticalBox).Visibility(EVisibility::HitTestInvisible);
 for(int I=0;I<140;I++) Scanlines->AddSlot().FillHeight(1).VAlign(VAlign_Bottom)[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(FLinearColor(0,0,0,.008f))]];
 DesktopHUD=SNew(SOverlay)
 +SOverlay::Slot()[SNew(SParkFlightInput).Visibility_Lambda([this]{return bFreeFlight?EVisibility::Visible:EVisibility::Collapsed;}).Look([this](FVector2D Delta){FreeDrag+=Delta;})]
 +SOverlay::Slot()[SNew(SParkCallouts).Controller(GetWorld()->GetFirstPlayerController())
 .Visibility_Lambda([this]{return !bFreeFlight&&(MapPhase==EMapPhase::Overview||(MapPhase==EMapPhase::ZoomOut&&Travel>=MapLegDuration*.88f))?EVisibility::Visible:EVisibility::Collapsed;})
 .Pins([this]{return MapPins;})
 .Choose([this](int32 Next){if(!FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest")))GoTo(Next);})]
 +SOverlay::Slot().HAlign(HAlign_Left).Padding(0,72,0,0)[SNew(SBox).WidthOverride_Lambda([this]{const float W=DesktopHUD.IsValid()?DesktopHUD->GetCachedGeometry().GetLocalSize().X:0;return W>0?FMath::Min(280.f,W*.26f):280.f;})[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(2)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Gray).Padding(FMargin(19,10))[SNew(SScaleBox).VAlign(VAlign_Top).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(238)
   .HeightOverride_Lambda([this]{const float H=DesktopHUD.IsValid()?DesktopHUD->GetCachedGeometry().GetLocalSize().Y:0;return H>0?FMath::Max(400.f,H-72.f-24.f-20.f):600.f;})
   [Side]]]]]]
 +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(24,96)[SNew(SBorder).Visibility_Lambda([this]{return CardExpansion<.01f?EVisibility::Visible:EVisibility::Collapsed;}).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(FMargin(3,3,7,7))[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Gray).Padding(12)[Text(TEXT("ISLA NUBLAR\nPARK CONTROL\n1993"),17)]]]
 +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(24,24)[SNew(STextBlock).Text(FText::FromString(TEXT("N\n↑"))).Justification(ETextJustify::Center).Font(FCoreStyle::GetDefaultFontStyle("Mono",30)).ColorAndOpacity(FLinearColor(.93,.96,.88))]
 +SOverlay::Slot().VAlign(VAlign_Top)[SNew(SBox).HeightOverride(72)
  [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(2)
   [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Gray).Padding(FMargin(24,12))
    [SNew(SHorizontalBox)
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SComboButton).HasDownArrow(false).ButtonStyle(&MenuStyle).ContentPadding(0)
      .OnGetMenuContent_Lambda([this]{
       FMenuBuilder Menu(true,nullptr);
       Menu.AddMenuEntry(FText::FromString(TEXT("Quit")),FText::FromString(TEXT("Exit git-meta park")),FSlateIcon(),FUIAction(FExecuteAction::CreateLambda([this]{UKismetSystemLibrary::QuitGame(this,GetWorld()->GetFirstPlayerController(),EQuitPreference::Quit,false);})));
       return Menu.MakeWidget();
      })
      .ButtonContent()[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(FMargin(10,4))
       [SNew(STextBlock).Text(FText::FromString(TEXT("git-meta"))).Font(Font(20)).ColorAndOpacity(FLinearColor(.94,.94,.9))]] ]
     +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(20,0)[SNew(SHorizontalBox)
      +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("park visitor survival monitoring system"))).Font(Font(20)).ColorAndOpacity(Ink)]
      +SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(16,0)[SNew(STextBlock).Text(FText::FromString(TEXT("it's UNIX, I know this..."))).Font(SideFont(18)).Justification(ETextJustify::Center).ColorAndOpacity(FLinearColor(.025,.22,.055))]
     ]
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12,0,24,0)
      [SNew(STextBlock).Text_Lambda([]{return FText::FromString(TEXT("v2.55.0 · ")+FDateTime::Now().ToString(TEXT("%H:%M")));}).Font(Font(18)).ColorAndOpacity(Ink)]
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(FMargin(10,5))
      [SNew(STextBlock).Text(FText::FromString(TEXT("s.chacon"))).Font(Font(18)).ColorAndOpacity(FLinearColor(.94,.94,.9))]]
    ]]]]
 +SOverlay::Slot()[Scanlines];
 GetWorld()->GetGameViewport()->AddViewportWidgetContent(DesktopHUD.ToSharedRef(),10); SetMouseMode(false);
}
void ASlideGameMode::EndPlay(const EEndPlayReason::Type Reason) {
 if(DesktopHUD.IsValid() && GetWorld() && GetWorld()->GetGameViewport()) GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(DesktopHUD.ToSharedRef());
 DesktopHUD.Reset(); Super::EndPlay(Reason);
}

FVector4 ASlideGameMode::ExpandedCardBounds() const {
 const FVector2D Size=DesktopHUD.IsValid()?DesktopHUD->GetCachedGeometry().GetLocalSize():FVector2D(1920,1080);
 const FVector4 Full(385,40,FMath::Max(400.,Size.X-425),FMath::Max(300.,Size.Y-80));
 return FMath::Lerp(CardStartRect,Full,CardExpansion);
}
