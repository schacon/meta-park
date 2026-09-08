#include "IslandScene.h"
#include "Engine/StaticMeshActor.h"

namespace {
struct FGateLeaf { TWeakObjectPtr<AStaticMeshActor> Actor; FTransform Closed; float Direction; };
TArray<FGateLeaf> GateLeaves;
float GateProgress=0;
}

void IslandScene::ResetGate() { GateLeaves.Reset();GateProgress=0; }
void IslandScene::RegisterGateLeaf(AStaticMeshActor* Actor,float Direction) {
 GateLeaves.Add({Actor,Actor->GetActorTransform(),Direction});
}
void IslandScene::TickGate(float Delta,bool Open) {
 GateProgress=FMath::Clamp(GateProgress+(Open?1.f:-1.f)*Delta/.95f,0.f,1.f);
 const float Ease=GateProgress*GateProgress*(3-2*GateProgress);
 for(const auto& Leaf:GateLeaves)if(Leaf.Actor.IsValid()) {
  const FQuat Swing=FRotator(0,Leaf.Direction*100*Ease,0).Quaternion();
  Leaf.Actor->SetActorRotation(Leaf.Closed.GetRotation()*Swing);
 }
}
float IslandScene::GateOpenFraction(){return GateProgress;}
bool IslandScene::ValidateGatePose() {
 if(GateLeaves.Num()!=2)return false;
 const float Ease=GateProgress*GateProgress*(3-2*GateProgress);
 for(const auto& Leaf:GateLeaves) {
  if(!Leaf.Actor.IsValid()||!Leaf.Actor->GetActorLocation().Equals(Leaf.Closed.GetLocation(),.01f))return false;
  const FQuat Expected=Leaf.Closed.GetRotation()*FRotator(0,Leaf.Direction*100*Ease,0).Quaternion();
  if(!Leaf.Actor->GetActorQuat().Equals(Expected,.0001f))return false;
 }
 return true;
}
