#include "IslandScene.h"
#include "Dom/JsonObject.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace {
struct FHabitat { FVector Position; FString Label, Species; FColor Color; };
FLinearColor Linear(FColor C) { return FLinearColor::FromSRGBColor(C); }
UMaterialInstanceDynamic* Material(UObject* Owner, FColor Color, const TCHAR* Path=TEXT("/Game/Models/M_IslandLit.M_IslandLit")) {
 auto* Base=LoadObject<UMaterialInterface>(nullptr,Path);
 auto* M=UMaterialInstanceDynamic::Create(Base,Owner); M->SetVectorParameterValue(TEXT("Tint"),Linear(Color)); return M;
}
AStaticMeshActor* Shape(UWorld* W, const TCHAR* Name, FVector P, FVector Scale, FColor Color, FRotator R=FRotator::ZeroRotator) {
 auto* A=W->SpawnActor<AStaticMeshActor>(P,R); auto* C=A->GetStaticMeshComponent();
 C->SetMobility(EComponentMobility::Movable); C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),Name,Name)));
 C->SetMaterial(0,Material(C,Color)); A->SetActorScale3D(Scale); return A;
}
void Beam(UWorld* W,FVector A,FVector B,float Width,FColor Color) {
 const FVector D=B-A; Shape(W,TEXT("Cube"),(A+B)*.5,FVector(D.Size()/100,Width/100,Width/100),Color,D.Rotation());
}
void Label(UWorld* W,FString Text,FVector P,float Size,FRotator R,FColor Color=FColor(235,226,195)) {
 auto* A=W->SpawnActor<AActor>(); auto* C=NewObject<UTextRenderComponent>(A); A->SetRootComponent(C); A->AddInstanceComponent(C);
 C->SetText(FText::FromString(Text)); C->SetWorldSize(Size); C->SetHorizontalAlignment(EHTA_Center); C->SetVerticalAlignment(EVRTA_TextCenter);
 C->SetTextRenderColor(Color); C->SetCastShadow(false); C->RegisterComponent(); C->SetWorldLocationAndRotation(P,R);
}
float Coast(float X,float Y) {
 const float N=Y/15000.f;
 if(FMath::Abs(N)>=1) return -FMath::Abs(Y)+14900;
 const float Center=-1000*N+450*FMath::Sin(N*4);
 float Half=8100*FMath::Sqrt(1-N*N)*(.81f+.19f*N)*(1+.065f*FMath::Sin(N*18)+.035f*FMath::Sin(N*39));
 const float Bay=1700*FMath::Exp(-FMath::Square((Y+1800)/4100));
 return X>Center ? Half-Bay-(X-Center) : Half+(X-Center);
}
float Height(float X,float Y,const TArray<FHabitat>& H) {
 const float Edge=Coast(X,Y);
 if(Edge<0) return FMath::Max(-1000.f,Edge*.8f-150);
 const float Ridge=850*FMath::Exp(-FMath::Square((X+2800)/2000)-FMath::Square((Y-5300)/7800));
 const float Peak=450*FMath::Exp(-FMath::Square((X+2300)/1500)-FMath::Square((Y-6300)/2500));
 const float Noise=45*FMath::PerlinNoise2D(FVector2D(X,Y)*.0006)+20*FMath::PerlinNoise2D(FVector2D(X,Y)*.002);
 float Z=130+Ridge+Peak+Noise;
 Z=FMath::Lerp(-140.f,Z,FMath::SmoothStep(0.f,1000.f,Edge));
 for(const auto& Habitat:H) {
  const float D=FVector2D::Distance(FVector2D(X,Y),FVector2D(Habitat.Position));
  Z=FMath::Lerp(Habitat.Position.Z,Z,FMath::SmoothStep(1550.f,2550.f,D));
 }
 return Z;
}
UInstancedStaticMeshComponent* Instances(UWorld* W,const TCHAR* ShapeName,FColor Color) {
 auto* A=W->SpawnActor<AActor>(); auto* C=NewObject<UInstancedStaticMeshComponent>(A); A->SetRootComponent(C); A->AddInstanceComponent(C);
 C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),ShapeName,ShapeName)));
 C->SetMaterial(0,Material(C,Color)); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->RegisterComponent(); return C;
}
void Dinosaur(UWorld* W,const FHabitat& H,FVector Center,float Yaw,float Size) {
 const FTransform T(FRotator(0,Yaw,0),Center,FVector(Size));
 const FColor Skin=H.Color, Dark=FColor(55,65,44), Horn=FColor(217,202,158);
 auto Part=[&](const TCHAR* Name,FVector P,FVector S,FRotator R=FRotator::ZeroRotator,FColor C=FColor::Transparent) {
  return Shape(W,Name,T.TransformPosition(P),S*Size,C==FColor::Transparent?Skin:C,(T.GetRotation()*R.Quaternion()).Rotator());
 };
 const bool Long=H.Species==TEXT("brachiosaurus");
 const bool Horned=H.Species==TEXT("triceratops");
 const bool Stego=H.Species==TEXT("stegosaurus");
 const bool Quad=Long||Horned||Stego;
 Part(TEXT("Sphere"),FVector(0,0,270),FVector(5.2,3.4,3.8));
 // Tail points backward along X (cone's local Z is rotated into -X).
 auto* Tail=Part(TEXT("Cone"),FVector(-425,0,230),FVector(1.6,1.6,6),FRotator(90,0,0));
 (void)Tail;
 for(int L=0;L<(Quad?4:2);L++) {
  float X=Quad?(L<2?-160:160):-95;
  float Y=(L%2==0?-95:95);
  Part(TEXT("Cylinder"),FVector(X,Y,100),FVector(.85,.85,2.3));
  Part(TEXT("Sphere"),FVector(X+40,Y,25),FVector(1.5,.8,.5),FRotator::ZeroRotator,Dark);
 }
 if(Long) {
  Part(TEXT("Cylinder"),FVector(210,0,570),FVector(1.35,1.35,6.8),FRotator(-18,0,0));
  Part(TEXT("Sphere"),FVector(300,0,885),FVector(1.9,1.25,1.2));
 } else {
  Part(TEXT("Sphere"),FVector(270,0,390),FVector(Horned?3.4:3.2,2.3,2.5));
  if(!Quad) for(int J=-1;J<=1;J+=2) Part(TEXT("Cylinder"),FVector(165,J*125,245),FVector(.35,.35,1.6),FRotator(0,0,40));
  if(Horned) {
   Part(TEXT("Sphere"),FVector(175,0,410),FVector(.8,3,2.9));
   for(int J=-1;J<=1;J+=2) Part(TEXT("Cone"),FVector(350,J*60,465),FVector(.45,.45,1.6),FRotator(-50,0,0),Horn);
   Part(TEXT("Cone"),FVector(440,0,400),FVector(.45,.45,1),FRotator(-65,0,0),Horn);
  }
  if(Stego) for(int J=0;J<6;J++) Part(TEXT("Cone"),FVector(-190+J*75,0,440),FVector(1.3,.4,1.7),FRotator::ZeroRotator,Horn);
  if(H.Species==TEXT("dilophosaurus")) for(int J=-1;J<=1;J+=2) Part(TEXT("Sphere"),FVector(220,J*90,410),FVector(.5,2.4,2.4),FRotator(0,0,J*30),Horn);
  if(H.Species==TEXT("parasaurolophus")) Part(TEXT("Cylinder"),FVector(235,0,515),FVector(.45,.45,2.5),FRotator(0,0,65),Horn);
 }
 const FVector Eye=Long?FVector(355,-60,915):FVector(355,-108,450);
 Part(TEXT("Sphere"),Eye,FVector(.65),FRotator::ZeroRotator,FColor(248,245,220));
 Part(TEXT("Sphere"),Eye+FVector(12,-24,0),FVector(.32),FRotator::ZeroRotator,FColor(20,27,30));
}
}

