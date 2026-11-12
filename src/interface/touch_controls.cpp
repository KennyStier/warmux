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
 * On-screen touch controls; see touch_controls.h.
 *****************************************************************************/

#include <WARMUX_config.h>

#ifdef HAVE_TOUCHSCREEN

#include <SDL.h>
#include "interface/touch_controls.h"
#include "character/character.h"
#include "game/game.h"
#include "graphic/color.h"
#include "graphic/surface.h"
#include "graphic/text.h"
#include "graphic/video.h"
#include "include/app.h"
#include "interface/interface.h"
#include "map/camera.h"
#include "map/map.h"
#include "team/macro.h"
#include "team/team.h"

// A finger that moves further than this (game pixels) is dragging, not tapping
static const int DRAG_DISTANCE = 12;
// Holding jump at least this long makes it a high jump
static const Uint32 HIGH_JUMP_MS = 300;

TouchControls::TouchControls()
  : enabled(false)
  , layout_for(-1, -1)
  , cache_scale(0)
{
  for (int i = 0; i < BTN_COUNT; i++) {
    radius[i] = 0;
    cache_tex[i][0] = cache_tex[i][1] = NULL;
  }
}

void TouchControls::Enable()
{
  enabled = true;
  fingers.clear();
  ManMachineInterface::Reset();
  // The game handles fingers itself; menus switch emulation back on
  SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
}

void TouchControls::Disable()
{
  Reset();
  enabled = false;
  SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
}

void TouchControls::Reset()
{
  // Release whatever is still held (a finger lifted while a menu was open)
  for (std::map<SDL_FingerID, Finger>::iterator it = fingers.begin();
       it != fingers.end(); ++it)
    if (it->second.target < BTN_COUNT)
      Release(it->second.target, 0);
  fingers.clear();
  ManMachineInterface::Reset();
}

// --- Layout -----------------------------------------------------------------

void TouchControls::Layout()
{
  const Surface &window = GetMainWindow();
  const int W = window.GetWidth(), H = window.GetHeight();
  layout_for = Point2i(W, H);
  for (int i = 0; i < BTN_COUNT; i++)
    for (int p = 0; p < 2; p++) {
      cache[i][p] = Surface();
      if (cache_tex[i][p])
        SDL_DestroyTexture(cache_tex[i][p]);
      cache_tex[i][p] = NULL;
    }

  // Sizes in game pixels; at the default scale the game is 540 high, so a
  // radius of 40 is about a centimetre on a phone
  const int r = H * 40 / 540, rf = H * 50 / 540, rs = H * 26 / 540;
  const int m = H * 14 / 540, gap = H * 10 / 540;

  // Keep clear of the bottom bar if it reaches into the corners
  int bottom = H - m;
  const Interface *interf = Interface::GetConstInstance();
  const int corner_w = 4 * r + 2 * m + gap;
  if (interf->IsDisplayed() && (W - interf->GetWidth()) / 2 < corner_w)
    bottom = H - interf->GetHeight() - m;

  // Left thumb: walking only, so the right thumb can jump while running
  radius[BTN_LEFT] = radius[BTN_RIGHT] = r;
  center[BTN_LEFT]  = Point2i(m + r, bottom - r);
  center[BTN_RIGHT] = Point2i(m + 3 * r + gap, bottom - r);

  // Right thumb: fire in the corner, jump above it, aim to their left
  radius[BTN_FIRE] = rf;
  radius[BTN_JUMP] = radius[BTN_UP] = radius[BTN_DOWN] = r;
  center[BTN_FIRE] = Point2i(W - m - rf, bottom - rf);
  center[BTN_JUMP] = Point2i(W - m - rf, bottom - 2 * rf - gap - r);
  center[BTN_UP]   = Point2i(W - m - 2 * rf - gap - r, bottom - 3 * r - gap);
  center[BTN_DOWN] = Point2i(W - m - 2 * rf - gap - r, bottom - r);

  // Top corners
  radius[BTN_WEAPONS] = radius[BTN_TIMER_LESS] = radius[BTN_TIMER_MORE] = rs;
  radius[BTN_CENTER] = rs;
  center[BTN_WEAPONS]    = Point2i(W - m - rs, m + rs);
  center[BTN_TIMER_MORE] = Point2i(W - m - 3 * rs - gap, m + rs);
  center[BTN_TIMER_LESS] = Point2i(W - m - 5 * rs - 2 * gap, m + rs);
  center[BTN_CENTER]     = Point2i(m + rs, m + rs);
}

