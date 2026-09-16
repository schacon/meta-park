#include "ParkExchange.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

namespace {
const FColor Local(23,134,151),Remote(192,117,36),Shared(85,104,122),Paper(253,252,246),RecordPaper(253,247,218),Ink(18,30,27),Gold(242,188,53),Red(190,48,49);
FVector At(float H,float V,float Depth=0){return FVector(Depth,-H,V);}
float Column(int32 Board){return (Board-1)*720.f;}
float RecordTop(int32 Index){return 294.f-Index*264.f;}
FQuat ClipboardTilt(int32 Board){return FQuat(FVector::ForwardVector,FMath::DegreesToRadians(Board==0?-9.f:Board==2?9.f:0.f));}
FVector TiltPoint(FVector P,int32 Board){const FVector Pivot=At(Column(Board),0);return Pivot+ClipboardTilt(Board).RotateVector(P-Pivot);}
FVector TiltTransferPoint(FVector P){return TiltPoint(P,-P.Y < -360?0:-P.Y > 360?2:1);}
FVector ValueAt(int32 Board,int32 Type,int32 I){return At(Column(Board)+(Type==1?(I%2-.5f)*206:0),Type==0?175:Type==1?30-(I/2)*65:-165-I*60,65);}
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
 Backdrop->SetSlateWidget(SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor_Lambda([this]{return FLinearColor(0,0,0,.88f*BackdropFade);}).Padding(0));
}
FParkExchange::~FParkExchange(){if(Actor.IsValid())Actor->Destroy();}
USceneComponent* FParkExchange::Box(FVector P,FVector Size,FColor Tint) {
 auto* A=Actor.Get();auto* C=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(C);C->SetupAttachment(A->GetRootComponent());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetRelativeLocation(P);Size.X=FMath::Min(Size.X,6.);C->SetRelativeScale3D(Size/100);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);
 auto* M=Materials.FindRef(Tint.ToPackedARGB()).Get();
 if(!M){M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_PropPaper.M_PropPaper")),A);M->SetVectorParameterValue(TEXT("Tint"),FLinearColor::FromSRGBColor(Tint));auto* Texture=UTexture2D::CreateTransient(1,1,PF_B8G8R8A8);auto& Bulk=Texture->GetPlatformData()->Mips[0].BulkData;auto* Pixel=static_cast<FColor*>(Bulk.Lock(LOCK_READ_WRITE));*Pixel=Tint;Bulk.Unlock();Texture->UpdateResource();M->SetTextureParameterValue(TEXT("SlateUI"),Texture);Materials.Add(Tint.ToPackedARGB(),M);}
 C->SetMaterial(0,M);C->RegisterComponent();Parts.Add(C);return C;
}
USceneComponent* FParkExchange::Label(FVector P,FVector2D Size,const FString& Text,int32 Font,FColor Tint,bool Left) {
 auto* A=Actor.Get();auto* W=NewObject<UWidgetComponent>(A);A->AddInstanceComponent(W);W->SetupAttachment(A->GetRootComponent());W->SetWidgetSpace(EWidgetSpace::World);W->SetDrawSize(Size*2);W->SetBlendMode(EWidgetBlendMode::Masked);W->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/M_PropLettering.M_PropLettering")));W->SetTwoSided(true);W->SetCollisionEnabled(ECollisionEnabled::NoCollision);W->SetCastShadow(false);W->SetTickWhenOffscreen(true);W->SetRelativeLocation(P);W->SetRelativeScale3D(FVector(.5));W->RegisterComponent();
 W->SetSlateWidget(SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBrush")).Padding(FMargin(Left?4.f:0.f,0.f)).HAlign(Left?HAlign_Left:HAlign_Center).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Text)).Font(FCoreStyle::GetDefaultFontStyle("Bold",Font*2)).Justification(Left?ETextJustify::Left:ETextJustify::Center).ColorAndOpacity(FLinearColor::FromSRGBColor(Tint))]);Parts.Add(W);return W;
}
void FParkExchange::Link(FVector A,FVector B,FColor Tint,float Width){const auto D=B-A;auto* C=Box((A+B)*.5f,FVector(Width,Width,D.Size()),Tint);C->SetRelativeRotation(FRotationMatrix::MakeFromZ(D).Rotator());}
const TArray<FExchangeValue>& FParkExchange::Values(int32 Board,int32 Type) const {
 static const TArray<FExchangeValue> Empty;
 if(!Objects.IsValidIndex(Type)||!Steps[Selected].States.IsValidIndex(Board))return Empty;
 const auto& State=Steps[Selected].States[Board];const auto& O=Objects[Type];
 return State==TEXT("base")?O.Base:State==TEXT("local")?O.Local:State==TEXT("remote")?O.Remote:State==TEXT("merged")?O.Result:Empty;
}
FColor FParkExchange::ValueColor(int32 Type,const FExchangeValue& Value) const {
 if(Type==0)return Value.Origin==TEXT("local")?FColor(6,95,169):Value.Origin==TEXT("remote")?FColor(174,64,28):FColor(75,92,116);
 // A value keeps its ink color through copying, local merging and publication.
 const int32 Index=FMath::Max(0,Objects[Type].Result.IndexOfByPredicate([&](const FExchangeValue& Other){return SameValue(Other,Value);}));
 const FColor Foods[]={FColor(174,42,69),FColor(30,109,48),FColor(115,51,159),FColor(0,111,118)};
 const FColor Times[]={FColor(25,80,176),FColor(174,82,0),FColor(113,54,173)};
 return Type==1?Foods[Index%4]:Times[Index%3];
}
void FParkExchange::Keeper(int32 Side) {
 const float H=Column(Side);const FColor Sleeve=Side==0?FColor(191,222,229):FColor(241,211,175),Skin(233,192,158);
 // Both pairs of hands hold their clipboards from the first frame.
 for(float Sign:{-1.f,1.f}) {
  Link(At(H+Sign*145,-460,25),At(H+Sign*281,-212,25),Sleeve,53);
  Link(At(H+Sign*281,-219,26),At(H+Sign*279,-172,26),Skin,38);
  auto* Hand=Cast<UStaticMeshComponent>(Box(At(H+Sign*278,-155,76),FVector(6,45,68),Skin));Hand->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
  for(int32 I=0;I<3;I++)Link(At(H+Sign*281,-136-I*14,82),At(H+Sign*260,-136-I*14,82),FColor(188,141,111),2);
  Link(At(H+Sign*270,-180,82),At(H+Sign*258,-153,82),Skin,15);
 }
}
void FParkExchange::Rebuild() {
 for(auto& P:Parts)if(P.IsValid())P->DestroyComponent();Parts.Reset();Transfers.Reset();Reveals.Reset();MovingValues=0;
 const auto& Step=Steps[Selected];const auto& Action=Step.Action;
 auto Tile=[&](FVector P,FVector2D Size,const FString& Text,FColor Tint,int32 Font=29,FColor TextTint=Ink){const FColor Fill=Tint==Local?FColor(212,235,239):Tint==Remote?FColor(248,228,202):Tint==Shared?FColor(230,234,234):Tint==Red?FColor(254,218,216):Tint;Box(P,FVector(20,Size.X,Size.Y),Fill);Label(P+FVector(12,0,0),Size-FVector2D(8,4),Text,Font,TextTint);};
 auto Reveal=[&](int32 First,float Time,bool Before=false){for(int32 K=First;K<Parts.Num();K++)Reveals.Add({Parts[K],Time,Before});};
 auto Move=[&](int32 First,FVector Offset,float Delay,float Duration=1.25f,bool Bounce=false){for(int32 K=First;K<Parts.Num();K++){auto* Part=Parts[K].Get();const auto End=Part->GetRelativeLocation();Transfers.Add({Part,End+Offset,End,Delay,Duration,Bounce});}};
 Label(At(0,620),{2080,88},Title,52,Paper);
 Label(At(-720,540),{590,70},TEXT("KEEPER 1"),38,Paper);Label(At(720,540),{590,70},TEXT("KEEPER 2"),38,Paper);
 const FString Status=Action==TEXT("create")?TEXT("FIRST EDITION") :Action==TEXT("copy")?TEXT("COPY OF B") :Action==TEXT("edit")?TEXT("STILL EDITION B") :Action==TEXT("blocked")?TEXT("FF-ONLY: BLOCKED") :Action==TEXT("merge")?TEXT("RECORD STAYS R") :Action==TEXT("publish")?TEXT("PUSH ACCEPTED"):TEXT("PUBLISHED R");
 for(int32 Board=0;Board<3;Board++) {
  const int32 BoardFirst=Parts.Num();if(Board!=1)Keeper(Board);
  const float H=Column(Board);const bool Empty=Step.States[Board]==TEXT("empty"),Merging=Board==0&&Action==TEXT("merge");
  if(Board==1) {
   // A printer chassis surrounds the continuous, append-only paper record.
   Box(At(0,-6,8),FVector(6,654,1078),FColor(12,26,29));
   Box(At(0,2,18),FVector(6,628,1048),FColor(48,76,79));
   Box(At(0,452,30),FVector(6,584,126),FColor(21,39,42));
   Label(At(0,478,48),{520,58},TEXT("CRDT-Rex"),43,Paper);
   Label(At(0,428,48),{480,38},Status,23,Action==TEXT("blocked")?FColor(255,151,123):FColor(151,237,184));
   Box(At(0,385,34),FVector(6,572,30),FColor(8,15,16));
   for(float Side:{-1.f,1.f}) {
    Box(At(Side*282,385,44),FVector(6,18,40),FColor(162,181,180));
    for(float Z:{490.f,-434.f}){auto* Screw=Cast<UStaticMeshComponent>(Box(At(Side*298,Z,35),FVector(5,10,10),FColor(192,204,202)));Screw->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));}
    for(int32 I=0;I<8;I++)Box(At(Side*296,-305-I*12,30),FVector(4,15,4),FColor(13,30,32));
   }
   Box(At(0,-66,42),FVector(4,540,892),Paper);
   Box(At(0,-519,28),FVector(6,582,16),FColor(15,31,33));
  } else {
   // Exposed walnut edges, grain and a steel clamp distinguish board from paper.
   Box(At(H,0,18),FVector(6,568,890),FColor(67,37,20));
   Box(At(H,0,26),FVector(6,550,874),FColor(141,89,45));
   for(int32 I=0;I<13;I++){const float X=H-264+I*44;Link(At(X,-428,30),At(X+9,428,30),I%2?FColor(115,67,33):FColor(173,115,61),2);}
   Box(At(H,-6,43),FVector(4,508,808),FColor(80,53,31));
   Box(At(H,0,49),FVector(4,500,800),Board==0?FColor(225,244,248):FColor(255,240,218));
   Box(At(H,409,60),FVector(6,196,56),FColor(53,65,66));
   Box(At(H,416,68),FVector(6,178,42),FColor(180,195,196));
   Box(At(H,439,68),FVector(6,98,18),FColor(220,229,228));
   Link(At(H-76,409,75),At(H+76,409,75),FColor(235,241,239),3);
  }
  if(Board==1) {
   Label(At(0,352,54),{484,44},TEXT("APPEND-ONLY RECORD"),24,Ink);
   for(float Z=-495;Z<375;Z+=36)for(float X:{-254.f,254.f})Box(At(X,Z,60),FVector(2,7,7),FColor(204,210,206));
   auto Entry=[&](int32 Index,const FString& Author,const FString& Id,const FString& State,bool Rejected=false){
    const float Top=RecordTop(Index);const int32 First=Parts.Num();const FColor Stroke=Rejected?Red:FColor(154,170,162);
    Box(At(0,Top-100,55),FVector(4,496,218),Rejected?FColor(255,230,225):RecordPaper);
    Label(At(0,Top,68),{470,40},Author+TEXT(" · ")+Id,28,Rejected?Red:Ink,true);
    Label(At(-144,Top-34,68),{198,28},TEXT("KEY"),18,Ink,true);Label(At(116,Top-34,68),{278,28},TEXT("VALUE"),18,Ink,true);
    for(float Offset:{50.f,99.f,156.f})Link(At(-244,Top-Offset,66),At(244,Top-Offset,66),Stroke,1.2f);
    Link(At(-36,Top-20,66),At(-36,Top-209,66),Stroke,1.2f);
    for(int32 Type=0;Type<3;Type++) {
     const auto& O=Objects[Type];const auto& Rows=State==TEXT("base")?O.Base:State==TEXT("remote")?O.Remote:State==TEXT("local")?O.Local:O.Result;
     const float Y=Top-(Type==0?76:Type==1?128:182);
     Label(At(-144,Y,68),{198,48},O.Key,20,Ink,true);
     for(int32 I=0;I<Rows.Num();I++) {
      const float Width=Type==1?139.f:Type==2?278.f/3:278.f;
      const float X=Type==1?46.5f+(I%2)*139:Type==2?-23+(I+.5f)*Width:116;
      const float RowY=Y+(Type==1&&Rows.Num()>2?14-(I/2)*28:0);
      const FString Text=Rows[I].Text+(I+1<Rows.Num()?TEXT(","):TEXT(""));
      Label(At(X,RowY,68),{Width,32},Text,21,ValueColor(Type,Rows[I]),true);
     }
    }
    return First;
   };
   for(int32 Index=0;Index<Step.Log.Num();Index++) {
    if(Index>0)for(float X=-244;X<244;X+=16)Link(At(X,RecordTop(Index)+38,66),At(FMath::Min(X+7,244.f),RecordTop(Index)+38,66),FColor(75,91,85),2.5f);
    const auto& Record=Records[Step.Log[Index]];const int32 First=Entry(Index,Record.Author,Record.Id,Record.State);
    const bool Append=(Action==TEXT("create")&&Index==0)||(Action==TEXT("push_remote")&&Index==1)||(Action==TEXT("publish")&&Index==2);
    if(Append){const float Delay=Action==TEXT("create")?1.8f:.2f;Move(First,At(Action==TEXT("push_remote")?720:-720,0,130),Delay,1.3f);Reveal(First,Delay);MovingValues++;}
   }
   if(Action==TEXT("blocked")) {
    // Show the proposed third full entry reaching the slot, then reject it.
    // It is never part of Step.Log, and the permanent record remains B, R.
    const int32 First=Entry(2,TEXT("Keeper 1"),TEXT("L (rejected)"),TEXT("local"),true);
    Move(First,At(-720,0,150),.15f,2.1f,true);Reveal(First,.15f);Reveal(First,2.35f,true);
    const int32 Stamp=Parts.Num();Tile(At(0,-266,200),{460,130},TEXT("3rd update BLOCKED\nnot added to the record"),Red,26);Reveal(Stamp,1.15f);
   }
   Label(At(0,-486,54),{480,36},FString::Printf(TEXT("%d accepted updates"),Step.Log.Num()),19,Ink);continue;
  }
  Label(At(H,348,60),{428,58},Board==1?TEXT("DINO CARE RECORD"):TEXT("MY CARE NOTES"),29,Ink);
  FString Revision;
  if(Board==1)Revision=TEXT("published edition ")+Step.Record;
  else if(Empty)Revision=TEXT("waiting for a copy");
  else if(Step.States[Board]==TEXT("base"))Revision=TEXT("working from B");
  else if(Board==2)Revision=Selected>=3?TEXT("published R"):TEXT("from B / local R");
  else if(Step.States[Board]==TEXT("merged"))Revision=Selected==6?TEXT("published M"):TEXT("merged M / parent R");
  else Revision=TEXT("from B / local L");
  Label(At(H,302,60),{428,44},Revision,21,Ink);Link(At(H-214,271,60),At(H+214,271,60),FColor(175,179,151),2);
  const TCHAR* Headings[]={TEXT("dino:name · string"),TEXT("feeding:foods · set"),TEXT("feeding:times · list")};
  const TCHAR* MergeHeadings[]={TEXT("dino:name · LOCAL WINS"),TEXT("feeding:foods · UNION"),TEXT("feeding:times · MERGED")};
  const float Y[]={231,88,-109};
  for(int32 Type=0;Type<3;Type++) {
   Label(At(H,Y[Type],60),{428,44},Merging?MergeHeadings[Type]:Headings[Type],22,Merging?FColor(142,87,13):Ink);
   const auto& Rows=Values(Board,Type);const auto& O=Objects[Type];
   if(Empty){Label(At(H,Type==0?175:Type==1?0:-195,55),{390,52},TEXT("—"),32,FColor(155,161,137));continue;}
   for(int32 I=0;I<Rows.Num();I++) {
    const auto& V=Rows[I];const auto End=ValueAt(Board,Type,I);const FVector2D Size(Type==1?194:400,Type==0?70:45);
    const FString Text=Type==2?FString::Printf(TEXT("%d  %s"),I+1,*V.Text):V.Text;
    if(Merging&&Type==0)Box(End-FVector(2,0,0),FVector(21,Size.X+12,Size.Y+12),Gold);
    const int32 First=Parts.Num();Tile(End,Size,Text,OriginColor(V.Origin),Type==0?43:27,ValueColor(Type,V));
    int32 From=-1;float Delay=.15f+Type*.32f;
    if(Action==TEXT("create")){if(Board==1){From=0;Delay=1.8f+Type*.4f;}else if(Board==0){Move(First,At(0,28,20),.65f+Type*.4f,.5f);Reveal(First,.65f+Type*.4f);}}
    else if(Action==TEXT("copy")&&Board==2)From=1;
    else if(Action==TEXT("edit")&&Board!=1&&V.Origin!=TEXT("base")) {
     const float Time=.6f+Type*.35f;Move(First,At(0,35,50),Time,.6f);Reveal(First,Time);MovingValues++;
     if(Type==0){const int32 Old=Parts.Num();Tile(End,Size,O.Base[0].Text,Shared,43,ValueColor(Type,O.Base[0]));Reveal(Old,Time,true);}
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
     const FVector Start=From==1?At(0,RecordTop(Step.Log.Num()-1)-(Type==0?76:Type==1?128:182),68):ValueAt(From,Type,FMath::Max(0,SourceIndex));
     Move(First,Start-End,Delay);Reveal(First,Delay);MovingValues++;
     if(Type==0&&Action!=TEXT("copy")&&Action!=TEXT("create")) {const int32 Old=Parts.Num();Tile(End,Size,Action==TEXT("publish")?O.Remote[0].Text:O.Base[0].Text,Shared,43,ValueColor(Type,Action==TEXT("publish")?O.Remote[0]:O.Base[0]));Reveal(Old,Delay+1.1f,true);}
    }
   }
  }
  FString Footer=Board==1?TEXT("The published source of truth"):Empty?TEXT("No local data yet"):TEXT("This keeper’s working copy");
  if(Board==0&&Action==TEXT("blocked"))Footer=TEXT("Rejected · keep local edits");
  if(Merging)Footer=TEXT("Ready to retry against R");
  Label(At(H,-352,60),{428,44},Footer,21,Board==0&&Action==TEXT("blocked")?Red:Ink);
  // Tilt the paper, lettering and gripping hands together around the board center.
  for(int32 K=BoardFirst;K<Parts.Num();K++)if(auto* Part=Parts[K].Get()) {
   Part->SetRelativeLocation(TiltPoint(Part->GetRelativeLocation(),Board));
   Part->SetRelativeRotation(ClipboardTilt(Board)*Part->GetRelativeRotation().Quaternion());
  }
 }
 // Travel endpoints are authored in the flat layout; map each to its tilted column.
 for(auto& T:Transfers){T.From=TiltTransferPoint(T.From);T.To=TiltTransferPoint(T.To);}
 BuiltStep=Selected;
}
void FParkExchange::Update(float Delta,FVector Position,FVector Eye,bool Showing) {
 if(!Actor.IsValid())return;Visible=Showing;Actor->SetActorHiddenInGame(!Showing);if(!Showing){BackdropFade=0;return;}
 BackdropFade=FMath::Min(1.f,BackdropFade+Delta/.45f);Age+=Delta;if(BuiltStep!=Selected)Rebuild();
 auto Rotation=(Eye-Position).Rotation();
 Actor->SetActorLocationAndRotation(Position+Rotation.RotateVector(At(0,-35)),Rotation);Actor->SetActorScale3D(FVector(.9f));
 for(auto& T:Transfers)if(T.Part.IsValid()) {
  const float P=FMath::Clamp((Age-T.Delay)/T.Duration,0.f,1.f),Ease=P*P*(3-2*P),Travel=T.Bounce?(Ease<.5f?Ease*2:2-Ease*2):Ease;
  T.Part->SetRelativeLocation(FMath::Lerp(T.From,T.To,Travel)+FVector(FMath::Sin(Travel*PI)*120,0,0));
 }
 for(auto& R:Reveals)if(R.Part.IsValid())R.Part->SetVisibility(true);
 for(auto& R:Reveals)if(R.Part.IsValid()&&!(R.Before?Age<R.At:Age>=R.At))R.Part->SetVisibility(false);

}
