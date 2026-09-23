/******************************************************************
*
*
*		----------------
*		  Input.cpp
*		----------------
*
*		Classe Input
*
*
*		La Classe Input représente toutes les entrées :
*
*		 - Clavier
*		 - Joystick
*
*
*
*		Prosper / LOADED -   V 0.2
*
*
*
******************************************************************/

#define BENINPUT_CPP_FILE

//-----------------------------------------------------------------------------
//		Headers
//-----------------------------------------------------------------------------

#include <cstring>

#include "graphics.h"
#include "input.h"
#include "ben_debug.h"
#include "control_alias.h"

namespace {
constexpr int INPUT_DEVICE_SWITCH_DEAD_ZONE = 12000;
constexpr Uint32 MENU_REPEAT_DELAY_MS = 350;
constexpr Uint32 MENU_REPEAT_INTERVAL_MS = 120;
}

//-----------------------------------------------------------------------------
//		Déclaration REELLE de l'objet 'in' global
//-----------------------------------------------------------------------------

Input		in;

//-----------------------------------------------------------------------------
// Nom: Input::Input() - CONSTRUCTEUR -
// Desc: Met à NULL les valeurs susceptibles de foirer
//-----------------------------------------------------------------------------

Input::Input()
    : n_joy(0),
      last_input_device(InputDevice::Keyboard),
      pause_pressed(false),
      menu_direction(0),
      menu_direction_started(0),
      menu_direction_repeated(0),
      menu_horizontal_direction(0),
      menu_horizontal_started(0),
      menu_horizontal_repeated(0),
      menu_confirm_held(false),
      menu_back_held(false)
{
	memset(js, 0, sizeof(js));
	memset(buffer, 0, sizeof(buffer));
	memset(specialsbuffer, 0, sizeof(specialsbuffer));
	pause_pressed = false;
	memset(aliastab, 0, sizeof(aliastab));
}

Input::~Input()
{
	close();
}

void Input::clearState()
{
	memset(buffer, 0, sizeof(buffer));
	memset(specialsbuffer, 0, sizeof(specialsbuffer));
	for (int i = 0; i < MAX_JOY; ++i) {
		memset(js[i].buttons, 0, sizeof(js[i].buttons));
		memset(&js[i].directions, 0, sizeof(js[i].directions));
	}
	menu_direction = 0;
	menu_horizontal_direction = 0;
	menu_confirm_held = false;
	menu_back_held = false;
}

int Input::joystickSlot(SDL_JoystickID instance_id) const
{
	for (int i = 0; i < MAX_JOY; ++i) {
		if (js[i].handle && js[i].instance_id == instance_id)
			return i;
	}
	return -1;
}

bool Input::openJoystick(int device_index)
{
	int slot = -1;
	for (int i = 0; i < MAX_JOY; ++i) {
		if (!js[i].handle) {
			slot = i;
			break;
		}
	}
	if (slot < 0) {
		debug << "Ignoring joystick " << device_index << ": maximum of "
		      << MAX_JOY << " devices reached\n";
		return false;
	}

	SDL_GameController* controller = nullptr;
	SDL_Joystick* handle = nullptr;
	if (SDL_IsGameController(device_index)) {
		controller = SDL_GameControllerOpen(device_index);
		if (controller) handle = SDL_GameControllerGetJoystick(controller);
	} else {
		handle = SDL_JoystickOpen(device_index);
	}
	if (!handle) {
		debug << "Cannot open joystick " << device_index << ": "
		      << SDL_GetError() << "\n";
		return false;
	}
	const SDL_JoystickID instance_id = SDL_JoystickInstanceID(handle);
	if (joystickSlot(instance_id) >= 0) {
		if (controller)
			SDL_GameControllerClose(controller);
		else
			SDL_JoystickClose(handle);
		return true;
	}

	js[slot].handle = handle;
	js[slot].controller = controller;
	js[slot].instance_id = instance_id;
	const char* name = SDL_JoystickName(handle);
	SDL_strlcpy(js[slot].name, name ? name : "Unknown joystick",
	            sizeof(js[slot].name));
	memset(js[slot].buttons, 0, sizeof(js[slot].buttons));
	memset(&js[slot].directions, 0, sizeof(js[slot].directions));
	++n_joy;
	debug << "Opened " << (controller ? "game controller" : "raw joystick")
	      << " slot " << slot << ": " << js[slot].name
	      << " (instance " << js[slot].instance_id << ")\n";
	return true;
}

