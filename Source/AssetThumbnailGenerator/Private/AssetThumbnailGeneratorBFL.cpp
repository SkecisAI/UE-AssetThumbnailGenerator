// Copyright 2024 skecis. All Rights Reserved.


#include "AssetThumbnailGeneratorBFL.h"

#include "AssetThumbnailGenerator.h"
#include "EditorValidatorSubsystem.h"
#include "ObjectTools.h"
#include "TextureCompiler.h"
#include "UnrealEdGlobals.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor/UnrealEdEngine.h"
#include "Misc/ScopedSlowTask.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "UObject/SavePackage.h"

constexpr int32 MAX_IMAGE_SIZE = 2048;

void UAssetThumbnailGeneratorBFL::GenerateThumbnail(UObject* Object, const int32 ImageWidth, const int32 ImageHeight)
{
	if (ImageWidth > MAX_IMAGE_SIZE || ImageHeight > MAX_IMAGE_SIZE)
	{
		UE_LOG(LogAssetThumbnailGenerator, Warning, TEXT("Size not supported!"));
		return;
	}
	
	if (!IsValid(Object))
	{
		UE_LOG(LogAssetThumbnailGenerator, Error, TEXT("Invalid object!"));
		return;
	}
	
	FObjectThumbnail* ObjectThumbnail = GenerateThumbnail_Internal(Object, ImageWidth, ImageHeight);
	if (!ObjectThumbnail)
	{
		UE_LOG(LogAssetThumbnailGenerator, Warning, TEXT("Generate thumbnail for %s failed"), *Object->GetName());
		return;
	}
	
	TArray<uint8> ImageData = ObjectThumbnail->GetUncompressedImageData();

	FString ThumbnailName = Object->GetName() + TEXT("_Thumb");
	FString PackagePathName = FPackageName::GetLongPackagePath(Object->GetPathName()) / ThumbnailName;

	UPackage* Package = CreatePackage(*PackagePathName);
	Package->FullyLoad();
	
	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, FName(ThumbnailName), RF_Public | RF_Standalone | RF_MarkAsRootSet);
	NewTexture->AddToRoot();
	FTexturePlatformData* PlatformData = new FTexturePlatformData();
	PlatformData->SizeX = ObjectThumbnail->GetImageWidth();
	PlatformData->SizeY = ObjectThumbnail->GetImageHeight();
	PlatformData->PixelFormat = PF_B8G8R8A8;
	NewTexture->SetPlatformData(PlatformData);
	
	FTexture2DMipMap* MipMap = new FTexture2DMipMap();
	PlatformData->Mips.Add(MipMap);
	MipMap->SizeX = ObjectThumbnail->GetImageWidth();
	MipMap->SizeY = ObjectThumbnail->GetImageHeight();

	MipMap->BulkData.Lock(LOCK_READ_WRITE);
	uint8* TextureData = MipMap->BulkData.Realloc(ImageData.Num() * 4);
	FMemory::Memcpy(TextureData, ImageData.GetData(), ImageData.Num());
	MipMap->BulkData.Unlock();

	NewTexture->Source.Init(ObjectThumbnail->GetImageWidth(), ObjectThumbnail->GetImageHeight(), 1, 1, TSF_BGRA8, ImageData.GetData());
	NewTexture->LODGroup = TEXTUREGROUP_UI;
	NewTexture->UpdateResource();
	Package->MarkPackageDirty();
	Package->FullyLoad();
	FAssetRegistryModule::AssetCreated(NewTexture);
	
	FSavePackageArgs SPA;
	SPA.TopLevelFlags = RF_Public | RF_Standalone;
	SPA.SaveFlags = SAVE_NoError;
	SPA.bForceByteSwapping = true;
	FString PackageFilename = FPackageName::LongPackageNameToFilename(PackagePathName, FPackageName::GetAssetPackageExtension());
	UPackage::SavePackage(Package, NewTexture, *PackageFilename, SPA);
}

FObjectThumbnail* UAssetThumbnailGeneratorBFL::GenerateThumbnail_Internal(UObject* Object, const int32 ImageWidth, const int32 ImageHeight)
{
	FThumbnailRenderingInfo* RenderInfo = GUnrealEd ? GUnrealEd->GetThumbnailManager()->GetRenderingInfo( Object ) : nullptr;
	if( RenderInfo != NULL && RenderInfo->Renderer != NULL )
	{
		ThumbnailTools::EThumbnailTextureFlushMode::Type TextureFlushMode = ThumbnailTools::EThumbnailTextureFlushMode::NeverFlush;

		if ( UTexture* Texture = Cast<UTexture>(Object))
		{
			FTextureCompilingManager::Get().FinishCompilation({Texture});
			Texture->WaitForStreaming();
		}

		if ( UMaterial* InMaterial = Cast<UMaterial>(Object) )
		{
			FScopedSlowTask SlowTask(0, NSLOCTEXT( "ObjectTools", "FinishingCompilationStatus", "Finishing Shader Compilation..." ) );
			SlowTask.MakeDialog();

			FMaterialResource* CurrentResource = InMaterial->GetMaterialResource(GMaxRHIFeatureLevel);
			if (CurrentResource)
			{
				if (!CurrentResource->IsGameThreadShaderMapComplete())
				{
					CurrentResource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::High);
				}
				CurrentResource->FinishCompilation();
			}
		}

		// Generate the thumbnail
		FObjectThumbnail NewThumbnail;
		ThumbnailTools::RenderThumbnail(Object, ImageWidth, ImageHeight, TextureFlushMode, nullptr, &NewThumbnail);

		UPackage* MyOutermostPackage = Object->GetOutermost();
		return ThumbnailTools::CacheThumbnail( Object->GetFullName(), &NewThumbnail, MyOutermostPackage );
	}

	return nullptr;
}
