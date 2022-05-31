/*
Copyright 2021 Valve Corporation under https://opensource.org/licenses/BSD-3-Clause

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "GenericPlatform/IInputInterface.h"
#include "XRMotionControllerBase.h"
#include "InputCoreTypes.h"
#include "IInputDevice.h"
#include "IHandTracker.h"

#include "tracker_openxr/openxr.h"
#include "tracker_openxr/openxr_reflection.h"

#include "IOpenXRExtensionPlugin.h"
#include "OpenXRCore.h"


UENUM()
enum ETrackerRole
{
	Foot_L		UMETA(DisplayName = "Foot (L)"),
	Foot_R		UMETA(DisplayName = "Foot (R)"),
	Shoulder_L  UMETA(DisplayName = "Shoulder (L)"),
	Shoulder_R  UMETA(DisplayName = "Shoulder (R)"),
	Elbow_L     UMETA(DisplayName = "Elbow (L)"),
	Elbow_R     UMETA(DisplayName = "Elbow (R)"),
	Knee_L		UMETA(DisplayName = "Knee (L)"),
	Knee_R		UMETA(DisplayName = "Knee (R)"),
	Waist		UMETA(DisplayName = "Waist"),
	Chest		UMETA(DisplayName = "Chest"),
	Camera		UMETA(DisplayName = "Camera"),
	Keyboard    UMETA(DisplayName = "Keyboard"),
	Unassigned	UMETA(DisplayName = "Unassigned"),
};

class FOpenXRViveTrackerModule : 
	public IModuleInterface,
	public IOpenXRExtensionPlugin,
	public IInputDevice,
	public FXRMotionControllerBase
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** IOpenXRExtensionPlugin */
	virtual FString GetDisplayName() override
	{
		return FString(TEXT("OpenXRViveTracker"));
	}

	virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
	virtual void PostCreateInstance(XrInstance InInstance) override;
	virtual void PostCreateSession(XrSession InSession) override;
	virtual void UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace) override;
	virtual void OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader) override;
	virtual void AddActionSets(TArray<XrActiveActionSet>& OutActionSets) override;
	virtual void PostSyncActions(XrSession InSession) override;

	/** IMotionController interface */
	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const FName MotionSource, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const override;
	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const override;
	virtual ETrackingStatus GetControllerTrackingStatus(const int32 ControllerIndex, const EControllerHand DeviceHand) const override;
	virtual FName GetMotionControllerDeviceTypeName() const override;
	virtual void EnumerateSources(TArray<FMotionControllerSource>& SourcesOut) const override;

	/** IInputDevice interface */
	virtual void Tick(float DeltaTime) override;
	virtual void SendControllerEvents() override;
	virtual void SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) override;
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	virtual void SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value) override {};
	virtual void SetChannelValues(int32 ControllerId, const FForceFeedbackValues& values) override {};
	virtual bool IsGamepadAttached() const override { return false; };

	/**
	* Check whether or not the tracker pose actions have all been generated
	* @return bool - Whether or not tracker pose actions have been generated
	*/
	bool IsActionsGenerated() { return m_bActionsGenerated;  }

	/**
	* Retrieve all the pose actions generated by this plugin
	* @return TArray<XrAction> - TArray of pose actions generated by this plugin
	*/
	TArray<XrAction>* GetPoseActions() { return &m_arrPoseActions; };

	/**
	* Retrieve a map of the pose action and it's corresponding location space
	* @return TMap<XrAction, XrSpace> - TMap of actions and their corresponding location space
	*/
	TMap<XrAction, XrSpace>* GetTrackerPoses() { return &m_mapActionSpace; }

	/**
	* Getter for the current predicted display time
	* @return XrTime - The current predicted display time
	*/
	XrTime GetPredictedDisplayTime() { return m_predictedDisplayTime; }

	/**
	* Getter for the application's base space
	* @return XrSpace - The application's base space
	*/
	XrSpace GetBaseSpace() { return m_baseSpace; }

	/**
	* Obtain the tracker transform from a give role
	* @param ETrackerRole - The assigned role of the tracker you want the transform of
	* @return FTransform - The transform of the tracker
	*/
	FTransform GetTrackerTransform(ETrackerRole trackerRole);

	// Singleton-like getter
	static inline FOpenXRViveTrackerModule& Get() { return FModuleManager::LoadModuleChecked<FOpenXRViveTrackerModule>("OpenXRViveTracker"); }

private:
	XrInstance m_xrInstance = XR_NULL_HANDLE;
	XrSession m_xrSession = XR_NULL_HANDLE;
	XrActionSet m_xrActionSet = XR_NULL_HANDLE;
	XrSessionState m_xrCurrentSessionState = XR_SESSION_STATE_UNKNOWN;

	bool m_bActionsGenerated = false;
	TArray<XrAction> m_arrPoseActions;
	TMap<XrAction, XrSpace> m_mapActionSpace;
	TArray<XrActionSuggestedBinding> m_arrActionBindings;

	XrTime m_predictedDisplayTime;
	XrSpace m_baseSpace = XR_NULL_HANDLE;

	TMap<XrAction, ETrackerRole> m_mapTrackerRoles;
	TMap<ETrackerRole, FTransform> m_mapTrackerTransforms;

	XrAction CreatePoseAction(const char* pName);
	void CreateTrackerBinding(ETrackerRole role, XrAction xrAction);
};

DEFINE_LOG_CATEGORY_STATIC(LogOpenXRViveTracker, Display, All);
