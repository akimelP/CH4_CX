// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CX : ModuleRules
{
	public CX(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"ChaosVehicles",
			"PhysicsCore",
			"UMG",
			"Slate",
            "SlateCore"
        });

		PublicIncludePaths.AddRange(new string[] {
			"CX",
			"CX/SportsCar",
			"CX/OffroadCar",
			"CX/Variant_Offroad",
			"CX/Variant_TimeTrial",
			"CX/Variant_TimeTrial/UI"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
