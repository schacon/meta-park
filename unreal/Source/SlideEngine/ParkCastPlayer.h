#pragma once
#include "CoreMinimal.h"
class FJsonObject;
class FJsonValue;
class SWidget;
struct FParkCastRun { int32 Column,Width,Flags;FString Text;FLinearColor Foreground,Background; };
class FParkCastPlayer {
public:
 explicit FParkCastPlayer(TSharedPtr<FJsonObject> Recording);
 void Tick(float Delta);
 void Seek(float Seconds);
 void Restart(){Paused=false;Seek(0);}
 void TogglePlayback(){if(Time>=Duration)Restart();else Paused=!Paused;}
 FString PlainText() const;
 float Time=0,Duration=0;
 bool Paused=false,CursorVisible=true;
 int32 Columns=80,Rows=24,CursorX=0,CursorY=0;
 FString Title;
 FLinearColor Foreground,Background;
 TArray<TArray<FParkCastRun>> Lines;
private:
 TArray<TSharedPtr<FJsonValue>> Frames;
 int32 NextFrame=0;
 void ApplyThroughTime();
};
TSharedRef<SWidget> MakeParkCastView(TFunction<TSharedPtr<FParkCastPlayer>()> Player);
