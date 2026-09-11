#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
class UMaterialInstanceDynamic;
class UWorld;class AActor;class ASceneCapture2D;class USceneComponent;class UPrimitiveComponent;class UWidgetComponent;class UStaticMeshComponent;class FParkScalar;

// An isolated native scene rendered into the desktop's viewport.
class FParkScalarScene {
 TWeakObjectPtr<AActor> Actor;
 TWeakObjectPtr<ASceneCapture2D> Capture;
 TArray<TWeakObjectPtr<USceneComponent>> Objects;
 TMap<uint64,TWeakObjectPtr<UMaterialInstanceDynamic>> ColorMaterials;
 UStaticMeshComponent* Box(FVector Center,FVector Size,FColor Color,bool Dynamic=true,bool Lit=true);
 void Link(FVector From,FVector To,FColor Color,float Width=3,bool Dashed=false,bool Dynamic=true);
 UWidgetComponent* Label(FVector Center,FVector2D Size,const TArray<FString>& Lines,FColor Color=FColor::White,int32 HighlightRow=-1);
 void Rebuild(const FParkScalar& State);
 struct FTransfer {TWeakObjectPtr<USceneComponent> Component;FVector Destination;};
 TArray<FTransfer> Transfers;
 float TransferOffset=0;
 void TrackTransfer(int32 Start,float Offset);
 struct FCreation {TWeakObjectPtr<USceneComponent> Component;FVector Destination,Scale,Pivot;float Start;FString Name,Description;};
 struct FReveal {TWeakObjectPtr<USceneComponent> Component;float At;};
 TArray<FCreation> Creations;
 TArray<FReveal> Reveals;
 void TrackCreation(int32 Start,FVector Pivot,float At,const FString& Name,const FString& Description);
 void RevealAfter(int32 Start,float At);
 int32 BuiltStep=-1;
 bool CameraReady=false;
public:
 FSlateBrush Brush;
 FVector Focus=FVector::ZeroVector,DesiredFocus=FVector::ZeroVector;
 float Distance=4200,DesiredDistance=4200;
 int32 SolidBoxes=0,TopLabels=0,TransferringObjects=0;
 float TransferProgress=1,CreationEndTime=0;
 FString CreationStatus;
 bool CreationObjectVisible(const FString& Name) const;
 int32 GetStep()const{return BuiltStep;}
 bool LabelsFit=true;
 FParkScalarScene(UWorld*);
 ~FParkScalarScene();
 void Update(const FParkScalar&,float Delta);
 FVector CameraLocation() const;
 bool IsValid() const;
};
