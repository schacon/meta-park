#include "SlideGameMode.h"
#include "IslandScene.h"
#include "ParkCamera.h"
#include "ParkTerminal.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Components/SceneComponent.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Misc/DateTime.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Engine/Texture2D.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// Catch Enter before either the workstation field or the button consumes it.
// All other keys stay within Slate while the simulated login owns input.
class SParkLoginRoot : public SCompoundWidget {
public:
 SLATE_BEGIN_ARGS(SParkLoginRoot){} SLATE_EVENT(FSimpleDelegate,Login) SLATE_ATTRIBUTE(bool,Locked) SLATE_EVENT(FOnKeyDown,PresentationKey) SLATE_END_ARGS()
 FSimpleDelegate Login;TAttribute<bool> Locked;FOnKeyDown PresentationKey;
 void Construct(const FArguments& Args){Login=Args._Login;Locked=Args._Locked;PresentationKey=Args._PresentationKey;}
 void SetContent(TSharedRef<SWidget> Content){ChildSlot[Content];}
 virtual bool SupportsKeyboardFocus() const override{return true;}
 virtual FReply OnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override {
  if(!Locked.Get())return PresentationKey.IsBound()?PresentationKey.Execute(Geometry,Event):FReply::Unhandled();
  if(Event.GetKey()==EKeys::Enter){Login.ExecuteIfBound();return FReply::Handled();}
  return FReply::Unhandled();
 }
 virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent&) override{return Locked.Get()?FReply::Handled():FReply::Unhandled();}
};

class SParkDesktopClock : public SLeafWidget {
public:
 SLATE_BEGIN_ARGS(SParkDesktopClock){} SLATE_END_ARGS()
 void Construct(const FArguments&){}
 virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(180,180);}
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override {
  const FVector2D Center=G.GetLocalSize()*.5;const float Radius=FMath::Min(Center.X,Center.Y)-8;
  const FLinearColor Ink(.018,.022,.018);
  auto Point=[&](float Angle,float Length){return Center+FVector2D(FMath::Sin(Angle),-FMath::Cos(Angle))*Length;};
  auto Line=[&](FVector2D A,FVector2D B,FLinearColor Color,float Width){FSlateDrawElement::MakeLines(Out,Layer+1,G.ToPaintGeometry(),{A,B},ESlateDrawEffect::None,Color,true,Width);};
  for(int I=0;I<60;I++)Line(Point(I*PI/30,Radius-2),Point(I*PI/30,Radius-(I%5?5:9)),Ink,1);
  for(int I=0;I<4;I++) {
   const FVector2D P=Point(I*PI/2,Radius-23)-FVector2D(I==0?9:5,9);
   FSlateDrawElement::MakeText(Out,Layer+1,G.ToPaintGeometry(FVector2D(24,22),FSlateLayoutTransform(P)),FString::FromInt(I==0?12:I*3),FCoreStyle::GetDefaultFontStyle("Regular",13),ESlateDrawEffect::None,Ink);
  }
  const FDateTime Now=FDateTime::Now();
  auto Hand=[&](float Angle,float Length,float Width){
   Line(Center,Point(Angle,Length),Ink,Width*.65f);
  };
  Hand((Now.GetHour()%12+Now.GetMinute()/60.f)*PI/6,Radius*.48f,7);
  Hand((Now.GetMinute()+Now.GetSecond()/60.f)*PI/30,Radius*.77f,5);
  return Layer+1;
 }
};

