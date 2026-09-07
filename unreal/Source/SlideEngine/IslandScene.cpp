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
 const float N=Y/13200.f;
 if(FMath::Abs(N)>=1) return -FMath::Abs(Y)+13100;
 const float Half=10800*FMath::Sqrt(1-N*N)*(1+.045f*FMath::Sin(N*17)+.025f*FMath::Sin(N*31));
 return Half-FMath::Abs(X+250*FMath::Sin(N*5));
}
float Height(float X,float Y,const TArray<FHabitat>& H) {
 const float Edge=Coast(X,Y);
 if(Edge<0) return FMath::Max(-1000.f,Edge*.65f-140);
 const float D=FVector2D::Distance(FVector2D(X,Y),FVector2D(0,7500));
 float Z=200+FMath::Max(0.f,1-D/6100)*6500;
 Z=FMath::Min(Z,5600.f);
 Z=FMath::Lerp(-140.f,Z,FMath::SmoothStep(0.f,950.f,Edge));
 const float Lake=FVector2D::Distance(FVector2D(X,Y),FVector2D(0,800));
 Z=FMath::Lerp(-70.f,Z,FMath::SmoothStep(1400.f,2050.f,Lake));
 for(const auto& Habitat:H) {
  const float HD=FVector2D::Distance(FVector2D(X,Y),FVector2D(Habitat.Position));
  Z=FMath::Lerp(Habitat.Position.Z,Z,FMath::SmoothStep(1600.f,2400.f,HD));
 }
 return Z;
}
void Facet(UWorld* W,FVector P,FVector Scale,FColor Color) {
 auto* A=W->SpawnActor<AActor>(); auto* Mesh=NewObject<UProceduralMeshComponent>(A); A->SetRootComponent(Mesh); A->AddInstanceComponent(Mesh); Mesh->RegisterComponent();
 TArray<FVector> V,N; TArray<int32> T; TArray<FVector2D> UV; TArray<FLinearColor> Colors; TArray<FProcMeshTangent> Tangents;
 auto Face=[&](FVector A0,FVector B,FVector C){int I=V.Num();const FVector Normal=FVector::CrossProduct(C-A0,B-A0).GetSafeNormal();for(auto Q:{A0,B,C}){V.Add(P+Q*Scale);N.Add(Normal);UV.Add(FVector2D::ZeroVector);Colors.Add(Linear(Color));}T.Append({I,I+1,I+2});};
 for(int I=0;I<7;I++) {
  float A0=I*2*PI/7,B0=(I+1)*2*PI/7;
  FVector L(FMath::Cos(A0)*.75,FMath::Sin(A0)*.75,-.35),R(FMath::Cos(B0)*.75,FMath::Sin(B0)*.75,-.35);
  FVector U(FMath::Cos(A0),FMath::Sin(A0),.35),UR(FMath::Cos(B0),FMath::Sin(B0),.35);
  Face(FVector(0,0,-1),L,R);Face(L,U,R);Face(R,U,UR);Face(U,FVector(0,0,1),UR);
 }
 Mesh->CreateMeshSection_LinearColor(0,V,T,N,UV,Colors,Tangents,false);
 Mesh->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_IslandTerrain.M_IslandTerrain")));
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
 constexpr int NX=32,NY=38; constexpr float Step=760;
 for(int Y=0;Y<=NY;Y++) for(int X=0;X<=NX;X++) {
  const float PX=(X-NX*.5f)*Step,PY=(Y-NY*.5f)*Step,Z=Height(PX,PY,Habitats);
  Vertices.Add(FVector(PX,PY,Z)); UV.Add(FVector2D(X/float(NX),Y/float(NY)));
  Normals.Add(FVector(Height(PX-40,PY,Habitats)-Height(PX+40,PY,Habitats),Height(PX,PY-40,Habitats)-Height(PX,PY+40,Habitats),80).GetSafeNormal());
  Tangents.Add(FProcMeshTangent(1,0,0));
  const float Edge=Coast(PX,PY); FColor C;
  if(Edge<650) C=FColor(245,222,147);
  else if(Z>2700) C=FColor(147,130,107);
  else C=FColor(122,163,65);
  Colors.Add(Linear(C));
 }
 for(int Y=0;Y<NY;Y++) for(int X=0;X<NX;X++) { int I=Y*(NX+1)+X; Triangles.Append({I,I+NX+1,I+1,I+1,I+NX+1,I+NX+2}); }
 // Separate triangle vertices and face normals retain the mockup's faceted terrain.
 TArray<FVector> FV,FN; TArray<int32> FT; TArray<FVector2D> FU; TArray<FLinearColor> FC;
 for(int I=0;I<Triangles.Num();I+=3) {
  const FVector A0=Vertices[Triangles[I]],B=Vertices[Triangles[I+1]],C=Vertices[Triangles[I+2]];
  const FVector Normal=FVector::CrossProduct(C-A0,B-A0).GetSafeNormal();
  const float Tone=.94f+(I%11)*.012f;
  for(int J=0;J<3;J++){int K=Triangles[I+J];FT.Add(FV.Num());FV.Add(Vertices[K]);FN.Add(Normal);FU.Add(UV[K]);FC.Add(Colors[K]*Tone);}
 }
 Terrain->CreateMeshSection_LinearColor(0,FV,FT,FN,FU,FC,Tangents,false);
 Terrain->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_IslandTerrain.M_IslandTerrain")));
 auto* Ocean=Shape(W,TEXT("Cube"),FVector(0,0,-370),FVector(2200,2200,2),FColor(28,79,102));
 Ocean->GetStaticMeshComponent()->SetMaterial(0,Material(Ocean,FColor(32,89,113),TEXT("/Game/Models/M_IslandWater.M_IslandWater")));
 // Bright shallow-water bands around the coast, below the raised land.
 for(float Factor:{1.075f,1.035f}) {
  TArray<FVector> CV;TArray<int32> CT;TArray<FVector> CN;TArray<FVector2D> CU;TArray<FLinearColor> CC;
  CV.Add(FVector(0,0,-270+(1.1f-Factor)*500));CN.Add(FVector::UpVector);CU.Add(FVector2D::ZeroVector);CC.Add(Linear(FColor(91,195,210)));
  for(int J=0;J<=96;J++){float Angle=J*2*PI/96;float Y=13200*FMath::Sin(Angle),N0=Y/13200;float X=10800*FMath::Cos(Angle)*(1+.045f*FMath::Sin(N0*17)+.025f*FMath::Sin(N0*31));CV.Add(FVector(X*Factor,Y*Factor,CV[0].Z));CN.Add(FVector::UpVector);CU.Add(FVector2D::ZeroVector);CC.Add(Linear(Factor>1.05?FColor(69,173,203):FColor(108,214,214)));if(J>0)CT.Append({0,J+1,J});}
  auto* Band=W->SpawnActor<AActor>();auto* M=NewObject<UProceduralMeshComponent>(Band);Band->SetRootComponent(M);Band->AddInstanceComponent(M);M->RegisterComponent();M->CreateMeshSection_LinearColor(0,CV,CT,CN,CU,CC,Tangents,false);M->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_IslandTerrain.M_IslandTerrain")));
 }
 FRandomStream Random(Seed); int32 Trees=0;
 for(int I=0;I<300;I++) {
  float X=Random.FRandRange(-9500,9500),Y=Random.FRandRange(-12000,12000);
  if(Coast(X,Y)<1000 || Height(X,Y,Habitats)>2200 || FVector2D::Distance(FVector2D(X,Y),FVector2D(0,800))<2200) continue;
  bool Clear=false; for(const auto& H:Habitats) if(FVector2D::Distance(FVector2D(X,Y),FVector2D(H.Position))<1900) Clear=true;
  if(Clear || (FMath::Abs(X)<2600 && Y<0)) continue;
  float Z=Height(X,Y,Habitats),S=Random.FRandRange(.8,1.4);
  Shape(W,TEXT("Cylinder"),FVector(X,Y,Z+180*S),FVector(.65,.65,3.6)*S,FColor(112,87,49));
  Facet(W,FVector(X,Y,Z+480*S),FVector(300,280,380)*S,I%2?FColor(76,133,45):FColor(113,165,52));Trees++;
 }
 // Central lagoon, rocky waterfall and a few palm silhouettes.
 auto* Lake=Shape(W,TEXT("Cylinder"),FVector(0,800,55),FVector(33,33,.1),FColor(61,173,196));
 Lake->GetStaticMeshComponent()->SetMaterial(0,Material(Lake,FColor(64,181,208),TEXT("/Game/Models/M_IslandWater.M_IslandWater")));
 for(int J=-2;J<=2;J++) if(J!=0) Facet(W,FVector(J*310,2300,470),FVector(420,400,640),FColor(136,137,116));
 Shape(W,TEXT("Cube"),FVector(0,2220,610),FVector(3.2,1.5,10.5),FColor(94,220,230));
 Shape(W,TEXT("Sphere"),FVector(0,2060,120),FVector(7,5,.5),FColor(167,240,228));
 for(FVector P:{FVector(-2300,-1200,200),FVector(2400,-1500,200),FVector(-8300,0,200),FVector(8500,-1000,200),FVector(-2000,-10500,200),FVector(2000,-10500,200)}) {
  Shape(W,TEXT("Cylinder"),P+FVector(0,0,350),FVector(.65,.65,7),FColor(138,112,56));
  for(int J=0;J<6;J++){float A0=J*2*PI/6;FVector Q(FMath::Cos(A0)*290,FMath::Sin(A0)*290,650);Facet(W,P+Q,FVector(400,75,50),FColor(64,135,67));}Trees++;
 }
 auto Road=[&](FVector P,FVector Q){const int Steps=FMath::Max(1,FMath::CeilToInt(FVector::Distance(P,Q)/350));for(int J=0;J<Steps;J++){FVector A0=FMath::Lerp(P,Q,float(J)/Steps),B=FMath::Lerp(P,Q,float(J+1)/Steps);A0.Z=Height(A0.X,A0.Y,Habitats)+12;B.Z=Height(B.X,B.Y,Habitats)+12;Beam(W,A0,B,125,FColor(225,216,161));}};
 for(const auto& H:Habitats){const float Side=H.Position.X>0?1:-1;Road(H.Position+FVector(-Side*1500,0,0),FVector(Side*2600,H.Position.Y,0));Road(FVector(Side*2600,H.Position.Y,0),FVector(Side*2600,-10300,0));}
 Road(FVector(-2600,-10300,0),FVector(2600,-10300,0));Road(FVector(0,-10300,0),FVector(0,-11600,0));
 // Visitor buildings, entrance, helipad and dock are landmarks, not extra slides.
 for(int J=0;J<3;J++){FVector P(-1000+J*900,-5800+J*250,350);Shape(W,TEXT("Cube"),P,FVector(7,7,4),FColor(224,228,211));Shape(W,TEXT("Cube"),P+FVector(0,0,260),FVector(5.2,5.2,1.2),FColor(194,203,192));Shape(W,TEXT("Cube"),P+FVector(0,-355,0),FVector(1.8,.1,2.4),FColor(48,97,111));}
 for(float X:{-950.f,950.f}) {Shape(W,TEXT("Cube"),FVector(X,-11200,1000),FVector(4.5,6,16),FColor(142,146,132));Shape(W,TEXT("Cone"),FVector(X,-11200,1900),FVector(1.5,1.5,3),FColor(246,151,35));}
 Shape(W,TEXT("Cube"),FVector(0,-11200,770),FVector(14,2,11),FColor(136,89,48));
 Shape(W,TEXT("Cube"),FVector(0,-11200,1520),FVector(15,2.2,4),FColor(232,204,118));
 Label(W,TEXT("JURASSIC PARK"),FVector(0,-11325,1520),175,FRotator(0,-90,0),FColor(221,88,32));
 Shape(W,TEXT("Cylinder"),FVector(-7100,-6300,270),FVector(13,13,.7),FColor(185,190,170));Label(W,TEXT("H"),FVector(-7100,-6300,311),650,FRotator(90,90,0));
 Shape(W,TEXT("Cube"),FVector(1200,-13300,-90),FVector(3.5,32,1),FColor(160,113,65));Shape(W,TEXT("Cube"),FVector(500,-14700,-90),FVector(18,3.5,1),FColor(160,113,65));
 Shape(W,TEXT("Cube"),FVector(300,-14900,-75),FVector(9,4,1.5),FColor(230,231,217));Shape(W,TEXT("Cube"),FVector(300,-14900,70),FVector(5,3,1.6),FColor(198,218,216));
 for(int I=0;I<Habitats.Num();I++) {
  const auto& H=Habitats[I];
  // Perimeter fences, raised identification signs and low-poly inhabitants.
  for(int J=0;J<24;J++) {
   const float A0=J*2*PI/24,A1=(J+1)*2*PI/24;
   FVector P=H.Position+FVector(1450*FMath::Sign(FMath::Cos(A0))*FMath::Sqrt(FMath::Abs(FMath::Cos(A0))),1450*FMath::Sign(FMath::Sin(A0))*FMath::Sqrt(FMath::Abs(FMath::Sin(A0))),0),Q=H.Position+FVector(1450*FMath::Sign(FMath::Cos(A1))*FMath::Sqrt(FMath::Abs(FMath::Cos(A1))),1450*FMath::Sign(FMath::Sin(A1))*FMath::Sqrt(FMath::Abs(FMath::Sin(A1))),0);
   Shape(W,TEXT("Cylinder"),P+FVector(0,0,130),FVector(.55,.55,2.6),FColor(232,230,207));
   for(float Z:{100.f,220.f}) Beam(W,P+FVector(0,0,Z),Q+FVector(0,0,Z),28,FColor(222,229,200));
  }
  Dinosaur(W,H,H.Position+FVector(0,0,0),-25,1.65);
 }
 return {Trees,Habitats.Num()};
}
