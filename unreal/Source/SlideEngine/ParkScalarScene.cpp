#include "ParkScalarScene.h"
#include "ParkScalar.h"
#include "Engine/World.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

namespace {
const FColor Purple(118,59,173),Teal(32,139,121),Gold(208,154,49),Highlight(255,222,117),White(232,243,230);
constexpr float RoomWidth=1440;
float RoomCenter(int32 Room){return (Room-1.5f)*RoomWidth;}
FVector CommitAt(int32 Room,int32 I){return FVector(RoomCenter(Room)-495+I*330,690,190);}
FVector TreeAt(int32 Room,int32 I){return FVector(RoomCenter(Room)-495+I*330,320,100);}
FVector BlobAt(int32 Room,int32 I){return FVector(RoomCenter(Room)-480+I*240,-90,65);}
}
FParkScalarScene::FParkScalarScene(UWorld* World) {
 auto* A=World->SpawnActor<AActor>();Actor=A;
 auto* Root=NewObject<USceneComponent>(A);A->AddInstanceComponent(Root);A->SetRootComponent(Root);Root->SetMobility(EComponentMobility::Movable);Root->RegisterComponent();
 A->SetActorLocation(FVector(0,0,-100000));
 auto* Sky=Box(FVector(0,0,0),FVector(35000),FColor(27,47,103),false,false);Sky->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
 Box(FVector(0,300,-38),FVector(RoomWidth*4,1230,40),FColor(30,84,38),false,false);
 for(int32 Room=0;Room<4;Room++) {
  const float X=RoomCenter(Room);
  Box(FVector(X,300,-12),FVector(RoomWidth-5,1230,16),Room==1?FColor(38,99,81):FColor(58,131,63),false,false);
  for(int32 I=0;I<=12;I++)Link(FVector(X-RoomWidth/2+I*120,-315,-2),FVector(X-RoomWidth/2+I*120,915,-2),FColor(84,155,106),1,false,false);
  for(int32 I=0;I<=12;I++)Link(FVector(X-RoomWidth/2,-300+I*100,-2),FVector(X+RoomWidth/2,-300+I*100,-2),FColor(84,155,106),1,false,false);
  Link(FVector(X-RoomWidth/2,-315,0),FVector(X-RoomWidth/2,915,0),FColor(160,197,166),4,false,false);
 }
 auto* Sun=NewObject<UDirectionalLightComponent>(A);A->AddInstanceComponent(Sun);Sun->SetupAttachment(Root);Sun->SetMobility(EComponentMobility::Movable);Sun->SetRelativeRotation(FRotator(-60,-35,0));Sun->SetIntensity(1.7);Sun->SetLightingChannels(false,true,false);Sun->SetCastShadows(false);Sun->RegisterComponent();
 auto* Fill=NewObject<UDirectionalLightComponent>(A);A->AddInstanceComponent(Fill);Fill->SetupAttachment(Root);Fill->SetMobility(EComponentMobility::Movable);Fill->SetRelativeRotation(FRotator(-35,140,0));Fill->SetIntensity(.6);Fill->SetLightingChannels(false,true,false);Fill->SetCastShadows(false);Fill->RegisterComponent();
 auto* Texture=NewObject<UTextureRenderTarget2D>(A);Texture->ClearColor=FLinearColor(.025,.04,.12,1);Texture->InitCustomFormat(2292,1109,PF_B8G8R8A8,false);Texture->UpdateResourceImmediate(true);
 auto* Camera=World->SpawnActor<ASceneCapture2D>();Capture=Camera;auto* C=Camera->GetCaptureComponent2D();C->TextureTarget=Texture;C->FOVAngle=48;C->CaptureSource=SCS_FinalColorLDR;C->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;C->bCaptureEveryFrame=false;C->bCaptureOnMovement=false;C->bAlwaysPersistRenderingState=true;
 C->ShowFlags.SetAtmosphere(false);C->ShowFlags.SetFog(false);C->ShowFlags.SetMotionBlur(false);C->ShowFlags.SetBloom(false);C->ShowFlags.SetTemporalAA(false);
 C->PostProcessSettings.bOverride_FilmGrainIntensity=true;C->PostProcessSettings.FilmGrainIntensity=0;
 C->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;C->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
 C->PostProcessSettings.bOverride_AutoExposureMethod=true;C->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;C->PostProcessSettings.bOverride_AutoExposureBias=true;C->PostProcessSettings.AutoExposureBias=0;
 Brush.SetResourceObject(Texture);Brush.ImageSize=FVector2D(2292,1109);Brush.DrawAs=ESlateBrushDrawType::Image;
}
FParkScalarScene::~FParkScalarScene(){if(Capture.IsValid())Capture->Destroy();if(Actor.IsValid())Actor->Destroy();}
bool FParkScalarScene::IsValid()const{return Actor.IsValid()&&Capture.IsValid();}
FVector FParkScalarScene::CameraLocation()const{return Capture.IsValid()?Capture->GetActorLocation()-Actor->GetActorLocation():FVector::ZeroVector;}
UStaticMeshComponent* FParkScalarScene::Box(FVector Center,FVector Size,FColor Color,bool Dynamic,bool Lit) {
 Center.X=-Center.X;
 auto* A=Actor.Get();auto* C=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(C);C->SetupAttachment(A->GetRootComponent());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetRelativeLocation(Center);C->SetRelativeScale3D(Size/100);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->SetLightingChannels(false,true,false);
 const uint64 Key=uint64(Color.ToPackedARGB())|(uint64(Lit)<<32);
 auto* M=ColorMaterials.FindRef(Key).Get();
 if(!M){M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,Lit?TEXT("/Game/Models/M_IslandLit.M_IslandLit"):TEXT("/Game/Models/M_PropPaper.M_PropPaper")),A);
  M->SetVectorParameterValue(TEXT("Tint"),FLinearColor::FromSRGBColor(Color));
  if(!Lit){auto* Texture=UTexture2D::CreateTransient(1,1,PF_B8G8R8A8);auto& Data=Texture->GetPlatformData()->Mips[0].BulkData;auto* Pixel=static_cast<FColor*>(Data.Lock(LOCK_READ_WRITE));*Pixel=Color;Data.Unlock();Texture->UpdateResource();M->SetTextureParameterValue(TEXT("SlateUI"),Texture);}
  ColorMaterials.Add(Key,M);
 }
 C->SetMaterial(0,M);C->RegisterComponent();if(Dynamic)Objects.Add(C);return C;
}
void FParkScalarScene::Link(FVector From,FVector To,FColor Color,float Width,bool Dashed,bool Dynamic) {
 const FVector Delta=To-From;const float Length=Delta.Size();if(Length<.01f)return;
 const FVector Axis=Delta/Length;
 for(float D=0;D<Length;D+=Dashed?30:Length){const float Segment=FMath::Min(Dashed?17.f:Length,Length-D);auto* C=Box(From+Axis*(D+Segment*.5f),FVector(Width,Width,Segment),Color,Dynamic,false);C->SetRelativeRotation(FRotationMatrix::MakeFromZ(FVector(-Axis.X,Axis.Y,Axis.Z)).Rotator());}
}
UWidgetComponent* FParkScalarScene::Label(FVector Center,FVector2D Size,const TArray<FString>& Lines,FColor Color,int32 HighlightRow) {
 auto* A=Actor.Get();auto* W=NewObject<UWidgetComponent>(A);A->AddInstanceComponent(W);W->SetupAttachment(A->GetRootComponent());W->SetWidgetSpace(EWidgetSpace::World);W->SetDrawSize(FVector2D(768,FMath::RoundToInt(768*Size.Y/Size.X)));W->SetBlendMode(EWidgetBlendMode::Masked);W->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_PropLettering.M_PropLettering")));W->SetTwoSided(true);W->SetCollisionEnabled(ECollisionEnabled::NoCollision);W->SetCastShadow(false);W->SetTickWhenOffscreen(true);
 // Widget pixels extend along local -Y / -Z. Lay both axes on the XY top face.
 W->SetRelativeRotation(FRotationMatrix::MakeFromYZ(FVector(1,0,0),FVector(0,1,0)).Rotator());W->SetRelativeLocation(FVector(-Center.X,Center.Y,Center.Z));W->SetRelativeScale3D(FVector(Size.X/768));W->RegisterComponent();
 const bool Metadata=Lines.Num()>1;
 auto Rows=SNew(SVerticalBox);
 for(int32 I=0;I<Lines.Num();I++) {
  auto Text=SNew(STextBlock).Text(FText::FromString(Lines[I])).Font(FCoreStyle::GetDefaultFontStyle(I==0?"Bold":"Mono",Metadata?90:(Lines[0].Len()==1?500:160))).ColorAndOpacity(FLinearColor::FromSRGBColor(I==HighlightRow?Highlight:Color));
  if(Metadata)Rows->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(120).VAlign(VAlign_Top)[Text]];
  else Rows->AddSlot().AutoHeight()[Text];
 }
 // Metadata uses identical pixel/world scale and line spacing on every box.
 // Never scale short trees up or long trees down to fill the face.
 if(Metadata)W->SetSlateWidget(SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBrush")).Padding(36).HAlign(HAlign_Left).VAlign(VAlign_Top)[Rows]);
 else W->SetSlateWidget(SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBrush")).Padding(36)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly).HAlign(HAlign_Center).VAlign(VAlign_Center)[Rows]]);
 Objects.Add(W);return W;
}
void FParkScalarScene::TrackTransfer(int32 Start,float Offset) {
 if(Offset==0)return;
 for(int32 I=Start;I<Objects.Num();I++)if(Objects[I].IsValid())Transfers.Add({Objects[I],Objects[I]->GetRelativeLocation()});
 TransferOffset=Offset;
}
void FParkScalarScene::TrackCreation(int32 Start,FVector Pivot,float At,const FString& Name,const FString& Description) {
 if(At<0)return;
 Pivot.X=-Pivot.X;
 for(int32 I=Start;I<Objects.Num();I++)if(Objects[I].IsValid())Creations.Add({Objects[I],Objects[I]->GetRelativeLocation(),Objects[I]->GetRelativeScale3D(),Pivot,At,I==Start?Name:FString(),Description});
 CreationEndTime=FMath::Max(CreationEndTime,At+.65f);
}
void FParkScalarScene::RevealAfter(int32 Start,float At) {
 if(At<0)return;
 for(int32 I=Start;I<Objects.Num();I++)Reveals.Add({Objects[I],At});
}
bool FParkScalarScene::CreationObjectVisible(const FString& Name) const {
 for(const auto& C:Creations)if(C.Name==Name&&C.Component.IsValid())return C.Component->IsVisible();
 return false;
}
void FParkScalarScene::Rebuild(const FParkScalar& S) {
 for(auto& O:Objects)if(O.IsValid())O->DestroyComponent();Objects.Reset();Transfers.Reset();Creations.Reset();Reveals.Reset();CreationEndTime=0;CreationStatus.Reset();TransferringObjects=0;SolidBoxes=0;TopLabels=0;LabelsFit=true;
 const auto& Step=S.Steps[S.Selected];
 auto Object=[&](FVector Center,FVector Size,FColor Color,const TArray<FString>& Lines,bool Fresh,float CopyOffset=0.f,float WriteAt=-1.f,const FString& Kind=FString(),int32 HighlightRow=-1){
  const int32 Start=Objects.Num();Box(Center,Size,Color);SolidBoxes++;auto* W=Label(Center+FVector(0,0,Size.Z*.5f+1.2f),FVector2D(Size.X-12,Size.Y-12),Lines,Fresh?Highlight:White,HighlightRow);TopLabels++;
  const auto Draw=W->GetDrawSize();const float Scale=W->GetRelativeScale3D().X;LabelsFit&=Draw.X*Scale<=Size.X-10&&Draw.Y*Scale<=Size.Y-10&&FMath::Abs(W->GetForwardVector().Z)>.99;
  if(Fresh){const FVector A=Center+FVector(-Size.X/2,-Size.Y/2,Size.Z/2+1),B=Center+FVector(Size.X/2,-Size.Y/2,Size.Z/2+1),C=Center+FVector(Size.X/2,Size.Y/2,Size.Z/2+1),D=Center+FVector(-Size.X/2,Size.Y/2,Size.Z/2+1);Link(A,B,Highlight,2);Link(B,C,Highlight,2);Link(C,D,Highlight,2);Link(D,A,Highlight,2);}
  TrackTransfer(Start,CopyOffset);if(CopyOffset!=0)TransferringObjects++;
  TrackCreation(Start,Center,WriteAt,Lines[0],TEXT("Writing ")+Kind+TEXT(" ")+Lines[0]);
 };
 for(int32 Room=0;Room<Step.Spaces.Num();Room++) {
  const auto& Space=Step.Spaces[Room];const float X=RoomCenter(Room);
  const FScalarSpace Previous=S.Selected>0?S.Steps[S.Selected-1].Spaces[Room]:FScalarSpace();
  const TCHAR* Names[]={TEXT("USER 1"),TEXT("REMOTE"),TEXT("USER 2"),TEXT("USER 3")};
  Label(FVector(X,-260,1),FVector2D(470,95),{Names[Room]});
  if(!Space.Commits){Label(FVector(X,320,1),FVector2D(360,120),{TEXT("(empty)")},FColor(166,199,170));continue;}
  const bool Writing=Step.Action==TEXT("write")&&Room==Step.Room;
  const bool Transfer=Step.From>=0&&Room==Step.Room;
  const bool Missing=Step.Action==TEXT("missing")&&Room==Step.Room;
  const float CopyOffset=Transfer?(Room-Step.From)*RoomWidth:0;
  TArray<int32> WrittenBlobs;for(int32 B:Space.Blobs)if(Writing&&!Previous.Blobs.Contains(B))WrittenBlobs.Add(B);
  auto BlobWriteAt=[&](int32 B){const int32 I=WrittenBlobs.Find(B);return I>=0?2.15f+I*.75f:-1.f;};
  auto CommitTransfer=[&](int32 I){return Transfer&&(I<Previous.FirstCommit||I>=Previous.FirstCommit+Previous.Commits);};
  auto NewCommit=[&](int32 I){return CommitTransfer(I)||(Writing&&I==Step.Commit);};
  auto BlobTransfer=[&](int32 B){return Transfer&&!Previous.Blobs.Contains(B);};
  auto NewBlob=[&](int32 B){return BlobTransfer(B)||WrittenBlobs.Contains(B);};
  TSet<int32> ReferencedBlobs;
  for(int32 I=Space.FirstCommit;I<Space.FirstCommit+Space.Commits;I++) {
   const auto& Commit=S.Commits[I];const auto CP=CommitAt(Room,I),TP=TreeAt(Room,I);const bool NewWrite=Writing&&I==Step.Commit;
   TArray<FString> CommitLines{Commit.Id},MessageLines;Commit.Delta.ParseIntoArrayLines(MessageLines);CommitLines.Append(MessageLines);
   Object(CP,FVector(270,170,38),Purple,CommitLines,NewCommit(I),CommitTransfer(I)?CopyOffset:0,NewWrite?.25f:-1.f,TEXT("commit"));
   TArray<FString> Entries{Commit.Tree};for(int32 B:Commit.Entries){Entries.Add(FString::Printf(TEXT("k%d -> %c"),B+1,TCHAR('A'+B)));ReferencedBlobs.Add(B);}
   Object(TP,FVector(270,250,32),Teal,Entries,NewCommit(I),CommitTransfer(I)?CopyOffset:0,NewWrite?1.2f:-1.f,TEXT("tree"),Missing&&Commit.Entries.Contains(0)?Commit.Entries.Find(0)+1:-1);
   const int32 PointerStart=Objects.Num();int32 RevealStart=Objects.Num();
   Link(CP+FVector(0,-85,-4),TP+FVector(0,125,0),NewCommit(I)?Highlight:White,4);RevealAfter(RevealStart,NewWrite?1.85f:-1.f);
   RevealStart=Objects.Num();
   // A shallow root never draws a parent edge to a commit it does not possess.
   if(I>Space.FirstCommit)Link(CP+FVector(-135,0,0),CommitAt(Room,I-1)+FVector(135,0,0),White,3,true);
   RevealAfter(RevealStart,NewWrite?.9f:-1.f);
   for(int32 B:Commit.Entries) {
    RevealStart=Objects.Num();Link(TP+FVector((B-2.f)*25,-125,0),BlobAt(Room,B)+FVector(0,55,0),NewCommit(I)||NewBlob(B)||(Missing&&B==0)?Highlight:FColor(169,204,177),3,!Space.Blobs.Contains(B));
    RevealAfter(RevealStart,NewWrite?(BlobWriteAt(B)>=0?BlobWriteAt(B)+.65f:1.85f):-1.f);
   }
   TrackTransfer(PointerStart,CommitTransfer(I)?CopyOffset:0);
  }
  const int32 Tip=Space.FirstCommit+Space.Commits-1,HeadStart=Objects.Num();
  Label(CommitAt(Room,Tip)+FVector(0,135,22),FVector2D(150,55),{TEXT("HEAD")});RevealAfter(HeadStart,Writing?.9f:-1.f);
  if(Space.FirstCommit>0) {
   Label(FVector(X-220,570,1),FVector2D(590,100),{TEXT("depth 1: C4 only")});
   if(Step.Action==TEXT("unavailable")&&Room==Step.Room){Label(FVector(X-240,340,1),FVector2D(660,115),{TEXT("k1: not found")},Highlight);Label(FVector(X-240,200,1),FVector2D(660,140),{TEXT("no earlier history")},Highlight);}
  }
  for(int32 B=0;B<5;B++) {
   const auto At=BlobAt(Room,B);
   if(Space.Blobs.Contains(B))Object(At,FVector(110,110,110),Gold,{FString::Chr('A'+B)},NewBlob(B),BlobTransfer(B)?CopyOffset:0,BlobWriteAt(B),TEXT("blob"));
   else if(ReferencedBlobs.Contains(B)) {
    const FColor Ghost=(Missing&&B==0)?Highlight:FColor(125,172,148);
    for(int32 Side=-1;Side<=1;Side+=2){Link(At+FVector(-55,Side*55,55),At+FVector(55,Side*55,55),Ghost,2,true);Link(At+FVector(Side*55,-55,55),At+FVector(Side*55,55,55),Ghost,2,true);for(int32 Other=-1;Other<=1;Other+=2)Link(At+FVector(Side*55,Other*55,-55),At+FVector(Side*55,Other*55,55),Ghost,2,true);}
    Label(At+FVector(0,0,57),FVector2D(98,98),{FString::Chr('A'+B)},Ghost);
   } else continue;
   const int32 KeyStart=Objects.Num();Label(At+FVector(0,-100,-63),FVector2D(190,55),{Space.Blobs.Contains(B)?FString::Printf(TEXT("k%d"),B+1):TEXT("not local")},White);RevealAfter(KeyStart,BlobWriteAt(B)>=0?BlobWriteAt(B)+.65f:-1.f);
  }
 }
 if(Step.From>=0){const float From=RoomCenter(Step.From),To=RoomCenter(Step.Room);Link(FVector(From,-440,12),FVector(To,-440,12),Highlight,6);Link(FVector(To,-440,12),FVector(To-90,-395,12),Highlight,6);Link(FVector(To,-440,12),FVector(To-90,-485,12),Highlight,6);}
 Capture->GetCaptureComponent2D()->ShowOnlyComponents.Reset();Capture->GetCaptureComponent2D()->ShowOnlyActorComponents(Actor.Get());BuiltStep=S.Selected;
}
void FParkScalarScene::Update(const FParkScalar& S,float Delta) {
 if(!IsValid())return;if(BuiltStep!=S.Selected)Rebuild(S);
 TransferProgress=FMath::Clamp(S.Age/2.4f,0.f,1.f);const float Ease=TransferProgress*TransferProgress*(3-2*TransferProgress);
 for(auto& T:Transfers)if(T.Component.IsValid())T.Component->SetRelativeLocation(T.Destination+FVector(TransferOffset*(1-Ease),0,FMath::Sin(Ease*PI)*100));
 CreationStatus.Reset();
 for(const auto& C:Creations)if(C.Component.IsValid()) {
  const float Progress=FMath::Clamp((S.Age-C.Start)/.65f,0.f,1.f);
  const float Scale=.03f+.97f*Progress*Progress*(3-2*Progress);
  C.Component->SetVisibility(S.Age>=C.Start);
  C.Component->SetRelativeScale3D(C.Scale*Scale);
  C.Component->SetRelativeLocation(C.Pivot+(C.Destination-C.Pivot)*Scale+FVector(0,0,65*(1-Scale)));
  if(CreationStatus.IsEmpty()&&S.Age<C.Start+.65f)CreationStatus=C.Description+TEXT("...");
 }
 for(const auto& R:Reveals)if(R.Component.IsValid())R.Component->SetVisibility(S.Age>=R.At);
 const auto& Step=S.Steps[S.Selected];
 if(!S.MovingCamera){DesiredFocus=FVector(0,260,60);DesiredDistance=7600;}
 else if(Step.From>=0) {
  const float From=RoomCenter(Step.From),To=RoomCenter(Step.Room);
  const bool FollowBlob=Step.Action==TEXT("hydrate")||Step.Action==TEXT("fetch");
  DesiredFocus=FVector(FollowBlob?FMath::Lerp(From,To,Ease):(From+To)*.5f,FollowBlob?100:260,80);
  DesiredDistance=FollowBlob?3000:4600+(Step.Room-Step.From-1)*1500;
 } else {DesiredFocus=FVector(RoomCenter(Step.Room)+(Step.Action==TEXT("write")?(Step.Commit-1.5f)*160:0),260,80);DesiredDistance=3000;}
 DesiredFocus+=S.PanOffset;
 DesiredDistance*=FMath::Lerp(1.35f,.65f,S.Height);
 const float Blend=1-FMath::Exp(-Delta*5);if(!CameraReady){Focus=DesiredFocus;Distance=DesiredDistance;CameraReady=true;}else{Focus=FMath::Lerp(Focus,DesiredFocus,Blend);Distance=FMath::Lerp(Distance,DesiredDistance,Blend);}
 const float Pitch=FMath::DegreesToRadians(FMath::Lerp(25.f,82.f,S.Tilt)),Yaw=FMath::DegreesToRadians(S.OrbitYaw);
 const FVector Offset(FMath::Sin(Yaw)*FMath::Cos(Pitch),-FMath::Cos(Yaw)*FMath::Cos(Pitch),FMath::Sin(Pitch));const FVector Eye=Focus+Offset*Distance;
 Capture->SetActorLocationAndRotation(Actor->GetActorLocation()+FVector(-Eye.X,Eye.Y,Eye.Z),FVector(Eye.X-Focus.X,Focus.Y-Eye.Y,Focus.Z-Eye.Z).Rotation());Capture->GetCaptureComponent2D()->CaptureScene();
}
