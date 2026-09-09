#include "SlideGameMode.h"

TArray<FParkMapPin> ASlideGameMode::StatusPins() const {
 static const TCHAR* Future[]={TEXT("SALAD BUFFERING"),TEXT("SNACK QUEUED"),TEXT("PILE PENDING"),TEXT("CLEVERLY IDLE"),TEXT("TOUR BUFFERING"),TEXT("ROTOR QUEUED"),TEXT("PLEASE HOLD")};
 static const TCHAR* Next[]={TEXT("LEAF ME ALONE"),TEXT("FEED THE LAWYER"),TEXT("CHECK THE PILE"),TEXT("HOLD YOUR BUTTS"),TEXT("SPARE NO CACHE"),TEXT("GET TO CHOPPA"),TEXT("LIFE FINDS A WAY")};
 static const TCHAR* Past[]={TEXT("TREE DOWNLOADED"),TEXT("LAWYER CONSUMED"),TEXT("PILE VERIFIED"),TEXT("CLEVER GIRL"),TEXT("CACHE SPARED"),TEXT("FLIGHT COMMITTED"),TEXT("MERGE GATE OPEN")};
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
