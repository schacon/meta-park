#pragma once
#include "CoreMinimal.h"
class FJsonObject;
class SWidget;
struct FParkFSVSystem { FString Label,Meta; };
class FParkFSV {
public:
 explicit FParkFSV(TSharedPtr<FJsonObject> Component);
 FString Title;
 TArray<FParkFSVSystem> Systems;
 TArray<float> Heights;
 int32 Selected=INDEX_NONE;
 void Tick(float Delta);
 bool Advance(int32 Direction);
 void Select(int32 System){Selected=FMath::Clamp(System,INDEX_NONE,Systems.Num()-1);}
};
TSharedRef<SWidget> MakeParkFSVView(TFunction<TSharedPtr<FParkFSV>()> State);
