// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MyCS : ModuleRules
{
	public MyCS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"NavigationSystem",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"MyCS",
			"MyCS/Variant_Horror",
			"MyCS/Variant_Horror/UI",
			"MyCS/Variant_Shooter",
			"MyCS/Variant_Shooter/AI",
			"MyCS/Variant_Shooter/UI",
			"MyCS/Variant_Shooter/Weapons",
			"MyCS/Variant_Multi"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
