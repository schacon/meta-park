#include "DinosaurGait.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
namespace {
FVector Vec(const TSharedPtr<FJsonObject>& O,const TCHAR* Key) {
 const auto& A=O->GetArrayField(Key);return FVector(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber())*100;
}
UStaticMesh* Mesh(const FString& Name) {
 auto* Result=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Models/Park/%s.%s"),*Name,*Name));
 checkf(Result,TEXT("Missing articulated dinosaur part %s. Run scripts/import_dinosaur_rigs.py."),*Name);return Result;
}
UStaticMeshComponent* Part(AStaticMeshActor* Actor,const FString& Name) {
 auto* C=NewObject<UStaticMeshComponent>(Actor);
 C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh(Name));
 C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Actor->AddInstanceComponent(C);C->SetupAttachment(Actor->GetRootComponent());C->RegisterComponent();return C;
}
}
FDinosaurGait::FDinosaurGait(AStaticMeshActor* Actor,const FString& Species) {
 FString Source;TSharedPtr<FJsonObject> Data;
 check(FFileHelper::LoadFileToString(Source,*(FPaths::ProjectContentDir()/TEXT("Slides/dinosaur-rigs.json"))));
 check(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source),Data));
 Scale=Actor->GetActorScale3D().X;
 for(const auto& Value:Data->GetArrayField(TEXT("rigs"))) {
  const auto R=Value->AsObject();if(R->GetStringField(TEXT("species"))!=Species)continue;
  Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh(R->GetStringField(TEXT("body"))));
  Stride=R->GetNumberField(TEXT("stride"))*100;Duty=R->GetNumberField(TEXT("duty"));Lift=R->GetNumberField(TEXT("lift"))*100;
  for(const auto& V:R->GetArrayField(TEXT("legs"))) {
   const auto L=V->AsObject();FLeg Leg;
   Leg.Hip=Vec(L,TEXT("hip"));Leg.Knee=Vec(L,TEXT("knee"));Leg.Ankle=Vec(L,TEXT("ankle"));Leg.Pole=Vec(L,TEXT("pole")).GetSafeNormal();
   Leg.UpperLength=FVector::Distance(Leg.Hip,Leg.Knee)*Scale;Leg.LowerLength=FVector::Distance(Leg.Knee,Leg.Ankle)*Scale;
   Leg.Offset=L->GetNumberField(TEXT("offset"));
   Leg.Upper=Part(Actor,L->GetStringField(TEXT("upper")));Leg.Lower=Part(Actor,L->GetStringField(TEXT("lower")));Leg.Foot=Part(Actor,L->GetStringField(TEXT("foot")));
   Legs.Add(Leg);
  }
  break;
 }
 checkf(Legs.Num()>0,TEXT("No articulated rig for %s"),*Species);
}
void FDinosaurGait::Reset() {
 Initialized=false;Cycle=0;PreviousTime=-1;MaxSlip=MaxReachError=MaxBoneError=MaxContactError=MaxLift=0;
 for(auto& L:Legs){L.Swing=false;L.PreviousSwing=false;L.Steps=0;}
}
void FDinosaurGait::Update(const FTransform& Pose,float Seconds,const TFunction<FTransform(float)>& PredictPose,const TFunction<float(FVector)>& Ground,bool Apply) {
 const float Delta=Seconds-PreviousTime;
 auto Grounded=[&](const FLeg& L,const FTransform& P) {
  FVector Foot=P.TransformPosition(L.Ankle);Foot.Z=Ground(Foot)+L.Ankle.Z*Scale;return Foot;
 };
 if(!Initialized||Delta<0||Delta>2) {
  Cycle=0;Initialized=true;PreviousPose=Pose;
  for(auto& L:Legs) {
   L.Plant=Grounded(L,Pose);L.PlantRotation=Pose.GetRotation();L.Swing=false;L.PreviousSwing=false;
   L.PreviousFoot=L.Plant;L.PreviousPhase=L.Offset;
  }
 }
 float Moved=0;
 for(const auto& L:Legs)Moved=FMath::Max(Moved,float(FVector::Dist2D(Pose.TransformPosition(L.Ankle),PreviousPose.TransformPosition(L.Ankle))));
 Cycle+=Moved/(Stride*Scale);
 const float Speed=Delta>0?Moved/Delta:0;
 const float Period=FMath::Clamp(Stride*Scale/FMath::Max(Speed,1.f),2.f,24.f);
 for(auto& L:Legs) {
  const float Phase=FMath::Frac(Cycle+L.Offset);
  if(!L.Swing&&Phase>=Duty&&L.PreviousPhase<Duty) {
   L.Swing=true;L.LiftCycle=Cycle;L.SwingFrom=L.Plant;L.Steps++;
   L.LiftTime=Seconds;L.SwingDuration=FMath::Clamp(Period*(1-Duty),.4f,1.4f);
   // Land ahead of the body, then let it pass over a stationary support foot.
   const FTransform Ahead=PredictPose(Seconds+Period*((1-Duty)+Duty*.35f));
   L.SwingTo=Grounded(L,Ahead);L.SwingRotation=Ahead.GetRotation();
   const FVector Nominal=Grounded(L,Pose);
   FVector Lead=L.SwingTo-Nominal;Lead.Z=0;Lead=Lead.GetClampedToMaxSize(Stride*Scale*.65f);
   L.SwingTo=Nominal+Lead;L.SwingTo.Z=Ground(L.SwingTo)+L.Ankle.Z*Scale;
  }
  FVector Target=L.Plant;FQuat FootRotation=L.PlantRotation;float SwingT=0;
  if(L.Swing) {
   // Finish a lifted step even when the body eases into a rest. Supporting
   // feet stay planted, and resting dinosaurs never freeze with a foot aloft.
   SwingT=FMath::Clamp(FMath::Max((Cycle-L.LiftCycle)/(1-Duty),(Seconds-L.LiftTime)/L.SwingDuration),0.f,1.f);
   const float Smooth=SwingT*SwingT*(3-2*SwingT);
   Target=FMath::Lerp(L.SwingFrom,L.SwingTo,Smooth);
   const float Height=Lift*Scale*FMath::Sin(PI*SwingT);Target.Z+=Height;MaxLift=FMath::Max(MaxLift,Height);
   FootRotation=FQuat::Slerp(L.PlantRotation,L.SwingRotation,Smooth)*FRotator(10*FMath::Sin(PI*SwingT),0,0).Quaternion();
   if(SwingT>=1) {L.Plant=L.SwingTo;L.PlantRotation=L.SwingRotation;L.Swing=false;Target=L.Plant;FootRotation=L.PlantRotation;}
  }
  const FVector Hip=Pose.TransformPosition(L.Hip),Direction=(Target-Hip).GetSafeNormal();
  const float Requested=FVector::Distance(Hip,Target);
  const float Distance=FMath::Clamp(Requested,FMath::Abs(L.UpperLength-L.LowerLength)+.01f,L.UpperLength+L.LowerLength-.01f);
  MaxReachError=FMath::Max(MaxReachError,FMath::Abs(Requested-Distance));
  FVector Bend=Pose.GetRotation().RotateVector(L.Pole);Bend=(Bend-Direction*FVector::DotProduct(Bend,Direction)).GetSafeNormal();
  const float Along=(L.UpperLength*L.UpperLength-L.LowerLength*L.LowerLength+Distance*Distance)/(2*Distance);
  const FVector Knee=Hip+Direction*Along+Bend*FMath::Sqrt(FMath::Max(0.f,L.UpperLength*L.UpperLength-Along*Along));
  const FQuat Upper=FQuat::FindBetweenNormals(Pose.GetRotation().RotateVector((L.Knee-L.Hip).GetSafeNormal()),(Knee-Hip).GetSafeNormal())*Pose.GetRotation();
  const FQuat Lower=FQuat::FindBetweenNormals(Pose.GetRotation().RotateVector((L.Ankle-L.Knee).GetSafeNormal()),(Target-Knee).GetSafeNormal())*Pose.GetRotation();
  MaxBoneError=FMath::Max(MaxBoneError,float(FMath::Abs(FVector::Distance(Hip,Knee)-L.UpperLength)));
  MaxBoneError=FMath::Max(MaxBoneError,float(FMath::Abs(FVector::Distance(Knee,Target)-L.LowerLength)));
  if(!L.Swing&&!L.PreviousSwing)MaxSlip=FMath::Max(MaxSlip,float(FVector::Distance(Target,L.PreviousFoot)));
  if(!L.Swing)MaxContactError=FMath::Max(MaxContactError,FMath::Abs(float(Target.Z-L.Ankle.Z*Scale-Ground(Target))));
  if(Apply) {
   L.Upper->SetWorldTransform(FTransform(Upper,Hip,FVector(Scale)));
   L.Lower->SetWorldTransform(FTransform(Lower,Knee,FVector(Scale)));
   L.Foot->SetWorldTransform(FTransform(FootRotation,Target,FVector(Scale)));
  }
  L.PreviousPhase=Phase;L.PreviousFoot=Target;L.PreviousSwing=L.Swing;
 }
 PreviousPose=Pose;PreviousTime=Seconds;
}
bool FDinosaurGait::Validate() const {
 bool Stepped=true;for(const auto& L:Legs)Stepped&=L.Steps>=2;
 const bool Good=Stepped&&MaxSlip<.01f&&MaxReachError<.1f&&MaxBoneError<.1f&&MaxContactError<.1f&&MaxLift>Lift*Scale*.8f;
 UE_LOG(LogTemp,Display,TEXT("DinosaurGait: %d legs steps=%d planted slip=%.3fcm reach=%.3fcm bone error=%.3fcm contact=%.3fcm lift=%.1fcm %s"),Legs.Num(),Stepped?1:0,MaxSlip,MaxReachError,MaxBoneError,MaxContactError,MaxLift,Good?TEXT("PASS"):TEXT("FAIL"));
 return Good;
}
