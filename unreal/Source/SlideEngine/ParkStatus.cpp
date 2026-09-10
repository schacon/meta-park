#include "SlideGameMode.h"

TArray<FParkMapPin> ASlideGameMode::StatusPins() const {
 static const TCHAR* Future[]={TEXT("SCIENCE QUEUED"),TEXT("ROTOR QUEUED"),TEXT("CLEVERLY IDLE"),TEXT("PLEASE HOLD"),TEXT("PILE PENDING"),TEXT("SNACK QUEUED"),TEXT("SALAD BUFFERING")};
 static const TCHAR* Next[]={TEXT("SPARE NO GENES"),TEXT("GET TO CHOPPA"),TEXT("HOLD YOUR BUTTS"),TEXT("LIFE FINDS A WAY"),TEXT("CHECK THE PILE"),TEXT("FEED THE LAWYER"),TEXT("LEAF ME ALONE")};
 static const TCHAR* Past[]={TEXT("GENES CHECKED IN"),TEXT("FLIGHT COMMITTED"),TEXT("CLEVER GIRL"),TEXT("MERGE GATE OPEN"),TEXT("PILE VERIFIED"),TEXT("LAWYER CONSUMED"),TEXT("TREE DOWNLOADED")};
 int32 Upcoming=INDEX_NONE;
 for(const auto& Pin:MapPins)if(!VisitedStations.Contains(Pin.SlideIndex)){Upcoming=Pin.SlideIndex;break;}
 auto Result=MapPins;
 for(auto& Pin:Result) {
  const int32 I=Pin.SlideIndex;if(I<0||I>=UE_ARRAY_COUNT(Future))continue;
  if(VisitedStations.Contains(I)) {
   Pin.StatusStage=-1;Pin.Status=Past[I];Pin.StatusColor=I%2?FLinearColor(.39,.41,.37):FLinearColor(.15,.61,.25);
  } else if(I==Upcoming) {
   Pin.StatusStage=1;Pin.Status=Next[I];
   Pin.StatusColor=FMath::Fmod(Elapsed,.8f)<.4f?FLinearColor(.85,.035,.02):FLinearColor(.28,.012,.008);
   Pin.StatusInk=FLinearColor(.96,.96,.88);
  } else {Pin.StatusStage=0;Pin.Status=Future[I];Pin.StatusColor=FLinearColor(.18,.43,.75);}
 }
 return Result;
}
