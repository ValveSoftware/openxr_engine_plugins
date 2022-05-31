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

#include "OpenXRViveTracker.h"

#define LOCTEXT_NAMESPACE "FOpenXRViveTrackerModule"

void FOpenXRViveTrackerModule::StartupModule()
{
	// Register this plugin as an OpenXR plugin	
	RegisterOpenXRExtensionModularFeature();

	// Fill-out tracker transform map
	m_mapTrackerTransforms.Add(ETrackerRole::Foot_L, FTransform::Identity);
	m_mapTrackerTransforms.Add(ETrackerRole::Foot_R, FTransform::Identity);
	m_mapTrackerTransforms.Add(ETrackerRole::Shoulder_L, FTransform::Identity);
	m_mapTrackerTransforms.Add(ETrackerRole::Shoulder_R, FTransform::Identity);
	m_mapTrackerTransforms.Add(ETrackerRole::Elbow_L, FTransform::Identity);
	m_mapTrackerTransforms.Add(ETrackerRole::Elbow_R, FTransform::Identity);
	m_mapTrackerTransforms.Add(ETrackerRole::Knee_L, FTransform::Identity);
	m_mapTrackerTransforms.Add(ETrackerRole::Knee_R, FTransform::Identity);
	m_mapTrackerTransforms.Add(ETrackerRole::Waist, FTransform::Identity);
	m_mapTrackerTransforms.Add(ETrackerRole::Chest, FTransform::Identity);
	m_mapTrackerTransforms.Add(ETrackerRole::Camera, FTransform::Identity);
	m_mapTrackerTransforms.Add(ETrackerRole::Keyboard, FTransform::Identity);

	UE_LOG( LogOpenXRViveTracker, Display, TEXT("Plugin started. OpenXR extension %s will be enabled."), 
		*FString(UTF8_TO_TCHAR(XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME)) );
}

void FOpenXRViveTrackerModule::ShutdownModule()
{
	// Cleanup actions
	for (auto& Action : m_mapTrackerRoles)
	{
		if (Action.Key != XR_NULL_HANDLE)
		{
			xrDestroyAction(Action.Key);
		}
	}

	// Cleanup action set
	if (m_xrActionSet != XR_NULL_HANDLE)
	{
		xrDestroyActionSet(m_xrActionSet);
	}

	UE_LOG(LogOpenXRViveTracker, Display, TEXT("Plugin shut down."));
}

bool FOpenXRViveTrackerModule::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	OutExtensions.Add(XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	return true;
}

void FOpenXRViveTrackerModule::PostCreateInstance(XrInstance InInstance)
{
	// Cache instance handle
	m_xrInstance = InInstance;

	// Create action set that'll host all tracker role actions
	XrActionSetCreateInfo xrActionSetCreateInfo{ XR_TYPE_ACTION_SET_CREATE_INFO };
	strcpy_s(xrActionSetCreateInfo.actionSetName, XR_MAX_ACTION_SET_NAME_SIZE, "tracker_actionset");
	strcpy_s(xrActionSetCreateInfo.localizedActionSetName, XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE, "Actionset for vive tracker actions");
	xrActionSetCreateInfo.priority = 0;

	XrResult result = xrCreateActionSet(m_xrInstance, &xrActionSetCreateInfo, &m_xrActionSet);
	
	if (result != XR_SUCCESS)
	{
		UE_LOG(LogOpenXRViveTracker, Error, TEXT("Error - Unable to create action set. Runtime returned error code (%i)"), (int32_t) result);
	}
	else
	{
		UE_LOG(LogOpenXRViveTracker, Display, TEXT("Created action set for trackers [tracker_actionset]"));
	}

}

