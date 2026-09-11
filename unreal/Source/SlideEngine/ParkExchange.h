#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
class UWorld;class AActor;class USceneComponent;
struct FExchangeValue {FString Text,Origin,Id;};
struct FExchangeObject {FString Type,Key;TArray<FExchangeValue> Base,Local,Remote,Result;};
struct FExchangeStep {int32 Phase=0;FString Action,Record,Label,Command,Headline,Note;TArray<FString> States;TArray<int32> Log;};
struct FExchangeRecord {FString Id,Parent,Author,State;};
// Three physical care clipboards in the park: Keeper 1, record, Keeper 2.
class FParkExchange {
 TWeakObjectPtr<AActor> Actor;
 TArray<TWeakObjectPtr<USceneComponent>> Parts;
 struct FTravel {TWeakObjectPtr<USceneComponent> Part;FVector From,To;float Delay=0,Duration=1.5f;bool Bounce=false;};
 TArray<FTravel> Transfers;
 struct FReveal {TWeakObjectPtr<USceneComponent> Part;float At;bool Before=false;};
 TArray<FReveal> Reveals;
 struct FKeeperPart {TWeakObjectPtr<USceneComponent> Part;FVector Rest;int32 Side;};
 TArray<FKeeperPart> Keepers;
 int32 BuiltStep=-1;float BackdropFade=0;
 USceneComponent* Box(FVector At,FVector Size,FColor Tint);
 USceneComponent* Label(FVector At,FVector2D Size,const FString&,int32 Font=30,FColor Tint=FColor::White);
 void Link(FVector From,FVector To,FColor Tint,float Width=5);
 void Keeper(int32 Side);
 void Rebuild();
public:
 explicit FParkExchange(UWorld*,TSharedPtr<FJsonObject> Component);
 ~FParkExchange();
 FString Title;TArray<FExchangeStep> Steps;TArray<FExchangeObject> Objects;TArray<FExchangeRecord> Records;int32 Selected=0;float Age=0;
 int32 MovingValues=0;bool Visible=false;
 const TArray<FExchangeValue>& Values(int32 Board,int32 Type) const;
 void Update(float Delta,FVector Position,FVector Eye,bool Showing);
 void Select(int32 Index){if(Steps.IsValidIndex(Index)){Selected=Index;Age=0;BuiltStep=-1;}}
 bool Advance(int32 Direction){if(!Steps.IsValidIndex(Selected+Direction))return false;Select(Selected+Direction);return true;}
};
