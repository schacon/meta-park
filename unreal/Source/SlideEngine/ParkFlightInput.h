#pragma once
#include "Widgets/SLeafWidget.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

// Only the scene catches drags; the sidebar and application menu stay clickable.
class SParkFlightInput : public SLeafWidget {
public:
 SLATE_BEGIN_ARGS(SParkFlightInput) {}
  SLATE_ARGUMENT(TFunction<void(FVector2D)>, Look)
 SLATE_END_ARGS()
 void Construct(const FArguments& Args){Look=Args._Look;}
 virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(100,100);}
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32 Layer,const FWidgetStyle&,bool) const override{return Layer;}
 virtual FReply OnMouseButtonDown(const FGeometry&,const FPointerEvent& Event) override {
  if(Event.GetEffectingButton()==EKeys::LeftMouseButton)return FReply::Handled().UseHighPrecisionMouseMovement(AsShared());
  return FReply::Unhandled();
 }
 virtual FReply OnMouseMove(const FGeometry&,const FPointerEvent& Event) override {
  if(HasMouseCapture()){Look(Event.GetCursorDelta());return FReply::Handled();}
  return FReply::Unhandled();
 }
 virtual FReply OnMouseButtonUp(const FGeometry&,const FPointerEvent& Event) override {
  if(Event.GetEffectingButton()==EKeys::LeftMouseButton&&HasMouseCapture())return FReply::Handled().ReleaseMouseCapture();
  return FReply::Unhandled();
 }
private:
 TFunction<void(FVector2D)> Look;
};