void Input::closeJoystick(SDL_JoystickID instance_id)
{
	const int slot = joystickSlot(instance_id);
	if (slot < 0) return;
	if (js[slot].controller)
		SDL_GameControllerClose(js[slot].controller);
	else
		SDL_JoystickClose(js[slot].handle);
	js[slot].handle = nullptr;
	js[slot].controller = nullptr;
	js[slot].instance_id = -1;
	memset(js[slot].buttons, 0, sizeof(js[slot].buttons));
	memset(&js[slot].directions, 0, sizeof(js[slot].directions));
	if (n_joy > 0) --n_joy;
	debug << "Closed joystick instance " << instance_id << "\n";
}



//-----------------------------------------------------------------------------
// Nom: Input::open(HWND, HINSTANCE, int, int)
// Desc: Ouvre BINPUT
//-----------------------------------------------------------------------------

bool Input::open(int flags)
{
	close();
	SDL_JoystickEventState(SDL_TRUE);
	SDL_GameControllerEventState(SDL_ENABLE);
	const int detected = SDL_NumJoysticks();
	for (int i = 0; i < detected; ++i) openJoystick(i);
	debug << n_joy << " joystick(s) opened\n";

	/*if (dinput != NULL) {
		debug << "Input::open->DINPUT déjà initialisé!\n";
		return false;
	}

	if (DirectInput8Create(inst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&dinput, NULL) != DI_OK) {
		debug << "Input::open->Ne peut pas ouvrir DINPUT\n";
		dinput = NULL;
		return false;
	}


	if (flags & BINPUT_KEYB) {
		if (dinput->CreateDevice(GUID_SysKeyboard, (LPDIRECTINPUTDEVICE8A *)&dikeyb, NULL) != DI_OK) {
			debug << "Input::open->Ne peut pas créer le clavier!\n";
			dikeyb = NULL;
			return false;
		} else {
			if (dikeyb->SetDataFormat(&c_dfDIKeyboard) != DI_OK) {
				debug << "Input::open->Ne peut pas initialiser le clavier (set data)\n";
				dikeyb->Release();
				dikeyb = NULL;
				return false;
			} else {
				if (dikeyb->SetCooperativeLevel(wh, cl) != DI_OK) {
					debug << "Input::open->Ne peut pas règler le coop du clavier\n";
					dikeyb->Release();
					dikeyb = NULL;
					return false;
				} else
					dikeyb->Acquire();
			}
		}
	}

	// Associe le joystick
	//
	if (flags & BINPUT_JOY) {
		if (dinput->EnumDevices(DI8DEVTYPE_GAMEPAD, EnumJoysticksCallback, this, DIEDFL_ATTACHEDONLY) != DI_OK) {
			debug << "Cannot enumerate joysticks\n";
		} else {
			debug << n_joy << " joystick(s) found\n";

			for (int i = 0; i < n_joy; i++) {
				if (dijoy[i] != NULL) {
					if (dijoy[i]->SetDataFormat(&c_dfDIJoystick) != DI_OK) {
						debug << "Cannot initialise joystick " << i << "\n";
						dijoy[i]->Release();
						dijoy[i] = NULL;
					} else {
						if (dijoy[i]->SetCooperativeLevel(wh, DISCL_EXCLUSIVE | DISCL_FOREGROUND) != DI_OK) {
							debug << "Cannot set priority level of joystick " << i << "\n";
							dijoy[i]->Release();
							dijoy[i] = NULL;
						} else {
							DIPROPRANGE diprg;

							diprg.diph.dwSize       = sizeof(diprg);
							diprg.diph.dwHeaderSize = sizeof(diprg.diph);
							diprg.diph.dwObj        = DIJOFS_X;
							diprg.diph.dwHow        = DIPH_BYOFFSET;
							diprg.lMin              = -1000;
							diprg.lMax              = +1000;

							dijoy[i]->SetProperty(DIPROP_RANGE, &diprg.diph);

							diprg.diph.dwObj        = DIJOFS_Y;
							dijoy[i]->SetProperty(DIPROP_RANGE, &diprg.diph);

							DIPROPDWORD dipdw;

							dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
							dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
							dipdw.diph.dwHow        = DIPH_BYOFFSET;
							dipdw.dwData            = 5000;

							dipdw.diph.dwObj         = DIJOFS_X;
							dijoy[i]->SetProperty(DIPROP_DEADZONE, &dipdw.diph);

							dipdw.diph.dwObj = DIJOFS_Y;
							dijoy[i]->SetProperty(DIPROP_DEADZONE, &dipdw.diph);


							if (dijoy[i]->Acquire() != DI_OK) {
								debug << "Cannot get joystick " << i << "\n";
							}
						}
					}
				}
			}
		}
	}*/

	return true;
}

