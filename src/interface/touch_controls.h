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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************
 * On-screen touch controls for touchscreen builds (HAVE_TOUCHSCREEN):
 *
 *   bottom left    walk left / right (slide between them)
 *   bottom right   fire (hold to charge; also shown outside the turn, where
 *                  it drops the falling crate), jump above it (tap: jump,
 *                  hold: high jump), aim up / down to their left
 *   top right      weapons menu, weapon timer - / + (timed weapons only)
 *   top left       centre the camera on the character
 *   elsewhere      drag to move the camera, tap to click (targets,
 *                  choosing a character, the bottom bar, weapon menu)
 *   Back button    pause menu
 *
 * Every finger is tracked on its own, so walking and aiming can happen at
 * the same time. Actions go through ManMachineInterface like the keyboard
 * and game controllers. SDL's touch-to-mouse emulation is switched off
 * during a game (menus turn it back on, see Menu::Run()); taps on the map
 * are turned into mouse clicks here.
 *****************************************************************************/

#ifndef TOUCH_CONTROLS_H
#define TOUCH_CONTROLS_H

#include <map>
#include <SDL_events.h>
#include <SDL_render.h>
#include <WARMUX_singleton.h>
#include <WARMUX_point.h>
#include "interface/man_machine_interface.h"
#include "graphic/surface.h"

class TouchControls : public Singleton<TouchControls>, public ManMachineInterface
{
public:
  typedef enum {
    BTN_LEFT, BTN_RIGHT, BTN_JUMP, BTN_FIRE, BTN_UP, BTN_DOWN,
    BTN_WEAPONS, BTN_TIMER_LESS, BTN_TIMER_MORE, BTN_CENTER,
    BTN_COUNT,
    BTN_NONE = BTN_COUNT,
    CAMERA_DRAG       // finger moving the camera instead of a button
  } Button;

private:
  struct Finger {
    int target;           // a Button, or CAMERA_DRAG
    Point2i start, last;  // in game coordinates
    Uint32 start_time;
  };
  std::map<SDL_FingerID, Finger> fingers;
  bool enabled;

  Point2i center[BTN_COUNT];
  int radius[BTN_COUNT];
  Point2i layout_for;     // window size the layout was computed for
  // Full-resolution copies of the buttons (normal, pressed)
  mutable Surface cache[BTN_COUNT][2];
  mutable SDL_Texture *cache_tex[BTN_COUNT][2];  // the same, on the GPU
  mutable int cache_scale;

  void Layout();
  bool Active() const;    // the local human player is playing
  bool Shown(int button) const;
  Button HitTest(const Point2i &p) const;
  Point2i ToGame(float x, float y) const;
  void Press(int button);
  void Release(int button, Uint32 held_ms);
  void Click(const Point2i &p);
  void DrawButton(int button, bool pressed) const;

protected:
  friend class Singleton<TouchControls>;
  TouchControls();

public:
  // Start and end of a game: touch-to-mouse emulation off/on
  void Enable();
  void Disable();
  bool HandleKeyEvent(const SDL_Event &event);
  void Draw();
  void Reset();
};

#endif
