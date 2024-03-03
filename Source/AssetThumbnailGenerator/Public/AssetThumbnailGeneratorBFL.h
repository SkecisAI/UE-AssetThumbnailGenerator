// Copyright 2024 skecis. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AssetThumbnailGeneratorBFL.generated.h"

/**
 * 
 */
UCLASS()
class ASSETTHUMBNAILGENERATOR_API UAssetThumbnailGeneratorBFL : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="AssetThumbnailGenerator")
	static void GenerateThumbnail(UObject* Object, const int32 ImageWidth = 256, const int32 ImageHeight = 256);

private:
	static FObjectThumbnail* GenerateThumbnail_Internal(UObject* Object, const int32 ImageWidth, const int32 ImageHeight);
};