void ASlideGameMode::CreateLoginHUD() {
 const FLinearColor Ink(.018,.022,.018),Gray(.69,.69,.66),Paper(.83,.84,.79),Teal(.025,.16,.17);
 const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");
 auto Font=[](int Size,bool Bold=false){return FSlateFontInfo(FPaths::ProjectContentDir()/(Bold?TEXT("Fonts/RobotoMono-Bold.ttf"):TEXT("Fonts/RobotoMono-Regular.ttf")),Size);};
 auto Label=[&](const TCHAR* Value,int Size=18){return SNew(STextBlock).Text(FText::FromString(Value)).Font(Font(Size)).ColorAndOpacity(Ink);};
 auto Rule=[&](){return SNew(SBox).HeightOverride(2)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(0)];};
 static FButtonStyle LoginButton=FButtonStyle().SetNormal(FSlateColorBrush(FLinearColor(.13,.61,.24))).SetHovered(FSlateColorBrush(FLinearColor(.24,.75,.35))).SetPressed(FSlateColorBrush(FLinearColor(.08,.4,.16)));
 LoginLogo=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Models/UI/T_ParkLogo.T_ParkLogo"));
 check(LoginLogo);LoginLogoBrush.SetResourceObject(LoginLogo);LoginLogoBrush.ImageSize=FVector2D(400,270);LoginLogoBrush.DrawAs=ESlateBrushDrawType::Image;
 static FEditableTextBoxStyle WorkstationStyle=FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
 WorkstationStyle.SetBackgroundImageNormal(FSlateColorBrush(Paper)).SetBackgroundImageHovered(FSlateColorBrush(FLinearColor(.9,.92,.85)))
  .SetBackgroundImageFocused(FSlateColorBrush(FLinearColor(.72,.91,.72))).SetBackgroundImageReadOnly(FSlateColorBrush(Paper))
  .SetForegroundColor(Ink).SetFocusedForegroundColor(Ink);
 static FButtonStyle DesktopButton=FButtonStyle().SetNormal(FSlateColorBrush(FLinearColor(.69,.69,.66))).SetHovered(FSlateColorBrush(FLinearColor(.82,.83,.79))).SetPressed(FSlateColorBrush(FLinearColor(.45,.47,.5)));
 auto Minimize=[&](FString Key){return SNew(SBox).WidthOverride(28).HeightOverride(24)
  [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(FLinearColor(.92,.93,.88)).Padding(FMargin(1,1,2,2))
   [SNew(SButton).ButtonStyle(&DesktopButton).ContentPadding(0)
    .ToolTipText(FText::FromString(TEXT("Minimize / restore")))
    .OnClicked_Lambda([this,Key]{if(MinimizedLoginWindows.Contains(Key))MinimizedLoginWindows.Remove(Key);else MinimizedLoginWindows.Add(Key);return FReply::Handled();})
    [SNew(STextBlock).Text(FText::FromString(TEXT("−"))).Font(Font(17,true)).Justification(ETextJustify::Center).ColorAndOpacity(Ink)]]];};
 auto Form=SNew(SVerticalBox);
 Form->AddSlot().AutoHeight()[SNew(SHorizontalBox)
  +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[Label(TEXT("Park control / login"),16)]
  +SHorizontalBox::Slot().AutoWidth()[Label(TEXT("v2.55.0"),16)]];
 auto FormBody=SNew(SVerticalBox);
 FormBody->AddSlot().AutoHeight().Padding(0,14,0,24)[Rule()];
 FormBody->AddSlot().AutoHeight().HAlign(HAlign_Center)[SNew(SBox).WidthOverride(400).HeightOverride(270)[SNew(SImage).Image(&LoginLogoBrush)]];
 FormBody->AddSlot().AutoHeight().Padding(0,22,0,28)[SNew(STextBlock).Text(FText::FromString(TEXT("park visitor survival monitoring system"))).Font(Font(16)).Justification(ETextJustify::Center).ColorAndOpacity(Ink)];
 auto Field=[&](const TCHAR* Name,TAttribute<FText> Value){
  FormBody->AddSlot().AutoHeight().Padding(0,0,0,12)[SNew(SHorizontalBox)
   +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(174)[Label(Name)]]
   +SHorizontalBox::Slot().FillWidth(1)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(2)
    [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Paper).Padding(FMargin(12,8))
     [SNew(SBox).HeightOverride(30)[SNew(STextBlock).Text(Value).Font(Font(21)).ColorAndOpacity(Ink)]]]]];
 };
 Field(TEXT("Login name"),TAttribute<FText>::CreateLambda([this]{return FText::FromString(LoginName());}));
 Field(TEXT("Password"),TAttribute<FText>::CreateLambda([this]{return FText::FromString(LoginPassword());}));
 FormBody->AddSlot().AutoHeight().Padding(0,0,0,20)[SNew(SHorizontalBox)
  +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(174)[Label(TEXT("Workstation"))]]
  +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(104)[SAssignNew(WorkstationField,SEditableTextBox).Style(&WorkstationStyle)
   .Text(FText::FromString(WorkstationText)).Font(Font(21)).Padding(FMargin(12,8)).ForegroundColor(Ink).BackgroundColor(Paper)
   .SelectAllTextWhenFocused(true).IsEnabled_Lambda([this]{return !bLoginTyping;})
   .OnTextChanged_Lambda([this](const FText& Value){WorkstationText=Value.ToString();LoginError.Empty();})]]];
 FormBody->AddSlot().AutoHeight()[SAssignNew(LoginButtonWidget,SButton).ButtonStyle(&LoginButton).ContentPadding(FMargin(14,10))
  .IsEnabled_Lambda([this]{return !bLoginTyping;}).OnClicked_Lambda([this]{StartLogin();return FReply::Handled();})
  [SNew(STextBlock).Text_Lambda([this]{return FText::FromString(bLoginTyping?TEXT("Logging in..."):TEXT("Login"));}).Font(Font(20,true)).Justification(ETextJustify::Center).ColorAndOpacity(Ink)]];
 FormBody->AddSlot().AutoHeight().Padding(0,16,0,0)[SNew(SBox).HeightOverride(26)
  [SNew(STextBlock).Text_Lambda([this]{return FText::FromString(!LoginError.IsEmpty()?LoginError:bLoginTyping?(LoginTime>1.9f?TEXT("Access granted. Opening park control..."):TEXT("Authenticating...")):TEXT("Press Enter to log in"));})
   .Font(Font(14)).Justification(ETextJustify::Center).ColorAndOpacity_Lambda([this,Ink]{return LoginError.IsEmpty()?Ink:FLinearColor(.55,.02,.01);})]];
 Form->AddSlot().AutoHeight()[FormBody];
 auto Scanlines=SNew(SVerticalBox).Visibility(EVisibility::HitTestInvisible);
 for(int I=0;I<150;I++)Scanlines->AddSlot().FillHeight(1).VAlign(VAlign_Bottom)[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(FLinearColor(0,0,0,.018f))]];
 auto AuxiliaryVisibility=[this]{return LoginHUD.IsValid()&&LoginHUD->GetCachedGeometry().GetLocalSize().X>=1380?EVisibility::SelfHitTestInvisible:EVisibility::Collapsed;};
 auto Window=[&](const TCHAR* Title,TSharedRef<SWidget> Body){const FString Key(Title);return SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(FMargin(2,2,6,6))
  [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Gray).Padding(3)[SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(FLinearColor(.35,.42,.47)).Padding(FMargin(4,3))
    [SNew(SHorizontalBox)
     +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Title)).Font(Font(14,true)).ColorAndOpacity(FLinearColor(.95,.95,.88))]
     +SHorizontalBox::Slot().AutoWidth().Padding(6,0,0,0)[Minimize(Key)]]]
   +SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this,Key]{return MinimizedLoginWindows.Contains(Key)?EVisibility::Collapsed:EVisibility::Visible;})
    [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Gray).Padding(10)[Body]]]]];};
 auto Clock=Window(TEXT("xclock"),SNew(SBox).WidthOverride(180).HeightOverride(180)[SNew(SParkDesktopClock)]);
 auto Console=Window(TEXT("Console /dev/console"),SNew(SBox)
  .HeightOverride_Lambda([this]{return FOptionalSize(FMath::Clamp(LoginHUD.IsValid()?float(LoginHUD->GetCachedGeometry().GetLocalSize().Y)-450.f:380.f,240.f,380.f));})
  [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(FLinearColor(.002,.002,.002)).Padding(10)
  [SNew(STextBlock).Text_Lambda([this]{if(!bLocked)return FText::FromString(TEXT("GITRIX 6.5.22 indigo-git tty1\nSilicon Raptors, Inc.\n\nSession: s.chacon\nPark control minimized.\n\nAll systems nominal.\n\nindigo-git:~ $_"));return FText::FromString(TEXT("GITRIX 6.5.22 indigo-git tty1\nCopyright 1991–1997\nSilicon Raptors, Inc.\nAll Rights Reserved.\n\nReconfigure devices [OK]\nMount filesystems   [OK]\nStart services      [OK]\n\nindigo-git login: ")+LoginName()+(LoginTime>1.9f?TEXT("\nStarting session..."):(FMath::Fmod(Elapsed,1.f)<.5f?TEXT("_"):TEXT(" "))));}).Font(Font(14)).ColorAndOpacity(FLinearColor(.82,.84,.8))]]);
 auto Tools=SNew(SVerticalBox);
 for(const TCHAR* Title:{TEXT("Desktop"),TEXT("Selected"),TEXT("Find"),TEXT("System")}) {
  const FString Category(Title);
  Tools->AddSlot().AutoHeight()[SNew(SComboButton).HasDownArrow(false).ButtonStyle(&DesktopButton).ContentPadding(FMargin(6,6))
   .OnGetMenuContent_Lambda([this,Category,Brush,Ink,Gray,Font]{
    auto Items=SNew(SVerticalBox);TArray<FString> Choices;
    if(Category==TEXT("Desktop"))Choices={bLocked?TEXT("Park login"):TEXT("Park control"),TEXT("xclock"),TEXT("Console")};
    else if(Category==TEXT("Find"))Choices={bLocked?TEXT("Workstation"):TEXT("Park control")};
    else if(Category==TEXT("System"))Choices={bLocked?TEXT("Login"):TEXT("Logout"),TEXT("Exit")};
    else Choices={bLocked?TEXT("Park login"):TEXT("Park control")};
    for(const FString& Choice:Choices)Items->AddSlot().AutoHeight()[SNew(SButton).ButtonStyle(&DesktopButton).ContentPadding(FMargin(14,8))
     .OnClicked_Lambda([this,Choice]{
      FSlateApplication::Get().DismissAllMenus();
      if(Choice==TEXT("Exit"))UKismetSystemLibrary::QuitGame(this,GetWorld()->GetFirstPlayerController(),EQuitPreference::Quit,false);
      else if(Choice==TEXT("Login"))StartLogin();
      else if(Choice==TEXT("Logout"))LogoutToLogin();
      else if(Choice==TEXT("Park control"))RestoreParkWindow();
      else {
       const FString Key=Choice==TEXT("Park login")||Choice==TEXT("Workstation")?TEXT("Login"):Choice==TEXT("Console")?TEXT("Console /dev/console"):Choice;
       MinimizedLoginWindows.Remove(Key);
       if(Choice==TEXT("Workstation"))FSlateApplication::Get().SetKeyboardFocus(WorkstationField,EFocusCause::SetDirectly);
       else FSlateApplication::Get().SetKeyboardFocus(LoginHUD,EFocusCause::SetDirectly);
      }
      return FReply::Handled();
     })[SNew(STextBlock).Text(FText::FromString(Choice)).Font(Font(16)).ColorAndOpacity(Ink)]];
    return SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(2)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Gray).Padding(3)[Items]];
   }).ButtonContent()[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().FillWidth(1)[Label(Title,17)]
    +SHorizontalBox::Slot().AutoWidth()[Label(TEXT("›"),17)]]];
 }
 auto Toolchest=Window(TEXT("Toolchest"),Tools);
 auto Root=SNew(SParkLoginRoot).Login(FSimpleDelegate::CreateLambda([this]{StartLogin();}))
  .Locked_Lambda([this]{return bLocked;})
  .PresentationKey(FOnKeyDown::CreateLambda([this](const FGeometry&,const FKeyEvent& Event){if(!Event.IsRepeat())HandleParkKey(Event.GetKey());return FReply::Handled();}));
 Root->SetContent(SNew(SOverlay)
  +SOverlay::Slot()[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Teal).Padding(0)]
  +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(24,30)[SNew(SBox).WidthOverride(256).Visibility_Lambda(AuxiliaryVisibility)[Clock]]
  +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(24,0,24,78)[SNew(SBox)
   .WidthOverride_Lambda([this]{return FOptionalSize(FMath::Clamp(LoginHUD.IsValid()?(float(LoginHUD->GetCachedGeometry().GetLocalSize().X)-660.f)*.5f-48.f:340.f,312.f,440.f));})
   .Visibility_Lambda(AuxiliaryVisibility)[Console]]
  +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24,30)[SNew(SBox).WidthOverride(220).Visibility_Lambda(AuxiliaryVisibility)[Toolchest]]
  +SOverlay::Slot().VAlign(VAlign_Bottom)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(2)
   [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Gray).Padding(FMargin(18,8))[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().FillWidth(1)[Label(TEXT("GITRIS Indigo-git  /  Park control"),15)]
    +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text_Lambda([]{return FText::FromString(TEXT("Desk 1    ")+FDateTime::Now().ToString(TEXT("%H:%M")));}).Font(Font(15)).ColorAndOpacity(Ink)]]]]
  +SOverlay::Slot().Padding(24,24,24,64)[SNew(SScaleBox).Visibility_Lambda([this]{return bLocked?EVisibility::Visible:EVisibility::Collapsed;}).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
   [SNew(SBox).WidthOverride(660)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(FMargin(3,3,10,10))
    [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Gray).Padding(38)[Form]]]]]
  +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(24,38,24,80)[SNew(SBox)
   .WidthOverride_Lambda([this]{const float W=LoginHUD.IsValid()?LoginHUD->GetCachedGeometry().GetLocalSize().X:1600;return FOptionalSize((bDesktopFSV||bQuestions)?W*.84f:(W>=1380?W-620:W*.88f));})
   .HeightOverride_Lambda([this]{return FOptionalSize(FMath::Max(240.f,float(LoginHUD.IsValid()?LoginHUD->GetCachedGeometry().GetLocalSize().Y:900)-180.f));})
   .Visibility_Lambda([this]{return !bLocked?EVisibility::Visible:EVisibility::Collapsed;})[SNew(SOverlay)
    +SOverlay::Slot()[SNew(SBox).Visibility_Lambda([this]{return bDesktopFSV?EVisibility::Collapsed:EVisibility::Visible;})[BuildCommandTerminal()]]
    +SOverlay::Slot()[SNew(SBox).Visibility_Lambda([this]{return bDesktopFSV?EVisibility::Visible:EVisibility::Collapsed;})[BuildFSVWindow()]]]]
  +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(24,0,24,80)[SNew(SBox).WidthOverride(192).Visibility_Lambda([this]{return !bLocked?EVisibility::Visible:EVisibility::Collapsed;})[BuildParkDock()]]
  +SOverlay::Slot()[Scanlines]
  +SOverlay::Slot()[BuildMinimizeAnimation()]
  +SOverlay::Slot()[BuildFinishPrompt()]);
 LoginHUD=Root;
 GetWorld()->GetGameViewport()->AddViewportWidgetContent(LoginHUD.ToSharedRef(),100);
}

