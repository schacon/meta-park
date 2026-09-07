#include "SlideGameMode.h"
#include "IslandScene.h"
#include "ParkCamera.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Camera/CameraActor.h"
#include "Components/SceneComponent.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

namespace {
FVector Vec(const TSharedPtr<FJsonObject>& O, const TCHAR* Key) {
 const auto& A = O->GetArrayField(Key); return FVector(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber());
}
FRotator Rot(const FVector& V) { return FRotator(V.X,V.Y,V.Z); }
void Tint(UStaticMeshComponent* Mesh, FColor Color, bool Unlit=false) {
 if(auto* Material=LoadObject<UMaterialInterface>(nullptr,Unlit?TEXT("/Game/Models/M_Retro.M_Retro"):TEXT("/Game/Models/M_IslandLit.M_IslandLit"))) {
  auto* Instance=UMaterialInstanceDynamic::Create(Material,Mesh);
  Instance->SetVectorParameterValue(TEXT("Tint"),FLinearColor::FromSRGBColor(Color));
  Mesh->SetMaterial(0,Instance);
 }
}
AStaticMeshActor* Block(UWorld* World,FVector P,FVector Scale,FColor Color,FRotator R=FRotator::ZeroRotator) {
 auto* A=World->SpawnActor<AStaticMeshActor>(P,R);
 auto* Mesh=A->GetStaticMeshComponent(); Mesh->SetMobility(EComponentMobility::Movable);
 Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
 Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); A->SetActorScale3D(Scale); Tint(Mesh,Color); return A;
}
}
ASlideGameMode::ASlideGameMode() { PrimaryActorTick.bCanEverTick=true; DefaultPawnClass=nullptr; }
void ASlideGameMode::BeginPlay() {
 Super::BeginPlay();
 if(FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest"))) {
  int32 W=1280,H=720;FParse::Value(FCommandLine::Get(),TEXT("SlideTestWidth="),W);FParse::Value(FCommandLine::Get(),TEXT("SlideTestHeight="),H);
  auto* Settings=GEngine->GetGameUserSettings();Settings->SetFullscreenMode(EWindowMode::Windowed);Settings->SetScreenResolution(FIntPoint(W,H));Settings->ApplyResolutionSettings(false);
 }
 FString Text; TSharedPtr<FJsonObject> Deck;
 if (!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("Slides/deck.json"))) ||
     !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Deck) || !Deck.IsValid() || Deck->GetIntegerField(TEXT("version"))!=1) {
  UE_LOG(LogTemp,Error,TEXT("Missing or invalid Slides/deck.json. Run npm run build."));
  if(GEngine) GEngine->AddOnScreenDebugMessage(-1,120,FColor::Red,TEXT("Missing deck. Run npm run build, then restart.")); return;
 }
 Camera=GetWorld()->SpawnActor<AParkCamera>();
 Camera->GetCameraComponent()->SetFieldOfView(65);
 Camera->GetCameraComponent()->bConstrainAspectRatio=false;
 auto* PC=GetWorld()->GetFirstPlayerController();
 PC->SetViewTarget(Camera); PC->bShowMouseCursor=false;
 FInputModeGameOnly InputMode; PC->SetInputMode(InputMode);
 auto* Light=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,1000),FRotator(-45,-30,0));
 auto* Sun=Cast<UDirectionalLightComponent>(Light->GetLightComponent());
 Sun->SetMobility(EComponentMobility::Movable); Sun->SetIntensity(2.2f); Sun->SetLightColor(FLinearColor(1,.91,.75));
 Sun->LightSourceAngle=5.f; Sun->SetDynamicShadowDistanceMovableLight(50000);
 auto* Fill=GetWorld()->SpawnActor<ASkyLight>(); Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
 Fill->GetLightComponent()->SetIntensity(.65f); Fill->GetLightComponent()->SetRealTimeCaptureEnabled(true);
 auto* Post=GetWorld()->SpawnActor<APostProcessVolume>(); Post->bUnbound=true;
 Post->Settings.bOverride_AmbientOcclusionIntensity=true; Post->Settings.AmbientOcclusionIntensity=1.f;
 Post->Settings.bOverride_AmbientOcclusionRadius=true; Post->Settings.AmbientOcclusionRadius=180;
 Post->Settings.bOverride_BloomIntensity=true; Post->Settings.BloomIntensity=.12;
 FString Scene; Deck->TryGetStringField(TEXT("scene"),Scene); bIsland=Scene==TEXT("isla-nublar");
 if(bIsland) {
  Sun->SetDynamicShadowDistanceMovableLight(250000);
  RouteSeed=Deck->GetIntegerField(TEXT("seed")); HabitatCount=Deck->GetArrayField(TEXT("habitats")).Num();
  const auto Stats=IslandScene::Build(GetWorld(),Deck->GetArrayField(TEXT("habitats")),RouteSeed);
  TreeCount=Stats.Trees; DinoCount=Stats.Dinosaurs;
 }
 double Duration; if(Deck->TryGetNumberField(TEXT("durationSeconds"),Duration)) TimerDuration=Duration;
 if(!bIsland) Block(GetWorld(),FVector(0,0,-1100),FVector(2000,2000,1),FColor::FromHex(TEXT("#488b67")));
 // Enclosing unlit sphere gives the flat cyan horizon of the workstation reference.
 auto* Sky=GetWorld()->SpawnActor<AStaticMeshActor>();
 Sky->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
 Sky->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
 Sky->GetStaticMeshComponent()->SetCastShadow(false);
 Sky->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Sky->SetActorScale3D(FVector(bIsland?100000:1800)); Tint(Sky->GetStaticMeshComponent(),FColor::FromHex(TEXT("#91b5c3")),true);
 const auto& Slides=Deck->GetArrayField(TEXT("slides"));
 for(int32 I=0; I<Slides.Num(); ++I) {
  auto S=Slides[I]->AsObject(); const FVector Origin=Vec(S,TEXT("position")); const FRotator Facing(0,S->GetNumberField(TEXT("yaw")),0);
  const FTransform Transform(Facing,Origin);
  const FLinearColor Accent=FLinearColor::FromSRGBColor(FColor::FromHex(S->GetStringField(TEXT("accent"))));
 if(!bIsland) Block(GetWorld(),Transform.TransformPosition(FVector(-250,0,-680)),FVector(12,23,.7),FColor::FromHex(TEXT("#e1e1d7")),Facing);
 if(!bIsland) for(int32 J=0;J<5;++J) {
  const FVector P=Transform.TransformPosition(FVector(-450, J*380-760,-400));
  Block(GetWorld(),P,FVector(2.4,2.4,2.4+J*.3),FColor::FromHex(J==2?TEXT("#9864bc"):TEXT("#799cc9")),Facing);
 }
 if(!bIsland && I>0) {
  FVector Previous=Vec(Slides[I-1]->AsObject(),TEXT("position"))-FVector(0,0,570);
  FVector Current=Origin-FVector(0,0,570); FVector Direction=Current-Previous;
  Block(GetWorld(),(Previous+Current)*.5,FVector(Direction.Size()/100,2.5,.3),FColor::FromHex(TEXT("#b87676")),Direction.Rotation());
 }
  auto* Frame=Block(GetWorld(),Transform.TransformPosition(FVector(-18,0,0)),FVector(.28,14.65,9.25),FColor(57,68,59),Facing);
  FString HabitatLabel;
  FString HabitatID; S->TryGetStringField(TEXT("habitat"),HabitatID);
  if(bIsland) {
   const auto Card=S->GetObjectField(TEXT("card"));
   HabitatLabel=Card->GetStringField(TEXT("label"));
   const FVector Target=Vec(Card,TEXT("position")); CardTargets.Add(Target);
   MapPins.Add({Target,Card->GetStringField(TEXT("code")),HabitatLabel,Card->GetStringField(TEXT("status")),I,true});
  }
  auto* SlideRoot=GetWorld()->SpawnActor<AActor>();
  auto* RootComponent=NewObject<USceneComponent>(SlideRoot); SlideRoot->SetRootComponent(RootComponent); SlideRoot->AddInstanceComponent(RootComponent); RootComponent->RegisterComponent();
  SlideRoot->SetActorLocationAndRotation(Origin,Facing);
  Panels.Add({SlideRoot,Origin,0});
  auto* Panel=GetWorld()->SpawnActor<AActor>(Origin,Facing);
  auto* Widget=NewObject<UWidgetComponent>(Panel); Panel->SetRootComponent(Widget); Panel->AddInstanceComponent(Widget);
  Widget->SetWidgetSpace(EWidgetSpace::World); Widget->SetDrawSize(FVector2D(1440,900)); Widget->SetTwoSided(true); Widget->SetPivot(FVector2D(.5,.5)); Widget->RegisterComponent(); Widget->SetWorldLocationAndRotation(Origin,Facing);
  Panel->AttachToActor(SlideRoot,FAttachmentTransformRules::KeepWorldTransform);
  Frame->AttachToActor(SlideRoot,FAttachmentTransformRules::KeepWorldTransform);
  TSharedRef<SVerticalBox> Body=SNew(SVerticalBox);
  const FString CardHeading=bIsland?FString::Printf(TEXT("[%d] %s  /  %s"),I+1,*S->GetObjectField(TEXT("card"))->GetStringField(TEXT("code")),*HabitatLabel):FString::Printf(TEXT("Slide %d / %d"),I+1,Slides.Num());
  Body->AddSlot().AutoHeight().Padding(0,0,0,28)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.13,.61,.24)).Padding(12)
   [SNew(STextBlock).Text(FText::FromString(CardHeading)).Font(FCoreStyle::GetDefaultFontStyle("Mono",24)).ColorAndOpacity(FLinearColor(.015,.025,.02))]];
  Body->AddSlot().AutoHeight().Padding(0,0,0,28)[SNew(STextBlock).Text(FText::FromString(S->GetStringField(TEXT("title")))).Font(FCoreStyle::GetDefaultFontStyle("Bold",48)).ColorAndOpacity(FLinearColor(.001,.001,.001)).AutoWrapText(true)];
  for(auto& Value:S->GetArrayField(TEXT("blocks"))) {
   auto B=Value->AsObject(); FString Kind=B->GetStringField(TEXT("kind")); FString Content=B->GetStringField(TEXT("text"));
   if(Kind==TEXT("li")) Content=TEXT("•  ")+Content;
   const bool Heading=Kind.StartsWith(TEXT("h")); const bool Code=Kind==TEXT("pre");
   Body->AddSlot().AutoHeight().Padding(0,0,0,18)[SNew(STextBlock).Text(FText::FromString(Content)).Font(FCoreStyle::GetDefaultFontStyle(Heading?"Bold":Code?"Mono":"Regular",Code?20:Heading?19:27)).ColorAndOpacity(Heading?Accent:FLinearColor(.004,.004,.004)).AutoWrapText(true)];
  }
  Body->AddSlot().FillHeight(1);
  Body->AddSlot().AutoHeight().Padding(0,0,0,45)[SNew(STextBlock).Text(FText::FromString(TEXT("1–7 choose an area    ← → previous / next    Esc overview    N notes"))).Font(FCoreStyle::GetDefaultFontStyle("Regular",15)).ColorAndOpacity(Accent)];
  auto Content=SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.035,.035,.035)).Padding(5)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.66,.67,.63,1)).Padding(48)[Body]];
  if(bIsland) {
   Content->SetVisibility(TAttribute<EVisibility>::CreateLambda([this,I]{return Index==I&&MapPhase==EMapPhase::Slide?EVisibility::Visible:EVisibility::Hidden;}));
   Widget->SetSlateWidget(SNew(SOverlay)
    +SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.66,.67,.63)).Padding(0)[Content]]
    +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(48)
     [SNew(STextBlock).Visibility_Lambda([this,I]{return Index==I&&MapPhase==EMapPhase::Loading?EVisibility::Visible:EVisibility::Collapsed;})
      .Text_Lambda([this,I]{return FText::FromString(FString::Printf(TEXT("ingen:~$ load %02d\nloading... %s"),I+1,FMath::Fmod(Travel,.16f)<.08f?TEXT("_"):TEXT(" ")));})
      .Font(FCoreStyle::GetDefaultFontStyle("Mono",26)).ColorAndOpacity(FLinearColor(.015,.025,.02))]);
  } else Widget->SetSlateWidget(Content);
  const float Distance=S->GetNumberField(TEXT("cameraDistance"));
  const float CameraRise=bIsland ? 450.f : 0.f;
  Views.Add({Transform.TransformPosition(FVector(Distance,0,CameraRise)),FRotator(-FMath::RadiansToDegrees(FMath::Atan2(CameraRise,Distance)),Facing.Yaw+180,0),float(S->GetNumberField(TEXT("transition")))});
  Notes.Add(S->GetStringField(TEXT("notes"))); Titles.Add(S->GetStringField(TEXT("title"))); HabitatNames.Add(HabitatLabel);
  for(auto& MV:S->GetArrayField(TEXT("models"))) {
   auto M=MV->AsObject(); FVector P=Transform.TransformPosition(Vec(M,TEXT("position"))); FRotator R=(Facing.Quaternion()*Rot(Vec(M,TEXT("rotation"))).Quaternion()).Rotator();
   AActor* Model=nullptr; const FString Asset=M->GetStringField(TEXT("actor"));
   if(!Asset.IsEmpty()) { if(auto* Class=LoadClass<AActor>(nullptr,*Asset)) Model=GetWorld()->SpawnActor<AActor>(Class,P,R); }
   else {
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,*M->GetStringField(TEXT("mesh")));
    if(Mesh) { auto* A=GetWorld()->SpawnActor<AStaticMeshActor>(P,R); A->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable); A->GetStaticMeshComponent()->SetStaticMesh(Mesh); if(M->GetStringField(TEXT("mesh")).StartsWith(TEXT("/Engine/BasicShapes/"))) Tint(A->GetStaticMeshComponent(),FColor::FromHex(TEXT("#9864bc"))); Model=A; }
   }
   if(!Model) { UE_LOG(LogTemp,Error,TEXT("Could not load model on slide %s"),*S->GetStringField(TEXT("id"))); continue; }
   Panels.Last().Models.Add(Model);
   Model->SetActorScale3D(Vec(M,TEXT("scale"))); Model->AttachToActor(SlideRoot,FAttachmentTransformRules::KeepWorldTransform);
   const TSharedPtr<FJsonObject>* Animation;
   if(M->TryGetObjectField(TEXT("animation"),Animation)) Motions.Add({Model,Model->GetRootComponent()->GetRelativeLocation(),Model->GetRootComponent()->GetRelativeRotation(),(*Animation)->GetStringField(TEXT("kind")),float((*Animation)->GetNumberField(TEXT("speed"))),float((*Animation)->GetNumberField(TEXT("amplitude")))});
  }
  if(bIsland) {
   const auto Card=S->GetObjectField(TEXT("card"));
   const FString Code=Card->GetStringField(TEXT("code"));
   const auto View=Card->GetObjectField(TEXT("view"));
   FVector Anchor=Vec(View,TEXT("anchor"));
   if(Code!=TEXT("GATE"))Anchor.Z=IslandScene::GroundHeight(Anchor.X,Anchor.Y)+8;
   const float SignHeight=View->GetNumberField(TEXT("signHeight"));
   const float SignYaw=View->GetNumberField(TEXT("signYaw"));
   SignAnchors.Add(Anchor);Panels.Last().RaisedPosition=Anchor+FVector(0,0,SignHeight);
   CameraStops.Add(Vec(View,TEXT("eye")));SignYaws.Add(SignYaw);
   CameraTargets.Add(View->HasField(TEXT("look"))?Vec(View,TEXT("look")):Panels.Last().RaisedPosition);
   const auto& Path=View->GetArrayField(TEXT("path"));
   for(int32 K=0;K<2;K++){const auto& V=Path[K]->AsArray();(K==0?SwoopA:SwoopB).Add(FVector(V[0]->AsNumber(),V[1]->AsNumber(),V[2]->AsNumber()));}
   // A low plinth marks the fixed ground location. Twin stems rise from it.
   Block(GetWorld(),Anchor+FVector(0,0,15),FVector(5,2,.3),FColor(125,139,125),FRotator(0,SignYaw,0));
   for(float Y:{-480.f,480.f}) {
    auto* Stem=Block(GetWorld(),Transform.TransformPosition(FVector(-24,Y,-SignHeight/2)),FVector(.22,.22,SignHeight/100),FColor(65,78,67),Facing);
    Stem->AttachToActor(SlideRoot,FAttachmentTransformRules::KeepWorldTransform);
   }
   SlideRoot->SetActorLocationAndRotation(Panels.Last().RaisedPosition,FRotator((CameraStops.Last()-Panels.Last().RaisedPosition).Rotation().Pitch,SignYaw,2));
  }
  RootComponent->SetVisibility(false,true);
 }
 if(!Views.IsEmpty()) {
  if(bIsland) {
   Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Perspective);
   Camera->GetCameraComponent()->SetFieldOfView(FMath::RadiansToDegrees(2*FMath::Atan(40000.f/400000.f)));
   Camera->SetActorLocationAndRotation(MapEye,(MapCenter-MapEye).Rotation()); bOverview=true;
  } else { GoTo(0,true); Overview(); }
  bTourStarted=false;
 }

 CreateDesktopHUD();
 UE_LOG(LogTemp,Display,TEXT("SlideEngine ready: %d slides, %d animated models; starting in park overview"),Views.Num(),Motions.Num());
}
void ASlideGameMode::GoTo(int32 Next,bool Instant) {
 if(Views.IsEmpty()) return;
 if(bIsland) {
  const int32 Selected=(Next%Views.Num()+Views.Num())%Views.Num();
  if(MapPhase==EMapPhase::Overview) { Index=Selected; BeginMapLeg(true); }
  else { PendingIndex=Selected; BeginMapLeg(false); }
  return;
 }
 Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Perspective);
 Index=FMath::Clamp(Next,0,Views.Num()-1); bFlying=false; bOverview=false; bTourStarted=true; SetMouseMode(false); Travel=Instant?Views[Index].Duration:0;
 FromPosition=Camera->GetActorLocation(); FromRotation=Camera->GetActorQuat();
 FlightLift=bIsland&&!Instant ? FMath::Clamp(FVector::Distance(FromPosition,Views[Index].Position)*.45f,3000.f,7200.f) : 0;
 if(Instant) Camera->SetActorLocationAndRotation(Views[Index].Position,Views[Index].Rotation);
}
void ASlideGameMode::Overview() {
 if(bIsland) { PendingIndex=-1; if(MapPhase!=EMapPhase::Overview)BeginMapLeg(false); return; }
 bFlying=false; bOverview=true; SetMouseMode(false);
 if(bIsland) { Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic); Camera->GetCameraComponent()->SetOrthoWidth(23500); }
 FVector Center=FVector::ZeroVector; for(const auto& V:Views) Center+=V.Position; Center/=Views.Num();
 FVector Eye=bIsland?FVector(3000,-28000,23000):Center+FVector(9000,-6000,10000);
 if(bIsland) { Center=FVector(2850,-750,0); Eye+=Center; }
 Camera->SetActorLocationAndRotation(Eye,(Center-Eye).Rotation());
}
void ASlideGameMode::Tick(float Delta) {
 Super::Tick(Delta); if(!Camera || Views.IsEmpty()) return; Elapsed+=Delta;
 if(bTourStarted&&!bTimerPaused) TimerElapsed+=Delta;
 if(bIsland) { TickParkNavigation(Delta); return; }
 for(int32 PanelIndex=0;PanelIndex<Panels.Num();PanelIndex++) {
  auto& Panel=Panels[PanelIndex];if(!Panel.Root.IsValid())continue;
  const float D=FVector::Distance(Camera->GetActorLocation(),Panel.RaisedPosition);
  const float Target=(bOverview||PanelIndex!=Index)?0.f:1-FMath::SmoothStep(2500.f,3900.f,D);
  Panel.Reveal=FMath::FInterpTo(Panel.Reveal,Target,Delta,4.f);
  Panel.Root->SetActorLocation(Panel.RaisedPosition-FVector(0,0,(1-Panel.Reveal)*3200));
  Panel.Root->SetActorScale3D(FVector(1,1,FMath::Max(.01f,Panel.Reveal)));
  Panel.Root->GetRootComponent()->SetVisibility(Panel.Reveal>.015f,true);
 }
 auto* PC=GetWorld()->GetFirstPlayerController();
 if(!FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest"))) {
 if(PC->WasInputKeyJustPressed(EKeys::Right)||PC->WasInputKeyJustPressed(EKeys::SpaceBar)||PC->WasInputKeyJustPressed(EKeys::PageDown)) GoTo(bOverview?Index:Index+1);
 if(PC->WasInputKeyJustPressed(EKeys::Left)||PC->WasInputKeyJustPressed(EKeys::PageUp)) GoTo(Index-1);
 if(PC->WasInputKeyJustPressed(EKeys::Home)) GoTo(0);
 if(PC->WasInputKeyJustPressed(EKeys::O)) {
  if(bOverview) GoTo(Index); else Overview();
 }
 if(PC->WasInputKeyJustPressed(EKeys::P)) bTimerPaused=!bTimerPaused;
 if(PC->WasInputKeyJustPressed(EKeys::F)) { bFlying=!bFlying; bOverview=false; Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Perspective); SetMouseMode(bFlying); if(!bFlying) GoTo(Index); }
 if(PC->WasInputKeyJustPressed(EKeys::N)&&GEngine) GEngine->AddOnScreenDebugMessage(42,20,FColor::Cyan,Notes[Index].IsEmpty()?TEXT("No speaker notes for this slide."):Notes[Index]);
 }
 if(bFlying) {
  float X,Y; PC->GetInputMouseDelta(X,Y); auto R=Camera->GetActorRotation(); R.Yaw+=X*.15; R.Pitch=FMath::Clamp(R.Pitch-Y*.15,-85.f,85.f); Camera->SetActorRotation(R);
  FVector Move=Camera->GetActorForwardVector()*float(PC->IsInputKeyDown(EKeys::W)-PC->IsInputKeyDown(EKeys::S))+Camera->GetActorRightVector()*float(PC->IsInputKeyDown(EKeys::D)-PC->IsInputKeyDown(EKeys::A))+FVector::UpVector*float(PC->IsInputKeyDown(EKeys::E)-PC->IsInputKeyDown(EKeys::Q));
  Camera->AddActorWorldOffset(Move.GetClampedToMaxSize(1)*Delta*(PC->IsInputKeyDown(EKeys::LeftShift)?3000:1000));
 } else if(!bOverview) {
  Travel+=Delta; float T=FMath::Clamp(Travel/Views[Index].Duration,0.f,1.f); T=T*T*(3-2*T);
  const FVector Base=FMath::Lerp(FromPosition,Views[Index].Position,T);
  const FVector Position=Base+FVector(0,0,4*T*(1-T)*FlightLift);
  FQuat Rotation=FQuat::Slerp(FromRotation,Views[Index].Rotation.Quaternion(),T);
  if(bIsland && FlightLift>0 && T>0 && T<1) {
   const FVector Look=FMath::Lerp(FromPosition,Views[Index].Position,FMath::Min(T+.16f,1.f))-FVector(0,0,1000);
   Rotation=FQuat::Slerp(Rotation,(Look-Position).Rotation().Quaternion(),FMath::Square(FMath::Sin(PI*T))*.92f);
  }
  Camera->SetActorLocationAndRotation(Position,Rotation);
  if(bIsland && Index==1 && T>=.5f && !bFlightTested && FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest"))) {
   bFlightTested=true;
   const bool Raised=Position.Z>Base.Z+2000;
   UE_LOG(LogTemp,Display,TEXT("SlideSmoke: raised flight=%s lift=%.0f"),Raised?TEXT("PASS"):TEXT("FAIL"),Position.Z-Base.Z);
   if(!Raised) FPlatformMisc::RequestExitWithStatus(false,1);
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/island-flight.png"),true,false);
  }
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest")) && !bTourStarted && Elapsed>2 && !bInitialCaptured) {
  bInitialCaptured=true; FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/park-overview.png"),true,false);
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest")) && !bTourStarted && Elapsed>3) {
  const bool Hidden=Panels.Num()>0 && Panels[0].Reveal<.02f;
  UE_LOG(LogTemp,Display,TEXT("SlideSmoke: initial panels retracted=%s"),Hidden?TEXT("PASS"):TEXT("FAIL"));
  if(!Hidden) FPlatformMisc::RequestExitWithStatus(false,1);
  GoTo(0);
 }
 // Optional deterministic integration run used by scripts/unreal.mjs smoke.
 if(SmokeOverviewStart>=0) {
  if(Elapsed-SmokeOverviewStart>1 && Elapsed-SmokeOverviewStart<2 && !bSmokeCaptured) {
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/overview.png"),true,false); bSmokeCaptured=true;
  }
  if(Elapsed-SmokeOverviewStart>3) { UE_LOG(LogTemp,Display,TEXT("SlideSmoke: PASS all slides and overview")); FPlatformMisc::RequestExit(false); }
 }
 else if(bTourStarted && FParse::Param(FCommandLine::Get(),TEXT("SlideSmokeTest"))) {
  const float StopTime=Index==0 ? Views[Index].Duration+3.f : Views[Index].Duration+1.f;
  if(Travel>=StopTime && !bSmokeCaptured) {
   const bool AtStop=Camera->GetActorLocation().Equals(Views[Index].Position,1.f) && Panels[Index].Reveal>.95f;
   UE_LOG(LogTemp,Display,TEXT("SlideSmoke: slide=%d camera=%s"),Index+1,AtStop?TEXT("PASS"):TEXT("FAIL"));
   if(!AtStop) { FPlatformMisc::RequestExitWithStatus(false,1); return; }
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/slide-%02d.png"),Index+1),true,false);
   bSmokeCaptured=true;
  }
  if(bSmokeCaptured && Travel>=StopTime+1.f) {
   if(Index==Views.Num()-1) { Overview(); SmokeOverviewStart=Elapsed; bSmokeCaptured=false; }
   else { GoTo(Index+1); bSmokeCaptured=false; }
  }
 }
 for(auto& M:Motions) if(M.Actor.IsValid()) {
  if(M.Kind==TEXT("spin")) M.Actor->SetActorRelativeRotation(M.Rotation+FRotator(0,Elapsed*M.Speed,0));
  else M.Actor->SetActorRelativeLocation(M.Origin+FVector(0,0,FMath::Sin(Elapsed*M.Speed)*M.Amplitude));
 }
}
