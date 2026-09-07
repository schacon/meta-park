#include "ParkCamera.h"
#include "Camera/CameraTypes.h"
void AParkCamera::CalcCamera(float DeltaTime,FMinimalViewInfo& OutResult) {
 Super::CalcCamera(DeltaTime,OutResult);
 // Center the world in the scene area to the right of the terminal sidebar.
 OutResult.OffCenterProjectionOffset=FVector2D(-.2f,0);
}
