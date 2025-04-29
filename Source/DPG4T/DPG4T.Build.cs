// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class DPG4T : ModuleRules
{
	public DPG4T(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Niagara", "MetasoundEngine", "GameplayCameras", "AnimGraphRuntime" });
	}
}
