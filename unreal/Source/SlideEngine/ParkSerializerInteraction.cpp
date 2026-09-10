#include "SlideGameMode.h"
#include "ParkSerializer.h"
#include "Widgets/SWidget.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
void ASlideGameMode::TestSerializer() {
 auto Check=[](bool OK,const TCHAR* What){UE_LOG(LogTemp,Display,TEXT("SerializerTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);return OK;};
 auto Click=[this](float X,float Y){const auto& G=SerializerView->GetCachedGeometry();const FVector2D P=G.LocalToAbsolute(FVector2D(X,Y));SerializerView->OnMouseButtonDown(G,FPointerEvent(0,P,P,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState()));};
 auto ClickMenu=[&](const FString& Wanted){float X=768;const FSlateFontInfo Font(FPaths::ProjectContentDir()/TEXT("Fonts/RobotoMono-Regular.ttf"),21);const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();for(const TCHAR* Label:{TEXT("File"),TEXT("View"),TEXT("Navigate"),TEXT("Help"),TEXT("Expand"),TEXT("Fold")}){const float Width=Measure->Measure(Label,Font).X;if(Wanted==Label){Click(X+Width/2,70);return;}X+=Width+34;}};
 auto Capture=[this](const TCHAR* Name){FTimerHandle H;const auto Path=FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name;GetWorld()->GetTimerManager().SetTimer(H,[Path]{FScreenshotRequest::RequestScreenshot(Path,true,false);},.35f,false);};
 auto HasHidden=[this]{return Serializer->Nodes.ContainsByPredicate([this](const FSerializerNode& N){return N.Rows.ContainsByPredicate([this](int32 R){return Serializer->Rows[R].Hidden;});});};
 if(SmokeStep==0&&Elapsed>1){GoTo(4);SmokeStep=1;}
 else if(SmokeStep==1&&MapPhase==EMapPhase::Slide){HandleParkKey(EKeys::Right);SmokeStep=2;}
 else if(SmokeStep==2&&DesktopMinimize==1&&Serializer.IsValid()){
  if(!Check(bDesktopSerializer&&!bDesktopFSV&&!Serializer->Started&&Serializer->VisibleNodes().IsEmpty()&&Serializer->TableRows().Num()==7&&Serializer->Committed==0,TEXT("desktop opens seven values and an empty Git explorer")))return;
  Capture(TEXT("serializer-empty.png"));Travel=0;SmokeStep=3;
 } else if(SmokeStep==3&&Travel>.6f){Click(360,650);Travel=0;SmokeStep=4;}
 else if(SmokeStep==4&&Travel>.4f){if(!Check(Serializer->Written>0&&!Serializer->Ready()&&!Serializer->VisibleNodes().IsEmpty(),TEXT("Serialize writes the first tree progressively")))return;SmokeStep=5;}
 else if(SmokeStep==5&&Serializer->Ready()){
  if(!Check(Serializer->Committed==1&&!HasHidden(),TEXT("first commit contains only initially visible metadata")))return;
  Click(520,267);
  if(!Check(Serializer->SelectedRow==0&&Serializer->SelectedNode>=0&&Serializer->Nodes[Serializer->SelectedNode].Value==TEXT("approved, later merged"),TEXT("clicking a table value locates its serialized blob")))return;
  Capture(TEXT("serializer-selected.png"));Travel=0;SmokeStep=6;
 } else if(SmokeStep==6&&Travel>.7f){
  ClickMenu(TEXT("Fold"));if(!Check(Serializer->VisibleNodes().Num()==1&&Serializer->SelectedRow==0,TEXT("Fold closes all paths and keeps inspected metadata")))return;
  Travel=0;SmokeStep=60;
 } else if(SmokeStep==60&&Travel>.2f){
  ClickMenu(TEXT("Expand"));if(!Check(Serializer->VisibleNodes().Num()==Serializer->Nodes.Num(),TEXT("Expand opens every path")))return;
  Travel=0;SmokeStep=61;
 } else if(SmokeStep==61&&Travel>.2f){
  Click(520,331);
  bool Focused=Serializer->SelectedRow==2&&Serializer->VisibleNodes().Contains(Serializer->SelectedNode);
  for(const TCHAR* Path:{TEXT("branch"),TEXT("path"),TEXT("project"),TEXT("commit/5a")}){const int32 N=Serializer->Nodes.IndexOfByPredicate([Path](const FSerializerNode& Node){return Node.Path==Path;});Focused&=N>=0&&!Serializer->Expanded.Contains(N);}
  if(!Check(Focused,TEXT("SQLite selection opens its path and folds unrelated branches")))return;
  Capture(TEXT("serializer-focused.png"));Travel=0;SmokeStep=62;
 } else if(SmokeStep==62&&Travel>.7f){
  Click(570,590);if(!Check(Serializer->AddedMetadata&&Serializer->TableRows().Num()==10&&Serializer->Committed==1&&!HasHidden(),TEXT("Add metadata reveals rows without changing the first commit")))return;
  Travel=0;SmokeStep=7;
 } else if(SmokeStep==7&&Travel>.2f){Click(360,650);if(!Check(Serializer->Serializing&&Serializer->SelectedCommit==1,TEXT("Serialize creates a second snapshot")))return;SmokeStep=8;}
 else if(SmokeStep==8&&Serializer->Ready()){
  if(!Check(Serializer->Committed==2&&HasHidden()&&!Serializer->CanSerialize(),TEXT("second commit retains original values and adds hidden metadata")))return;
  Click(520,493);
  if(!Check(Serializer->SelectedRow==7&&Serializer->IsNewNode(Serializer->SelectedNode)&&Serializer->Nodes[Serializer->SelectedNode].Value==TEXT("rust"),TEXT("new values are green and can be inspected")))return;
  Capture(TEXT("serializer-all-values.png"));Travel=0;SmokeStep=9;
 } else if(SmokeStep==9&&Travel>.7f){
  Click(850,290);if(!Check(Serializer->SelectedCommit==0&&Serializer->TableRows().Num()==7&&!HasHidden(),TEXT("selecting first commit restores its original table and tree")))return;
  Travel=0;SmokeStep=10;
 } else if(SmokeStep==10&&Travel>.2f){
  Click(850,180);if(!Check(Serializer->SelectedCommit==1&&Serializer->TableRows().Num()==10&&HasHidden(),TEXT("selecting second commit restores all ten values")))return;
  Travel=0;SmokeStep=11;
 } else if(SmokeStep==11&&Travel>.2f){
  Click(995,147);if(!Check(!Serializer->Expanded.Contains(0)&&Serializer->VisibleNodes().Num()==1,TEXT("tree disclosure collapses a folder")))return;
  Travel=0;SmokeStep=12;
 } else if(SmokeStep==12&&Travel>.2f){
  Click(995,147);Capture(TEXT("serializer-commits.png"));Travel=0;SmokeStep=13;
 } else if(SmokeStep==13&&Travel>.7f){
  const int32 Readme=Serializer->Nodes.IndexOfByPredicate([](const FSerializerNode& N){return N.Path==TEXT("README.md");});
  if(!Check(Readme>=0,TEXT("multiline README fixture is available")))return;
  Serializer->SelectNode(Readme);Capture(TEXT("serializer-readme.png"));Travel=0;SmokeStep=130;
 } else if(SmokeStep==130&&Travel>.7f){
  const auto& G=SerializerView->GetCachedGeometry();const FVector2D P=G.LocalToAbsolute(FVector2D(1200,905));
  SerializerView->OnMouseWheel(G,FPointerEvent(0,P,P,TSet<FKey>{},FKey(),-3,FModifierKeysState()));
  if(!Check(Serializer->ValueScroll>0,TEXT("multiline values scroll inside their pane")))return;
  Capture(TEXT("serializer-readme-scrolled.png"));Travel=0;SmokeStep=131;
 } else if(SmokeStep==131&&Travel>.7f){
  Click(520,267);if(!Check(Serializer->ValueScroll==0,TEXT("selecting another value resets the pane scroll")))return;
  HandleParkKey(EKeys::Right);SmokeStep=14;
 }
 else if(SmokeStep==14&&DesktopMinimize==0){
  if(!Check(PageIndex==2&&!bCommandDesktop,TEXT("next slide restores the park")))return;
  HandleParkKey(EKeys::Left);SmokeStep=15;
 } else if(SmokeStep==15&&DesktopMinimize==1){
  if(!Check(Serializer->Ready()&&Serializer->Committed==2,TEXT("returning to Serializer preserves both commits")))return;
  LogoutToLogin();if(!Check(bLocked&&!Serializer.IsValid()&&PageSerializers.IsEmpty(),TEXT("logout clears serializer state")))return;
  UE_LOG(LogTemp,Display,TEXT("SerializerTest: PASS"));FPlatformMisc::RequestExitWithStatus(false,0);
 }
}
