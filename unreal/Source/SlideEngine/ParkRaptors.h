#pragma once
#include "CoreMinimal.h"
class UWorld;class AActor;class ACameraActor;class USceneComponent;class UWidgetComponent;class UPointLightComponent;class FJsonObject;
struct FRaptorIssues {FString Label;TArray<FString> Problems;int32 Workers=0;};
class FParkRaptors {
 TWeakObjectPtr<AActor> Actor;
 TArray<UWidgetComponent*> Labels;
 USceneComponent *Clipboard=nullptr,*Paper=nullptr;
 UPointLightComponent* Spotlight=nullptr;
 TArray<USceneComponent*> Ring;
 FVector OriginalEye,OriginalLook;
 FQuat OriginalRotation;
 float OriginalWidth=0,Flight=0,Flip=1;
 int32 Displayed=INDEX_NONE,PreviousDisplayed=INDEX_NONE;
 TSharedRef<class SWidget> BuildRecord(TFunction<int32()> Record);
public:
 TArray<FRaptorIssues> Raptors;
 int32 Selected=INDEX_NONE;
 FParkRaptors(UWorld*,TSharedPtr<FJsonObject>,ACameraActor*,FVector Look,float Width);
 ~FParkRaptors();
 bool Advance(int32 Direction);
 void Update(float Delta,ACameraActor*,FVector& Look,float& Width);
 void Restore(ACameraActor*,FVector& Look,float& Width);
 bool Ready()const{return Flip==1&&Flight==1;}
 FString Label()const;
 FString Problems()const;
 int32 Workers()const;
};
