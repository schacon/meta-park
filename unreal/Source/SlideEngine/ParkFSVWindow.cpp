#include "SlideGameMode.h"
#include "ParkFSV.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"
#include "Brushes/SlateColorBrush.h"
TSharedRef<SWidget> ASlideGameMode::BuildFSVWindow() {
 static const FButtonStyle Button=FButtonStyle().SetNormal(FSlateColorBrush(FLinearColor(.66,.66,.63))).SetHovered(FSlateColorBrush(FLinearColor(.84,.84,.79))).SetPressed(FSlateColorBrush(FLinearColor(.46,.48,.43)));
 const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");
 const FLinearColor Gray(.69,.69,.66),Ink(.015,.02,.025);
 auto Font=[](int32 Size){return FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("Fonts/RobotoMono-Regular.ttf"),Size);};
 auto Label=[&](const TCHAR* Text,int32 Size=16){return SNew(STextBlock).Text(FText::FromString(Text)).Font(Font(Size)).ColorAndOpacity(Ink);};
 return SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Ink).Padding(FMargin(2,2,7,7))
 [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(Gray).Padding(3)
  [SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(FLinearColor(.39,.40,.30)).Padding(FMargin(12,8))
    [SNew(SHorizontalBox)
     +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(FSV.IsValid()?FSV->Title:TEXT("Filesystem viewer"));}).Font(Font(18)).ColorAndOpacity(FLinearColor::White)]
     +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(&Button).IsFocusable(false).ContentPadding(FMargin(8,0)).OnClicked_Lambda([this]{RestoreParkWindow();return FReply::Handled();})[Label(TEXT("−"))]]]]
   +SVerticalBox::Slot().AutoHeight().Padding(10,5)[Label(TEXT("File   Vis   Colors                                        Help"))]
   +SVerticalBox::Slot().FillHeight(1)[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(190)
     [SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(FLinearColor(.8,.8,.76)).Padding(12)
      [SNew(SVerticalBox)
       +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)[SNew(SButton).ButtonStyle(&Button).IsFocusable(false).OnClicked_Lambda([this]{if(FSV.IsValid())FSV->Select(INDEX_NONE);return FReply::Handled();})[Label(TEXT("/systems"))]]
       +SVerticalBox::Slot().FillHeight(1)[SNew(STextBlock).Text_Lambda([this]{FString Text;if(FSV.IsValid())for(int32 I=0;I<FSV->Systems.Num();I++)Text+=(I==FSV->Selected?TEXT("> "):TEXT("  "))+FSV->Systems[I].Label+TEXT("\n\n");return FText::FromString(Text);}).Font(Font(15)).ColorAndOpacity(Ink).AutoWrapText(true)]
      ]]]
    +SHorizontalBox::Slot().FillWidth(1)[SNew(SBorder).BorderImage(Brush).BorderBackgroundColor(FLinearColor(.003,.003,.007)).Padding(1)
     [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(900).HeightOverride(620)[MakeParkFSVView([this]{return FSV;})]]]]]
   +SVerticalBox::Slot().AutoHeight().Padding(10,6)[SNew(SHorizontalBox)
    +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(FSV.IsValid()?FString::Printf(TEXT("%d systems    /systems%s%s"),FSV->Systems.Num(),FSV->Selected>=0?TEXT("/"):TEXT(""),FSV->Selected>=0?*FSV->Systems[FSV->Selected].Label:TEXT("")):TEXT(""));}).Font(Font(14)).ColorAndOpacity(Ink)]
    +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(&Button).IsFocusable(false).OnClicked_Lambda([this]{HandleParkKey(EKeys::Left);return FReply::Handled();})[Label(TEXT("‹"))]]
    +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(&Button).IsFocusable(false).OnClicked_Lambda([this]{HandleParkKey(EKeys::Right);return FReply::Handled();})[Label(TEXT("›"))]]]
   +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(12,3,12,9)
    [SNew(STextBlock).Text(FText::FromString(TEXT("it's UNIX, I know this..."))).Font(Font(16)).ColorAndOpacity(FLinearColor(.025,.22,.055))]
  ]];
}
