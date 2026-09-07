#include "DnaMascot.h"
#include "Engine/World.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

DnaMascot::FResult DnaMascot::Create(UWorld* W) {
 auto* A=W->SpawnActor<AActor>();
 auto* Root=NewObject<USceneComponent>(A); A->SetRootComponent(Root); A->AddInstanceComponent(Root); Root->RegisterComponent();
 A->SetActorLocation(FVector(0,0,-150000));
 auto Part=[&](const TCHAR* Shape,FVector P,FVector Scale,FColor Color,FRotator Rotation=FRotator::ZeroRotator) {
  auto* C=NewObject<UStaticMeshComponent>(A); A->AddInstanceComponent(C); C->SetupAttachment(Root);
  C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),Shape,Shape)));
  auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_IslandLit.M_IslandLit")),A);
  M->SetVectorParameterValue(TEXT("Tint"),FLinearColor::FromSRGBColor(Color)); C->SetMaterial(0,M);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCastShadow(false); C->SetRelativeLocation(P); C->SetRelativeScale3D(Scale); C->SetRelativeRotation(Rotation); C->RegisterComponent();
 };
 auto Link=[&](FVector P,FVector Q,float Width,FColor C) { const FVector D=Q-P; Part(TEXT("Cylinder"),(P+Q)*.5,FVector(Width/100,Width/100,D.Size()/100),C,(D.Rotation().Quaternion()*FRotator(90,0,0).Quaternion()).Rotator()); };
 const FColor Red(231,74,65),Blue(58,126,215),Yellow(243,202,67),Green(80,178,132),White(245,239,206),Black(24,34,37);
 for(int I=0;I<11;I++) {
  const float Angle=I*.65f,Z=-25+I*11;
  const FVector P(20*FMath::Sin(Angle),25*FMath::Cos(Angle),Z),Q(-P.X,-P.Y,Z);
  Part(TEXT("Sphere"),P,FVector(.23),I%2?Red:Blue); Part(TEXT("Sphere"),Q,FVector(.23),I%2?Yellow:Green);
  Link(P,Q,6,White);
 }
 // Two eyes and a friendly open smile on the front of the helix.
 for(float Side:{-1.f,1.f}) {
  Part(TEXT("Sphere"),FVector(26,Side*13,79),FVector(.30,.27,.39),White);
  Part(TEXT("Sphere"),FVector(39,Side*13,79),FVector(.12,.13,.20),Black);
  Link(FVector(0,Side*24,25),FVector(4,Side*55,8),9,Blue);
  Part(TEXT("Sphere"),FVector(4,Side*61,10),FVector(.27,.34,.29),White);
  for(int Finger=0;Finger<3;Finger++) Part(TEXT("Sphere"),FVector(8+Finger*6,Side*68,17),FVector(.12,.16,.18),White);
  Link(FVector(0,Side*12,-30),FVector(2,Side*24,-58),10,Blue);
  Part(TEXT("Sphere"),FVector(11,Side*26,-65),FVector(.43,.29,.20),Red);
 }
 Part(TEXT("Sphere"),FVector(31,0,52),FVector(.12,.30,.20),Black);
 Part(TEXT("Sphere"),FVector(37,0,57),FVector(.07,.22,.07),White);
 auto* Texture=NewObject<UTextureRenderTarget2D>(W); Texture->ClearColor=FLinearColor(.025,.04,.035,1); Texture->InitAutoFormat(384,384); Texture->UpdateResourceImmediate(true);
 auto* Capture=W->SpawnActor<ASceneCapture2D>(A->GetActorLocation()+FVector(310,0,20),FRotator(0,180,0));
 auto* C=Capture->GetCaptureComponent2D(); C->TextureTarget=Texture; C->FOVAngle=38; C->CaptureSource=SCS_FinalColorLDR;
 C->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList; C->ShowOnlyActorComponents(A);
 C->bCaptureEveryFrame=true; C->ShowFlags.SetAtmosphere(false); C->ShowFlags.SetFog(false); C->ShowFlags.SetMotionBlur(false);
 return {A,Capture,Texture};
}
