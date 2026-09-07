#include "IslandScene.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace {
AStaticMeshActor* Asset(UWorld* World,const FString& Name,FVector Position=FVector::ZeroVector,float Yaw=0,float Scale=1) {
 const FString Path=FString::Printf(TEXT("/Game/Models/Park/%s.%s"),*Name,*Name);
 auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
 if(!Mesh) { UE_LOG(LogTemp,Fatal,TEXT("Missing Blender asset %s. Run npm run unreal:import-assets."),*Path); return nullptr; }
 auto* Actor=World->SpawnActor<AStaticMeshActor>(Position,FRotator(0,Yaw,0));
 auto* Component=Actor->GetStaticMeshComponent();
 Component->SetMobility(EComponentMobility::Movable);
 Component->SetStaticMesh(Mesh); if(Name==TEXT("SM_Water"))Component->SetCastShadow(false); Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Actor->SetActorScale3D(FVector(Scale)); return Actor;
}
FVector Position(const TArray<TSharedPtr<FJsonValue>>& Values) {
 return FVector(Values[0]->AsNumber(),Values[1]->AsNumber(),Values[2]->AsNumber());
}
}
IslandScene::FStats IslandScene::Build(UWorld* World,const TArray<TSharedPtr<FJsonValue>>& Habitats,int32 Seed) {
 FString Source;
 if(!FFileHelper::LoadFileToString(Source,*(FPaths::ProjectContentDir()/TEXT("Slides/park-assets.json")))) {
  UE_LOG(LogTemp,Fatal,TEXT("Missing park asset placement manifest. Run npm run unreal:import-assets.")); return {};
 }
 TSharedPtr<FJsonObject> Manifest;
 if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source),Manifest) || !Manifest.IsValid()) {
  UE_LOG(LogTemp,Fatal,TEXT("Invalid Blender park placement manifest")); return {};
 }
 // Scenery uses Blender placements; habitat inhabitants still follow the editable deck layout.
 TSet<FString> Species;
 for(const auto& Habitat:Habitats) Species.Add(TEXT("SM_")+Habitat->AsObject()->GetStringField(TEXT("species")));
 int32 Count=0,Trees=0;
 for(const auto& Value:Manifest->GetArrayField(TEXT("placements"))) {
  auto P=Value->AsObject();const FString Name=P->GetStringField(TEXT("asset"));
  if(Name==TEXT("SM_Enclosure") || Species.Contains(Name)) continue;
  Asset(World,Name,Position(P->GetArrayField(TEXT("position")))*100,P->GetNumberField(TEXT("yaw")),P->GetNumberField(TEXT("scale")));Count++;
  if(Name.StartsWith(TEXT("SM_Tree")) || Name==TEXT("SM_Palm")) Trees++;
 }
 for(int32 I=0;I<Habitats.Num();I++) {
  const auto& Value=Habitats[I]; auto H=Value->AsObject();const FVector P=Position(H->GetArrayField(TEXT("position")));
  Asset(World,TEXT("SM_Enclosure"),P,0,1.4f);
  Asset(World,TEXT("SM_")+H->GetStringField(TEXT("species")),P,155,2.7f);Count+=2;
 }
 auto* Ocean=World->SpawnActor<AStaticMeshActor>(FVector(0,0,-400),FRotator::ZeroRotator);
 auto* C=Ocean->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);
 C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
 C->SetCollisionEnabled(ECollisionEnabled::NoCollision);Ocean->SetActorScale3D(FVector(2200,2200,1));
 auto* Mat=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_IslandWater.M_IslandWater")),C);
 Mat->SetVectorParameterValue(TEXT("Tint"),FLinearColor::FromSRGBColor(FColor(53,149,189)));C->SetMaterial(0,Mat);
 UE_LOG(LogTemp,Display,TEXT("ParkAssets: %d Blender mesh instances, %d species, %d trees; route seed %d"),Count,Habitats.Num(),Trees,Seed);
 return {Trees,Habitats.Num()};
}
