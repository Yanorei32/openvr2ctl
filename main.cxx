#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <numbers>
#include <string>
#include <thread>

#ifdef _MSC_BUILD
#include <fcntl.h>
#include <io.h>
#endif

#ifdef __CYGWIN__
#include <windows.h>
#endif

#include "openvr.h"

struct Vector2f_t {
	float x, y;
};

Vector2f_t getVector2ActionState(
	vr::VRActionHandle_t action
) {
	Vector2f_t v = {0};

	vr::InputAnalogActionData_t actionData;

	vr::VRInput()->GetAnalogActionData(
		action,
		&actionData,
		sizeof(actionData),
		vr::k_ulInvalidInputValueHandle
	);

	if (actionData.bActive) {
		v.x = actionData.x;
		v.y = actionData.y;
	}

	return v;
}

float getVector1ActionState(
	vr::VRActionHandle_t action
) {
	vr::InputAnalogActionData_t actionData;

	vr::VRInput()->GetAnalogActionData(
		action,
		&actionData,
		sizeof(actionData),
		vr::k_ulInvalidInputValueHandle
	);

	return actionData.bActive ? actionData.x : 0.0f;
}

bool getDigitalActionState(
	vr::VRActionHandle_t action
) {
	vr::InputDigitalActionData_t actionData;

	vr::VRInput()->GetDigitalActionData(
		action,
		&actionData,
		sizeof(actionData),
		vr::k_ulInvalidInputValueHandle
	);

	return actionData.bActive && actionData.bState;
}

bool getDigitalActionRisingEdge(
	vr::VRActionHandle_t action
) {
	vr::InputDigitalActionData_t actionData;

	vr::VRInput()->GetDigitalActionData(
		action,
		&actionData,
		sizeof(actionData),
		vr::k_ulInvalidInputValueHandle
	);

	return actionData.bActive && actionData.bChanged && actionData.bState;
}

float rad2deg(float rad) {
	return rad * 180 / std::numbers::pi;
}

int main(int, char**) {
#ifdef _MSC_BUILD
	setmode(fileno(stdout), O_BINARY);
#endif

	std::cerr << "VR Application Overlay initialization" << std::endl;

	vr::EVRInitError error = vr::VRInitError_None;
	vr::VR_Init(&error, vr::VRApplication_Overlay);

	if (error != vr::VRInitError_None) {
		std::cerr << __LINE__ << ": ";
		std::cerr << "error " << vr::VR_GetVRInitErrorAsSymbol(error) << std::endl;

		return error;
	}

	std::cerr << "Action set initialization" << std::endl;

	vr::VRActionSetHandle_t	defaultSetHandle			= vr::k_ulInvalidActionSetHandle;
	vr::VRActionHandle_t	defaultToggleControl		= vr::k_ulInvalidActionHandle;
	vr::VRActionSetHandle_t	controlSetHandle			= vr::k_ulInvalidActionSetHandle;
	vr::VRActionHandle_t	controlSnapTurnLeft			= vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t	controlSnapTurnRight		= vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t	controlSpeedVector1			= vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t	controlSpeedVector2			= vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t	controlDirection			= vr::k_ulInvalidActionHandle;


	auto actionsPathCptr = (std::filesystem::current_path() / "actions.json").string().c_str();

#ifdef __CYGWIN__
	char winpath[MAX_PATH];
	GetModuleFileName(NULL, winpath, MAX_PATH);
	std::strcpy(std::strrchr(winpath, '\\') + 1, "actions.json");

	actionsPathCptr = winpath;
#endif

	vr::VRInput()->SetActionManifestPath(
		actionsPathCptr
	);

	vr::VRInput()->GetActionSetHandle(
		"/actions/default",
		&defaultSetHandle
	);

	vr::VRInput()->GetActionHandle(
		"/actions/default/in/ToggleControl",
		&defaultToggleControl
	);

	vr::VRInput()->GetActionSetHandle(
		"/actions/control",
		&controlSetHandle
	);

	vr::VRInput()->GetActionHandle(
		"/actions/control/in/SnapTurnLeft",
		&controlSnapTurnLeft
	);

	vr::VRInput()->GetActionHandle(
		"/actions/control/in/SnapTurnRight",
		&controlSnapTurnRight
	);

	vr::VRInput()->GetActionHandle(
		"/actions/control/in/SpeedVector1",
		&controlSpeedVector1
	);

	vr::VRInput()->GetActionHandle(
		"/actions/control/in/SpeedVector2",
		&controlSpeedVector2
	);

	vr::VRInput()->GetActionHandle(
		"/actions/control/in/Direction",
		&controlDirection
	);

	vr::VRActiveActionSet_t sets[2] = { 0 };
	sets[0].ulActionSet = defaultSetHandle;
	sets[1].ulActionSet = controlSetHandle;
	sets[1].nPriority = 0x01FFFFFF;

	std::cerr << "Enter to main loop" << std::endl;

	bool inControl = true;
	float oldSpeed = NAN, oldDirection = NAN;

	while (1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 100));

		if (!inControl) {
			vr::VRInput()->UpdateActionState(&sets[0], sizeof(sets[0]), 1);

			if (getDigitalActionRisingEdge(defaultToggleControl)) {
				std::cerr << "Activate State: Control" << std::endl;
				inControl = true;
			}

			continue;
		}

		vr::VRInput()->UpdateActionState(&sets[0], sizeof(sets[0]), 2);

		if (getDigitalActionRisingEdge(defaultToggleControl)) {
			std::cerr << "Activate State: Default" << std::endl;
			inControl = false;
			continue;
		}

		auto speedVec1 = getVector1ActionState(controlSpeedVector1);

		auto speedVec2Raw = getVector2ActionState(controlSpeedVector2);

		auto speedVec2 = std::min(
			std::hypotf(speedVec2Raw.x, speedVec2Raw.y),
			1.0f
		);

		auto speed = std::max(speedVec1, speedVec2);

		auto directionRaw = getVector2ActionState(controlDirection);

		auto direction = rad2deg(
			-std::atan2(-directionRaw.x, directionRaw.y)
		);

#ifdef DEBUG
		if (speed != oldSpeed || oldDirection != direction) {
#else
		if (1) {
#endif
			std::cout << "move " << speed << " " << direction << std::endl;
		}

		oldSpeed = speed;
		oldDirection = direction;

		if (speed == 0.0f && getDigitalActionRisingEdge(controlSnapTurnLeft)) {
			std::cout << "snapturn left" << std::endl;
		}

		if (speed == 0.0f && getDigitalActionRisingEdge(controlSnapTurnRight)) {
			std::cout << "snapturn right" << std::endl;
		}
	}

	return 0;
}