//-----------------------------------------------------------------------------
// Nom: Input::update()
// Desc: Met à jour les entrées
//-----------------------------------------------------------------------------

void Input::update()
{
	SDL_Event e;
	while (SDL_PollEvent(&e)){

		if (e.type == SDL_QUIT)
		{
			app_killed = true;
			//exit(0);
		}	

		if (e.type == SDL_KEYDOWN)
		{
			last_input_device = InputDevice::Keyboard;
			if (e.key.keysym.sym >= 0 && e.key.keysym.sym < 256)
			{
				buffer[e.key.keysym.sym] = 1;
			}
			else
				specialsbuffer[e.key.keysym.sym & 0xFFF] = 1;
		}
		if (e.type == SDL_KEYUP)
		{
			if (e.key.keysym.sym >= 0 && e.key.keysym.sym < 256)
			{
				buffer[e.key.keysym.sym] = 0;
			}
			else
				specialsbuffer[e.key.keysym.sym & 0xFFF] = 0;
		}

		if (e.type == SDL_MOUSEBUTTONDOWN)
		{
			last_input_device = InputDevice::Keyboard;
		}
		if (e.type == SDL_MOUSEMOTION && (e.motion.xrel != 0 || e.motion.yrel != 0))
			last_input_device = InputDevice::Keyboard;

		if (e.type == SDL_WINDOWEVENT &&
		    e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
			clearState();
			debug << "Input focus lost; cleared held input state\n";
		}
		if (e.type == SDL_WINDOWEVENT &&
		    e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
			debug << "Input focus gained\n";

		if (e.type == SDL_JOYDEVICEADDED)
			openJoystick(e.jdevice.which);
		if (e.type == SDL_JOYDEVICEREMOVED)
			closeJoystick(e.jdevice.which);

		if (e.type == SDL_JOYBUTTONDOWN || e.type == SDL_JOYBUTTONUP)
		{
			const int slot = joystickSlot(e.jbutton.which);
			if (e.type == SDL_JOYBUTTONDOWN && slot >= 0)
				last_input_device = js[slot].controller
				                        ? InputDevice::GameController
				                        : InputDevice::RawJoystick;
			if (slot >= 0 && e.jbutton.button < sizeof(js[slot].buttons))
				js[slot].buttons[e.jbutton.button] =
				    e.type == SDL_JOYBUTTONDOWN;
		}
		if (e.type == SDL_CONTROLLERBUTTONDOWN) {
			last_input_device = InputDevice::GameController;
			const DIJOYSTATE* player_one = controllerForPlayer(0);
			if (player_one && player_one->instance_id == e.cbutton.which &&
			    e.cbutton.button == SDL_CONTROLLER_BUTTON_START)
				pause_pressed = true;
		}
		if (e.type == SDL_CONTROLLERAXISMOTION &&
		    (e.caxis.value < -INPUT_DEVICE_SWITCH_DEAD_ZONE ||
		     e.caxis.value > INPUT_DEVICE_SWITCH_DEAD_ZONE))
			last_input_device = InputDevice::GameController;
		if (e.type == SDL_JOYHATMOTION)
		{
			const int slot = joystickSlot(e.jhat.which);
			if (slot < 0) continue;
			last_input_device = js[slot].controller
			                        ? InputDevice::GameController
			                        : InputDevice::RawJoystick;
			js[slot].directions.down = false;
			js[slot].directions.right = false;
			js[slot].directions.left = false;
			js[slot].directions.up = false;

			if (e.jhat.hat & SDL_HAT_UP)
				js[slot].directions.up = true;
			if (e.jhat.hat & SDL_HAT_LEFT)
				js[slot].directions.left = true;
			if (e.jhat.hat & SDL_HAT_RIGHT)
				js[slot].directions.right = true;
			if (e.jhat.hat & SDL_HAT_DOWN)
				js[slot].directions.down = true;
		}

		if (e.type == SDL_JOYAXISMOTION)
		{
			const int slot = joystickSlot(e.jaxis.which);
			if (slot < 0) continue;
			if (e.jaxis.value < -INPUT_DEVICE_SWITCH_DEAD_ZONE ||
			    e.jaxis.value > INPUT_DEVICE_SWITCH_DEAD_ZONE)
				last_input_device = js[slot].controller
				                        ? InputDevice::GameController
				                        : InputDevice::RawJoystick;
		
			/* Horizontal movement */

			if (e.jaxis.axis == 0)
			{
				if (e.jaxis.value < -DEAD_ZONE)
				{
					js[slot].directions.left = 1;
					js[slot].directions.right = 0;
				}

				else if (e.jaxis.value > DEAD_ZONE)
				{
					js[slot].directions.right = 1;
					js[slot].directions.left = 0;
				}

				else
				{
					js[slot].directions.left = 0;
					js[slot].directions.right = 0;
				}
			}

			/* Vertical movement */

			if (e.jaxis.axis == 1)
			{
				if (e.jaxis.value < -DEAD_ZONE)
				{
					js[slot].directions.up = 1;
					js[slot].directions.down = 0;
				}

				else if (e.jaxis.value > DEAD_ZONE)
				{
					js[slot].directions.down = 1;
					js[slot].directions.up = 0;
				}

				else
				{
					js[slot].directions.up = 0;
					js[slot].directions.down = 0;
				}
			}
		}

	}
	/*if (dikeyb != NULL)
		dikeyb->GetDeviceState(sizeof(buffer), (void *)&buffer);

	for (int i = 0; i < n_joy; i++) {
		if (dijoy[i] != NULL) {
			dijoy[i]->Poll();
			dijoy[i]->GetDeviceState(sizeof(DIJOYSTATE), (void*) &js[i]);
		}
	}*/
}

//-----------------------------------------------------------------------------
// Nom: Input::waitKey()
// Desc: Attends que l'utilisateur tape une touche et renvoie sa valeur
//-----------------------------------------------------------------------------

unsigned int Input::waitKey()
{
	unsigned int	key = 0;

	while (1)
	{
		update();
		if (app_killed) return 0;
		for (int i = 0; i < 255; i++)
		{
			if (buffer[i] != 0)
			{
				return i;
			}
		}
		for (int i = 0; i < 0xFFF; i++)
		{
			if (specialsbuffer[i] != 0)
			{
				return i | 0x40000000;
			}
		}

		for (int i = 0; i < 128; i++)
		{
			for (int k = 0; k < MAX_JOY; k++)
			{
				if (js[k].handle && js[k].buttons[i] == 1)
				{
					int k2 = (k + 1) * (1 << 10) + i;
					return k2;
				}
					//return (js[k].buttons[i] | (0x400));
			}
		}

		for (int k = 0; k < MAX_JOY; k++)
		{
			if (!js[k].handle) continue;
			if (js[k].directions.down)
			{
				int k2 = (k + 1) * (1 << 10) + JOY_DOWN;
				return k2;
			}
			if (js[k].directions.left)
			{
				int k2 = (k + 1) * (1 << 10) + JOY_LEFT;
				return k2;
			}
			if (js[k].directions.right)
			{
				int k2 = (k + 1) * (1 << 10) + JOY_RIGHT;
				return k2;
			}
			if (js[k].directions.up)
			{
				int k2 = (k + 1) * (1 << 10) + JOY_UP;
				return k2;
			}
			//return (js[k].buttons[i] | (0x400));
		}
	}
	/*unsigned int	i = 0;
	int				j;
	int				k;

	while (key == 0) {
		update();

		for (i = 0; i < 256; i++)
			if (scanKey(i))
				key = i;

		for (i = 0; i < (unsigned int)n_joy; i++) {
			k = (i + 1) * (1 << 10);

			for (j = 0; j < 14; j++)
				if (scanKey(k + j))
					key = k + j;
		}
	}*/

	return key;
}

//-----------------------------------------------------------------------------
// Nom: Input::waitClean()
// Desc: Attends qu'aucune touche ne soit enfoncée
//-----------------------------------------------------------------------------

void Input::waitClean()
{
	//Waits for all keys to be released?
	while (1)
	{
		bool j = false;
		update();
		if (app_killed) return;
		for (int i = 0; i < 255; i++)
		{
			if (buffer[i] != 0)
			{
				j = true;
			}
		}
		for (int i = 0; i < 0xFFF; i++)
		{
			if (specialsbuffer[i] != 0)
			{
				j = true;
			}
		}

		for (int i = 0; i < 128; i++)
		{
			for (int k = 0; k < MAX_JOY; k++)
			{
				if (js[k].handle && js[k].buttons[i] == 1)
				{
					j = true;
				}
			}
		}

		for (int k = 0; k < MAX_JOY; k++)
		{
			if (js[k].handle && (js[k].directions.down || js[k].directions.up || js[k].directions.left || js[k].directions.right))
			j = true;
		}

		if (!j)
			return;
	}
	/*unsigned int		i, j = 1;		// Bcoz si j = 0 alors on sort tout de suite!

	while (j) {
		update();
		j = 0;
		for (i = 0; i < 256; i++)
			j |= scanKey(i);

		for (i = 0; i < (unsigned int)n_joy; i++) {
			unsigned int k = (i + 1) * (1 << 10);

			for (int l = 0; l < 14; l++)
				j |= scanKey(k + l);
		}
	}*/

}

//-----------------------------------------------------------------------------
// Nom: Input::setAlias()
// Desc: Règle la valeur d'un alias
//-----------------------------------------------------------------------------
void Input::setAlias(int a, unsigned int val)
{
	if (a >= 0 && a < static_cast<int>(sizeof(aliastab) / sizeof(aliastab[0])))
		aliastab[a] = val;
}

const DIJOYSTATE* Input::controllerForPlayer(int player) const
{
	int found = 0;
	for (int i = 0; i < MAX_JOY; ++i) {
		if (!js[i].controller) continue;
		if (found++ == player) return &js[i];
	}
	return nullptr;
}

bool Input::controllerAliasPressed(int alias) const
{
	const int player = alias >= ALIAS_P2_UP ? 1 : 0;
	const DIJOYSTATE* state = controllerForPlayer(player);
	if (!state) return false;
	SDL_GameController* controller = state->controller;
	const bool left = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ||
	                  SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) < -DEAD_ZONE;
	const bool right = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ||
	                   SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) > DEAD_ZONE;
	const bool up = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP) ||
	                SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) < -DEAD_ZONE;
	const bool down = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
	                  SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) > DEAD_ZONE;

	switch (alias) {
		case ALIAS_P1_LEFT: case ALIAS_P2_LEFT: return left;
		case ALIAS_P1_RIGHT: case ALIAS_P2_RIGHT: return right;
		case ALIAS_P1_UP: case ALIAS_P2_UP: return up;
		case ALIAS_P1_DOWN: case ALIAS_P2_DOWN: return down;
		case ALIAS_P1_FIRE: case ALIAS_P2_FIRE:
			return SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X);
		case ALIAS_P1_JUMP: case ALIAS_P2_JUMP:
			return SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A);
		case ALIAS_P1_SUPER: case ALIAS_P2_SUPER:
			return SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B);
		default: return false;
	}
}

