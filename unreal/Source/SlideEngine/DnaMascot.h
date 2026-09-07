#pragma once
#include "CoreMinimal.h"
class UWorld;
class UTextureRenderTarget2D;
class ASceneCapture2D;
namespace DnaMascot {
 struct FResult { AActor* Actor; ASceneCapture2D* Capture; UTextureRenderTarget2D* Texture; };
 FResult Create(UWorld* World);
}
