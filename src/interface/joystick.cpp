/******************************************************************************
 *  Warmux is a convivial mass murder game.
 *  Copyright (C) 2001-2011 Warmux Team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 ******************************************************************************
 * Game controller support (SDL2 GameController API).
 *****************************************************************************/

#include "interface/joystick.h"
#include <SDL.h>

#include "game/game.h"
#include "team/team.h"
#include "team/teams_list.h"
#include "weapon/weapon.h"

// Stick thresholds, with hysteresis so a stick resting near the edge of the
// dead zone doesn't chatter
static const int AXIS_PRESS   = 16000;
static const int AXIS_RELEASE = 8000;

Joystick::Joystick()
  : ManMachineInterface()
  , init(false)
{
  for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; i++)
    axis_dir[i] = 0;

  if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
    fprintf(stderr, "Unable to initialize game controllers: %s\n", SDL_GetError());
    // Don't crash for this, just ignore further request
    return;
  }
  init = true;

  // Controllers already plugged in; later ones arrive as device events
  for (int i = 0; i < SDL_NumJoysticks(); i++)
    if (SDL_IsGameController(i))
      Open(i);
  SDL_AddEventWatch(DeviceEventWatch, this);

  RegisterEvent(SDL_CONTROLLERAXISMOTION);
  RegisterEvent(SDL_CONTROLLERBUTTONDOWN);
  RegisterEvent(SDL_CONTROLLERBUTTONUP);
}

Joystick::~Joystick()
{
  if (!init)
    return;
  SDL_DelEventWatch(DeviceEventWatch, this);
  for (std::map<SDL_JoystickID, SDL_GameController*>::iterator it = controllers.begin();
       it != controllers.end(); ++it)
    SDL_GameControllerClose(it->second);
  controllers.clear();
  SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
  init = false;
}

void Joystick::Open(int device_index)
{
  SDL_GameController *pad = SDL_GameControllerOpen(device_index);
  if (!pad) {
    fprintf(stderr, "Can't open game controller %d: %s\n", device_index, SDL_GetError());
    return;
  }
  SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad));
  if (controllers.count(id)) {
    SDL_GameControllerClose(pad); // already open
    return;
  }
  controllers[id] = pad;
  printf("o Game controller connected: %s\n", SDL_GameControllerName(pad));
}

void Joystick::Close(SDL_JoystickID id)
{
  std::map<SDL_JoystickID, SDL_GameController*>::iterator it = controllers.find(id);
  if (it == controllers.end())
    return;
  printf("o Game controller disconnected: %s\n", SDL_GameControllerName(it->second));
  SDL_GameControllerClose(it->second);
  controllers.erase(it);
  // Don't leave a direction or button stuck
  Reset();
}

int Joystick::DeviceEventWatch(void *data, SDL_Event *event)
{
  Joystick *self = static_cast<Joystick*>(data);
  if (event->type == SDL_CONTROLLERDEVICEADDED)
    self->Open(event->cdevice.which);         // device index
  else if (event->type == SDL_CONTROLLERDEVICEREMOVED)
    self->Close(event->cdevice.which);        // instance id
  return 0;
}

void Joystick::PressAndRelease(Key_t action)
{
  HandleKeyPressed(action);
  HandleKeyReleased(action);
}

// LB/RB: weapons are grouped in categories, and selecting a category again
// cycles through the weapons in it (as F1-F7 do on the keyboard)
void Joystick::CycleWeapon(bool next_category)
{
  if (!GameIsRunning() || Game::GetInstance()->ReadState() != Game::PLAYING)
    return;
  int category = ActiveTeam().GetWeapon().Category();
  if (category < Weapon::HEAVY || category > Weapon::TOOL)
    category = Weapon::HEAVY;
  else if (next_category)
    category = (category == Weapon::TOOL) ? Weapon::HEAVY : category + 1;
  PressAndRelease((Key_t)(KEY_WEAPONS1 + (category - Weapon::HEAVY)));
}

