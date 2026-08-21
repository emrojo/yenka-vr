using UnrealBuildTool;

public class YenkaVR : ModuleRules
{
	public YenkaVR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicIncludePaths.AddRange(new string[] {
			ModuleDirectory
		});

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			"HeadMountedDisplay",
			"PhysicsCore",
			"Chaos",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"OnlineSubsystemSteam",
			"SteamSockets",
			"VoiceChat"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { 
			"OpenXRHMD",
			"OpenXRHandTracking"
		});
	}
}
