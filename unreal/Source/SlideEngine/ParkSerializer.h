#pragma once
#include "CoreMinimal.h"
class FJsonObject;class SWidget;
struct FSerializerRow {FString Target,ShortTarget,Key,Type,Value,Path;bool Hidden=false,Excluded=false;};
struct FSerializerNode {FString Name,Path,Type,Object,Value;int32 Parent=-1,Depth=0,Bytes=0;TArray<int32> Rows;};
struct FSerializerSnapshot {FString Id,Message,Tree;TArray<int32> Rows;TArray<FSerializerNode> Nodes;};
class FParkSerializer {
public:
 explicit FParkSerializer(TSharedPtr<FJsonObject>);
 FString Title,Ref,Tree;
 TArray<FSerializerRow> Rows;TArray<FSerializerNode> Nodes;TArray<FSerializerSnapshot> Snapshots;
 TSet<int32> Expanded;int32 SelectedNode=-1,SelectedRow=-1,SelectedCommit=0,Committed=0,Written=0,Scroll=0,TableScroll=0;
 bool Started=false,Serializing=false,AddedMetadata=false,ViewingDraft=true;float Clock=0,ValueScroll=0;
 void Serialize();void AddMetadata();void Tick(float Delta);
 bool Ready()const{return Committed>0&&!Serializing;}
 bool CanSerialize()const{return !Serializing&&(Committed==0||(AddedMetadata&&Committed<Snapshots.Num()));}
 bool CanAddMetadata()const{return Ready()&&!AddedMetadata&&Snapshots.Num()>1;}
 TArray<int32> VisibleNodes()const;TArray<int32> TableRows()const;
 bool IsNewNode(int32 Node)const;
 void ExpandAll();void FoldAll();
 void SelectRow(int32 Row);void SelectNode(int32 Node);void SelectCommit(int32 Commit);
private:
 void LoadSnapshot(int32 Commit);
};
TSharedRef<SWidget> MakeParkSerializerView(TFunction<TSharedPtr<FParkSerializer>()> State,TFunction<void()> Restore);