int Input::scanAlias(int alias) const
{
	if (alias < 0 || alias >= static_cast<int>(sizeof(aliastab) / sizeof(aliastab[0])))
		return 0;
	return scanKey(aliastab[alias]) || controllerAliasPressed(alias);
}

bool Input::menuConfirmPressed() const
{
	const DIJOYSTATE* state = controllerForPlayer(0);
	return state && SDL_GameControllerGetButton(
	                    state->controller, SDL_CONTROLLER_BUTTON_A);
}

int Input::menuVerticalMove()
{
	const bool up = scanKey(DIK_UP) || scanAlias(ALIAS_P1_UP);
	const bool down = scanKey(DIK_DOWN) || scanAlias(ALIAS_P1_DOWN);
	const int direction = up == down ? 0 : (up ? -1 : 1);
	const Uint32 now = SDL_GetTicks();
	if (direction == 0) {
		menu_direction = 0;
		return 0;
	}
	if (direction != menu_direction) {
		menu_direction = direction;
		menu_direction_started = now;
		menu_direction_repeated = now;
		return direction;
	}
	if (now - menu_direction_started >= MENU_REPEAT_DELAY_MS &&
	    now - menu_direction_repeated >= MENU_REPEAT_INTERVAL_MS) {
		menu_direction_repeated = now;
		return direction;
	}
	return 0;
}

