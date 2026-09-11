#include "ParkScalar.h"
#include "ParkScalarScene.h"
FParkScalar::FParkScalar(TSharedPtr<FJsonObject> C,UWorld* World) {
 Scene=MakeShared<FParkScalarScene>(World);
 Title=C->GetStringField(TEXT("title"));
 for(const auto& V:C->GetArrayField(TEXT("commits"))){auto J=V->AsObject();FScalarCommit R;R.Id=J->GetStringField(TEXT("id"));R.Tree=J->GetStringField(TEXT("tree"));R.Parent=J->GetStringField(TEXT("parent"));R.Delta=J->GetStringField(TEXT("delta"));for(const auto& E:J->GetArrayField(TEXT("entries")))R.Entries.Add(E->AsNumber());Commits.Add(R);}
 for(const auto& V:C->GetArrayField(TEXT("steps"))){auto J=V->AsObject();FScalarStep R;R.Action=J->GetStringField(TEXT("action"));R.Room=J->GetIntegerField(TEXT("room"));R.From=J->GetIntegerField(TEXT("from"));R.Commit=J->GetIntegerField(TEXT("commit"));R.Label=J->GetStringField(TEXT("label"));R.Actor=J->GetStringField(TEXT("actor"));R.Description=J->GetStringField(TEXT("description"));R.Command=J->GetStringField(TEXT("command"));R.Detail=J->GetStringField(TEXT("detail"));for(const auto& E:J->GetArrayField(TEXT("spaces"))){auto O=E->AsObject();FScalarSpace P;P.FirstCommit=O->GetIntegerField(TEXT("firstCommit"));P.Commits=O->GetIntegerField(TEXT("commits"));for(const auto& B:O->GetArrayField(TEXT("blobs")))P.Blobs.Add(B->AsNumber());R.Spaces.Add(P);}Steps.Add(R);}
}
FParkScalar::~FParkScalar()=default;
void FParkScalar::Select(int32 Step){Highlights.Reset();Selected=FMath::Clamp(Step,0,Steps.Num()-1);Clock=0;Age=0;if(MovingCamera){PanOffset=FVector::ZeroVector;OrbitYaw=-8;Tilt=.46f;Height=.5f;}if(Selected==Steps.Num()-1)Playing=false;}
bool FParkScalar::Advance(int32 Direction){Playing=false;const int32 Next=Selected+Direction;if(!Steps.IsValidIndex(Next))return false;Select(Next);return true;}
void FParkScalar::Tick(float Delta){for(auto& H:Highlights)if(H.Released)H.Age+=Delta;Highlights.RemoveAll([](const FScalarHighlight& H){return H.Released&&H.Age>=2;});Age+=Delta;if(Playing){Clock+=Delta;if(Clock>FMath::Max(FMath::Lerp(6.f,1.2f,Speed),FMath::Max(Scene->CreationEndTime+.35f,Steps[Selected].From>=0?3.f:0.f)))Select(Selected+1);}Scene->Update(*this,Delta);}

void FParkScalar::Pan(FVector2D Delta,float ViewWidth) {
 const float Yaw=FMath::DegreesToRadians(OrbitYaw),Pitch=FMath::DegreesToRadians(FMath::Lerp(25.f,82.f,Tilt));
 const FVector Right(FMath::Cos(Yaw),FMath::Sin(Yaw),0),Up(-FMath::Sin(Yaw)*FMath::Sin(Pitch),FMath::Cos(Yaw)*FMath::Sin(Pitch),FMath::Cos(Pitch));
 const float UnitsPerPixel=2*Scene->Distance*FMath::Tan(FMath::DegreesToRadians(24.f))/ViewWidth;
 PanOffset-=(Right*Delta.X-Up*Delta.Y)*UnitsPerPixel;
}

FString FParkScalar::VisibleCommands() const {
 TArray<FString> Lines;Steps[Selected].Command.ParseIntoArrayLines(Lines);
 for(auto& Line:Lines)Line=Steps[Selected].Actor+TEXT("$ ")+Line;
 const FString Block=FString::Join(Lines,TEXT("\n"));
 return Block.Left(FMath::FloorToInt(Block.Len()*FMath::Clamp(Age,0.f,1.f)));
}
