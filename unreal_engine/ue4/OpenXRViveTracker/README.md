# UE4 OpenXR Vive Tracker

Provides XR_HTCX_vive_tracker_interaction extension support


**I. Pre-requisites**

 1. For UE4.27.1, you need to apply the following Pull Request to your engine: 
    https://github.com/EpicGames/UnrealEngine/pull/8621
 2. For UE5 Early Access, ensure your build already includes the following commit:
    https://github.com/EpicGames/UnrealEngine/commit/a14fa0214533f4baa438eb7df057e22b3f5fcb86
 3. To use this source build, your project *must* be a C++ project. For more info check out: 
    https://docs.unrealengine.com/4.27/en-US/ProgrammingAndScripting/ProgrammingWithCPP/CPPProgrammingQuickStart/
 4. This plugin requires the engine's built-in OpenXR plugin
 5. An OpenXR runtime such as SteamVR that implements the XR_HTCX_vive_tracker_interaction extension.


**II. Building**

 1. If your project doesn't already have a "Plugins" folder, add it to the root directory of your project one (where your .uproject file resides)
 2. Under the "Plugins" directory, add the *entire* OpenXRViveTracker folder from this repo.
 3. Right-click on your .uproject file and select Generate Visual Studio Project Files.
 4. Open the generated .sln file by double clicking on it.
 5. Rebuild your entire project. (Build > Rebuild Solution)


**III. Key Components**
 1. **ViveTrackerComponenent** - This is a scene component that updates its world location from values obtained from an active openxr runtime. Make sure to set the "Tracker Role" property of the component to the assigned tracker role of your tracker in the runtime. You also need to set the "Player Start Location" to the world location of the PlayerStart in your level.
 2. **ViveTrackerFunctionLibrary** - Contains helper functions to interact with the plugin. The "Get Tracker Transform" function retrieves a tracker's base world location. You MUST add the PlayerStart location of your VR Pawn or Character in your level if it is not set to 0,0,0
 3. **OpenXRViveTracker Module** - Plugin's main module that extends the engine's built-in OpenXR plugin to support the XR_HTCX_vive_tracker_interaction extension.
 4. **RenderModels** - Under the plugin's content folder, you will find reference rendermodels of various trackers including Vive Tracker 1.0, Vive Tracker 3.0 and Tundra Labs' tracker.
 