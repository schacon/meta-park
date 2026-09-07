#include "SlideGameMode.h"
#include "ParkCallouts.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

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
 auto Text=[&](const TCHAR* Value,int Size=20) { return SNew(STextBlock).Text(FText::FromString(Value)).Font(Font(Size)).ColorAndOpacity(Ink); };
 auto Side=SNew(SVerticalBox);
 auto Rule=[&](){Side->AddSlot().AutoHeight().Padding(0,8)[SNew(SBox).HeightOverride(2)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink)]];};
 auto Row=[&](const TCHAR* Label,TAttribute<FText> Value) {
  Side->AddSlot().AutoHeight().Padding(0,2)[SNew(SHorizontalBox)
   +SHorizontalBox::Slot().FillWidth(1)[Text(Label)]
   +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(Value).Font(Font(20)).ColorAndOpacity(Ink)]];
 };
 Side->AddSlot().AutoHeight().Padding(0,10,0,0)[Text(TEXT("ingen:~$"),23)]; Rule();
 for(const FString Label:{TEXT("Park Status"),TEXT("Dinosaurs"),TEXT("Enclosures"),TEXT("Research"),TEXT("Facilities"),TEXT("Maps"),TEXT("System")}) {
  Side->AddSlot().AutoHeight()[SNew(SBorder).BorderImage(Brush).Padding(0)
   .BorderBackgroundColor_Lambda([this,Active,Label]{return ParkSection==Label?Active:FLinearColor::Transparent;})
   [SNew(SButton).ButtonStyle(&MenuStyle).IsFocusable(false).ContentPadding(FMargin(9,1))
    .OnClicked_Lambda([this,Label]{if(!FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest"))){ParkSection=Label;bAllHabitats=Label==TEXT("Dinosaurs")||Label==TEXT("Enclosures");Overview();}return FReply::Handled();})
    [SNew(STextBlock).Text_Lambda([this,Label]{return FText::FromString((ParkSection==Label?TEXT("> "):TEXT("  "))+Label);}).Font(Font(20)).ColorAndOpacity(Ink)]]];
 }
 Rule();
 Side->AddSlot().AutoHeight().Padding(0,0,0,8)[Text(TEXT("PARK STATUS"),18)];
 Side->AddSlot().AutoHeight().HAlign(HAlign_Left)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Active).Padding(FMargin(10,4))[Text(TEXT("OPERATIONAL"),22)]];
 Rule();
 Row(TEXT("Visitors"),FText::FromString(TEXT("1,248")));
 Row(TEXT("Staff"),FText::FromString(TEXT("312")));
 Row(TEXT("Dinosaurs"),TAttribute<FText>::CreateLambda([this]{return FText::AsNumber(DinoCount);}));
 Row(TEXT("Enclosures"),TAttribute<FText>::CreateLambda([this]{return FText::AsNumber(HabitatCount);}));
 Rule();
 auto Meter=[&](const TCHAR* Label,int Percent) {
  auto Cells=SNew(SHorizontalBox);
  for(int I=0;I<5;I++)Cells->AddSlot().AutoWidth().Padding(2,0)[SNew(SBox).WidthOverride(19).HeightOverride(22)
   [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(1.5)
    [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(I<FMath::RoundToInt(Percent/20.f)?Active:FLinearColor(.38,.4,.36))]]];
  Side->AddSlot().AutoHeight().Padding(0,5)[SNew(SHorizontalBox)
   +SHorizontalBox::Slot().FillWidth(1)[Text(Label,17)]
   +SHorizontalBox::Slot().AutoWidth().Padding(0,0,5,0)[Text(*FString::Printf(TEXT("%d%%"),Percent),17)]
   +SHorizontalBox::Slot().AutoWidth()[Cells]];
 };
 Meter(TEXT("Power"),87);Meter(TEXT("Water"),92);Meter(TEXT("Security"),94);
 Rule();
 Side->AddSlot().AutoHeight().Padding(0,0,0,9)[Text(TEXT("Weather"),20)];
 static FSlateRoundedBoxBrush SunHalo(FLinearColor(.98,.73,.18),32.f);
 static FSlateRoundedBoxBrush SunDisc(FLinearColor(1,.43,.005),24.f);
 Side->AddSlot().AutoHeight()[SNew(SHorizontalBox)
  +SHorizontalBox::Slot().AutoWidth().Padding(12,0,0,0)[SNew(SBox).WidthOverride(64).HeightOverride(64)
   [SNew(SBorder).BorderImage(&SunHalo).Padding(8)[SNew(SBorder).BorderImage(&SunDisc)]]]
  +SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Right).VAlign(VAlign_Center)[Text(TEXT("Sunny\n26°C"),20)]];
 Rule();
 Side->AddSlot().FillHeight(1).MinHeight(12);
 Side->AddSlot().AutoHeight().Padding(0,6,0,12)[Text(TEXT("L I F E\nF I N D S\nA  W A Y  _"),22)];
 auto Scanlines=SNew(SVerticalBox).Visibility(EVisibility::HitTestInvisible);
 for(int I=0;I<140;I++) Scanlines->AddSlot().FillHeight(1).VAlign(VAlign_Bottom)[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(FLinearColor(0,0,0,.008f))]];
 DesktopHUD=SNew(SOverlay)
 +SOverlay::Slot()[SNew(SParkCallouts).Controller(GetWorld()->GetFirstPlayerController())
 .Visibility_Lambda([this]{return bOverview?EVisibility::Visible:EVisibility::Collapsed;})
 .Pins([this]{TArray<FParkMapPin> Result;for(const auto& P:MapPins){const bool Visible=ParkSection==TEXT("Research")?P.Code==TEXT("VC"):ParkSection==TEXT("Facilities")?P.SlideIndex<0:bAllHabitats?P.SlideIndex>=0:P.bHero;if(Visible)Result.Add(P);}return Result;})
 .Choose([this](int32 Next){if(!FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest")))GoTo(Next);})]
 +SOverlay::Slot().HAlign(HAlign_Left)[SNew(SBox).WidthOverride(350)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(2)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Gray).Padding(FMargin(25,12))[Side]]]]
 +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(24,26)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(FMargin(3,3,7,7))[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Gray).Padding(12)[Text(TEXT("ISLA NUBLAR\nPARK CONTROL\n1993"),17)]]]
 +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(24,135)
 [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(2)
  [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Gray).Padding(10)
   [SNew(STextBlock).Text_Lambda([this]{const int S=FMath::Max(0,FMath::CeilToInt(TimerDuration-TimerElapsed));return FText::FromString(FString::Printf(TEXT("SLIDE %02d / %02d\n%02d:%02d REMAINING"),bTourStarted?Index+1:0,Views.Num(),S/60,S%60));}).Font(Font(15)).ColorAndOpacity(Ink)]]]
 +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(24,24)[SNew(STextBlock).Text(FText::FromString(TEXT("N\n↑"))).Justification(ETextJustify::Center).Font(FCoreStyle::GetDefaultFontStyle("Mono",30)).ColorAndOpacity(FLinearColor(.93,.96,.88))]
 +SOverlay::Slot()[Scanlines];
 GetWorld()->GetGameViewport()->AddViewportWidgetContent(DesktopHUD.ToSharedRef(),10); SetMouseMode(false);
}
void ASlideGameMode::EndPlay(const EEndPlayReason::Type Reason) {
 if(DesktopHUD.IsValid() && GetWorld() && GetWorld()->GetGameViewport()) GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(DesktopHUD.ToSharedRef());
 DesktopHUD.Reset(); Super::EndPlay(Reason);
}
