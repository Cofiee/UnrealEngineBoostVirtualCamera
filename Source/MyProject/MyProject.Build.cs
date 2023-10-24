// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class MyProject : ModuleRules
{
	private string ProjectRoot
	{
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../")); }
	}

	private string ThirdPartyPath
	{
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, ProjectRoot, "ThirdParty")); }
	}

	public MyProject(ReadOnlyTargetRules Target) : base(Target)
	{
		// For boost::
		bEnableUndefinedIdentifierWarnings = false;
		bUseRTTI = true;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"HeadMountedDisplay", 
			"EnhancedInput", 
			"ImageWrapper", 
			"RenderCore",
			"Renderer",
			"RHI" 
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "MyLib", "include"));

		LoadBoostLibs(Target);
	}

	private bool LoadBoostLibs(ReadOnlyTargetRules Target)
	{
		bool isLibrarySupported = false;

		//libs
		string dirPath = Path.Combine(ThirdPartyPath, @"Boost\lib");
		DirectoryInfo d = new DirectoryInfo(dirPath);
		FileInfo[] Files = d.GetFiles("*.lib"); //Getting .lib files
		foreach (FileInfo file in Files)
		{
			PublicAdditionalLibraries.Add(Path.Combine(dirPath, file.Name));
		}
		System.Console.WriteLine("++++++++++++ Set Boost Libraries: " + dirPath + "\r");

		//includes
		string includesPath = Path.Combine(ThirdPartyPath, @"Boost\include");
		PublicIncludePaths.Add(includesPath);
		System.Console.WriteLine("++++++++++++ Set Boost Includes: " + includesPath + "\r");

		return isLibrarySupported;
	}
}
