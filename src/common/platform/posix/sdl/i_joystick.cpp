/*
** i_joystick.cpp
**
**---------------------------------------------------------------------------
** Copyright 2005-2016 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/
#include <SDL.h>

#include "basics.h"
#include "cmdlib.h"

#include "m_joy.h"
#include "keydef.h"

#define DEFAULT_DEADZONE 0.25f;

EXTERN_CVAR(Bool, invertmouse)

// Very small deadzone so that floating point magic doesn't happen
#define MIN_DEADZONE 0.000001f

// A trigger is considered "pressed" once it passes the midpoint of its travel.
#define TRIGGER_THRESHOLD 0.5

class SDLInputJoystick: public IJoystickConfig
{
public:
	SDLInputJoystick(int DeviceIndex) : DeviceIndex(DeviceIndex), Controller(NULL), Multiplier(1.0f), Enabled(true), TriggerButtons(0)
	{
		// Prefer the GameController API for recognized pads: it exposes a stable,
		// standardized layout (A/B/X/Y, dpad, shoulders, triggers, sticks) which is
		// what the default "pad_*" bindings expect. Browser gamepads come through
		// this path on the web. Unrecognized devices fall back to a raw joystick.
		if(SDL_IsGameController(DeviceIndex))
		{
			Controller = SDL_GameControllerOpen(DeviceIndex);
			Device = NULL;
			if(Controller != NULL)
			{
				// Two sticks: LeftX/LeftY and RightX/RightY. Triggers are reported
				// as digital pad buttons instead of analog axes.
				NumAxes = 4;
				NumHats = 0;
				SetDefaultConfig();
			}
		}
		else
		{
			Device = SDL_JoystickOpen(DeviceIndex);
			if(Device != NULL)
			{
				NumAxes = SDL_JoystickNumAxes(Device);
				NumHats = SDL_JoystickNumHats(Device);
				SetDefaultConfig();
			}
		}
	}
	~SDLInputJoystick()
	{
		if(IsValid())
			M_SaveJoystickConfig(this);
		if(Controller != NULL)
			SDL_GameControllerClose(Controller);
		if(Device != NULL)
			SDL_JoystickClose(Device);
	}

	bool IsValid() const
	{
		return Device != NULL || Controller != NULL;
	}

	bool IsController() const
	{
		return Controller != NULL;
	}

	FString GetName()
	{
		if(Controller != NULL)
			return SDL_GameControllerName(Controller);
		return SDL_JoystickName(Device);
	}
	float GetSensitivity()
	{
		return Multiplier;
	}
	void SetSensitivity(float scale)
	{
		Multiplier = scale;
	}

	int GetNumAxes()
	{
		return NumAxes + NumHats*2;
	}
	float GetAxisDeadZone(int axis)
	{
		return Axes[axis].DeadZone;
	}
	EJoyAxis GetAxisMap(int axis)
	{
		return Axes[axis].GameAxis;
	}
	const char *GetAxisName(int axis)
	{
		return Axes[axis].Name.GetChars();
	}
	float GetAxisScale(int axis)
	{
		return Axes[axis].Multiplier;
	}

	void SetAxisDeadZone(int axis, float zone)
	{
		Axes[axis].DeadZone = clamp(zone, MIN_DEADZONE, 1.f);
	}
	void SetAxisMap(int axis, EJoyAxis gameaxis)
	{
		Axes[axis].GameAxis = gameaxis;
	}
	void SetAxisScale(int axis, float scale)
	{
		Axes[axis].Multiplier = scale;
	}

	// Used by the saver to not save properties that are at their defaults.
	bool IsSensitivityDefault()
	{
		return Multiplier == 1.0f;
	}
	bool IsAxisDeadZoneDefault(int axis)
	{
		return Axes[axis].DeadZone <= MIN_DEADZONE;
	}
	bool IsAxisMapDefault(int axis)
	{
		if(Controller != NULL)
			return Axes[axis].GameAxis == ControllerAxes[axis];
		if(axis >= 5)
			return Axes[axis].GameAxis == JOYAXIS_None;
		return Axes[axis].GameAxis == DefaultAxes[axis];
	}
	bool IsAxisScaleDefault(int axis)
	{
		return Axes[axis].Multiplier == 1.0f;
	}

	void SetDefaultConfig()
	{
		for(int i = 0;i < GetNumAxes();i++)
		{
			AxisInfo info;
			if(Controller != NULL)
			{
				static const char *const names[4] = {"Left Stick X", "Left Stick Y", "Right Stick X", "Right Stick Y"};
				info.Name = names[i];
			}
			else if(i < NumAxes)
				info.Name.Format("Axis %d", i+1);
			else
				info.Name.Format("Hat %d (%c)", (i-NumAxes)/2 + 1, (i-NumAxes)%2 == 0 ? 'x' : 'y');
			info.DeadZone = DEFAULT_DEADZONE;
			info.Multiplier = 1.0f;
			info.Value = 0.0;
			info.ButtonValue = 0;
			if(Controller != NULL)
				info.GameAxis = ControllerAxes[i];
			else if(i >= 5)
				info.GameAxis = JOYAXIS_None;
			else
				info.GameAxis = DefaultAxes[i];
			Axes.Push(info);
		}
	}

	bool GetEnabled()
	{
		return Enabled;
	}
	
	void SetEnabled(bool enabled)
	{
		Enabled = enabled;
	}

	bool AllowsEnabledInBackground() { return false; }
	bool GetEnabledInBackground() { return false; }
	void SetEnabledInBackground(bool enabled) {}

	FString GetIdentifier()
	{
		char id[16];
		snprintf(id, countof(id), "JS:%d", DeviceIndex);
		return id;
	}

	void AddAxes(float axes[NUM_JOYAXIS])
	{
		// Add to game axes.
		for (int i = 0; i < GetNumAxes(); ++i)
		{
			if(Axes[i].GameAxis != JOYAXIS_None)
			{
				float val = float(Axes[i].Value * Multiplier * Axes[i].Multiplier);
				// Follow the mouse's "invert vertical look" preference for the look
				// (pitch) axis so a gamepad's right stick looks the same way.
				if(Axes[i].GameAxis == JOYAXIS_Pitch && invertmouse)
					val = -val;
				axes[Axes[i].GameAxis] -= val;
			}
		}
	}

	void ProcessInput()
	{
		uint8_t buttonstate;

		if(Controller != NULL)
		{
			// Standardized stick layout: left stick = move, right stick = look.
			static const SDL_GameControllerAxis sdlaxes[4] = {
				SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY,
				SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY };
			for (int i = 0; i < NumAxes; ++i)
			{
				buttonstate = 0;
				Axes[i].Value = SDL_GameControllerGetAxis(Controller, sdlaxes[i])/32767.0;
				Axes[i].Value = Joy_RemoveDeadZone(Axes[i].Value, Axes[i].DeadZone, &buttonstate);
			}

			// Sticks double as digital pad buttons for menu navigation.
			buttonstate = Joy_XYAxesToButtons(Axes[0].Value, Axes[1].Value);
			Joy_GenerateButtonEvents(Axes[0].ButtonValue, buttonstate, 4, KEY_PAD_LTHUMB_RIGHT);
			Axes[0].ButtonValue = buttonstate;
			buttonstate = Joy_XYAxesToButtons(Axes[2].Value, Axes[3].Value);
			Joy_GenerateButtonEvents(Axes[2].ButtonValue, buttonstate, 4, KEY_PAD_RTHUMB_RIGHT);
			Axes[2].ButtonValue = buttonstate;

			// Analog triggers report as the pad trigger buttons the defaults expect.
			buttonstate  = (SDL_GameControllerGetAxis(Controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT)/32767.0  > TRIGGER_THRESHOLD) ? 1 : 0;
			buttonstate |= (SDL_GameControllerGetAxis(Controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT)/32767.0 > TRIGGER_THRESHOLD) ? 2 : 0;
			Joy_GenerateButtonEvents(TriggerButtons, buttonstate, 2, KEY_PAD_LTRIGGER);
			TriggerButtons = buttonstate;
			return;
		}

		for (int i = 0; i < NumAxes; ++i)
		{
			buttonstate = 0;

			Axes[i].Value = SDL_JoystickGetAxis(Device, i)/32767.0;
			Axes[i].Value = Joy_RemoveDeadZone(Axes[i].Value, Axes[i].DeadZone, &buttonstate);

			// Map button to axis
			// X and Y are handled differently so if we have 2 or more axes then we'll use that code instead.
			if (NumAxes == 1 || (i >= 2 && i < NUM_JOYAXISBUTTONS))
			{
				Joy_GenerateButtonEvents(Axes[i].ButtonValue, buttonstate, 2, KEY_JOYAXIS1PLUS + i*2);
				Axes[i].ButtonValue = buttonstate;
			}
		}

		if(NumAxes > 1)
		{
			buttonstate = Joy_XYAxesToButtons(Axes[0].Value, Axes[1].Value);
			Joy_GenerateButtonEvents(Axes[0].ButtonValue, buttonstate, 4, KEY_JOYAXIS1PLUS);
			Axes[0].ButtonValue = buttonstate;
		}

		// Map POV hats to buttons and axes.  Why axes?  Well apparently I have
		// a gamepad where the left control stick is a POV hat (instead of the
		// d-pad like you would expect, no that's pressure sensitive).  Also
		// KDE's joystick dialog maps them to axes as well.
		for (int i = 0; i < NumHats; ++i)
		{
			AxisInfo &x = Axes[NumAxes + i*2];
			AxisInfo &y = Axes[NumAxes + i*2 + 1];

			buttonstate = SDL_JoystickGetHat(Device, i);

			// If we're going to assume that we can pass SDL's value into
			// Joy_GenerateButtonEvents then we might as well assume the format here.
			if(buttonstate & 0x1) // Up
				y.Value = -1.0;
			else if(buttonstate & 0x4) // Down
				y.Value = 1.0;
			else
				y.Value = 0.0;
			if(buttonstate & 0x2) // Left
				x.Value = 1.0;
			else if(buttonstate & 0x8) // Right
				x.Value = -1.0;
			else
				x.Value = 0.0;

			if(i < 4)
			{
				Joy_GenerateButtonEvents(x.ButtonValue, buttonstate, 4, KEY_JOYPOV1_UP + i*4);
				x.ButtonValue = buttonstate;
			}
		}
	}

protected:
	struct AxisInfo
	{
		FString Name;
		float DeadZone;
		float Multiplier;
		EJoyAxis GameAxis;
		double Value;
		uint8_t ButtonValue;
	};
	static const EJoyAxis DefaultAxes[5];
	static const EJoyAxis ControllerAxes[4];

	int					DeviceIndex;
	SDL_Joystick		*Device;
	SDL_GameController	*Controller;

	float				Multiplier;
	bool				Enabled;
	TArray<AxisInfo>	Axes;
	int					NumAxes;
	int					NumHats;
	uint8_t				TriggerButtons;

	friend class SDLInputJoystickManager;
};

// [Nash 4 Feb 2024] seems like on Linux, the third axis is actually the Left Trigger, resulting in the player uncontrollably looking upwards.
const EJoyAxis SDLInputJoystick::DefaultAxes[5] = {JOYAXIS_Side, JOYAXIS_Forward, JOYAXIS_None, JOYAXIS_Yaw, JOYAXIS_Pitch};
// Game controllers use a fixed layout: left stick moves, right stick looks.
const EJoyAxis SDLInputJoystick::ControllerAxes[4] = {JOYAXIS_Side, JOYAXIS_Forward, JOYAXIS_Yaw, JOYAXIS_Pitch};

class SDLInputJoystickManager
{
public:
	SDLInputJoystickManager()
	{
		Rescan();
	}
	~SDLInputJoystickManager()
	{
		for(unsigned int i = 0;i < Joysticks.Size();i++)
			delete Joysticks[i];
	}

	// Rebuild the device list from SDL's current view. On the web, browsers only
	// expose gamepads after the first user gesture, so the device list has to be
	// refreshed when SDL reports a controller was added/removed (hotplug) instead
	// of relying on the one-time enumeration done at startup. Returns the first
	// device so the joystick menu can select it.
	IJoystickConfig *Rescan()
	{
		for(unsigned int i = 0;i < Joysticks.Size();i++)
			delete Joysticks[i];
		Joysticks.Clear();

		IJoystickConfig *newone = NULL;
		for(int i = 0;i < SDL_NumJoysticks();i++)
		{
			SDLInputJoystick *device = new SDLInputJoystick(i);
			if(device->IsValid())
			{
				Joysticks.Push(device);
				if(newone == NULL)
					newone = device;
			}
			else
				delete device;
		}
		return newone;
	}

	void AddAxes(float axes[NUM_JOYAXIS])
	{
		for(unsigned int i = 0;i < Joysticks.Size();i++)
			Joysticks[i]->AddAxes(axes);
	}
	void GetDevices(TArray<IJoystickConfig *> &sticks)
	{
		for(unsigned int i = 0;i < Joysticks.Size();i++)
		{
			M_LoadJoystickConfig(Joysticks[i]);
			sticks.Push(Joysticks[i]);
		}
	}

	void ProcessInput() const
	{
		for(unsigned int i = 0;i < Joysticks.Size();++i)
			if(Joysticks[i]->Enabled) Joysticks[i]->ProcessInput();
	}
protected:
	TArray<SDLInputJoystick *> Joysticks;
};
static SDLInputJoystickManager *JoystickManager;

void I_StartupJoysticks()
{
#ifndef NO_SDL_JOYSTICK
	// Init the GameController subsystem (it pulls in JOYSTICK) so recognized
	// pads report a standardized layout that matches the default pad_* binds.
	// Browser gamepads are surfaced through this backend on the web. No haptic
	// subsystem is requested (the web SDL2 build sets SDL_HAPTIC_DISABLED).
	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) >= 0)
		JoystickManager = new SDLInputJoystickManager();
#endif
}
void I_ShutdownInput()
{
	if(JoystickManager)
	{
		delete JoystickManager;
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
	}
}

void I_GetJoysticks(TArray<IJoystickConfig *> &sticks)
{
	sticks.Clear();

	if (JoystickManager)
		JoystickManager->GetDevices(sticks);
}

void I_GetAxes(float axes[NUM_JOYAXIS])
{
	for (int i = 0; i < NUM_JOYAXIS; ++i)
	{
		axes[i] = 0;
	}
	if (use_joystick && JoystickManager)
	{
		JoystickManager->AddAxes(axes);
	}
}

void I_ProcessJoysticks()
{
	if (use_joystick && JoystickManager)
		JoystickManager->ProcessInput();
}

IJoystickConfig *I_UpdateDeviceList()
{
#ifndef NO_SDL_JOYSTICK
	// Refresh the joystick list after a hotplug event (the only way browser
	// gamepads ever appear) and report any newly connected device.
	if (JoystickManager)
		return JoystickManager->Rescan();
#endif
	return NULL;
}