int Input::menuHorizontalMove()
{
	const bool left = scanKey(DIK_LEFT) || scanAlias(ALIAS_P1_LEFT);
	const bool right = scanKey(DIK_RIGHT) || scanAlias(ALIAS_P1_RIGHT);
	const int direction = left == right ? 0 : (left ? -1 : 1);
	const Uint32 now = SDL_GetTicks();
	if (direction == 0) {
		menu_horizontal_direction = 0;
		return 0;
	}
	if (direction != menu_horizontal_direction) {
		menu_horizontal_direction = direction;
		menu_horizontal_started = now;
		menu_horizontal_repeated = now;
		return direction;
	}
	if (now - menu_horizontal_started >= MENU_REPEAT_DELAY_MS &&
	    now - menu_horizontal_repeated >= MENU_REPEAT_INTERVAL_MS) {
		menu_horizontal_repeated = now;
		return direction;
	}
	return 0;
}

bool Input::menuConfirmActionPressed(bool allow_controller)
{
	const bool pressed = scanKey(DIK_RETURN) ||
	                     scanKey(getAlias(ALIAS_P1_FIRE)) ||
	                     (allow_controller && menuConfirmPressed());
	const bool triggered = pressed && !menu_confirm_held;
	menu_confirm_held = pressed;
	return triggered;
}

