#pragma once
#include "CoreMinimal.h"
class AStaticMeshActor;
class UStaticMeshComponent;

// Rigid Blender limb segments driven by a two-bone IK chain per leg.
// A stance foot is a world-space anchor, independent of the moving body.
class FDinosaurGait {
 struct FLeg {
  TWeakObjectPtr<UStaticMeshComponent> Upper,Lower,Foot;
  FVector Hip,Knee,Ankle,Pole;
  float Offset=0,UpperLength=0,LowerLength=0;
  FVector Plant,SwingFrom,SwingTo,PreviousFoot;
  FQuat PlantRotation,SwingRotation;
  float LiftCycle=0,PreviousPhase=0,LiftTime=0,SwingDuration=1;
  bool Swing=false,PreviousSwing=false;
  int32 Steps=0;
 };
 TArray<FLeg> Legs;
 float Scale=1,Stride=60,Duty=.78f,Lift=22,Cycle=0,PreviousTime=-1;
 FTransform PreviousPose;
 bool Initialized=false;
 float MaxSlip=0,MaxReachError=0,MaxBoneError=0,MaxContactError=0,MaxLift=0;
public:
 FDinosaurGait(AStaticMeshActor* Actor,const FString& Species);
 void Update(const FTransform& Pose,float Seconds,const TFunction<FTransform(float)>& PredictPose,const TFunction<float(FVector)>& Ground,bool Apply=true);
 void Reset();
 bool Validate() const;
 bool AllFeetPlanted() const { for(const auto& L:Legs)if(L.Swing)return false;return true; }
};