void FOpenXRViveTrackerModule::PostCreateSession(XrSession InSession)
{
	// Cache session handle
	m_xrSession = InSession;

	// Bind tracker actions
	CreateTrackerBinding(ETrackerRole::Foot_L, CreatePoseAction("tracker_foot_l"));
	CreateTrackerBinding(ETrackerRole::Foot_R, CreatePoseAction("tracker_foot_r"));
	CreateTrackerBinding(ETrackerRole::Shoulder_L, CreatePoseAction("tracker_shoulder_l"));
	CreateTrackerBinding(ETrackerRole::Shoulder_R, CreatePoseAction("tracker_shoulder_r"));
	CreateTrackerBinding(ETrackerRole::Elbow_L, CreatePoseAction("tracker_elbow_l"));
	CreateTrackerBinding(ETrackerRole::Elbow_R, CreatePoseAction("tracker_elbow_r"));
	CreateTrackerBinding(ETrackerRole::Knee_L, CreatePoseAction("tracker_knee_l"));
	CreateTrackerBinding(ETrackerRole::Knee_R, CreatePoseAction("tracker_knee_r"));
	CreateTrackerBinding(ETrackerRole::Waist, CreatePoseAction("tracker_waist"));
	CreateTrackerBinding(ETrackerRole::Chest, CreatePoseAction("tracker_chest"));
	CreateTrackerBinding(ETrackerRole::Camera, CreatePoseAction("tracker_camera"));
	CreateTrackerBinding(ETrackerRole::Keyboard, CreatePoseAction("tracker_keyboard"));

	m_bActionsGenerated = true;

	// Bind actions to tracker interaction profile and suggest bindings
	if (m_arrActionBindings.Num() > 0)
	{
		XrPath xrPath;
		xrStringToPath(m_xrInstance, "/interaction_profiles/htc/vive_tracker_htcx", &xrPath);

		XrInteractionProfileSuggestedBinding xrInteractionProfileSuggestedBinding{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
		xrInteractionProfileSuggestedBinding.interactionProfile = xrPath;
		xrInteractionProfileSuggestedBinding.suggestedBindings = m_arrActionBindings.GetData();
		xrInteractionProfileSuggestedBinding.countSuggestedBindings = (uint32_t)m_arrActionBindings.Num();

		XrResult result = xrSuggestInteractionProfileBindings(m_xrInstance, &xrInteractionProfileSuggestedBinding);

		if (result != XR_SUCCESS)
			UE_LOG(LogOpenXRViveTracker, Error, TEXT("Unable to suggest vive tracker interaction profile bindings to runtime (%i)"), (int32_t)result);
	}
}


void FOpenXRViveTrackerModule::UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace)
{
	m_predictedDisplayTime = DisplayTime;
	m_baseSpace = TrackingSpace;
}

