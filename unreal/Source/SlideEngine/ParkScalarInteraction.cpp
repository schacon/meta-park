#include "SlideGameMode.h"
#include "ParkScalar.h"
#include "ParkScalarScene.h"
#include "ParkSpeedGraph.h"
#include "Widgets/SWidget.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"
#include "Engine/World.h"
#include "TimerManager.h"
void ASlideGameMode::TestScalar() {
 auto Check=[](bool OK,const TCHAR* What){UE_LOG(LogTemp,Display,TEXT("ScalarTest: %s=%s"),What,OK?TEXT("PASS"):TEXT("FAIL"));if(!OK)FPlatformMisc::RequestExitWithStatus(false,1);return OK;};
 auto Pointer=[this](FVector2D At,FKey Key=EKeys::LeftMouseButton,float Wheel=0,bool Shift=false){const auto P=ScalarView->GetCachedGeometry().LocalToAbsolute(At);return FPointerEvent(0,P,P,TSet<FKey>{Key},Key,Wheel,FModifierKeysState(Shift,false,false,false,false,false,false,false,false));};
 auto Click=[&](float X,float Y){ScalarView->OnMouseButtonDown(ScalarView->GetCachedGeometry(),Pointer(FVector2D(X,Y)));};
 auto Capture=[this](const TCHAR* Name){FTimerHandle H;const auto Path=FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name;GetWorld()->GetTimerManager().SetTimer(H,[Path]{FScreenshotRequest::RequestScreenshot(Path,true,false);},.15f,false);};
 auto Swipe=[&](FVector2D At,FVector2D Delta,EGestureEvent Gesture=EGestureEvent::Scroll){const auto& G=ScalarView->GetCachedGeometry();const FVector2D P=G.LocalToAbsolute(At);return ScalarView->OnTouchGesture(G,FPointerEvent(P,P,TSet<FKey>{},FModifierKeysState(),Gesture,EGesturePhase::Update,Delta,true));};
 auto CheckCreation=[&](int32 Step){
  const float SavedAge=Scalar->Age;
  const FString Commit=Scalar->Commits[Step].Id,Tree=Scalar->Commits[Step].Tree;
  const FScalarSpace Previous=Step>0?Scalar->Steps[Step-1].Spaces[0]:FScalarSpace();
  TArray<int32> NewBlobs;for(int32 B:Scalar->Steps[Step].Spaces[0].Blobs)if(!Previous.Blobs.Contains(B))NewBlobs.Add(B);
  const float Times[]={0.f,.6f,1.5f,2.5f,3.2f,3.8f};
  bool OK=true;
  for(int32 I=0;I<6;I++) {
   Scalar->Age=Times[I];Scalar->Scene->Update(*Scalar,0);
   OK&=Scalar->Scene->CreationObjectVisible(Commit)==(I>=1);
   OK&=Scalar->Scene->CreationObjectVisible(Tree)==(I>=2);
   for(int32 B=0;B<5;B++){const int32 NewIndex=NewBlobs.Find(B);OK&=Scalar->Scene->CreationObjectVisible(FString::Chr('A'+B))==(NewIndex>=0&&I>=3+NewIndex);}
  }
  OK&=Scalar->Scene->CreationStatus.IsEmpty();
  Scalar->Age=SavedAge;Scalar->Scene->Update(*Scalar,0);
  return Check(OK,Step!=2?TEXT("commit, tree, and only new blobs appear in order"):TEXT("prune creates commit and tree without recreating blobs"));
 };
 if(SmokeStep==0&&Elapsed>1){GoTo(6);SmokeStep=1;}
 else if(SmokeStep==1&&MapPhase==EMapPhase::Slide){HandleParkKey(EKeys::Right);SmokeStep=2;}
 else if(SmokeStep==2&&DesktopMinimize==1&&Scalar.IsValid()&&Scalar->Age>.8f){
  if(FParse::Param(FCommandLine::Get(),TEXT("ScalarImageOnly"))||FParse::Param(FCommandLine::Get(),TEXT("ScalarEndingOnly"))){Scalar->Select(Scalar->Steps.Num()-1);HandleParkKey(EKeys::Right);Travel=0;SmokeStep=21;return;}
  if(!Check(bDesktopScalar&&Scalar->Scene->IsValid()&&Scalar->Scene->SolidBoxes==4&&Scalar->Scene->TopLabels==4&&Scalar->Scene->LabelsFit,TEXT("native boxes carry fitted labels on their top planes")))return;
  if(!CheckCreation(0))return;
  const int32 SavedStep=Scalar->Selected;const float SavedAge=Scalar->Age;bool Typed=true;
  for(int32 I=0;I<Scalar->Steps.Num();I++){
   Scalar->Selected=I;Scalar->Age=1;const FString Full=Scalar->VisibleCommands();
   Scalar->Age=0;Typed&=Scalar->VisibleCommands().IsEmpty();
   Scalar->Age=.5f;Typed&=Scalar->VisibleCommands()==Full.Left(Full.Len()/2);
   Scalar->Age=2;Typed&=Scalar->VisibleCommands()==Full;
  }
  Scalar->Selected=SavedStep;Scalar->Age=SavedAge;
  if(!Check(Typed,TEXT("each stage types its complete command block over one second")))return;
  Scalar->Reset();Scalar->Scene->Update(*Scalar,0);
  if(!Check(!Scalar->Scene->CreationObjectVisible(TEXT("C1")),TEXT("reset replays creation even on the same step")))return;
  SmokeStep=201;
 } else if(SmokeStep==201&&Scalar->Age>.55f){Capture(TEXT("scalar-3d-write-commit.png"));SmokeStep=202;
 } else if(SmokeStep==202&&Scalar->Age>1.5f){Capture(TEXT("scalar-3d-write-tree.png"));SmokeStep=203;
 } else if(SmokeStep==203&&Scalar->Age>2.5f){Capture(TEXT("scalar-3d-write-blob.png"));SmokeStep=204;
 } else if(SmokeStep==204&&Scalar->Age>3.8f){Capture(TEXT("scalar-3d-first.png"));Travel=0;SmokeStep=205;
 } else if(SmokeStep==205&&Travel>.4f){
  const float Yaw=Scalar->OrbitYaw,Tilt=Scalar->Tilt;const auto& G=ScalarView->GetCachedGeometry();
  ScalarView->OnMouseButtonDown(G,Pointer(FVector2D(1150,590),EKeys::LeftMouseButton,0,true));
  for(int32 I=1;I<=40;I++){const float Angle=I*2*PI/40;ScalarView->OnMouseMove(G,Pointer(FVector2D(970+180*FMath::Cos(Angle),590+110*FMath::Sin(Angle)),EKeys::LeftMouseButton,0,true));}
  ScalarView->OnMouseButtonUp(G,Pointer(FVector2D(1150,590),EKeys::LeftMouseButton,0,true));
  if(!Check(Scalar->Highlights.Num()==1&&Scalar->Highlights[0].Points.Num()>30&&Scalar->Highlights[0].Released&&Scalar->OrbitYaw==Yaw&&Scalar->Tilt==Tilt,TEXT("Shift-drag draws a freehand outline without moving the camera")))return;
  Capture(TEXT("scalar-3d-highlight.png"));Travel=0;SmokeStep=206;
 } else if(SmokeStep==206&&Travel>1){
  if(!Check(Scalar->Highlights.Num()==1&&Scalar->Highlights[0].Age>.25f&&Scalar->Highlights[0].Age<2,TEXT("released outline fades gradually")))return;
  const auto& G=ScalarView->GetCachedGeometry();const float Yaw=Scalar->OrbitYaw;
  for(int32 Stroke=0;Stroke<2;Stroke++) {
   const auto Down=Pointer(FVector2D(1150,590),EKeys::LeftMouseButton,0,true);
   const auto Reply=Stroke==0?ScalarView->OnMouseButtonDoubleClick(G,Down):ScalarView->OnMouseButtonDown(G,Down);
   if(!Check(Reply.IsEventHandled(),TEXT("repeat and double-click presses start new strokes")))return;
   for(int32 I=1;I<=40;I++){const float Angle=I*2*PI/40;ScalarView->OnMouseMove(G,Pointer(FVector2D(970+180*FMath::Cos(Angle),590+(85-30*Stroke)*FMath::Sin(Angle)),EKeys::LeftMouseButton,0,true));}
   ScalarView->OnMouseButtonUp(G,Pointer(FVector2D(1150,590),EKeys::LeftMouseButton,0,true));
  }
  if(!Check(Scalar->Highlights.Num()==3&&Scalar->Highlights[0].ColorIndex!=Scalar->Highlights[1].ColorIndex&&Scalar->Highlights[1].ColorIndex!=Scalar->Highlights[2].ColorIndex&&Scalar->Highlights[1].Points.Num()>30&&Scalar->Highlights[2].Points.Num()>30&&Scalar->OrbitYaw==Yaw,TEXT("overlapping strokes keep separate paths and cycle colors without orbiting")))return;
  Capture(TEXT("scalar-3d-highlight-overlapping.png"));SmokeStep=207;
 } else if(SmokeStep==207&&Travel>2.3f){
  if(!Check(Scalar->Highlights.Num()==2&&Scalar->Highlights[0].ColorIndex==1&&Scalar->Highlights[1].ColorIndex==2,TEXT("older outline disappears while newer outlines keep fading")))return;
  Capture(TEXT("scalar-3d-highlight-fading.png"));SmokeStep=208;
 } else if(SmokeStep==208&&Travel>3.3f){
  if(!Check(Scalar->Highlights.IsEmpty(),TEXT("all overlapping outlines expire")))return;
  const auto& G=ScalarView->GetCachedGeometry();ScalarView->OnMouseButtonDown(G,Pointer(FVector2D(900,550),EKeys::LeftMouseButton,0,true));ScalarView->OnMouseMove(G,Pointer(FVector2D(1100,600),EKeys::LeftMouseButton,0,true));ScalarView->OnMouseButtonUp(G,Pointer(FVector2D(1100,600),EKeys::LeftMouseButton,0,true));
  if(!Check(Scalar->Highlights.Num()==1&&Scalar->Highlights[0].ColorIndex==3&&Scalar->Highlights[0].Points.Num()>=2,TEXT("highlighting restarts after all previous strokes fade, with the next color")))return;
  Travel=0;SmokeStep=209;
 } else if(SmokeStep==209&&Travel>2.3f){if(!Check(Scalar->Highlights.IsEmpty(),TEXT("later strokes also finish fading")))return;Travel=0;SmokeStep=3;
 } else if(SmokeStep==3&&Travel>.6f){
  const float Yaw=Scalar->OrbitYaw;Click(950,500);ScalarView->OnMouseMove(ScalarView->GetCachedGeometry(),Pointer(FVector2D(1080,540)));ScalarView->OnMouseButtonUp(ScalarView->GetCachedGeometry(),Pointer(FVector2D(1080,540)));
  if(!Check(FMath::Abs(Scalar->OrbitYaw-Yaw)>30,TEXT("mouse drag orbits the native camera")))return;
  const float Zoom=Scalar->Height;ScalarView->OnMouseWheel(ScalarView->GetCachedGeometry(),Pointer(FVector2D(950,500),FKey(),1));if(!Check(Scalar->Height>Zoom,TEXT("wheel zooms the scene")))return;
  const float PinchZoom=Scalar->Height;const FVector PinchPan=Scalar->PanOffset;
  if(!Check(Swipe(FVector2D(950,500),FVector2D(.15,0),EGestureEvent::Magnify).IsEventHandled()&&Scalar->Height>PinchZoom&&Scalar->PanOffset.Equals(PinchPan),TEXT("spreading two fingers zooms in without panning")))return;
  const float ZoomedIn=Scalar->Height;
  if(!Check(Swipe(FVector2D(950,500),FVector2D(-.15,0),EGestureEvent::Magnify).IsEventHandled()&&Scalar->Height<ZoomedIn&&FMath::IsNearlyEqual(Scalar->Height,PinchZoom),TEXT("pinching two fingers together zooms out")))return;
  const FVector VerticalStart=Scalar->PanOffset;Swipe(FVector2D(950,500),FVector2D(0,20));
  if(!Check(Scalar->PanOffset.Z>VerticalStart.Z,TEXT("vertical two-finger panning uses the inverted direction")))return;
  const float SwipeYaw=Scalar->OrbitYaw,SwipeZoom=Scalar->Height;const FVector BeforePan=Scalar->PanOffset;
  const auto SwipeReply=Swipe(FVector2D(950,235),FVector2D(30,-20));
  if(!Check(SwipeReply.IsEventHandled()&&!Scalar->PanOffset.Equals(BeforePan)&&Scalar->OrbitYaw==SwipeYaw&&Scalar->Height==SwipeZoom,TEXT("two-finger swipe pans the enlarged viewport without orbiting or zooming")))return;
  const FVector AfterPan=Scalar->PanOffset;
  if(!Check(!Swipe(FVector2D(950,100),FVector2D(30,20)).IsEventHandled()&&Scalar->PanOffset.Equals(AfterPan),TEXT("trackpad gestures over commands do not pan")))return;
  Travel=0;SmokeStep=4;
 } else if(SmokeStep==4&&Travel>.7f){Capture(TEXT("scalar-3d-orbit.png"));Click(760,52);if(!Check(!Scalar->MovingCamera&&Scalar->PanOffset.IsZero(),TEXT("camera button selects full scene and recenters")))return;Travel=0;SmokeStep=5;}
 else if(SmokeStep==5&&Travel>1){if(!Check(Scalar->Scene->Focus.Equals(FVector(0,260,60),20),TEXT("full scene frames all four spaces")))return;Swipe(FVector2D(950,500),FVector2D(25,15));if(!Check(!Scalar->PanOffset.IsZero(),TEXT("trackpad also pans in full scene mode")))return;Capture(TEXT("scalar-3d-full.png"));Travel=0;SmokeStep=6;}
 else if(SmokeStep==6&&Travel>.4f){Click(100,205);Travel=0;SmokeStep=60;}
 else if(SmokeStep==60&&Travel>.6f){if(!Check(!Scalar->MovingCamera&&Scalar->Scene->DesiredFocus.Equals(FVector(0,260,60)+Scalar->PanOffset,.01)&&Scalar->Scene->DesiredDistance>4000,TEXT("full scene camera stays put across steps")))return;HandleParkKey(EKeys::C);Travel=0;SmokeStep=7;}
 else if(SmokeStep==7&&Travel>3.4f){if(!Check(Scalar->Selected==1&&Scalar->MovingCamera&&Scalar->Scene->DesiredFocus.X<-1000&&Scalar->Scene->LabelsFit,TEXT("next step focuses the action and fits four-entry tree")))return;if(!CheckCreation(1))return;Capture(TEXT("scalar-3d-add.png"));Travel=0;SmokeStep=8;}
 else if(SmokeStep==8&&Travel>.4f){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=9;}
 else if(SmokeStep==9&&Travel>2.2f){if(!Check(Scalar->Selected==2&&Scalar->Scene->SolidBoxes==10,TEXT("prune retains all historical objects")))return;if(!CheckCreation(2))return;Capture(TEXT("scalar-3d-prune.png"));Travel=0;SmokeStep=95;}
 else if(SmokeStep==95&&Travel>.4f){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=96;}
 else if(SmokeStep==96&&Travel>3){if(!Check(Scalar->Selected==3&&Scalar->Scene->SolidBoxes==13&&Scalar->Commits[3].Entries==TArray<int32>{2,3,4},TEXT("C4 adds E and leaves three values at the tip")))return;if(!CheckCreation(3))return;Capture(TEXT("scalar-3d-fourth-commit.png"));Travel=0;SmokeStep=10;}
 else if(SmokeStep==10&&Travel>.4f){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=11;}
 else if(SmokeStep==11&&Travel>.8f){if(!Check(Scalar->Scene->TransferringObjects==13&&Scalar->Scene->SolidBoxes==26&&Scalar->Scene->TransferProgress>0&&Scalar->Scene->TransferProgress<1,TEXT("push animates thirteen copies and preserves the source")))return;Capture(TEXT("scalar-3d-push-moving.png"));SmokeStep=12;}
 else if(SmokeStep==12&&Travel>2.8f){if(!Check(Scalar->Scene->TransferProgress==1,TEXT("pushed copies arrive at remote")))return;Capture(TEXT("scalar-3d-pushed.png"));Travel=0;SmokeStep=13;}
 else if(SmokeStep==13&&Travel>.4f){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=14;}
 else if(SmokeStep==14&&Travel>.8f){if(!Check(Scalar->Scene->TransferringObjects==8&&Scalar->Scene->SolidBoxes==34&&Scalar->Steps[5].Spaces[2].Blobs.IsEmpty(),TEXT("clone animates only eight metadata objects, no blobs")))return;Capture(TEXT("scalar-3d-setup-moving.png"));SmokeStep=15;}
 else if(SmokeStep==15&&Travel>2.8f){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=150;}
 else if(SmokeStep==150&&Travel>.8f){if(!Check(Scalar->Selected==6&&Scalar->Scene->TransferringObjects==3&&Scalar->Scene->SolidBoxes==37&&Scalar->Scene->TransferProgress<1&&Scalar->Steps[6].Spaces[2].Blobs==TArray<int32>{2,3,4},TEXT("hydrate animates only C, D and E after metadata arrives")))return;Capture(TEXT("scalar-3d-hydrate-moving.png"));SmokeStep=151;}
 else if(SmokeStep==151&&Travel>2.8f){if(!Check(Scalar->Scene->TransferProgress==1,TEXT("hydrated blobs arrive before get")))return;HandleParkKey(EKeys::Right);Travel=0;SmokeStep=16;}
 else if(SmokeStep==16&&Travel>.8f){if(!Check(Scalar->Selected==7&&Scalar->Scene->TransferringObjects==0&&!Scalar->Steps[7].Spaces[2].Blobs.Contains(0),TEXT("get stops on a missing object before fetching")))return;Capture(TEXT("scalar-3d-missing.png"));Travel=0;SmokeStep=17;}
 else if(SmokeStep==17&&Travel>.4f){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=18;}
 else if(SmokeStep==18&&Travel>.8f){if(!Check(Scalar->Selected==8&&Scalar->Scene->TransferringObjects==1&&Scalar->Scene->TransferProgress<1&&Scalar->Steps[8].Spaces[2].Blobs==TArray<int32>{0,2,3,4},TEXT("promisor fetch animates only A")))return;Capture(TEXT("scalar-3d-fetch-moving.png"));SmokeStep=19;}
 else if(SmokeStep==19&&Travel>2.8f){Capture(TEXT("scalar-3d-fetched.png"));Travel=0;SmokeStep=191;}
 else if(SmokeStep==191&&Travel>.4f){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=192;}
 else if(SmokeStep==192&&Travel>.8f){const auto& Local=Scalar->Steps[9].Spaces[3];if(!Check(Scalar->Selected==9&&Local.FirstCommit==3&&Local.Commits==1&&Local.Blobs==TArray<int32>{2,3,4}&&Scalar->Scene->SolidBoxes==43&&Scalar->Scene->TransferringObjects==5&&Scalar->Scene->TransferProgress<1,TEXT("User 3 shallow clone copies only C4, T4, C, D and E")))return;Capture(TEXT("scalar-3d-shallow-moving.png"));SmokeStep=193;}
 else if(SmokeStep==193&&Travel>2.8f){HandleParkKey(EKeys::Right);Travel=0;SmokeStep=194;}
 else if(SmokeStep==194&&Travel>1.2f){if(!Check(Scalar->Selected==10&&Scalar->Scene->TransferringObjects==0&&Scalar->Scene->SolidBoxes==43&&Scalar->Steps[10].Action==TEXT("unavailable"),TEXT("shallow get cannot find k1 and fetches no blob")))return;Capture(TEXT("scalar-3d-shallow-missing-history.png"));Travel=0;SmokeStep=190;}
 else if(SmokeStep==190&&Travel>.4f){Click(100,158);if(!Check(Scalar->Selected==9,TEXT("back returns to shallow clone")))return;Click(100,107);if(!Check(Scalar->Selected==0&&!Scalar->Playing,TEXT("reset rewinds")))return;Click(100,255);Scalar->Speed=1;Travel=0;SmokeStep=200;}
 else if(SmokeStep==200&&Travel>1.5f){if(!Check(Scalar->Selected==0&&Scalar->Playing,TEXT("fast autoplay waits for object creation")))return;SmokeStep=20;}
 else if(SmokeStep==20&&Travel>4.1f){if(!Check(Scalar->Selected>=1,TEXT("autoplay advances")))return;Click(100,785);if(!Check(Scalar->Selected==10&&!Scalar->Playing,TEXT("step list selects final shallow read")))return;HandleParkKey(EKeys::Right);Travel=0;SmokeStep=21;}
 else if(SmokeStep==21&&Travel>.55f&&SpeedGraph.IsValid()&&SpeedGraph->Reveal>0){if(!Check(PageIndex==2&&!bCommandDesktop&&UsesPhysicalProp()&&SpeedGraph->Reveal<1,TEXT("the brontosaurus display pulls down after Scalar")))return;Capture(TEXT("scalar-speed-screen-opening.png"));Travel=0;SmokeStep=211;}
 else if(SmokeStep==211&&Travel>2){if(!Check(SpeedGraph.IsValid()&&SpeedGraph->Reveal==1&&SpeedGraph->Rows.Num()==4&&SpeedGraph->Rows[2].Seconds>SpeedGraph->Rows[3].Seconds,TEXT("four dinosaur timings appear on the fully lowered screen")))return;Capture(TEXT("scalar-speed-screen.png"));Travel=0;SmokeStep=210;}
 else if(SmokeStep==210&&Travel>.6f){HandleParkKey(EKeys::Left);Travel=0;SmokeStep=22;}
 else if(SmokeStep==22&&Travel>.7f&&DesktopMinimize==1){if(!Check(Scalar->Selected==10,TEXT("return preserves scalar state")))return;TWeakPtr<FParkScalarScene> Scene=Scalar->Scene;LogoutToLogin();if(!Check(!Scalar.IsValid()&&PageScalars.IsEmpty()&&!Scene.IsValid(),TEXT("logout destroys native scene and clears progress")))return;UE_LOG(LogTemp,Display,TEXT("ScalarTest: PASS"));FPlatformMisc::RequestExitWithStatus(false,0);}
}
