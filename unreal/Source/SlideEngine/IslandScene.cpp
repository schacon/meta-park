#include "IslandScene.h"
#include "DinosaurGait.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace {
struct FSplash { TWeakObjectPtr<AStaticMeshActor> Actor; FVector Origin; float Scale,Phase; };
TArray<FSplash> Splashes;
struct FWander {
 TWeakObjectPtr<AStaticMeshActor> Actor;
 TSharedPtr<FDinosaurGait> Gait;
 FBox BindBounds;
 FVector Center, LocalCenter, FenceCenter;
 FVector2D Amplitude;
 float Phase, Rate, Scale, FootOffset, Floor;
 bool Raptor;
 float WalkDuration, RestDuration, ScheduleOffset;
};
TArray<FWander> Wanderers;
// Integrate a walking speed envelope so pauses stop the route clock itself.
// Position and heading remain continuous through both stops and restarts.
float WalkingTime(const FWander& D,float Seconds) {
 constexpr float Ramp=.8f;
 const float Period=D.WalkDuration+D.RestDuration;
 const float Time=Seconds+D.ScheduleOffset,Cycles=FMath::FloorToFloat(Time/Period),T=Time-Cycles*Period;
 auto Integral=[](float U){return U*U*U-.5f*U*U*U*U;};
 float Active=0;
 if(T<Ramp)Active=Ramp*Integral(T/Ramp);
 else if(T<D.WalkDuration-Ramp)Active=T-Ramp*.5f;
 else if(T<D.WalkDuration) {
  const float U=(T-(D.WalkDuration-Ramp))/Ramp;
  Active=D.WalkDuration-1.5f*Ramp+Ramp*(U-Integral(U));
 } else Active=D.WalkDuration-Ramp;
 return Cycles*(D.WalkDuration-Ramp)+Active;
}
FVector2D WanderPoint(const FWander& D,float U) {
 return FVector2D(D.Amplitude.X*(FMath::Sin(U)+.2f*FMath::Sin(U*1.71f+D.Phase)),
                  D.Amplitude.Y*(FMath::Cos(U)+.18f*FMath::Sin(U*1.31f-D.Phase)));
}
FTransform WanderPose(const FWander& D,float Seconds) {
 const float U=D.Phase+WalkingTime(D,Seconds)*D.Rate;
 const FVector2D Offset=WanderPoint(D,U);
 // The route tangent remains defined while the animation clock is paused.
 const FVector2D Tangent=FVector2D(D.Amplitude.X*(FMath::Cos(U)+.342f*FMath::Cos(U*1.71f+D.Phase)),
  D.Amplitude.Y*(-FMath::Sin(U)+.2358f*FMath::Cos(U*1.31f-D.Phase)))*D.Rate;
 const FRotator Facing(0,FMath::RadiansToDegrees(FMath::Atan2(Tangent.Y,Tangent.X)),0);
 const FVector Center=D.Center+FVector(Offset.X,Offset.Y,0);
 FVector Root=Center-Facing.RotateVector(D.LocalCenter*D.Scale);
 Root.Z=(D.Raptor?D.Floor:IslandScene::GroundHeight(Center.X,Center.Y))-D.FootOffset;
 return FTransform(Facing,Root,FVector(D.Scale));
}
void Wander(AStaticMeshActor* Actor,FVector Center,FVector2D Amplitude,int32 Seed,bool Raptor,bool Quad,FVector FenceCenter) {
 FRandomStream Random(Seed);
 const FBox Bounds=Actor->GetStaticMeshComponent()->GetStaticMesh()->GetBoundingBox();
 const float Scale=Actor->GetActorScale3D().X;
 FString Species=Actor->GetStaticMeshComponent()->GetStaticMesh()->GetName();Species.RemoveFromStart(TEXT("SM_"));
 auto Gait=MakeShared<FDinosaurGait>(Actor,Species);
 const FVector LocalCenter(Bounds.GetCenter().X,Bounds.GetCenter().Y,0);
 Wanderers.Add({Actor,Gait,Bounds,Center,LocalCenter,FenceCenter,Amplitude,Random.FRandRange(0,2*PI),
  (Random.RandRange(0,1)?1.f:-1.f)*12*PI/Random.FRandRange(180,260),Scale,float(Bounds.Min.Z*Scale),float(Center.Z),Raptor,
  Random.FRandRange(12,19),Random.FRandRange(7,11),Random.FRandRange(0,25)});
}
TArray<FVector> GroundVertices;
TArray<FIntVector> GroundTriangles;
AStaticMeshActor* Asset(UWorld* World,const FString& Name,FVector Position=FVector::ZeroVector,float Yaw=0,float Scale=1) {
 const FString Path=FString::Printf(TEXT("/Game/Models/Park/%s.%s"),*Name,*Name);
 auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
 if(!Mesh) { UE_LOG(LogTemp,Fatal,TEXT("Missing Blender asset %s. Run npm run unreal:import-assets."),*Path); return nullptr; }
 auto* Actor=World->SpawnActor<AStaticMeshActor>(Position,FRotator(0,Yaw,0));
 auto* Component=Actor->GetStaticMeshComponent();
 Component->SetMobility(EComponentMobility::Movable);
 Component->SetStaticMesh(Mesh); if(Name==TEXT("SM_Water"))Component->SetCastShadow(false); Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Actor->SetActorScale3D(FVector(Scale)); return Actor;
}
FVector Position(const TArray<TSharedPtr<FJsonValue>>& Values) {
 return FVector(Values[0]->AsNumber(),Values[1]->AsNumber(),Values[2]->AsNumber());
}
}
void IslandScene::Animate(float Seconds) {
 for(const auto& D:Wanderers)if(D.Actor.IsValid()) {
  auto Pose=WanderPose(D,Seconds);
  D.Actor->SetActorTransform(Pose);
  D.Gait->Update(Pose,Seconds,[&D](float Time){return WanderPose(D,Time);},[&D](FVector P){return D.Raptor?D.Floor:GroundHeight(P.X,P.Y);});
 }
 for(const auto& S:Splashes)if(S.Actor.IsValid()) {
  const float Pulse=FMath::Sin(Seconds*4.f+S.Phase);
  S.Actor->SetActorScale3D(FVector(1+.22f*Pulse,1+.22f*Pulse,1+.32f*Pulse)*S.Scale);
  S.Actor->SetActorLocation(S.Origin+FVector(0,0,12*FMath::Max(0.f,Pulse)));
 }
}
bool IslandScene::ValidateWandering() {
 const FVector2D Fence[]={FVector2D(-2,-15),{-9,-14.5},{-14,-10.8},{-15,-3},{-13,7},{-8,13.5},{0,15},{10,13},{14.5,7},{14,-3},{10,-12},{2,-15}};
 bool Good=Wanderers.Num()==6;
 float MaxStep=0,MinClearance=MAX_flt,MinSeparation=MAX_flt;
 for(float Time=0;Time<=2100;Time+=2) {
  TArray<FVector> Raptors;
  for(const auto& D:Wanderers) {
   const FTransform Pose=WanderPose(D,Time);
   const FBox Bounds=D.BindBounds;
   for(float X:{float(Bounds.Min.X),float(Bounds.Max.X)})for(float Y:{float(Bounds.Min.Y),float(Bounds.Max.Y)}) {
    const FVector P=Pose.TransformPosition(FVector(X,Y,0))-D.FenceCenter;
    if(D.Raptor){MinClearance=FMath::Min(MinClearance,float(FMath::Min(1777-FMath::Abs(P.X),1225-FMath::Abs(P.Y))));Good&=FMath::Abs(P.X)<1777&&FMath::Abs(P.Y)<1225;}
    else for(int32 I=0;I<12;I++) {
     const FVector2D A=Fence[I]*140,B=Fence[(I+1)%12]*140,E=B-A,V=FVector2D(P.X,P.Y)-A;
     Good&=-(E.X*V.Y-E.Y*V.X)/E.Size()>20;
     MinClearance=FMath::Min(MinClearance,float(-(E.X*V.Y-E.Y*V.X)/E.Size()));
    }
   }
   if(D.Raptor)Raptors.Add(Pose.TransformPosition(D.LocalCenter));
   // Preserve continuous motion even during the tightest turn. Double the
   // previous movement allowance to match the doubled route speed.
   Good&=FVector::Dist2D(Pose.GetLocation(),WanderPose(D,Time+.1f).GetLocation())<60;
   MaxStep=FMath::Max(MaxStep,float(FVector::Dist2D(Pose.GetLocation(),WanderPose(D,Time+.1f).GetLocation())));
  }
  for(int32 I=0;I<Raptors.Num();I++)for(int32 J=I+1;J<Raptors.Num();J++){const float Distance=FVector::Dist2D(Raptors[I],Raptors[J]);Good&=Distance>1080;MinSeparation=FMath::Min(MinSeparation,Distance);}
 }
 for(const auto& D:Wanderers) {
  const float Rest=D.WalkDuration+2-D.ScheduleOffset;
  Good&=WanderPose(D,Rest).Equals(WanderPose(D,Rest+3),.001f);
  const float Walk=2-D.ScheduleOffset;
  Good&=!WanderPose(D,Walk).Equals(WanderPose(D,Walk+3),1.f);
 }
 UE_LOG(LogTemp,Display,TEXT("SlideSmoke: faster walking with stationary rests stays inside fences and raptors remain separated over 35 minutes=%s"),Good?TEXT("PASS"):TEXT("FAIL"));
 UE_LOG(LogTemp,Display,TEXT("SlideSmoke: maximum 0.1s movement=%.2fcm minimum boundary clearance=%.2fcm raptor separation=%.2fcm"),MaxStep,MinClearance,MinSeparation);
 return Good;
}
bool IslandScene::ValidateArticulatedGaits() {
 bool Good=true;
 for(const auto& D:Wanderers) {
  for(float Start:{0.f,900.f,1980.f}) {
   D.Gait->Reset();
   for(int32 Frame=0;Frame<=1200;Frame++) {
    const float Time=Start+Frame*.1f;
    D.Gait->Update(WanderPose(D,Time),Time,[&D](float T){return WanderPose(D,T);},[&D](FVector P){return D.Raptor?D.Floor:GroundHeight(P.X,P.Y);},false);
    if(Frame>20&&WanderPose(D,Time).Equals(WanderPose(D,Time-2),.001f))Good&=D.Gait->AllFeetPlanted();
   }
   Good&=D.Gait->Validate();
  }
  D.Gait->Reset();
 }
 Animate(0);
 UE_LOG(LogTemp,Display,TEXT("SlideSmoke: articulated legs, grounded stance feet, fixed bone lengths and lifted recovery steps=%s"),Good?TEXT("PASS"):TEXT("FAIL"));
 return Good;
}
float IslandScene::GroundHeight(float X,float Y) {
 for(const auto& T:GroundTriangles) {
  const FVector A=GroundVertices[T.X],B=GroundVertices[T.Y],C=GroundVertices[T.Z];
  const double D=(B.Y-C.Y)*(A.X-C.X)+(C.X-B.X)*(A.Y-C.Y);if(FMath::Abs(D)<.001)continue;
  const double U=((B.Y-C.Y)*(X-C.X)+(C.X-B.X)*(Y-C.Y))/D;
  const double V=((C.Y-A.Y)*(X-C.X)+(A.X-C.X)*(Y-C.Y))/D;
  if(U>=-.0001&&V>=-.0001&&U+V<=1.0001)return U*A.Z+V*B.Z+(1-U-V)*C.Z;
 }
 return 200.f;
}
IslandScene::FStats IslandScene::Build(UWorld* World,const TArray<TSharedPtr<FJsonValue>>& Habitats,int32 Seed) {
 FString Source;
 if(!FFileHelper::LoadFileToString(Source,*(FPaths::ProjectContentDir()/TEXT("Slides/park-assets.json")))) {
  UE_LOG(LogTemp,Fatal,TEXT("Missing park asset placement manifest. Run npm run unreal:import-assets.")); return {};
 }
 TSharedPtr<FJsonObject> Manifest;
 if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source),Manifest) || !Manifest.IsValid()) {
  UE_LOG(LogTemp,Fatal,TEXT("Invalid Blender park placement manifest")); return {};
 }
 GroundVertices.Reset();GroundTriangles.Reset();Splashes.Reset();Wanderers.Reset();
 FString GroundJson;TSharedPtr<FJsonObject> Ground;
 if(FFileHelper::LoadFileToString(GroundJson,*(FPaths::ProjectContentDir()/TEXT("Slides/park-ground.json")))&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(GroundJson),Ground)) {
  for(const auto& V:Ground->GetArrayField(TEXT("vertices")))GroundVertices.Add(Position(V->AsArray())*100);
  for(const auto& T:Ground->GetArrayField(TEXT("triangles"))){const auto& A=T->AsArray();GroundTriangles.Add(FIntVector(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber()));}
 }
 // Scenery uses Blender placements; habitat inhabitants still follow the editable deck layout.
 TSet<FString> Species;
 for(const auto& Habitat:Habitats) Species.Add(TEXT("SM_")+Habitat->AsObject()->GetStringField(TEXT("species")));
 int32 Count=0,Trees=0,Dinosaurs=0;
 for(const auto& Value:Manifest->GetArrayField(TEXT("placements"))) {
  auto P=Value->AsObject();const FString Name=P->GetStringField(TEXT("asset"));
  if(Name==TEXT("SM_DungPile") || Name==TEXT("SM_Enclosure") || Name==TEXT("SM_RaptorPen") || Name==TEXT("SM_RaptorBeacon") || Species.Contains(Name)) continue;
  auto* Placed=Asset(World,Name,Position(P->GetArrayField(TEXT("position")))*100,P->GetNumberField(TEXT("yaw")),P->GetNumberField(TEXT("scale")));Count++;
  if(Name==TEXT("SM_WaterSplash"))Splashes.Add({Placed,Placed->GetActorLocation(),float(P->GetNumberField(TEXT("scale"))),Splashes.Num()*1.7f});
  if(Name.StartsWith(TEXT("SM_Tree")) || Name==TEXT("SM_Palm")) Trees++;
 }
 for(int32 I=0;I<Habitats.Num();I++) {
  const auto& Value=Habitats[I]; auto H=Value->AsObject();const FVector P=Position(H->GetArrayField(TEXT("position")));
  if(H->GetStringField(TEXT("species"))==TEXT("velociraptor")) {
   Asset(World,TEXT("SM_RaptorPen"),P,0,1.2f);
   for(float Y:{-10.35f,10.35f})for(float X:{-15.6f,-5.2f,5.2f,15.6f})Asset(World,TEXT("SM_RaptorBeacon"),P+FVector(X,Y,10.55f)*120,0,1.2f);
   Count+=8;
   const FVector Offsets[]={FVector(-600,-200,70),FVector(600,300,70),FVector(500,-600,70)};
   const float Yaws[]={155,220,120};
   for(int32 J=0;J<3;J++) {
    auto* Dino=Asset(World,TEXT("SM_velociraptor"),P+Offsets[J],Yaws[J],1.1f);
    Wander(Dino,P+FVector((J-1)*1170,0,78),FVector2D(30,210),Seed+I*97+J*31,true,false,P);
   }
   Count+=4;Dinosaurs+=3;continue;
  }
  Dinosaurs++;
  Asset(World,TEXT("SM_Enclosure"),P,0,1.4f);
  const FString Kind=H->GetStringField(TEXT("species"));
  auto* Dino=Asset(World,TEXT("SM_")+Kind,P,155,2.7f);Count+=2;
  const bool Tric=Kind==TEXT("triceratops"),Bronto=Kind==TEXT("brachiosaurus");
  const FVector Center=P+(Tric?FVector(-370,260,0):FVector::ZeroVector);
  Wander(Dino,Center,FVector2D(Tric?80:Bronto?150:240),Seed+I*97,false,Tric||Bronto,P);
  if(H->GetStringField(TEXT("species"))==TEXT("triceratops")) {
   const FVector Heap(P.X+1000,P.Y-700,GroundHeight(P.X+1000,P.Y-700));
   Asset(World,TEXT("SM_DungPile"),Heap,0,1.f);Count++;
  }
 }
 auto* Ocean=World->SpawnActor<AStaticMeshActor>(FVector(0,0,-400),FRotator::ZeroRotator);
 auto* C=Ocean->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);
 C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
 C->SetCollisionEnabled(ECollisionEnabled::NoCollision);Ocean->SetActorScale3D(FVector(2200,2200,1));
 auto* Mat=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_IslandWater.M_IslandWater")),C);
 Mat->SetVectorParameterValue(TEXT("Tint"),FLinearColor::FromSRGBColor(FColor(53,149,189)));C->SetMaterial(0,Mat);
 Animate(0);
 UE_LOG(LogTemp,Display,TEXT("ParkAssets: %d Blender mesh instances, %d species, %d trees; route seed %d"),Count,Habitats.Num(),Trees,Seed);
 return {Trees,Dinosaurs};
}
