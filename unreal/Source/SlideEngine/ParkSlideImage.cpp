#include "ParkSlideImage.h"
#include "Dom/JsonObject.h"
#include "Misc/Base64.h"
#include "ImageUtils.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Images/SImage.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Layout/SScaleBox.h"

class SParkSlideImage : public SImage {
 TStrongObjectPtr<UTexture2D> Texture;
 FSlateBrush Brush;
public:
 SLATE_BEGIN_ARGS(SParkSlideImage){}SLATE_ARGUMENT(TSharedPtr<FJsonObject>,Image)SLATE_END_ARGS()
 void Construct(const FArguments& Args) {
  TArray<uint8> Bytes;
  if(FBase64::Decode(Args._Image->GetStringField(TEXT("data")),Bytes))Texture.Reset(FImageUtils::ImportBufferAsTexture2D(Bytes));
  if(ensureMsgf(Texture.IsValid(),TEXT("Could not decode slide image: %s"),*Args._Image->GetStringField(TEXT("alt")))) {
   Brush.SetResourceObject(Texture.Get());Brush.ImageSize=FVector2D(Texture->GetSizeX(),Texture->GetSizeY());Brush.DrawAs=ESlateBrushDrawType::Image;
   UE_LOG(LogTemp,Display,TEXT("SlideImage: decoded %dx%d PNG"),Texture->GetSizeX(),Texture->GetSizeY());
  }
  SImage::Construct(SImage::FArguments().Image(&Brush));
 }
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool)const override {
  // Convert authored image colors for the board's linear world-widget surface.
  FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(),&Brush,ESlateDrawEffect::ReverseGamma);
  return Layer;
 }
};
TSharedRef<SWidget> MakeParkSlideImage(TSharedPtr<FJsonObject> Image) {
 return SNew(SScaleBox).Stretch(EStretch::ScaleToFit).HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SParkSlideImage).Image(Image)];
}