bool TouchControls::Active() const
{
  if (!ActiveTeam().IsLocalHuman() || ActiveCharacter().IsDead())
    return false;
  Game::game_loop_state_t state = Game::GetInstance()->ReadState();
  return state == Game::PLAYING || state == Game::HAS_PLAYED;
}

bool TouchControls::Shown(int button) const
{
  switch (button) {
  case BTN_FIRE:
    // Also outside the turn: it drops the falling crate straight away
    return true;
  case BTN_TIMER_LESS:
  case BTN_TIMER_MORE:
    return Active() && ActiveTeam().GetWeapon().HasAdjustableTimer();
  default:
    return Active();
  }
}

TouchControls::Button TouchControls::HitTest(const Point2i &p) const
{
  for (int i = 0; i < BTN_COUNT; i++) {
    if (!Shown(i))
      continue;
    // A little larger than drawn: fingers are imprecise
    int r = radius[i] * 5 / 4;
    if (p.SquareDistance(center[i]) <= r * r)
      return (Button)i;
  }
  return BTN_NONE;
}

Point2i TouchControls::ToGame(float x, float y) const
{
  Video *video = AppWarmux::GetInstance()->video;
  Point2i win = video->GetWindowSize();
  return video->WindowToGame(int(x * win.x), int(y * win.y));
}

// --- Actions ----------------------------------------------------------------

void TouchControls::Press(int button)
{
  switch (button) {
  case BTN_LEFT:  HandleKeyPressed(KEY_MOVE_LEFT); break;
  case BTN_RIGHT: HandleKeyPressed(KEY_MOVE_RIGHT); break;
  case BTN_UP:    HandleKeyPressed(KEY_UP); break;
  case BTN_DOWN:  HandleKeyPressed(KEY_DOWN); break;
  case BTN_FIRE:  HandleKeyPressed(KEY_SHOOT); break;
  default: break; // the others act on release
  }
}

void TouchControls::Release(int button, Uint32 held_ms)
{
  switch (button) {
  case BTN_LEFT:  HandleKeyReleased(KEY_MOVE_LEFT); break;
  case BTN_RIGHT: HandleKeyReleased(KEY_MOVE_RIGHT); break;
  case BTN_UP:    HandleKeyReleased(KEY_UP); break;
  case BTN_DOWN:  HandleKeyReleased(KEY_DOWN); break;
  case BTN_FIRE:  HandleKeyReleased(KEY_SHOOT); break;
  case BTN_JUMP: {
    Key_t jump = held_ms >= HIGH_JUMP_MS ? KEY_HIGH_JUMP : KEY_JUMP;
    HandleKeyPressed(jump);
    HandleKeyReleased(jump);
    break;
  }
  case BTN_WEAPONS:
    HandleKeyPressed(KEY_TOGGLE_WEAPONS_MENUS);
    HandleKeyReleased(KEY_TOGGLE_WEAPONS_MENUS);
    break;
  case BTN_TIMER_LESS:
    HandleKeyPressed(KEY_WEAPON_LESS);
    HandleKeyReleased(KEY_WEAPON_LESS);
    break;
  case BTN_TIMER_MORE:
    HandleKeyPressed(KEY_WEAPON_MORE);
    HandleKeyReleased(KEY_WEAPON_MORE);
    break;
  case BTN_CENTER:
    HandleKeyPressed(KEY_CENTER);
    HandleKeyReleased(KEY_CENTER);
    break;
  default: break;
  }
}

// A tap away from the buttons: a left click at that spot, through the normal
// mouse handling (bottom bar, weapon menu, targets, choosing a character)
void TouchControls::Click(const Point2i &p)
{
  Video *video = AppWarmux::GetInstance()->video;
  Point2i win = video->GetWindowSize();
  Point2i game = video->window.GetSize();
  int wx = game.x ? p.x * win.x / game.x : p.x;
  int wy = game.y ? p.y * win.y / game.y : p.y;
  SDL_WarpMouseInWindow(video->GetSDLWindow(), wx, wy);

  SDL_Event e;
  SDL_zero(e);
  e.type = SDL_MOUSEBUTTONDOWN;
  e.button.button = SDL_BUTTON_LEFT;
  e.button.state = SDL_PRESSED;
  e.button.clicks = 1;
  e.button.x = wx;
  e.button.y = wy;
  SDL_PushEvent(&e);
  e.type = SDL_MOUSEBUTTONUP;
  e.button.state = SDL_RELEASED;
  SDL_PushEvent(&e);
}

