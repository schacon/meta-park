#include "ParkExchange.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

namespace {
const FColor Local(23,134,151),Remote(192,117,36),Shared(85,104,122),Paper(245,241,214),Ink(31,56,45),Gold(242,188,53),Red(190,48,49);
FVector At(float H,float V,float Depth=0){return FVector(Depth,-H,V);}
float Column(int32 Board){return (Board-1)*560.f;}
FVector ValueAt(int32 Board,int32 Type,int32 I){return At(Column(Board)+(Type==1?(I%2-.5f)*206:0),Type==0?95:Type==1?-30-(I/2)*56:-190-I*51,65);}
FColor OriginColor(const FString& Origin){return Origin==TEXT("local")?Local:Origin==TEXT("remote")?Remote:Shared;}
bool SameValue(const FExchangeValue& A,const FExchangeValue& B){return A.Id.IsEmpty()?A.Text==B.Text:A.Id==B.Id;}
}
FParkExchange::FParkExchange(UWorld* World,TSharedPtr<FJsonObject> Component) {
 Title=Component->GetStringField(TEXT("title"));
 for(auto V:Component->GetArrayField(TEXT("steps"))){auto O=V->AsObject();FExchangeStep S;S.Phase=O->GetIntegerField(TEXT("phase"));for(auto Entry:O->GetArrayField(TEXT("log")))S.Log.Add(Entry->AsNumber());S.Action=O->GetStringField(TEXT("action"));S.Record=O->GetStringField(TEXT("record"));for(auto State:O->GetArrayField(TEXT("states")))S.States.Add(State->AsString());S.Label=O->GetStringField(TEXT("label"));S.Command=O->GetStringField(TEXT("command"));S.Headline=O->GetStringField(TEXT("headline"));S.Note=O->GetStringField(TEXT("note"));Steps.Add(S);}
 for(auto V:Component->GetArrayField(TEXT("objects"))){auto O=V->AsObject();FExchangeObject Object;Object.Type=O->GetStringField(TEXT("type"));Object.Key=O->GetStringField(TEXT("key"));
  auto Read=[&](const TCHAR* Key,TArray<FExchangeValue>& Rows){for(auto V:O->GetArrayField(Key)){auto R=V->AsObject();FExchangeValue Row;Row.Text=R->GetStringField(TEXT("text"));Row.Origin=R->GetStringField(TEXT("origin"));R->TryGetStringField(TEXT("id"),Row.Id);Rows.Add(Row);}};
  Read(TEXT("base"),Object.Base);Read(TEXT("local"),Object.Local);Read(TEXT("remote"),Object.Remote);Read(TEXT("result"),Object.Result);Objects.Add(Object);
 }
 for(auto V:Component->GetArrayField(TEXT("records"))){auto O=V->AsObject();FExchangeRecord R;R.Id=O->GetStringField(TEXT("id"));O->TryGetStringField(TEXT("parent"),R.Parent);R.Author=O->GetStringField(TEXT("author"));R.State=O->GetStringField(TEXT("state"));Records.Add(R);}
 auto* A=World->SpawnActor<AActor>();Actor=A;A->SetActorHiddenInGame(true);auto* Root=NewObject<USceneComponent>(A);A->AddInstanceComponent(Root);A->SetRootComponent(Root);Root->SetMobility(EComponentMobility::Movable);Root->RegisterComponent();
 // A translucent world-space veil sits behind the demonstration. It dims the
 // scenery without changing the lighting or contrast of the physical objects.
 auto* Backdrop=NewObject<UWidgetComponent>(A);A->AddInstanceComponent(Backdrop);Backdrop->SetupAttachment(Root);Backdrop->SetWidgetSpace(EWidgetSpace::World);Backdrop->SetDrawSize(FVector2D(16,16));Backdrop->SetBlendMode(EWidgetBlendMode::Transparent);Backdrop->SetBackgroundColor(FLinearColor::Transparent);Backdrop->SetTwoSided(true);Backdrop->SetCollisionEnabled(ECollisionEnabled::NoCollision);Backdrop->SetCastShadow(false);Backdrop->SetTickWhenOffscreen(true);Backdrop->SetRelativeLocation(FVector(-180,0,0));Backdrop->SetRelativeScale3D(FVector(1000));Backdrop->RegisterComponent();
 Backdrop->SetSlateWidget(SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor_Lambda([this]{return FLinearColor(0,0,0,.92f*BackdropFade);}).Padding(0));
}
FParkExchange::~FParkExchange(){if(Actor.IsValid())Actor->Destroy();}
USceneComponent* FParkExchange::Box(FVector P,FVector Size,FColor Tint) {
 auto* A=Actor.Get();auto* C=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(C);C->SetupAttachment(A->GetRootComponent());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetRelativeLocation(P);C->SetRelativeScale3D(Size/100);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);
 auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_IslandLit.M_IslandLit")),C);M->SetVectorParameterValue(TEXT("Tint"),FLinearColor::FromSRGBColor(Tint));C->SetMaterial(0,M);C->RegisterComponent();Parts.Add(C);return C;
}
USceneComponent* FParkExchange::Label(FVector P,FVector2D Size,const FString& Text,int32 Font,FColor Tint) {
 auto* A=Actor.Get();auto* W=NewObject<UWidgetComponent>(A);A->AddInstanceComponent(W);W->SetupAttachment(A->GetRootComponent());W->SetWidgetSpace(EWidgetSpace::World);W->SetDrawSize(Size*2);W->SetBlendMode(EWidgetBlendMode::Masked);W->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_PropLettering.M_PropLettering")));W->SetTwoSided(true);W->SetCollisionEnabled(ECollisionEnabled::NoCollision);W->SetCastShadow(false);W->SetTickWhenOffscreen(true);W->SetRelativeLocation(P);W->SetRelativeScale3D(FVector(.5));W->RegisterComponent();
 W->SetSlateWidget(SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBrush")).Padding(0).HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Text)).Font(FCoreStyle::GetDefaultFontStyle("Bold",Font*2)).Justification(ETextJustify::Center).ColorAndOpacity(FLinearColor::FromSRGBColor(Tint))]);Parts.Add(W);return W;
}
void FParkExchange::Link(FVector A,FVector B,FColor Tint,float Width){const auto D=B-A;auto* C=Box((A+B)*.5f,FVector(Width,Width,D.Size()),Tint);C->SetRelativeRotation(FRotationMatrix::MakeFromZ(D).Rotator());}
const TArray<FExchangeValue>& FParkExchange::Values(int32 Board,int32 Type) const {
 static const TArray<FExchangeValue> Empty;
 if(!Objects.IsValidIndex(Type)||!Steps[Selected].States.IsValidIndex(Board))return Empty;
 const auto& State=Steps[Selected].States[Board];const auto& O=Objects[Type];
 return State==TEXT("base")?O.Base:State==TEXT("local")?O.Local:State==TEXT("remote")?O.Remote:State==TEXT("merged")?O.Result:Empty;
}
void FParkExchange::Keeper(int32 Side) {
 const int32 First=Parts.Num();const float H=Column(Side);const auto Shirt=Side==0?Local:Remote;const FColor Skin(220,167,121),Hat(190,170,107),Boot(51,45,34);
 auto Sphere=[&](FVector P,FVector Size,FColor C){auto* Part=Cast<UStaticMeshComponent>(Box(P,Size,C));Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));};
 Box(At(H,351,85),FVector(48,70,65),Shirt);Box(At(H,324,85),FVector(49,72,12),Boot);
 for(float S:{-1.f,1.f}) {
  Link(At(H+S*20,319,85),At(H+S*24,288,90),FColor(102,100,63),23);Box(At(H+S*24,279,100),FVector(43,27,14),Boot);
  Link(At(H+S*39,373,85),At(H+S*62,339,97),Shirt,19);Sphere(At(H+S*63,334,100),FVector(23),Skin);
 }
 Sphere(At(H,404,88),FVector(55,58,58),Skin);
 Box(At(H,429,90),FVector(82,103,9),Hat);Box(At(H,442,88),FVector(54,62,23),Hat);
 Box(At(H,433,117),FVector(5,63,6),Boot);
 for(float E:{-1.f,1.f})Sphere(At(H+E*10,407,116),FVector(7),Boot);
 Link(At(H-7,395,116),At(H+7,395,116),Boot,3);
 Box(At(H-17,355,112),FVector(4,15,21),Paper);
 // A small pencil makes the writing action readable even in silhouette.
 Link(At(H+67,333,116),At(H+79,367,116),Gold,6);
 for(int32 K=First;K<Parts.Num();K++)Keepers.Add({Parts[K],Parts[K]->GetRelativeLocation(),Side});
}
void FParkExchange::Rebuild() {
 for(auto& P:Parts)if(P.IsValid())P->DestroyComponent();Parts.Reset();Transfers.Reset();Reveals.Reset();Keepers.Reset();MovingValues=0;
 const auto& Step=Steps[Selected];const auto& Action=Step.Action;
 auto Tile=[&](FVector P,FVector2D Size,const FString& Text,FColor Tint,int32 Font=29){Box(P,FVector(20,Size.X,Size.Y),Tint);Label(P+FVector(12,0,0),Size-FVector2D(8,4),Text,Font);};
 auto Reveal=[&](int32 First,float Time,bool Before=false){for(int32 K=First;K<Parts.Num();K++)Reveals.Add({Parts[K],Time,Before});};
 auto Move=[&](int32 First,FVector Offset,float Delay,float Duration=1.25f,bool Bounce=false){for(int32 K=First;K<Parts.Num();K++){auto* Part=Parts[K].Get();const auto End=Part->GetRelativeLocation();Transfers.Add({Part,End+Offset,End,Delay,Duration,Bounce});}};
 Label(At(0,552),{1660,80},Title,48,Paper);
 Label(At(-560,485),{510,65},TEXT("KEEPER 1"),35,Paper);Label(At(0,485),{530,65},TEXT("CRDT-Rex"),42,Paper);Label(At(560,485),{510,65},TEXT("KEEPER 2"),35,Paper);
 for(float H:{-280.f,280.f})Link(At(H,465,-60),At(H,-397,-60),FColor(85,102,87),4);
 Keeper(0);if(Selected>0)Keeper(2);
 const FString Status=Action==TEXT("create")?TEXT("FIRST EDITION") :Action==TEXT("copy")?TEXT("COPY OF B") :Action==TEXT("edit")?TEXT("STILL EDITION B") :Action==TEXT("blocked")?TEXT("FF-ONLY: BLOCKED") :Action==TEXT("merge")?TEXT("RECORD STAYS R") :Action==TEXT("publish")?TEXT("PUSH ACCEPTED"):TEXT("PUBLISHED R");
 Tile(At(0,366,45),{465,85},Status,Action==TEXT("blocked")?Red:Action==TEXT("publish")?FColor(43,121,76):Shared,27);
 for(int32 Board=0;Board<3;Board++) {
  const float H=Column(Board);const bool Empty=Step.States[Board]==TEXT("empty"),Merging=Board==0&&Action==TEXT("merge");
  Box(At(H,-53,18),FVector(42,500,684),FColor(110,77,45));Box(At(H,-53,43),FVector(8,469,652),Paper);
  Box(At(H,283,60),FVector(30,170,37),Shared);Box(At(H,297,60),FVector(24,100,19),Shared);
  if(Board==1) {
   Label(At(0,243,54),{440,44},TEXT("APPEND-ONLY RECORD"),24,Ink);
   for(int32 Index=0;Index<Step.Log.Num();Index++) {
    const auto& Record=Records[Step.Log[Index]];const float Top=200-Index*185;const int32 First=Parts.Num();
    const auto Tint=Index==0?Shared:Index==1?Remote:Local;
    Box(At(0,Top-76,55),FVector(14,446,176),Tint);Box(At(0,Top-76,64),FVector(4,436,166),FColor(238,236,214));
    Label(At(0,Top,68),{420,33},Record.Id+TEXT(" / ")+Record.Author+(Record.Parent.IsEmpty()?TEXT(""):TEXT(" / after ")+Record.Parent),22,Tint);
    for(int32 Type=0;Type<3;Type++) {
     const auto& O=Objects[Type];const auto& Rows=Record.State==TEXT("base")?O.Base:Record.State==TEXT("remote")?O.Remote:O.Result;
     FString Value;for(int32 I=0;I<Rows.Num();I++){if(I)Value+=TEXT(", ");Value+=Rows[I].Text;}
     if(Type==0)Label(At(0,Top-34,68),{420,34},O.Key+TEXT(" = ")+Value,23,OriginColor(Rows[0].Origin));
     else {Label(At(0,Top-(Type==1?64:119),68),{420,29},O.Key,20,Ink);Label(At(0,Top-(Type==1?90:145),68),{426,32},(Type==1?TEXT("{ "):TEXT("[ "))+Value+(Type==1?TEXT(" }"):TEXT(" ]")),22,Ink);}
    }
    const bool Append=(Action==TEXT("create")&&Index==0)||(Action==TEXT("push_remote")&&Index==1)||(Action==TEXT("publish")&&Index==2);
    if(Append){const float Delay=Action==TEXT("create")?1.8f:.2f;Move(First,At(Action==TEXT("push_remote")?560:-560,0,130),Delay,1.3f);Reveal(First,Delay);MovingValues++;}
   }
   Label(At(0,-359,54),{440,39},TEXT("Full snapshots · earlier entries kept"),20,Ink);continue;
  }
  Label(At(H,240,54),{440,54},Board==1?TEXT("DINO CARE RECORD"):TEXT("MY CARE NOTES"),27,Ink);
  FString Revision;
  if(Board==1)Revision=TEXT("published edition ")+Step.Record;
  else if(Empty)Revision=TEXT("waiting for a copy");
  else if(Step.States[Board]==TEXT("base"))Revision=TEXT("working from B");
  else if(Board==2)Revision=Selected>=3?TEXT("published R"):TEXT("from B / local R");
  else if(Step.States[Board]==TEXT("merged"))Revision=Selected==6?TEXT("published M"):TEXT("merged M / parent R");
  else Revision=TEXT("from B / local L");
  Label(At(H,201,54),{440,40},Revision,21,Ink);Link(At(H-217,178,54),At(H+217,178,54),FColor(175,179,151),2);
  const TCHAR* Headings[]={TEXT("dino:name · string"),TEXT("feeding:foods · set"),TEXT("feeding:times · list")};
  const TCHAR* MergeHeadings[]={TEXT("dino:name · LOCAL WINS"),TEXT("feeding:foods · UNION"),TEXT("feeding:times · MERGED")};
  const float Y[]={148,25,-141};
  for(int32 Type=0;Type<3;Type++) {
   Label(At(H,Y[Type],55),{440,40},Merging?MergeHeadings[Type]:Headings[Type],22,Merging?FColor(142,87,13):Ink);
   const auto& Rows=Values(Board,Type);const auto& O=Objects[Type];
   if(Empty){Label(At(H,Type==0?95:Type==1?-40:-190,55),{390,52},TEXT("—"),32,FColor(155,161,137));continue;}
   for(int32 I=0;I<Rows.Num();I++) {
    const auto& V=Rows[I];const auto End=ValueAt(Board,Type,I);const FVector2D Size(Type==1?194:400,Type==0?70:45);
    const FString Text=Type==2?FString::Printf(TEXT("%d  %s"),I+1,*V.Text):V.Text;
    if(Merging&&Type==0)Box(End-FVector(2,0,0),FVector(21,Size.X+12,Size.Y+12),Gold);
    const int32 First=Parts.Num();Tile(End,Size,Text,OriginColor(V.Origin),Type==0?43:27);
    int32 From=-1;float Delay=.15f+Type*.32f;
    if(Action==TEXT("create")){if(Board==1){From=0;Delay=1.8f+Type*.4f;}else if(Board==0){Move(First,At(0,28,20),.65f+Type*.4f,.5f);Reveal(First,.65f+Type*.4f);}}
    else if(Action==TEXT("copy")&&Board==2)From=1;
    else if(Action==TEXT("edit")&&Board!=1&&V.Origin!=TEXT("base")) {
     const float Time=.6f+Type*.35f;Move(First,At(0,35,50),Time,.6f);Reveal(First,Time);MovingValues++;
     if(Type==0){const int32 Old=Parts.Num();Tile(End,Size,O.Base[0].Text,Shared,43);Reveal(Old,Time,true);}
    }
    else if(Action==TEXT("push_remote")&&Board==1&&V.Origin!=TEXT("base"))From=2;
    else if(Action==TEXT("merge")&&Board==0) {
     if(V.Origin==TEXT("remote")){From=1;Delay=Type==1?.9f:1.7f;}
     else {const int32 Previous=O.Local.IndexOfByPredicate([&](const FExchangeValue& P){return SameValue(P,V);});if(Previous>=0&&Previous!=I)Move(First,ValueAt(Board,Type,Previous)-End,1.5f,.8f);}
    }
    else if(Action==TEXT("publish")&&Board==1) {
     if(V.Origin==TEXT("local"))From=0;
     else {const int32 Previous=O.Remote.IndexOfByPredicate([&](const FExchangeValue& P){return SameValue(P,V);});if(Previous>=0&&Previous!=I)Move(First,ValueAt(Board,Type,Previous)-End,.8f,.7f);}
    }
    if(From>=0) {
     const auto& Source=Values(From,Type);const int32 SourceIndex=Source.IndexOfByPredicate([&](const FExchangeValue& P){return SameValue(P,V);});
     const FVector Start=From==1?At(0,200-(Step.Log.Num()-1)*185-(Type==0?34:Type==1?90:145),68):ValueAt(From,Type,FMath::Max(0,SourceIndex));
     Move(First,Start-End,Delay);Reveal(First,Delay);MovingValues++;
     if(Type==0&&Action!=TEXT("copy")&&Action!=TEXT("create")) {const int32 Old=Parts.Num();Tile(End,Size,Action==TEXT("publish")?O.Remote[0].Text:O.Base[0].Text,Shared,43);Reveal(Old,Delay+1.1f,true);}
    }
   }
  }
  FString Footer=Board==1?TEXT("The published source of truth"):Empty?TEXT("No local data yet"):TEXT("This keeper’s working copy");
  if(Board==0&&Action==TEXT("blocked"))Footer=TEXT("Rejected · keep local edits");
  if(Merging)Footer=TEXT("Ready to retry against R");
  Label(At(H,-358,54),{445,44},Footer,21,Board==0&&Action==TEXT("blocked")?Red:Ink);
 }
 if(Action==TEXT("blocked")) {
  const int32 First=Parts.Num();Tile(At(-305,344,180),{185,64},TEXT("PUSH L"),Local,30);
  Move(First,At(-255,0,0),.2f,1.9f,true);Reveal(First,.2f);
  // The proposed revision reaches the checkpoint, then returns to its owner.
  Link(At(-254,382,180),At(-254,307,180),Red,12);
 }
 Box(At(0,-495,45),FVector(24,1660,151),Paper);
 Label(At(0,-450,61),{1630,56},Step.Headline,33,Ink);Label(At(0,-501,61),{1630,48},Step.Note,24,Ink);
 Label(At(0,-547,61),{1630,43},Step.Command,22,FColor(77,91,70));
 BuiltStep=Selected;
}
void FParkExchange::Update(float Delta,FVector Position,FVector Eye,bool Showing) {
 if(!Actor.IsValid())return;Visible=Showing;Actor->SetActorHiddenInGame(!Showing);if(!Showing){BackdropFade=0;return;}
 BackdropFade=FMath::Min(1.f,BackdropFade+Delta/.45f);Age+=Delta;if(BuiltStep!=Selected)Rebuild();
 auto Rotation=(Eye-Position).Rotation();Rotation.Yaw+=3;Rotation.Pitch-=3;
 Actor->SetActorLocationAndRotation(Position,Rotation);Actor->SetActorScale3D(FVector(.89f));
 for(auto& T:Transfers)if(T.Part.IsValid()) {
  const float P=FMath::Clamp((Age-T.Delay)/T.Duration,0.f,1.f),Ease=P*P*(3-2*P),Travel=T.Bounce?(Ease<.5f?Ease*2:2-Ease*2):Ease;
  T.Part->SetRelativeLocation(FMath::Lerp(T.From,T.To,Travel)+FVector(FMath::Sin(Travel*PI)*120,0,0));
 }
 for(auto& R:Reveals)if(R.Part.IsValid())R.Part->SetVisibility(R.Before?Age<R.At:Age>=R.At);
 for(auto& K:Keepers)if(K.Part.IsValid()) {
  const bool Enter=(Selected==0&&K.Side==0)||(Selected==1&&K.Side==2);const float T=Enter?FMath::SmoothStep(0.f,1.f,FMath::Clamp(Age/1.1f,0.f,1.f)):1;
  const bool Writing=(Selected==0&&K.Side==0)||Selected==2||(Selected==5&&K.Side==0);
  K.Part->SetRelativeLocation(K.Rest+At((K.Side==0?-600:600)*(1-T),(Writing&&Age<2.5f?FMath::Sin(Age*13)*2:0)+FMath::Sin(T*PI)*8,0));
 }
}
