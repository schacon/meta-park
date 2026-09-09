#include "SlideGameMode.h"
#include "ParkCastPlayer.h"
#include "Widgets/Input/SSlider.h"
#include "Dom/JsonObject.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Widgets/SViewport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Misc/Paths.h"

namespace {
FSlateFontInfo TerminalFont(int32 Size){return FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("Fonts/RobotoMono-Regular.ttf"),Size);}
constexpr float MinimizeSeconds=.45f;
const FButtonStyle& WorkstationButton() {
 static FButtonStyle Style=FButtonStyle().SetNormal(FSlateColorBrush(FLinearColor(.69,.69,.66))).SetHovered(FSlateColorBrush(FLinearColor(.82,.83,.79))).SetPressed(FSlateColorBrush(FLinearColor(.45,.47,.5)));
 return Style;
}
}

void ASlideGameMode::CaptureParkWindow() {
 // Capture the complete Slate viewport, including the map's title bar and sidebar.
 // The stored frame becomes both the shrinking window and its dock thumbnail.
 const auto Viewport=GetWorld()->GetGameViewport()->GetGameViewportWidget();
 TArray<FColor> Pixels;FIntVector Size;
 if(!Viewport.IsValid()||!FSlateApplication::Get().TakeScreenshot(Viewport.ToSharedRef(),Pixels,Size)||Size.X<=0||Size.Y<=0)return;
 for(auto& Pixel:Pixels)Pixel.A=255;
 ParkSnapshot=UTexture2D::CreateTransient(Size.X,Size.Y,PF_B8G8R8A8);
 auto& Bulk=ParkSnapshot->GetPlatformData()->Mips[0].BulkData;
 void* Data=Bulk.Lock(LOCK_READ_WRITE);FMemory::Memcpy(Data,Pixels.GetData(),Pixels.Num()*sizeof(FColor));Bulk.Unlock();
 ParkSnapshot->UpdateResource();
 ParkSnapshotBrush.SetResourceObject(ParkSnapshot);ParkSnapshotBrush.ImageSize=FVector2D(Size.X,Size.Y);ParkSnapshotBrush.DrawAs=ESlateBrushDrawType::Image;
}

