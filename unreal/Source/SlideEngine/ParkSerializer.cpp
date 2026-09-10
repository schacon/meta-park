#include "ParkSerializer.h"
#include "Dom/JsonObject.h"
FParkSerializer::FParkSerializer(TSharedPtr<FJsonObject> C) {
 Title=C->GetStringField(TEXT("title"));Ref=C->GetStringField(TEXT("ref"));
 for(auto& V:C->GetArrayField(TEXT("rows"))) {auto J=V->AsObject();FSerializerRow R;R.Target=J->GetStringField(TEXT("target"));R.ShortTarget=J->GetStringField(TEXT("shortTarget"));R.Key=J->GetStringField(TEXT("key"));R.Type=J->GetStringField(TEXT("type"));R.Value=J->GetStringField(TEXT("value"));R.Path=J->GetStringField(TEXT("path"));R.Hidden=J->GetBoolField(TEXT("hidden"));R.Excluded=J->GetBoolField(TEXT("excluded"));Rows.Add(R);}
 for(auto& V:C->GetArrayField(TEXT("snapshots"))) {auto J=V->AsObject();FSerializerSnapshot Snapshot;Snapshot.Id=J->GetStringField(TEXT("commit"));Snapshot.Message=J->GetStringField(TEXT("message"));Snapshot.Tree=J->GetStringField(TEXT("tree"));for(auto& R:J->GetArrayField(TEXT("rows")))Snapshot.Rows.Add(R->AsNumber());
  for(auto& Entry:J->GetArrayField(TEXT("nodes"))) {auto NJson=Entry->AsObject();FSerializerNode N;N.Name=NJson->GetStringField(TEXT("name"));N.Path=NJson->GetStringField(TEXT("path"));N.Type=NJson->GetStringField(TEXT("type"));N.Object=NJson->GetStringField(TEXT("object"));N.Value=NJson->GetStringField(TEXT("value"));N.Parent=NJson->GetIntegerField(TEXT("parent"));N.Depth=NJson->GetIntegerField(TEXT("depth"));N.Bytes=NJson->GetIntegerField(TEXT("bytes"));for(auto& R:NJson->GetArrayField(TEXT("rows")))N.Rows.Add(R->AsNumber());Snapshot.Nodes.Add(N);}
  Snapshots.Add(Snapshot);
 }
 LoadSnapshot(0);
}
void FParkSerializer::LoadSnapshot(int32 C){SelectedCommit=C;Nodes=Snapshots[C].Nodes;Tree=Snapshots[C].Tree;SelectedNode=-1;SelectedRow=-1;Scroll=0;TableScroll=0;ValueScroll=0;Expanded.Reset();for(int32 I=0;I<Nodes.Num();I++)Expanded.Add(I);}
void FParkSerializer::Serialize(){if(!CanSerialize())return;LoadSnapshot(Committed==0?0:1);Started=true;Serializing=true;ViewingDraft=false;Written=0;Clock=0;}
void FParkSerializer::AddMetadata(){if(!CanAddMetadata())return;AddedMetadata=true;ViewingDraft=true;TableScroll=0;}
void FParkSerializer::Tick(float Delta){if(Serializing){Clock+=Delta;Written=FMath::Min(Snapshots[SelectedCommit].Rows.Num(),FMath::FloorToInt(Clock/.16f));if(Written==Snapshots[SelectedCommit].Rows.Num()){Serializing=false;Committed=FMath::Max(Committed,SelectedCommit+1);}}}
TArray<int32> FParkSerializer::TableRows()const {return Snapshots[ViewingDraft&&AddedMetadata?1:SelectedCommit].Rows;}
TArray<int32> FParkSerializer::VisibleNodes()const {
 TArray<int32> Result;if(!Started||(Serializing&&Written==0))return Result;
 const auto& ActiveRows=Snapshots[SelectedCommit].Rows;
 for(int32 I=0;I<Nodes.Num();I++) {
  const auto& N=Nodes[I];bool WrittenNode=!Serializing||N.Rows.IsEmpty();
  for(int32 R:N.Rows){const int32 Position=ActiveRows.Find(R);if(Position>=0&&Position<Written)WrittenNode=true;}
  if(!WrittenNode)continue;
  bool Open=true;for(int32 P=N.Parent;P>=0;P=Nodes[P].Parent)if(!Expanded.Contains(P)){Open=false;break;}
  if(Open)Result.Add(I);
 }
 return Result;
}
bool FParkSerializer::IsNewNode(int32 I)const {if(SelectedCommit!=1||!Nodes.IsValidIndex(I))return false;const auto& N=Nodes[I];return N.Type==TEXT("blob")&&N.Rows.ContainsByPredicate([this](int32 R){return Rows[R].Hidden;});}
void FParkSerializer::SelectRow(int32 R){if(!Rows.IsValidIndex(R))return;SelectedRow=R;SelectedNode=-1;ValueScroll=0;const int32 Position=Snapshots[SelectedCommit].Rows.Find(R);if(!Started||Position<0||(Serializing&&Position>=Written))return;for(int32 I=0;I<Nodes.Num();I++)if(Nodes[I].Type==TEXT("blob")&&Nodes[I].Rows.Contains(R)){SelectedNode=I;break;}if(SelectedNode>=0){Expanded.Reset();for(int32 P=Nodes[SelectedNode].Parent;P>=0;P=Nodes[P].Parent)Expanded.Add(P);Scroll=FMath::Max(0,VisibleNodes().Find(SelectedNode)-7);}}
void FParkSerializer::SelectNode(int32 N){ValueScroll=0;SelectedNode=N;SelectedRow=Nodes.IsValidIndex(N)&&!Nodes[N].Rows.IsEmpty()?Nodes[N].Rows[0]:-1;}
void FParkSerializer::SelectCommit(int32 C){if(Serializing||C<0||C>=Committed)return;LoadSnapshot(C);ViewingDraft=false;Written=Snapshots[C].Rows.Num();}

void FParkSerializer::ExpandAll(){Expanded.Reset();for(int32 I=0;I<Nodes.Num();I++)if(Nodes[I].Type==TEXT("tree"))Expanded.Add(I);Scroll=0;}
void FParkSerializer::FoldAll(){Expanded.Reset();Scroll=0;}