bool Input::menuBackPressed()
{
	const DIJOYSTATE* state = controllerForPlayer(0);
	const bool pressed = state && SDL_GameControllerGetButton(
	    state->controller, SDL_CONTROLLER_BUTTON_B);
	const bool triggered = pressed && !menu_back_held;
	menu_back_held = pressed;
	return triggered;
}

bool Input::pausePressed()
{
	const bool pressed = pause_pressed;
	pause_pressed = false;
	return pressed;
}

//-----------------------------------------------------------------------------
// Nom: Input::close()
// Desc: Ferme toutes les entrées
//-----------------------------------------------------------------------------

void Input::close()
{
	for (int i = 0; i < MAX_JOY; ++i) {
		if (js[i].controller)
			SDL_GameControllerClose(js[i].controller);
		else if (js[i].handle)
			SDL_JoystickClose(js[i].handle);
		js[i].handle = nullptr;
		js[i].controller = nullptr;
		js[i].instance_id = -1;
	}
	n_joy = 0;
	clearState();
	/*if (dikeyb != NULL) {
		dikeyb->Unacquire();
		dikeyb->Release();
		dikeyb = NULL;
	}

	if (dinput != NULL) {
		dinput->Release();
		dinput = NULL;
	}

	for (int i = 0; i < n_joy; i++) {
		if (dijoy[i] != NULL) {
			dijoy[i]->Release();
			dijoy[i] = NULL;
		}
	}

	n_joy = 0;*/
}