void ASlideGameMode::TickCommandDesktop(float Delta,TSharedPtr<FJsonObject> Component) {
 if(!LoginHUD.IsValid()||bLocked)return;
 const bool Requested=Component.IsValid();
 if(Requested&&!bCommandDesktop) {
  if(DesktopMinimize==0)CaptureParkWindow();
  LoginHUD->SetVisibility(EVisibility::Visible);
  DesktopHUD->SetVisibility(EVisibility::Hidden); // Retain geometry and the camera's viewport reservation.
  FSlateApplication::Get().SetKeyboardFocus(LoginHUD);
 }
 if(Requested) {
  DesktopCommandKey=(uint64(Index)<<32)|uint32(PageIndex);
  if(!PageCasts.Contains(DesktopCommandKey)) {
   UE_LOG(LogTemp,Display,TEXT("Playing cast: %s"),*Component->GetStringField(TEXT("src")));
   PageCasts.Add(DesktopCommandKey,MakeShared<FParkCastPlayer>(Component->GetObjectField(TEXT("cast"))));
  }
  CastPlayer=PageCasts[DesktopCommandKey];
  if(DesktopMinimize==1)CastPlayer->Tick(Delta);
 }
 bCommandDesktop=Requested;
 DesktopMinimize=FMath::Clamp(DesktopMinimize+(Requested?1.f:-1.f)*Delta/MinimizeSeconds,0.f,1.f);
 if(!Requested&&DesktopMinimize==0&&LoginHUD->GetVisibility()!=EVisibility::Collapsed) {
  LoginHUD->SetVisibility(EVisibility::Collapsed);DesktopHUD->SetVisibility(EVisibility::Visible);
  FSlateApplication::Get().SetAllUserFocusToGameViewport();
 }
}
void ASlideGameMode::RestoreParkWindow() {
 if(bLocked||!bCommandDesktop||DesktopMinimize<1)return;
 // The dock returns to the preceding presentation step; arrows can also advance or leave the group.
 if(PageIndex>0)AdvancePage(-1);else Overview();
 FSlateApplication::Get().SetKeyboardFocus(LoginHUD);
}
TSharedRef<SWidget> ASlideGameMode::BuildCommandTerminal() {
 const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");
 return SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(FLinearColor(.018,.022,.018)).Padding(FMargin(2,2,7,7))
  [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(FLinearColor(.69,.69,.66)).Padding(3)
   [SNew(SVerticalBox)
    +SVerticalBox::Slot().AutoHeight()[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(FLinearColor(.35,.42,.47)).Padding(FMargin(12,8))
     [SNew(SHorizontalBox)
      +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(TEXT("Terminal — ")+(CastPlayer.IsValid()?CastPlayer->Title:TEXT("recording")));}).Font(TerminalFont(18)).ColorAndOpacity(FLinearColor(.95,.95,.88))]
      +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(&WorkstationButton()).IsFocusable(false).ContentPadding(FMargin(8,0)).OnClicked_Lambda([this]{RestoreParkWindow();return FReply::Handled();})
       [SNew(STextBlock).Text(FText::FromString(TEXT("−"))).Font(TerminalFont(18)).ColorAndOpacity(FLinearColor::Black)]]]]
    +SVerticalBox::Slot().FillHeight(1).Padding(3)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor_Lambda([this]{return CastPlayer.IsValid()?CastPlayer->Background:FLinearColor::Black;}).Padding(20)
     [MakeParkCastView([this]{return CastPlayer;})]]
    +SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SHorizontalBox)
     +SHorizontalBox::Slot().AutoWidth()[SAssignNew(CastPauseButton,SButton).ButtonStyle(&WorkstationButton()).IsFocusable(false).OnClicked_Lambda([this]{if(CastPlayer.IsValid())CastPlayer->TogglePlayback();return FReply::Handled();})
      [SNew(STextBlock).Text_Lambda([this]{return FText::FromString(CastPlayer.IsValid()&&!CastPlayer->Paused&&CastPlayer->Time<CastPlayer->Duration?TEXT("Pause"):TEXT("Play"));}).Font(TerminalFont(15)).ColorAndOpacity(FLinearColor::Black)]]
     +SHorizontalBox::Slot().AutoWidth().Padding(10,0)[SAssignNew(CastReplayButton,SButton).ButtonStyle(&WorkstationButton()).IsFocusable(false).OnClicked_Lambda([this]{if(CastPlayer.IsValid())CastPlayer->Restart();return FReply::Handled();})
      [SNew(STextBlock).Text(FText::FromString(TEXT("Replay"))).Font(TerminalFont(15)).ColorAndOpacity(FLinearColor::Black)]]
     +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(10,0)[SNew(SSlider).IsFocusable(false)
      .Value_Lambda([this]{return CastPlayer.IsValid()&&CastPlayer->Duration>0?CastPlayer->Time/CastPlayer->Duration:0.f;})
      .OnValueChanged_Lambda([this](float Value){if(CastPlayer.IsValid())CastPlayer->Seek(Value*CastPlayer->Duration);})]
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(CastPlayer.IsValid()?FString::Printf(TEXT("%02d:%02d / %02d:%02d"),int32(CastPlayer->Time)/60,int32(CastPlayer->Time)%60,int32(CastPlayer->Duration)/60,int32(CastPlayer->Duration)%60):TEXT(""));}).Font(TerminalFont(14)).ColorAndOpacity(FLinearColor::Black)]
    ]]];
}
TSharedRef<SWidget> ASlideGameMode::BuildParkDock() {
 return SAssignNew(MapDockButton,SButton).ButtonStyle(&WorkstationButton()).IsFocusable(false).ContentPadding(4)
  .ToolTipText(FText::FromString(TEXT("Restore park control")))
  .OnClicked_Lambda([this]{RestoreParkWindow();return FReply::Handled();})
  [SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride_Lambda([this]{const FVector2D Size=LoginHUD->GetCachedGeometry().GetLocalSize();return FOptionalSize(184.f*Size.Y/FMath::Max(1.,Size.X));})[SNew(SImage).Image(&ParkSnapshotBrush)]]
   +SVerticalBox::Slot().AutoHeight().Padding(0,5,0,0)[SNew(STextBlock).Text(FText::FromString(TEXT("Park control"))).Font(TerminalFont(15)).Justification(ETextJustify::Center).ColorAndOpacity(FLinearColor::Black)]];
}
TSharedRef<SWidget> ASlideGameMode::BuildMinimizeAnimation() {
 auto Image=SNew(SImage).Image(&ParkSnapshotBrush)
  .Visibility_Lambda([this]{return !bLocked&&DesktopMinimize>0&&DesktopMinimize<1?EVisibility::HitTestInvisible:EVisibility::Collapsed;});
 Image->SetRenderTransformPivot(FVector2D::ZeroVector);
 Image->SetRenderTransform(TAttribute<TOptional<FSlateRenderTransform>>::CreateLambda([this]()->TOptional<FSlateRenderTransform>{
  const FVector2D Size=LoginHUD->GetCachedGeometry().GetLocalSize();
  const float Ease=DesktopMinimize*DesktopMinimize*(3-2*DesktopMinimize);
  const float Scale=FMath::Lerp(1.f,184.f/FMath::Max(1.,Size.X),Ease);
  const FVector2D Dock(28,Size.Y-113-184*Size.Y/FMath::Max(1.,Size.X));
  return FSlateRenderTransform(FScale2D(Scale),Dock*Ease);
 }));
 return Image;
}