IslandScene::FStats IslandScene::Build(UWorld* W,const TArray<TSharedPtr<FJsonValue>>& Json,int32 Seed) {
 TArray<FHabitat> Habitats;
 for(const auto& V:Json) {
  auto O=V->AsObject(); auto P=O->GetArrayField(TEXT("position"));
  Habitats.Add({FVector(P[0]->AsNumber(),P[1]->AsNumber(),P[2]->AsNumber()),O->GetStringField(TEXT("label")),O->GetStringField(TEXT("species")),FColor::FromHex(O->GetStringField(TEXT("color")))});
 }
 auto* A=W->SpawnActor<AActor>(); auto* Terrain=NewObject<UProceduralMeshComponent>(A); A->SetRootComponent(Terrain); A->AddInstanceComponent(Terrain); Terrain->RegisterComponent();
 TArray<FVector> Vertices,Normals; TArray<int32> Triangles; TArray<FVector2D> UV; TArray<FLinearColor> Colors; TArray<FProcMeshTangent> Tangents;
 constexpr int NX=60,NY=90; constexpr float Step=366.66667f;
 for(int Y=0;Y<=NY;Y++) for(int X=0;X<=NX;X++) {
  const float PX=(X-NX*.5f)*Step,PY=(Y-NY*.5f)*Step,Z=Height(PX,PY,Habitats);
  Vertices.Add(FVector(PX,PY,Z)); UV.Add(FVector2D(X/float(NX),Y/float(NY)));
  Normals.Add(FVector(Height(PX-40,PY,Habitats)-Height(PX+40,PY,Habitats),Height(PX,PY-40,Habitats)-Height(PX,PY+40,Habitats),80).GetSafeNormal());
  Tangents.Add(FProcMeshTangent(1,0,0));
  const float Edge=Coast(PX,PY); FColor C;
  if(Edge<550) C=FColor(183,169,122);
  else if(Z>2600) C=FColor(102,110,100);
  else C=FColor(108,159,102);
  Colors.Add(Linear(C));
 }
 for(int Y=0;Y<NY;Y++) for(int X=0;X<NX;X++) { int I=Y*(NX+1)+X; Triangles.Append({I,I+NX+1,I+1,I+1,I+NX+1,I+NX+2}); }
 Terrain->CreateMeshSection_LinearColor(0,Vertices,Triangles,Normals,UV,Colors,Tangents,false);
 Terrain->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_IslandTerrain.M_IslandTerrain")));
 auto* Ocean=Shape(W,TEXT("Cube"),FVector(0,0,-370),FVector(2200,2200,2),FColor(28,79,102));
 Ocean->GetStaticMeshComponent()->SetMaterial(0,Material(Ocean,FColor(32,89,113),TEXT("/Game/Models/M_IslandWater.M_IslandWater")));
 auto* Trunks=Instances(W,TEXT("Cylinder"),FColor(80,68,45));
 auto* Leaves=Instances(W,TEXT("Sphere"),FColor(69,129,76));
 auto* LeavesLight=Instances(W,TEXT("Sphere"),FColor(102,156,83));
 FRandomStream Random(Seed);
 for(int I=0;I<620;I++) {
  float X=Random.FRandRange(-8500,8500),Y=Random.FRandRange(-14500,14500);
  if(Coast(X,Y)<850) continue;
  bool Clear=false; for(const auto& H:Habitats) if(FVector2D::Distance(FVector2D(X,Y),FVector2D(H.Position))<1900) Clear=true;
  if(Clear) continue;
  const float Z=Height(X,Y,Habitats),Size=Random.FRandRange(.7,1.6);
  Trunks->AddInstance(FTransform(FRotator::ZeroRotator,FVector(X,Y,Z+180*Size),FVector(.38,.38,3.6)*Size),true);
  auto* Canopy=I%2?Leaves:LeavesLight;
  Canopy->AddInstance(FTransform(FRotator(0,Random.FRandRange(0,360),0),FVector(X,Y,Z+370*Size),FVector(5,5,5.5)*Size),true);
 }
 for(int I=0;I<Habitats.Num();I++) {
  const auto& H=Habitats[I];
  // Perimeter fences, raised identification signs and low-poly inhabitants.
  for(int J=0;J<24;J++) {
   const float A0=J*2*PI/24,A1=(J+1)*2*PI/24;
   FVector P=H.Position+FVector(1450*FMath::Cos(A0),1450*FMath::Sin(A0),0),Q=H.Position+FVector(1450*FMath::Cos(A1),1450*FMath::Sin(A1),0);
   Shape(W,TEXT("Cylinder"),P+FVector(0,0,130),FVector(.32,.32,2.6),FColor(87,95,85));
   for(float Z:{100.f,220.f}) Beam(W,P+FVector(0,0,Z),Q+FVector(0,0,Z),10,FColor(150,150,128));
  }
  Label(W,H.Label.ToUpper(),H.Position+FVector(0,-1750,90),160,FRotator(90,90,0));
  Dinosaur(W,H,H.Position+FVector(0,0,0),-25,1.65);
 }
 Label(W,TEXT("ISLA NUBLAR"),FVector(0,-17400,-80),560,FRotator(90,90,0));
 Label(W,TEXT("N"),FVector(0,16700,-50),650,FRotator(90,90,0));
 Beam(W,FVector(0,15500,-40),FVector(0,16250,-40),60,FColor(220,217,180));
 return {Trunks->GetInstanceCount(),Habitats.Num()};
}
