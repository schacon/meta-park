#pragma once
#include "CoreMinimal.h"
class AActor;
class ACameraActor;
class UWorld;
class USceneComponent;
class UStaticMeshComponent;
class FJsonObject;
struct FColdSpecimen {FString Species,Description;};
class FParkColdStorage {
 TWeakObjectPtr<AActor> Actor;
 USceneComponent* Rack=nullptr;
 UStaticMeshComponent* Lid=nullptr;
 TArray<USceneComponent*> Vials;
 float Reveal=0,Angle=0;
 int32 Moving=INDEX_NONE;
 enum class EPhase {Idle,Return,Rotate,Extract};
 EPhase Phase=EPhase::Idle;
public:
 explicit FParkColdStorage(UWorld* World,TSharedPtr<FJsonObject> Component);
 ~FParkColdStorage();
 TArray<FColdSpecimen> Specimens;
 int32 Selected=INDEX_NONE;
 float Lift=0;
 bool Advance(int32 Direction);
 void Update(float Delta,ACameraActor* Camera,bool Visible);
 bool Ready() const{return Selected>=0&&Phase==EPhase::Idle&&Lift==1;}
 bool IsHidden() const{return Reveal==0;}
 FString Species() const;
 FString Description() const;
};