// --- Events -----------------------------------------------------------------

bool TouchControls::HandleKeyEvent(const SDL_Event &event)
{
  if (!enabled)
    return false;

  // Android's Back button: pause menu
  if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_AC_BACK) {
    HandleKeyPressed(KEY_PAUSE);
    HandleKeyReleased(KEY_PAUSE);
    return true;
  }
  if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_AC_BACK)
    return true;

  if (event.type != SDL_FINGERDOWN && event.type != SDL_FINGERMOTION
      && event.type != SDL_FINGERUP)
    return false;

  if (layout_for != GetMainWindow().GetSize())
    Layout();

  const SDL_TouchFingerEvent &f = event.tfinger;
  Point2i pos = ToGame(f.x, f.y);

  if (event.type == SDL_FINGERDOWN) {
    Finger finger;
    finger.target = HitTest(pos);
    finger.start = finger.last = pos;
    finger.start_time = SDL_GetTicks();
    fingers[f.fingerId] = finger;
    if (finger.target < BTN_COUNT)
      Press(finger.target);
    return true;
  }

  std::map<SDL_FingerID, Finger>::iterator it = fingers.find(f.fingerId);
  if (it == fingers.end())
    return true;
  Finger &finger = it->second;

  if (event.type == SDL_FINGERMOTION) {
    if (finger.target == BTN_LEFT || finger.target == BTN_RIGHT) {
      // Slide from one walk button to the other to turn around
      Button now = HitTest(pos);
      if ((now == BTN_LEFT || now == BTN_RIGHT) && now != finger.target) {
        Release(finger.target, 0);
        finger.target = now;
        Press(now);
      }
    } else if (finger.target == BTN_NONE
               && finger.start.SquareDistance(pos) > DRAG_DISTANCE * DRAG_DISTANCE) {
      finger.target = CAMERA_DRAG;
    }
    if (finger.target == CAMERA_DRAG && pos != finger.last) {
      Camera::GetInstance()->SetAutoCrop(false);
      Camera::GetInstance()->SetXY(finger.last - pos);
    }
    finger.last = pos;
    return true;
  }

  // SDL_FINGERUP
  Uint32 held = SDL_GetTicks() - finger.start_time;
  if (finger.target < BTN_COUNT)
    Release(finger.target, held);
  else if (finger.target == BTN_NONE)
    Click(pos);
  fingers.erase(it);
  return true;
}

// --- Drawing ----------------------------------------------------------------

static void Triangle(Surface &s, const Point2i &c, int size, int dx, int dy, const Color &col)
{
  // Arrow pointing along (dx, dy), one of the four axis directions
  Sint16 vx[3], vy[3];
  int px = -dy, py = dx; // perpendicular
  vx[0] = c.x + dx * size;          vy[0] = c.y + dy * size;
  vx[1] = c.x - dx * size + px * size; vy[1] = c.y - dy * size + py * size;
  vx[2] = c.x - dx * size - px * size; vy[2] = c.y - dy * size - py * size;
  s.FilledPolygon(vx, vy, 3, col);
}

