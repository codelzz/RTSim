// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class WirelessSignal : ModuleRules
	{
		public WirelessSignal(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
					"../Shaders/Shared"
				}
				);

			PrivateIncludePaths.AddRange(
				new string[]
				{
					"../../../../../../../../Program Files/Epic Games/UE_4.27/Engine/Source/Runtime/Renderer/Private",
				}
			);
			
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"CoreUObject",
					"Engine",
					"Landscape",
					"RenderCore",
					"Renderer",
					"RHI",
					"Projects",
					"UnrealEd",
					"SlateCore",
					"Slate",
					"IntelOIDN",
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);
		}
	}
}