//-----------------------------------------------------------------------------

bool Input::anyKeyPressed()
{
	unsigned int	i;
	unsigned int	k;
	int key = 0;

	update();
	for (int i = 0; i < 255; i++)
	{
		if (buffer[i] != 0)
		{
			return true;
		}
	}
	for (int i = 0; i < 0xFFF; i++)
	{
		if (specialsbuffer[i] != 0)
		{
			return true;
		}
	}

	for (int i = 0; i < 128; i++)
	{
		for (int k = 0; k < MAX_JOY; k++)
		{
			if (js[k].handle && js[k].buttons[i] == 1)
			{
				return true;
			}
		}
	}

	for (int k = 0; k < MAX_JOY; k++)
	{
		if (js[k].handle && (js[k].directions.down || js[k].directions.up || js[k].directions.left || js[k].directions.right))
			return true;
	}

	return false;


	/*int j;

	update();

	for (i = 0; i < 256; i++)
		if (scanKey(i))
			key = i;

	for (i = 0; i < (unsigned int)n_joy; i++) {
		k = (i + 1) * (1 << 10);

		for (j = 0; j < 14; j++)
			if (scanKey(k + j))
				key = k + j;
	}*/
}

int Input::scanKey(unsigned int k) const
{
	/*int j = (k >> 10);

	if (j == 0)
		return buffer[k] & 0x80;

	j -= 1;

	if (j < 0 || j >= n_joy)
		return 0;
	else {
		int		z = 0;
		int		k2 = k & 0x3FF;

		switch (k2) {
			case JOY_UP:
				if (js[j].lY < -200) z = 1;
				break;

			case JOY_DOWN:
				if (js[j].lY > 200) z = 1;
				break;

			case JOY_LEFT:
				if (js[j].lX < -200) z = 1;
				break;

			case JOY_RIGHT:
				if (js[j].lX > 200) z = 1;
				break;

			default:
				z = (js[j].rgbButtons[k2] & 0x80);
				break;
		}

		return z;
	}*/
	const unsigned int joystick_number = (k >> 10) & 0xFF;
	if (joystick_number > 0)
	{
		/*
		REMEMBER
		That the joystick number starts from 1, but the array starts from 0
		*/
		int	j = static_cast<int>(joystick_number);
		int b = k & 0xFF;
		if (j < 1 || j > MAX_JOY || !js[j - 1].handle)
			return 0;

		if (b == JOY_UP)
			return js[j - 1].directions.up;
		if (b == JOY_DOWN)
			return js[j - 1].directions.down;
		if (b == JOY_RIGHT)
			return js[j - 1].directions.right;
		if (b == JOY_LEFT)
			return js[j - 1].directions.left;

		if (b >= static_cast<int>(sizeof(js[j - 1].buttons)))
			return 0;
		int r = js[j-1].buttons[b];
		return r;
	}
	else if (k < sizeof(buffer))
		return buffer[k];
	else 
		return specialsbuffer[k & 0xFFF];

	return 0;
}

//-----------------------------------------------------------------------------
// Nom: Input::waitKey()
// Desc: Attends que l'utilisateur tape une touche et renvoie sa valeur
//-----------------------------------------------------------------------------

bool Input::reAcquire()
{
	/*if (dinput == NULL)
		return false;

	if (dikeyb != NULL)
		dikeyb->Acquire();

	for (int i = 0; i < MAX_JOY; i++)
		if (dijoy[i] != NULL)
			dijoy[i]->Acquire();*/

	return true;
}
