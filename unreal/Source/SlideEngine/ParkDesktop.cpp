#include "SlideGameMode.h"
#include "DnaMascot.h"
#include "Engine/GameViewportClient.h"
#include "Engine/TextureRenderTarget2D.h"
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
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void ASlideGameMode::SetMouseMode(bool Fly) {
 auto* PC=GetWorld()->GetFirstPlayerController(); if(!PC) return;
 if(FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest"))) Fly=true;
 PC->bShowMouseCursor=!Fly;
 if(Fly) { FInputModeGameOnly Mode; PC->SetInputMode(Mode); }
 else { FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false); Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); PC->SetInputMode(Mode); }
}
void ASlideGameMode::CreateDesktopHUD() {
 const auto DNA=DnaMascot::Create(GetWorld()); MascotActor=DNA.Actor; MascotCapture=DNA.Capture; MascotTexture=DNA.Texture;
 MascotBrush.SetResourceObject(MascotTexture); MascotBrush.ImageSize=FVector2D(210,210);
 const FLinearColor Ink(.025,.045,.05), Gray(.61,.63,.59), Active(.14,.56,.24);
 const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");
 static FButtonStyle MenuStyle=FButtonStyle().SetNormal(FSlateColorBrush(FLinearColor::Transparent)).SetHovered(FSlateColorBrush(FLinearColor(.32,.64,.35))).SetPressed(FSlateColorBrush(FLinearColor(.18,.48,.25)));
 auto Text=[&](const TCHAR* Value,int Size=19) { return SNew(STextBlock).Text(FText::FromString(Value)).Font(FCoreStyle::GetDefaultFontStyle("Mono",Size)).ColorAndOpacity(Ink); };
 auto Side=SNew(SVerticalBox);
 auto Rule=[&](){Side->AddSlot().AutoHeight().Padding(0,11)[SNew(SBox).HeightOverride(2)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink)]];};
 auto Button=[&](const TCHAR* Label,TFunction<void()> Action) { return SNew(SButton).ButtonStyle(&MenuStyle).IsFocusable(false).ContentPadding(FMargin(7,3)).OnClicked_Lambda([Action]{if(!FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest")))Action();return FReply::Handled();})[Text(Label,20)]; };
 Side->AddSlot().AutoHeight().Padding(6,4)[Text(TEXT("ingen:~$"),24)]; Rule();
 Side->AddSlot().AutoHeight()[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor_Lambda([this,Active]{return bOverview?Active:FLinearColor::Transparent;}).Padding(0)[Button(TEXT("> Park Status"),[this]{Overview();})]];
 Side->AddSlot().AutoHeight()[Button(TEXT("  Next slide  →"),[this]{GoTo(bOverview?Index:Index+1);})];
 Side->AddSlot().AutoHeight()[Button(TEXT("  Previous    ←"),[this]{GoTo(Index-1);})];
 Side->AddSlot().AutoHeight()[Button(TEXT("  Map / return"),[this]{if(bOverview)GoTo(Index);else Overview();})];
 Side->AddSlot().AutoHeight()[Button(TEXT("  Pause timer"),[this]{bTimerPaused=!bTimerPaused;})]; Rule();
 Side->AddSlot().AutoHeight().Padding(6,0,0,5)[Text(TEXT("PARK STATUS"),19)];
 Side->AddSlot().AutoHeight()[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Active).Padding(FMargin(8,4))[Text(TEXT("OPERATIONAL"),21)]]; Rule();
 Side->AddSlot().AutoHeight().Padding(6,0)[SNew(STextBlock).Text_Lambda([this]{int Open=0;for(const auto& P:Panels)if(P.Reveal>.1f)Open++;return FText::FromString(FString::Printf(TEXT("Dinosaurs      %2d\nEnclosures     %2d\nTrees         %3d\nDisplays       %2d"),DinoCount,HabitatCount,TreeCount,Open));}).Font(FCoreStyle::GetDefaultFontStyle("Mono",18)).ColorAndOpacity(Ink)]; Rule();
 Side->AddSlot().AutoHeight().Padding(6,0,0,5)[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(FString::Printf(TEXT("Slide    %02d of %02d"),bTourStarted?Index+1:0,Views.Num()));}).Font(FCoreStyle::GetDefaultFontStyle("Mono",19)).ColorAndOpacity(Ink)];
 Side->AddSlot().AutoHeight().Padding(6,0)[SNew(STextBlock).Text_Lambda([this]{const int S=FMath::Max(0,FMath::CeilToInt(TimerDuration-TimerElapsed));return FText::FromString(FString::Printf(TEXT("%02d:%02d"),S/60,S%60));}).Font(FCoreStyle::GetDefaultFontStyle("Mono",34)).ColorAndOpacity(Ink)];
 Side->AddSlot().AutoHeight().Padding(6,0)[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(!bTourStarted?TEXT("ready to begin"):bTimerPaused?TEXT("clock paused"):TimerElapsed>=TimerDuration?TEXT("time elapsed"):TEXT("time remaining"));}).Font(FCoreStyle::GetDefaultFontStyle("Mono",14)).ColorAndOpacity(Ink)];
 Side->AddSlot().AutoHeight().Padding(6,8)[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(bOverview?TEXT("ISLAND OVERVIEW"):HabitatNames.IsValidIndex(Index)&&!HabitatNames[Index].IsEmpty()?HabitatNames[Index]:Titles[Index]);}).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Mono",15)).ColorAndOpacity(Ink)];
 Side->AddSlot().FillHeight(1); Rule();
 Side->AddSlot().AutoHeight().Padding(6,0,0,5)[Text(TEXT("MR. DNA"),16)];
 Side->AddSlot().AutoHeight().HAlign(HAlign_Center)[SNew(SBox).WidthOverride(175).HeightOverride(175)[SNew(SImage).Image(&MascotBrush)]];
 Side->AddSlot().AutoHeight().Padding(6,7)[Text(TEXT("LIFE FINDS A WAY _"),15)];
 auto Scanlines=SNew(SVerticalBox).Visibility(EVisibility::HitTestInvisible);
 for(int I=0;I<140;I++) Scanlines->AddSlot().FillHeight(1).VAlign(VAlign_Bottom)[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(FLinearColor(0,0,0,.008f))]];
 DesktopHUD=SNew(SOverlay)
 +SOverlay::Slot().HAlign(HAlign_Left)[SNew(SBox).WidthOverride(350)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(2)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Gray).Padding(FMargin(20,10))[Side]]]]
 +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(24,34)[SNew(STextBlock).Text(FText::FromString(TEXT("Isla Nublar\nPark control / 1993"))).Justification(ETextJustify::Right).Font(FCoreStyle::GetDefaultFontStyle("Mono",21)).ColorAndOpacity(FLinearColor(.93,.96,.88))]
 +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(24,24)[SNew(STextBlock).Text(FText::FromString(TEXT("N\n↑"))).Justification(ETextJustify::Center).Font(FCoreStyle::GetDefaultFontStyle("Mono",30)).ColorAndOpacity(FLinearColor(.93,.96,.88))]
 +SOverlay::Slot()[Scanlines];
 GetWorld()->GetGameViewport()->AddViewportWidgetContent(DesktopHUD.ToSharedRef(),10); SetMouseMode(false);
}
void ASlideGameMode::EndPlay(const EEndPlayReason::Type Reason) {
 if(DesktopHUD.IsValid() && GetWorld() && GetWorld()->GetGameViewport()) GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(DesktopHUD.ToSharedRef());
 DesktopHUD.Reset(); Super::EndPlay(Reason);
}
