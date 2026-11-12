using UnrealBuildTool;

public class Labrynn : ModuleRules
{
	public Labrynn(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore",
			"RenderCore",
			"ApplicationCore",   // IPlatformInputDeviceMapper (inyección de teclas en pruebas)
			"AIModule",          // UAISense_Hearing: el jadeo emite ruido para una IA futura
			"OnlineSubsystem",
			"OnlineSubsystemUtils"
		});

		// Steam se carga como plugin en runtime (no hace falta linkearlo).
		DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");
	}
}

