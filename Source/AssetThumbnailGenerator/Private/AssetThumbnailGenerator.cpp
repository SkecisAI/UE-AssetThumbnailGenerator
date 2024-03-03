// Copyright 2024 skecis. All Rights Reserved.

#include "AssetThumbnailGenerator.h"

#include "AssetThumbnailGeneratorBFL.h"
#include "ContentBrowserMenuContexts.h"
#include "EditorValidatorSubsystem.h"

#define LOCTEXT_NAMESPACE "FAssetThumbnailGeneratorModule"

DEFINE_LOG_CATEGORY(LogAssetThumbnailGenerator)

void FAssetThumbnailGeneratorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu");
	Menu->AddDynamicSection("DynamicAssetThumbnail", FNewToolMenuDelegate::CreateLambda([](UToolMenu* InMenu)
	{
		const UContentBrowserAssetContextMenuContext* Context = InMenu->FindContext<UContentBrowserAssetContextMenuContext>();
		if (Context && Context->AssetContextMenu.IsValid())
		{
			FToolMenuSection& Section = InMenu->AddSection("AssetTrickSection", LOCTEXT("CustomAssetSectionName", "Asset Trick"));
			{
				auto CreateThumbnailSizeOptions = [Context](UToolMenu* SubMenu)
				{
					FToolMenuSection& SizeSection = SubMenu->AddSection("ThumbnailSizes");
					auto GetSizeName = [](int Size)
					{
						return FName(FString::Printf(TEXT("%dx%d"), Size, Size));	
					};

					for (int Size = 64; Size <= 2048; Size <<= 1)
					{
						SizeSection.AddMenuEntry(
							GetSizeName(Size),
							FText::FromName(GetSizeName(Size)),
							FText::GetEmpty(),
							FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([Size, Context]()
							{
								TArray<UObject*> Objects = Context->LoadSelectedObjectsIfNeeded();
								UEditorValidatorSubsystem* EditorValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
								if (IsValid(EditorValidatorSubsystem))
								{
									FValidateAssetsResults Results;
									FValidateAssetsSettings Settings;
									Settings.bSkipExcludedDirectories = true;
									Settings.bLoadAssetsForValidation = true;
									Settings.bShowIfNoFailures = false;
									
									EditorValidatorSubsystem->ValidateAssetsWithSettings(Context->SelectedAssets, Settings, Results);
								}
								
								for (const FAssetData& AssetData : Context->SelectedAssets)
								{
									UAssetThumbnailGeneratorBFL::GenerateThumbnail(AssetData.GetAsset(), Size, Size);
								}
								// for (UObject* Object : Objects)
								// {
								// 	UAssetThumbnailGeneratorBFL::GenerateThumbnail(Object, Size, Size);
								// }
							})),
							EUserInterfaceActionType::Button
							);
					}
				};
				
				Section.AddEntry(FToolMenuEntry::InitSubMenu(
					"ThumbnailSize",
					LOCTEXT("ExportThumbnailName", "Export thumbnail here"),
					LOCTEXT("ExportThumbnailName", "Export thumbnail for asset with _Thumb suffix. Tips: Sometimes it may be blank and need to be used more than once"),
					FNewToolMenuDelegate::CreateLambda(CreateThumbnailSizeOptions)
				));
			}
		}
	}));
}

void FAssetThumbnailGeneratorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAssetThumbnailGeneratorModule, AssetThumbnailGenerator)