void FOpenXRViveTrackerModule::OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader)
{
	if (m_xrInstance == XR_NULL_HANDLE)
		return;

	const XrEventDataSessionStateChanged& xrEventDataSessionStateChanged = *reinterpret_cast<const XrEventDataSessionStateChanged*>(InHeader);
	const XrEventDataViveTrackerConnectedHTCX& xrEventDataViveTrackerConnectedHTCX = *reinterpret_cast<const XrEventDataViveTrackerConnectedHTCX*>(InHeader);

	// Check for session state change
	if (InHeader->type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED && xrEventDataSessionStateChanged.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
	{
		m_xrCurrentSessionState = xrEventDataSessionStateChanged.state;

		//UE_LOG(LogOpenXRViveTracker, Display, TEXT("Session State changing from %s to %s"),
		//	XrEnumToString(m_xrCurrentSessionState), XrEnumToString(xrEventDataSessionStateChanged.state));
	}

	// Check for newly connected tracker
	else if (InHeader->type == XR_TYPE_EVENT_DATA_VIVE_TRACKER_CONNECTED_HTCX && xrEventDataViveTrackerConnectedHTCX.type == XR_TYPE_EVENT_DATA_VIVE_TRACKER_CONNECTED_HTCX)
	{
		// Log tracker reported by runtime
		uint32_t nCount;
		char sPersistentPath[XR_MAX_PATH_LENGTH];
		char sRolePath[XR_MAX_PATH_LENGTH];

		xrPathToString(m_xrInstance, xrEventDataViveTrackerConnectedHTCX.paths->persistentPath, sizeof(sPersistentPath), &nCount, sPersistentPath);
		xrPathToString(m_xrInstance, xrEventDataViveTrackerConnectedHTCX.paths->rolePath, sizeof(sRolePath), &nCount, sRolePath);

		UE_LOG( LogOpenXRViveTracker, Display, TEXT("Tracker connected event received for [%s] with role [%s]"), 
			*FString(UTF8_TO_TCHAR(sPersistentPath)), *FString(UTF8_TO_TCHAR(sRolePath)) );


		// Report all active trackers
		PFN_xrEnumerateViveTrackerPathsHTCX xrEnumerateViveTrackerPathsHTCX = nullptr;
		XrResult result = xrGetInstanceProcAddr(m_xrInstance, "xrEnumerateViveTrackerPathsHTCX", (PFN_xrVoidFunction*)&xrEnumerateViveTrackerPathsHTCX);

		uint32_t nPaths;
		TArray< XrViveTrackerPathsHTCX > allViveTrackerPaths;
		result = xrEnumerateViveTrackerPathsHTCX(m_xrInstance, 0, &nPaths, nullptr);
		if (result == XR_SUCCESS)
		{
			UE_LOG(LogOpenXRViveTracker, Display, TEXT("Number of tracker paths now active is %i"), nPaths);

			for (size_t i = 0; i < nPaths; i++)
			{
				XrViveTrackerPathsHTCX xrViveTrackerPaths{ XR_TYPE_VIVE_TRACKER_PATHS_HTCX };
				allViveTrackerPaths.Add(xrViveTrackerPaths);
			}

			result = xrEnumerateViveTrackerPathsHTCX(m_xrInstance, allViveTrackerPaths.Num(), &nPaths, allViveTrackerPaths.GetData());
		}
	}
}

void FOpenXRViveTrackerModule::AddActionSets(TArray<XrActiveActionSet>& OutActionSets)
{
	XrActiveActionSet xrActiveActionSet{ m_xrActionSet, XR_NULL_PATH };
	OutActionSets.Add(xrActiveActionSet);
}

void FOpenXRViveTrackerModule::PostSyncActions(XrSession InSession)
{
	if (GetBaseSpace() == XR_NULL_HANDLE && m_arrPoseActions.Num() > 0)
		return;

	for (XrAction action : m_arrPoseActions)
	{
		XrSpace* pSpace = m_mapActionSpace.Find(action);
		ETrackerRole* pRole = m_mapTrackerRoles.Find(action);

		if (pSpace && pRole)
		{
			XrSpaceLocation spaceLocation{ XR_TYPE_SPACE_LOCATION };
			XrResult result = xrLocateSpace(*pSpace, GetBaseSpace(), GetPredictedDisplayTime(), &spaceLocation);

			// Update tracker poses
			if (result == XR_SUCCESS)
			{
				FTransform* trackerTransform = m_mapTrackerTransforms.Find(*pRole);
				if (trackerTransform)
				{
					if (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT &&
						spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
					{
						// Set orientation
						trackerTransform->SetRotation(ToFQuat(spaceLocation.pose.orientation));

						// Set location
						FVector location = ToFVector(spaceLocation.pose.position, 100.f);
						trackerTransform->SetLocation(FVector(location.X, location.Y, location.Z));

						//FString sRole = StaticEnum<ETrackerRole>()->GetValueAsString(*pRole);
						//UE_LOG(LogOpenXRViveTracker, Display, TEXT("[Tracker %s] x[%f] y[%f] z[%f]"), *sRole,
						//	trackerTransform->GetLocation().X, trackerTransform->GetLocation().Y, trackerTransform->GetLocation().Z);
					}
				}
			}
			else
			{
				FString sRole = StaticEnum<ETrackerRole>()->GetValueAsString(*pRole);
				UE_LOG(LogOpenXRViveTracker, Error, TEXT("Unable to get tracker pose for role (%s), error. Runtime returned error (%i)"), 
					*sRole, (int32_t)result);
			}
		}
	}
}

bool FOpenXRViveTrackerModule::GetControllerOrientationAndPosition(const int32 ControllerIndex, const FName MotionSource, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const
{
	return true;
}

bool FOpenXRViveTrackerModule::GetControllerOrientationAndPosition(const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const
{
	return true;
}

ETrackingStatus FOpenXRViveTrackerModule::GetControllerTrackingStatus(const int32 ControllerIndex, const EControllerHand DeviceHand) const
{
	return ETrackingStatus::NotTracked;
}

FName FOpenXRViveTrackerModule::GetMotionControllerDeviceTypeName() const
{
	return FName("ViveTracker");
}

void FOpenXRViveTrackerModule::EnumerateSources(TArray<FMotionControllerSource>& SourcesOut) const
{

}

void FOpenXRViveTrackerModule::Tick(float DeltaTime)
{

}

void FOpenXRViveTrackerModule::SendControllerEvents()
{

}

void FOpenXRViveTrackerModule::SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
{

}

bool FOpenXRViveTrackerModule::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	return true;
}


FTransform FOpenXRViveTrackerModule::GetTrackerTransform(ETrackerRole trackerRole)
{
	if (trackerRole == ETrackerRole::Unassigned)
		return FTransform::Identity;

	FTransform* trackerTransform = m_mapTrackerTransforms.Find(trackerRole);
	if (trackerTransform)
		return *trackerTransform;

	UE_LOG(LogOpenXRViveTracker, Warning, TEXT("Unable to obtain tracker pose - Unknown tracker role."));
	return FTransform::Identity;
}

XrAction FOpenXRViveTrackerModule::CreatePoseAction(const char* pName)
{
	if (m_xrSession == XR_NULL_HANDLE || m_bActionsGenerated)
		return XR_NULL_HANDLE;

	// Create action
	XrActionCreateInfo xrActionCreateInfo{ XR_TYPE_ACTION_CREATE_INFO };
	strcpy_s(xrActionCreateInfo.actionName, XR_MAX_ACTION_SET_NAME_SIZE, pName);
	strcpy_s(xrActionCreateInfo.localizedActionName, XR_MAX_ACTION_SET_NAME_SIZE, pName);
	xrActionCreateInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
	xrActionCreateInfo.countSubactionPaths = 0;
	xrActionCreateInfo.subactionPaths = NULL;

	XrAction xrAction = XR_NULL_HANDLE;
	XrResult result = xrCreateAction(m_xrActionSet, &xrActionCreateInfo, &xrAction);

	if (result == XR_SUCCESS)
	{
		// Create a corresponding action space
		XrPosef xrPose{};
		xrPose.orientation.w = 1.f;

		XrActionSpaceCreateInfo xrActionSpaceCreateInfo{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
		xrActionSpaceCreateInfo.action = xrAction;
		xrActionSpaceCreateInfo.poseInActionSpace = xrPose;
		xrActionSpaceCreateInfo.subactionPath = XR_NULL_PATH;

		XrSpace xrSpace = XR_NULL_HANDLE;
		result = xrCreateActionSpace(m_xrSession, &xrActionSpaceCreateInfo, &xrSpace);

		if (result == XR_SUCCESS)
		{
			// Add action to array of created actions for this session
			m_arrPoseActions.Add(xrAction);
			m_mapActionSpace.Add(xrAction, xrSpace);
			UE_LOG(LogOpenXRViveTracker, Display, TEXT("Created tracker pose action [%s]"), *FString(UTF8_TO_TCHAR(pName)));
		}
		else
		{
			UE_LOG(LogOpenXRViveTracker, Error, TEXT("Unable to create an action space for action %s. Runtime returned error (%i)"), 
				*FString(UTF8_TO_TCHAR(pName)), (int32_t) (result));
			return XR_NULL_HANDLE;
		}
	}
	else
	{
		UE_LOG(LogOpenXRViveTracker, Error, TEXT("Unable to create action %s. Runtime returned error (%i)"),
			*FString(UTF8_TO_TCHAR(pName)), (int32_t)(result));
		return XR_NULL_HANDLE;
	}

	return xrAction;
}

void FOpenXRViveTrackerModule::CreateTrackerBinding(ETrackerRole role, XrAction xrAction)
{
	if (m_xrInstance == XR_NULL_HANDLE || xrAction == XR_NULL_HANDLE || m_bActionsGenerated)
		return;

	
	FString sInputPath;
	switch (role)
	{
	case Foot_L:
		sInputPath = "/user/vive_tracker_htcx/role/left_foot/input/grip/pose";
		break;
	case Foot_R:
		sInputPath = "/user/vive_tracker_htcx/role/right_foot/input/grip/pose";
		break;
	case Shoulder_L:
		sInputPath = "/user/vive_tracker_htcx/role/left_shoulder/input/grip/pose";
		break;
	case Shoulder_R:
		sInputPath = "/user/vive_tracker_htcx/role/right_shoulder/input/grip/pose";
		break;
	case Elbow_L:
		sInputPath = "/user/vive_tracker_htcx/role/left_elbow/input/grip/pose";
		break;
	case Elbow_R:
		sInputPath = "/user/vive_tracker_htcx/role/right_elbow/input/grip/pose";
		break;
	case Knee_L:
		sInputPath = "/user/vive_tracker_htcx/role/left_knee/input/grip/pose";
		break;
	case Knee_R:
		sInputPath = "/user/vive_tracker_htcx/role/right_knee/input/grip/pose";
		break;
	case Waist:
		sInputPath = "/user/vive_tracker_htcx/role/waist/input/grip/pose";
		break;
	case Chest:
		sInputPath = "/user/vive_tracker_htcx/role/chest/input/grip/pose";
		break;
	case Camera:
		sInputPath = "/user/vive_tracker_htcx/role/camera/input/grip/pose";
		break;
	case Keyboard:
		sInputPath = "/user/vive_tracker_htcx/role/keyboard/input/grip/pose";
		break;
	case Unassigned:
	default:
		return;
		break;
	}


	XrPath xrPath;
	XrResult result = xrStringToPath(m_xrInstance, TCHAR_TO_UTF8(*sInputPath), &xrPath);

	if (result == XR_SUCCESS)
	{
		// Add action binding
		XrActionSuggestedBinding xrActionSuggestedBinding;
		xrActionSuggestedBinding.action = xrAction;
		xrActionSuggestedBinding.binding = xrPath;
		m_arrActionBindings.Add(xrActionSuggestedBinding);

		// Add to tracker roles map
		m_mapTrackerRoles.Add(xrAction, role);
		UE_LOG(LogOpenXRViveTracker, Display, TEXT("... bound to [%s]"), *sInputPath);
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOpenXRViveTrackerModule, OpenXRViveTracker)