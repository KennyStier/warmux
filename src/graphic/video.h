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
 *  MERCHANTABILITY or FITNESS FOR A ARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU GeneralPublic License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *****************************************************************************/

#ifndef VIDEO_H
#define VIDEO_H

#include <string>
#include <list>
#include <vector>
#include "surface.h"
#include <WARMUX_base.h>

// SDL 1.2 reported the wheel as buttons 4 and 5, and the GUI code is built
// around that. Video installs an event filter turning SDL_MOUSEWHEEL into
// press/release pairs of these buttons (side buttons are moved to 6 and 7).
#define WX_BUTTON_WHEELUP   4
#define WX_BUTTON_WHEELDOWN 5

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
union SDL_Event;

/*
 * The game renders everything in software into `window`, a 32-bit surface
 * addressed in the game's logical resolution. Flip() uploads it to a
 * streaming texture which the SDL2 renderer scales to the real window, so
 * the window can be resized, made fullscreen at desktop resolution or shown
 * on a HiDPI display without the game knowing.
 *
 * When the window is a whole multiple (scale) of the game size, `window`
 * is a full-resolution surface (Surface::SetScale()): text and shapes are
 * sharp. During a game frame, stretching every bitmap in software is too
 * slow, so the frame is drawn at game resolution into world_surface (the
 * GPU stretches it) and only text and the touch controls go to
 * screen_surface, a transparent full-resolution overlay the GPU puts on top
 * (Surface::SetHiResOverlay()). Images look the same either way.
 * Menus draw into screen_surface alone; EndLayers() merges the last game
 * frame into it first, as in-game menus draw over it.
 */
class Video
{
  uint m_max_fps;     // If equals to zero, it means no limit
  uint m_max_delay;
  bool SDLReady;
  bool fullscreen;
  SDL_Surface *icon;

  SDL_Window   *sdl_window;
  SDL_Renderer *renderer;
  SDL_Texture  *texture;
  SDL_Surface  *screen_surface;  // `window`'s pixels outside the world layer
  int           screen_scale;    // pixels per game unit of screen_surface
  SDL_Surface  *world_surface;   // world layer, game resolution (scale > 1 only)
  SDL_Texture  *world_texture;
  bool          layered;         // last frame was drawn in two layers
  bool          world_fresh;     // world layer just (re)created: needs a full draw
  // Interface layer: areas drawn since it was last cleared, areas changed
  // since the last upload, or everything (after a menu or a resize)
  std::vector<Rectanglei> ui_drawn, ui_pending;
  bool          ui_full;
  // Full-resolution images the GPU draws on top of a layered frame
  struct Sprite { SDL_Texture *texture; float x, y, w, h; Uint8 alpha; };
  std::vector<Sprite> sprites;
  void FreeSurfaces();

  // Size the window currently has (in points), applied to the game surface
  // by ApplyPendingResize() at a point where it is safe to do so
  Point2i window_size;
  // Display scale in percent (0 = auto: render at the window's logical size)
  uint scale_percent;
  Point2i TargetGameSize() const;

  std::list<Point2i> available_configs;
  void ComputeAvailableConfigs();

  bool __SetConfig(const int w, const int h, const bool _fullscreen);
  bool ResizeGameSurface(int w, int h);
  bool CreateWindowAndRenderer(int w, int h, bool fullscreen);

  void InitSDL(void);

  static int WindowEventWatch(void *data, SDL_Event *event);

public:
  Surface window;
  void SetWindowCaption(const std::string& caption) const;
  void SetMaxFps(uint max_fps);
  uint GetMaxFps() const { return m_max_fps; };
  uint GetMaxDelay() const { return m_max_delay; };
  void AddUniqueConfigSorted(int w, int h);

  Video();
  ~Video();

  bool IsFullScreen() const { return fullscreen; };

  const std::list<Point2i>& GetAvailableConfigs() const { return available_configs; };

  // in case of failure, it continues to use the old configuration
  // return true only if the change is successful
  bool SetConfig(int width, int height, bool fullscreen);
  void ToggleFullscreen();

  // The game renders at the window's size. Window size changes are
  // recorded as they happen and applied here, between frames.
  bool HasPendingResize() const;
  bool ApplyPendingResize();

  // 100 renders one game pixel per screen pixel (sharpest on HiDPI screens),
  // larger values make the interface bigger; 0 follows the desktop scale.
  // Takes effect at the next safe point, like a window resize.
  void SetScale(uint percent) { scale_percent = percent; }
  uint GetScale() const { return scale_percent; }
  Point2i GetWindowSize() const { return window_size; }

  // Game frames (see above). BeginWorldLayer() returns true if the world
  // must be drawn in full (new layer).
  bool BeginWorldLayer();
  void EndLayers();

  // A texture from a (full-resolution) surface, for QueueSprite()
  SDL_Texture *CreateTexture(Surface &s);
  // During a layered frame: have the GPU draw `t` on top of everything at
  // pixel_pos (in full-resolution pixels) with this opacity. False when
  // frames are not layered; then the caller draws by itself.
  bool QueueSprite(SDL_Texture *t, const Point2i &pixel_pos, Uint8 alpha);

  // Upload the game surface and show it
  void Flip(void);
  // Show the last uploaded frame again (after an expose or resize)
  void Present(void);

  // Convert between window pixels and game (logical) coordinates
  Point2i WindowToGame(int x, int y) const;
  Point2i GameToWindow(int x, int y) const;

  SDL_Window *GetSDLWindow() const { return sdl_window; }

  bool SaveScreenshot();
};

Surface& GetMainWindow();
// SDL_GetMouseState() with the position converted to game coordinates
Uint32 GetGameMouseState(int *x, int *y);
void SwapWindowClip(Rectanglei& clip);

#endif