void Joystick::HandleAxis(SDL_GameControllerAxis axis, Sint16 value)
{
  int &dir = axis_dir[axis];
  int new_dir = dir;
  if (dir == 0) {
    if (value >= AXIS_PRESS)       new_dir = 1;
    else if (value <= -AXIS_PRESS) new_dir = -1;
  } else if (value < AXIS_RELEASE && value > -AXIS_RELEASE) {
    new_dir = 0;
  } else if ((dir > 0) != (value > 0)) {
    new_dir = -dir; // flicked straight to the other side
  }
  if (new_dir == dir)
    return;

  Key_t negative, positive;
  switch (axis) {
  case SDL_CONTROLLER_AXIS_LEFTX:  negative = KEY_MOVE_LEFT;        positive = KEY_MOVE_RIGHT;        break;
  case SDL_CONTROLLER_AXIS_LEFTY:  negative = KEY_UP;               positive = KEY_DOWN;              break;
  case SDL_CONTROLLER_AXIS_RIGHTX: negative = KEY_MOVE_CAMERA_LEFT; positive = KEY_MOVE_CAMERA_RIGHT; break;
  case SDL_CONTROLLER_AXIS_RIGHTY: negative = KEY_MOVE_CAMERA_UP;   positive = KEY_MOVE_CAMERA_DOWN;  break;
  case SDL_CONTROLLER_AXIS_TRIGGERLEFT:  negative = KEY_NONE; positive = KEY_WEAPON_LESS; break;
  case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: negative = KEY_NONE; positive = KEY_WEAPON_MORE; break;
  default: return;
  }

  if (dir < 0 && negative != KEY_NONE) HandleKeyReleased(negative);
  if (dir > 0 && positive != KEY_NONE) HandleKeyReleased(positive);
  dir = new_dir;
  if (dir < 0 && negative != KEY_NONE) HandleKeyPressed(negative);
  if (dir > 0 && positive != KEY_NONE) HandleKeyPressed(positive);
}

void Joystick::HandleButton(SDL_GameControllerButton button, bool pressed)
{
  Key_t action;
  switch (button) {
  case SDL_CONTROLLER_BUTTON_A:             action = KEY_SHOOT;          break;
  case SDL_CONTROLLER_BUTTON_B:             action = KEY_JUMP;           break;
  case SDL_CONTROLLER_BUTTON_X:             action = KEY_HIGH_JUMP;      break;
  case SDL_CONTROLLER_BUTTON_Y:             action = KEY_BACK_JUMP;      break;
  case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     action = KEY_MOVE_LEFT;      break;
  case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    action = KEY_MOVE_RIGHT;     break;
  case SDL_CONTROLLER_BUTTON_DPAD_UP:       action = KEY_UP;             break;
  case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     action = KEY_DOWN;           break;
  case SDL_CONTROLLER_BUTTON_BACK:          action = KEY_NEXT_CHARACTER; break;
  case SDL_CONTROLLER_BUTTON_START:         action = KEY_PAUSE;          break;
  case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    action = KEY_CENTER;         break;
  case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
    if (!pressed) CycleWeapon(false);
    return;
  case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
    if (!pressed) CycleWeapon(true);
    return;
  default:
    return;
  }

  if (pressed)
    HandleKeyPressed(action);
  else
    HandleKeyReleased(action);
}

bool Joystick::HandleKeyEvent(const SDL_Event& evnt)
{
  if (!init || !IsRegistredEvent(evnt.type))
    return false;

  switch (evnt.type) {
  case SDL_CONTROLLERAXISMOTION:
    HandleAxis((SDL_GameControllerAxis)evnt.caxis.axis, evnt.caxis.value);
    return true;
  case SDL_CONTROLLERBUTTONDOWN:
  case SDL_CONTROLLERBUTTONUP:
    HandleButton((SDL_GameControllerButton)evnt.cbutton.button,
                 evnt.type == SDL_CONTROLLERBUTTONDOWN);
    return true;
  default:
    return false;
  }
}

void Joystick::Reset()
{
  ManMachineInterface::Reset();
  for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; i++)
    axis_dir[i] = 0;
}

SDL_Keycode Joystick::MenuKey(const SDL_Event& evnt)
{
  // Tracks the left stick so one push gives one step
  static int stick_x = 0, stick_y = 0;

  if (evnt.type == SDL_CONTROLLERBUTTONDOWN) {
    switch (evnt.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_DPAD_UP:    return SDLK_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  return SDLK_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  return SDLK_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return SDLK_RIGHT;
    case SDL_CONTROLLER_BUTTON_A:
    case SDL_CONTROLLER_BUTTON_START:      return SDLK_RETURN;
    case SDL_CONTROLLER_BUTTON_B:
    case SDL_CONTROLLER_BUTTON_BACK:       return SDLK_ESCAPE;
    default:                               return SDLK_UNKNOWN;
    }
  }

  if (evnt.type == SDL_CONTROLLERAXISMOTION) {
    int v = evnt.caxis.value;
    int *state;
    SDL_Keycode neg, pos;
    if (evnt.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
      state = &stick_x; neg = SDLK_LEFT; pos = SDLK_RIGHT;
    } else if (evnt.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
      state = &stick_y; neg = SDLK_UP; pos = SDLK_DOWN;
    } else {
      return SDLK_UNKNOWN;
    }
    if (*state == 0 && (v >= AXIS_PRESS || v <= -AXIS_PRESS)) {
      *state = (v > 0) ? 1 : -1;
      return (v > 0) ? pos : neg;
    }
    if (*state != 0 && v < AXIS_RELEASE && v > -AXIS_RELEASE)
      *state = 0;
  }
  return SDLK_UNKNOWN;
}