FString ASlideGameMode::LoginName() const {
 return bLoginTyping?FString(TEXT("s.chacon")).Left(FMath::Clamp(FMath::FloorToInt((LoginTime-.15f)/.085f),0,8)):TEXT("");
}
FString ASlideGameMode::LoginPassword() const {
 return bLoginTyping?FString::ChrN(FMath::Clamp(FMath::FloorToInt((LoginTime-1.f)/.085f),0,8),TEXT('*')):TEXT("");
}
void ASlideGameMode::StartLogin() {
 if(!bLocked||bLoginTyping)return;
 MinimizedLoginWindows.Remove(TEXT("Login"));
 const FString Value=WorkstationText.TrimStartAndEnd();
 bool Digits=!Value.IsEmpty()&&Value.Len()<=3;
 for(TCHAR C:Value)Digits&=C>=TEXT('0')&&C<=TEXT('9');
 const int32 Minutes=Digits?FCString::Atoi(*Value):0;
 if(Minutes<6||Minutes>180){LoginError=TEXT("Use a workstation number from 6 to 180.");return;}
 WorkstationMinutes=Minutes;WorkstationText=FString::FromInt(Minutes);
 WorkstationField->SetText(FText::FromString(WorkstationText));
 LoginError.Empty();LoginTime=0;bLoginTyping=true;
 FSlateApplication::Get().SetKeyboardFocus(LoginHUD,EFocusCause::SetDirectly);
}
void ASlideGameMode::ResetPresentationSession() {
 bConfirmFinish=false;bQuestions=false;bDesktopFSV=false;
 bCommandDesktop=false;DesktopMinimize=0;DesktopCommandKey=MAX_uint64;
 ResetStationPage();ViewedColdSpecimens.Empty();ViewedRaptors.Empty();ViewedFSVSystems.Empty();ViewedPages.Empty();VisitedStations.Empty();bEditingTime=false;TimeError.Empty();
 TimerElapsed=0;TimerDuration=WorkstationMinutes*60.f;bTimerStarted=false;bTimerPaused=false;bTourStarted=false;
 FRandomStream Random(FMath::Rand());
 VisitorDepartures.Build((WorkstationMinutes-5)*100,100,50,Random);
 StaffDepartures.Build(WorkstationMinutes*10,10,10,Random);
 PendingIndex=-1;Index=0;bFreeFlight=false;bFreeTravelling=false;bFlying=false;bOverview=true;
 FreeDrag=FVector2D::ZeroVector;Travel=0;CardExpansion=0;MapPhase=EMapPhase::Overview;
 CameraLook=MapCenter;CameraFrameWidth=OverviewFrameWidth;
 Camera->SetActorLocationAndRotation(MapEye,(MapCenter-MapEye).Rotation());
 Cast<AParkCamera>(Camera)->FrameScene(CameraFrameWidth,FVector::Distance(MapEye,MapCenter));
 for(auto& Entry:PageTerminals)Entry.Value->Update(0,Camera,false);
 IslandScene::TickGate(1,false);
 for(auto& Panel:Panels) {
  Panel.Reveal=0;
  if(Panel.Root.IsValid())Panel.Root->GetRootComponent()->SetVisibility(false,true);
  for(auto& Model:Panel.Models)if(Model.IsValid())Model->SetActorHiddenInGame(true);
 }
 for(auto& Base:SignBases)if(Base.IsValid())Base->SetActorHiddenInGame(false);
}
void ASlideGameMode::LogoutToLogin() {
 if(!LoginHUD.IsValid())return;
 MinimizedLoginWindows.Empty();
 bLocked=true;bLoginTyping=false;LoginTime=0;LockTime=0;LoginError.Empty();ResetPresentationSession();
 DesktopHUD->SetVisibility(EVisibility::Collapsed);LoginHUD->SetVisibility(EVisibility::Visible);
 FSlateApplication::Get().DismissAllMenus();
 auto* PC=GetWorld()->GetFirstPlayerController();PC->bShowMouseCursor=true;PC->FlushPressedKeys();
 FInputModeUIOnly Mode;Mode.SetWidgetToFocus(LoginHUD);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Mode);
}
void ASlideGameMode::FinishLogin() {
 ResetPresentationSession();bLocked=false;bLoginTyping=false;
 LoginHUD->SetVisibility(EVisibility::Collapsed);DesktopHUD->SetVisibility(EVisibility::Visible);
 GetWorld()->GetFirstPlayerController()->FlushPressedKeys();SetMouseMode(false);
 FSlateApplication::Get().SetAllUserFocusToGameViewport();
 // Existing navigation checks measure time from entry to the map.
 if(FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest"))&&!FParse::Param(FCommandLine::Get(),TEXT("LoginTest")))Elapsed=0;
}
void ASlideGameMode::TickLogin(float Delta) {
 LockTime+=Delta;
 if(bLoginTyping){LoginTime+=Delta;if(LoginTime>=2.35f)FinishLogin();}
 if(FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest"))) {
  if(FParse::Param(FCommandLine::Get(),TEXT("LoginTest")))TestLogin();
  else if(!bLoginTyping&&bLocked&&LockTime>.5f)StartLogin();
 }
}
