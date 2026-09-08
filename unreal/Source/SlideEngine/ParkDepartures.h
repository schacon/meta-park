#pragma once
#include "CoreMinimal.h"

// Precompute random batches against presentation time, so pauses and low frame
// rates never change minute totals or the exact final departure time.
struct FParkDepartures {
 struct FBatch { float Time; int32 Count; };
 int32 Initial=0;
 TArray<FBatch> Batches;
 void Build(int32 People,int32 PerMinute,int32 MaxBatch,FRandomStream& Random) {
  Initial=People;Batches.Reset();
  for(int32 Minute=0,Left=People;Left>0;Minute++) {
   int32 Quota=FMath::Min(Left,PerMinute);Left-=Quota;
   TArray<int32> Counts;TArray<float> Intervals;float Total=0;
   while(Quota>0) {
    const int32 Count=Random.RandRange(1,FMath::Min(MaxBatch,Quota));
    Counts.Add(Count);Quota-=Count;
    const float Interval=Random.FRandRange(1.f,5.f);Intervals.Add(Interval);Total+=Interval;
   }
   float Passed=0;
   for(int32 I=0;I<Counts.Num();I++) {
    Passed+=Intervals[I];
    const float Time=I==Counts.Num()-1?(Minute+1)*60.f:Minute*60.f+60.f*Passed/Total;
    Batches.Add({Time,Counts[I]});
   }
  }
 }
 int32 Remaining(float Seconds) const {
  int32 Count=Initial;
  for(const auto& Batch:Batches) {if(Batch.Time>Seconds)break;Count-=Batch.Count;}
  return FMath::Max(0,Count);
 }
};