// The button's shapes on `target`, centred on c. `solid`: opaque colours,
// for the cached copies that get their transparency when blitted
static void DrawShapes(Surface &target, int b, const Point2i &c, int r,
                       bool pressed, bool solid)
{
  Color fill, line, glyph;
  if (solid) {
    fill  = pressed ? Color(225, 225, 225, 255) : Color(25, 25, 25, 255);
    line  = Color(255, 255, 255, 255);
    glyph = pressed ? Color(40, 40, 40, 255) : Color(255, 255, 255, 255);
  } else {
    fill  = pressed ? Color(255, 255, 255, 150) : Color(0, 0, 0, 90);
    line  = Color(255, 255, 255, pressed ? 230 : 160);
    glyph = Color(255, 255, 255, pressed ? 255 : 200);
  }

  target.FilledCircleColor(c.x, c.y, r, fill);
  target.CircleColor(c.x, c.y, r, line);

  int g = r * 2 / 5;
  switch (b) {
  case TouchControls::BTN_LEFT:  Triangle(target, c, g, -1, 0, glyph); break;
  case TouchControls::BTN_RIGHT: Triangle(target, c, g, 1, 0, glyph); break;
  case TouchControls::BTN_UP:    Triangle(target, c, g, 0, -1, glyph); break;
  case TouchControls::BTN_DOWN:  Triangle(target, c, g, 0, 1, glyph); break;
  case TouchControls::BTN_JUMP:
    Triangle(target, c - Point2i(0, g / 2), g * 3 / 4, 0, -1, glyph);
    Triangle(target, c + Point2i(0, g * 3 / 4), g * 3 / 4, 0, -1, glyph);
    break;
  case TouchControls::BTN_FIRE:
    target.FilledCircleColor(c.x, c.y, r / 3, Color(255, 90, 60, solid || pressed ? 255 : 210));
    target.CircleColor(c.x, c.y, r * 2 / 3, glyph);
    break;
  case TouchControls::BTN_TIMER_LESS:
  case TouchControls::BTN_TIMER_MORE: {
    int w = r / 2, t = r / 8 + 1;
    target.BoxColor(Rectanglei(c.x - w, c.y - t, 2 * w, 2 * t), glyph);
    if (b == TouchControls::BTN_TIMER_MORE)
      target.BoxColor(Rectanglei(c.x - t, c.y - w, 2 * t, 2 * w), glyph);
    break;
  }
  case TouchControls::BTN_WEAPONS: {
    // Three bars: a menu
    int w = r / 2, t = r / 10 + 1;
    for (int i = -1; i <= 1; i++)
      target.BoxColor(Rectanglei(c.x - w, c.y + i * r / 3 - t, 2 * w, 2 * t), glyph);
    break;
  }
  case TouchControls::BTN_CENTER:
    target.CircleColor(c.x, c.y, r / 2, glyph);
    target.FilledCircleColor(c.x, c.y, r / 6, glyph);
    break;
  }
}

void TouchControls::DrawButton(int b, bool pressed) const
{
  Surface &window = GetMainWindow();
  const Point2i &c = center[b];
  const int r = radius[b];
  const int k = window.HiResScale();

  if (k > 1) {
    // Full resolution: a cached copy per state, drawn once (drawing the
    // blended circles every frame is slow at this size)
    Surface &cached = cache[b][pressed ? 1 : 0];
    if (cached.IsNull() || cache_scale != k) {
      int size = 2 * r + 3;
      Surface tmp(Point2i(size * k, size * k), SDL_SWSURFACE, true);
      tmp.Fill(0); // transparent
      tmp.SetScale(k);
      DrawShapes(tmp, b, Point2i(r + 1, r + 1), r, pressed, true);
      tmp.SetScale(1);
      cached = tmp;
      SDL_Texture *&old_tex = cache_tex[b][pressed ? 1 : 0];
      if (old_tex) {
        SDL_DestroyTexture(old_tex);
        old_tex = NULL;
      }
    }
    cache_scale = k;
    // The GPU draws it (blending this much in software costs several ms)
    Video *video = AppWarmux::GetInstance()->video;
    SDL_Texture *&tex = cache_tex[b][pressed ? 1 : 0];
    if (!tex)
      tex = video->CreateTexture(cached);
    Point2i pixel_pos((c.x - r - 1) * k, (c.y - r - 1) * k);
    Uint8 alpha = pressed ? 200 : 140;
    if (!video->QueueSprite(tex, pixel_pos, alpha)) {
      SDL_SetSurfaceAlphaMod(cached.GetSurface(), alpha);
      window.BlitHiRes(cached, pixel_pos);
    }
  } else {
    DrawShapes(window, b, c, r, pressed, false);
    GetWorld().ToRedrawOnScreen(Rectanglei(c - Point2i(r + 1, r + 1),
                                           Point2i(2 * r + 3, 2 * r + 3)));
  }
}

void TouchControls::Draw()
{
  if (!enabled)
    return;
  if (layout_for != GetMainWindow().GetSize())
    Layout();

  bool pressed[BTN_COUNT] = { false };
  for (std::map<SDL_FingerID, Finger>::const_iterator it = fingers.begin();
       it != fingers.end(); ++it)
    if (it->second.target < BTN_COUNT)
      pressed[it->second.target] = true;

  for (int i = 0; i < BTN_COUNT; i++)
    if (Shown(i))
      DrawButton(i, pressed[i]);
}

#endif // HAVE_TOUCHSCREEN
