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

#ifndef JOYSTICK_H
#define JOYSTICK_H
//-----------------------------------------------------------------------------
#include <map>
#include <SDL_gamecontroller.h>
#include <SDL_keycode.h>
#include "interface/man_machine_interface.h"
#include <WARMUX_singleton.h>
//-----------------------------------------------------------------------------

/*
 * Game controllers, with the standard (Xbox-style) layout SDL2 knows for
 * nearly every pad:
 *
 *   Left stick / D-pad   walk (left/right), aim (up/down)
 *   A                    fire (hold to charge)
 *   B / X / Y            jump / high jump / back jump
 *   RB                   next weapon in the current category
 *   LB                   next weapon category
 *   LT / RT              weapon setting (e.g. timer) down / up
 *   Right stick, R3      move camera, re-centre on the character
 *   Back                 next character
 *   Start                pause menu
 *
 * Controllers are opened as they are plugged in. Menus are navigated with
 * the D-pad/left stick, A and B (see Joystick::MenuDirection()).
 */
class Joystick : public Singleton<Joystick>, public ManMachineInterface
{
  bool init;
  std::map<SDL_JoystickID, SDL_GameController*> controllers;

  // Current direction of each analogue axis: -1, 0 or +1
  int axis_dir[SDL_CONTROLLER_AXIS_MAX];

  void Open(int device_index);
  void Close(SDL_JoystickID id);
  static int DeviceEventWatch(void *data, SDL_Event *event);

  void HandleAxis(SDL_GameControllerAxis axis, Sint16 value);
  void HandleButton(SDL_GameControllerButton button, bool pressed);
  void PressAndRelease(Key_t action);
  void CycleWeapon(bool next_category);

protected:
  friend class Singleton<Joystick>;
  Joystick();
  ~Joystick();

public:
  int GetNumberOfJoystick() const { return (int)controllers.size(); }
  bool HandleKeyEvent(const SDL_Event& event);
  void Reset();

  // For menus: map a controller event to a navigation key (SDLK_UP, DOWN,
  // LEFT, RIGHT, RETURN or ESCAPE), or SDLK_UNKNOWN if it is not one
  static SDL_Keycode MenuKey(const SDL_Event& event);
};

//-----------------------------------------------------------------------------
#endif /* JOYSTICK_H */
