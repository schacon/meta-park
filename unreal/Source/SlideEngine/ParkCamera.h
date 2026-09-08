#pragma once
#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "ParkCamera.generated.h"
UCLASS()
class SLIDEENGINE_API AParkCamera : public ACameraActor {
 GENERATED_BODY()
public:
 void FrameScene(float FrameWidth, float LookDistance);
};
