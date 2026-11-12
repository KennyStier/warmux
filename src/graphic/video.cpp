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

#include <algorithm>
#include <iostream>
#include <SDL.h>
#include <SDL_image.h>

#include <WARMUX_file_tools.h>

#include "game/config.h"
#include "graphic/video.h"
#include "include/app.h"
#include "include/constant.h"
#include "map/camera.h"

static int SDLCALL InputFilter(void *data, SDL_Event *event);

Video::Video()
  : SDLReady(false)
  , fullscreen(false)
  , icon(NULL)
  , sdl_window(NULL)
  , renderer(NULL)
  , texture(NULL)
  , scale_percent(0)
{
  SetMaxFps(50);

  Config * config = Config::GetInstance();
  SetMaxFps(config->GetMaxFps());
  scale_percent = config->GetVideoScale();

  InitSDL();

  // The icon must be larger then 32x32 pixels as some desktops display
  // larger icons; the window manager scales it down as needed.
  icon = IMG_Load((config->GetDataDir() + "icon/warmux_128x128.xpm").c_str());

  window.SetSurface(NULL, false);
  window.SetAutoFree(false);

  ComputeAvailableConfigs();

  // See if we can add the current config resolution
  int w = config->GetVideoWidth();
  int h = config->GetVideoHeight();
  if (!w || !h) {
    // No size configured yet (first run): use the largest available mode
    // that fits comfortably inside the desktop, leaving room for panels and
    // decorations, instead of the smallest one.
    w = available_configs.begin()->GetX();
    h = available_configs.begin()->GetY();
    SDL_DisplayMode desktop;
    if (SDL_GetDesktopDisplayMode(0, &desktop) == 0 && desktop.w > 0 && desktop.h > 0) {
      int max_w = desktop.w * 9 / 10;
      int max_h = desktop.h * 9 / 10;
      for (std::list<Point2i>::const_iterator res = available_configs.begin();
           res != available_configs.end(); ++res) {
        if (res->GetX() <= max_w && res->GetY() <= max_h
            && res->GetX() * res->GetY() > w * h) {
          w = res->GetX();
          h = res->GetY();
        }
      }
    }
  }

  SetConfig(w, h, config->IsVideoFullScreen());

  if (window.IsNull()) {
    Error(Format("Unable to initialize SDL window: %s", SDL_GetError()));
    exit (1);
  }
  AddUniqueConfigSorted(window.GetWidth(), window.GetHeight());

  SetWindowCaption(std::string("WarMUX ") + Constants::WARMUX_VERSION);

  SDL_AddEventWatch(WindowEventWatch, this);
  SDL_SetEventFilter(InputFilter, this);
}

Video::~Video()
{
  SDL_SetEventFilter(NULL, NULL);
  SDL_DelEventWatch(WindowEventWatch, this);
  if (icon) {
    SDL_FreeSurface(icon);
    icon = NULL;
  }
  window.SetSurface(NULL, true);
  if (texture)
    SDL_DestroyTexture(texture);
  if (renderer)
    SDL_DestroyRenderer(renderer);
  if (sdl_window)
    SDL_DestroyWindow(sdl_window);
  if (SDLReady) {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDLReady = false;
  }
}

void Video::SetMaxFps(uint max_fps)
{
  m_max_fps = max_fps;
  if (0 < m_max_fps)
    m_max_delay = 1000/m_max_fps;
  else
    m_max_delay = 0;
}

static bool CompareConfigs(const Point2i& a, const Point2i& b)
{
  return (a.x < b.x) || ((a.x == b.x) && (a.y < b.y));
}

void Video::AddUniqueConfigSorted(int w, int h)
{
  Point2i p(w, h);

  for (std::list<Point2i>::iterator res = available_configs.begin();
       res != available_configs.end();
       res++) {
    // Are they identical ?
    if (p == *res)
      return;

    // Is it bigger?
    if (!CompareConfigs(*res, p)) {
      available_configs.insert(res, p);
      return;
    }
  }
  available_configs.push_back(p);
}

void Video::ComputeAvailableConfigs()
{
  // Since the renderer scales the game to the window, any resolution works.
  // Offer the display's own modes...
  int n = SDL_GetNumDisplayModes(0);
  for (int i = 0; i < n; i++) {
    SDL_DisplayMode mode;
    if (SDL_GetDisplayMode(0, i, &mode) == 0 && mode.w >= 480 && mode.h >= 320)
      AddUniqueConfigSorted(mode.w, mode.h);
  }

  // ...plus standard resolutions such as 1600x1200, 1280x1024, 1024x768, 800x600.
  for (std::list<Point2i>::iterator res = Config::GetInstance()->GetResolutionAvailable().begin();
       res != Config::GetInstance()->GetResolutionAvailable().end();
       res++) {
    AddUniqueConfigSorted((*res).GetX(), (*res).GetY());
  }
}

bool Video::CreateWindowAndRenderer(int w, int h, bool fs)
{
  Uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
  if (fs)
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

#ifdef ANDROID
  // Without this SDL picks the orientation from the window size, overriding
  // the manifest
  SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif

  sdl_window = SDL_CreateWindow("WarMUX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                w, h, flags);
  if (!sdl_window)
    return false;

  if (icon)
    SDL_SetWindowIcon(sdl_window, icon);

  // Smooth scaling looks better than nearest-neighbour at arbitrary ratios
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

  renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    fprintf(stderr, "WARNING: no accelerated renderer (%s), using software\n", SDL_GetError());
    renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (!renderer) {
    SDL_DestroyWindow(sdl_window);
    sdl_window = NULL;
    return false;
  }
  return true;
}

// Minimum size the game's layout supports
static const int MIN_GAME_W = 480, MIN_GAME_H = 320;

bool Video::ResizeGameSurface(int width, int height)
{
  if (width < MIN_GAME_W)  width = MIN_GAME_W;
  if (height < MIN_GAME_H) height = MIN_GAME_H;

  // (Re)create the game surface and its texture if the resolution changed
  if (window.IsNull() || window.GetWidth() != width || window.GetHeight() != height) {
    SDL_Texture *new_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                                                 SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!new_texture)
      return false;
    SDL_Surface *screen = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32,
                                                         SDL_PIXELFORMAT_RGB888);
    if (!screen) {
      SDL_DestroyTexture(new_texture);
      return false;
    }
    if (texture)
      SDL_DestroyTexture(texture);
    texture = new_texture;
    window.SetSurface(screen, true);
  }

  SDL_RenderSetLogicalSize(renderer, width, height);
  return true;
}

bool Video::__SetConfig(const int width, const int height, const bool _fullscreen)
{
  if (!sdl_window) {
    if (!CreateWindowAndRenderer(width, height, _fullscreen))
      return false;
  } else {
    if (SDL_SetWindowFullscreen(sdl_window, _fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) < 0)
      return false;
    if (!_fullscreen)
      SDL_SetWindowSize(sdl_window, width, height);
  }
  fullscreen = _fullscreen;

  // Render at the size the window really has (fullscreen, or as tiled by
  // the window manager), not just the size that was asked for
  int ww = width, wh = height;
  SDL_GetWindowSize(sdl_window, &ww, &wh);
  if (ww <= 0 || wh <= 0) {
    ww = width;
    wh = height;
  }
  window_size = Point2i(ww, wh);
  Point2i target = TargetGameSize();
  return ResizeGameSurface(target.x, target.y);
}

Point2i Video::TargetGameSize() const
{
#ifdef ANDROID
  if (scale_percent == 0 && renderer) {
    // Phones have no desktop scale factor, and drawing 1:1 on a dense
    // screen makes everything tiny: aim for a game 540 pixels high (200% on a
    // 1080p phone)
    static const int AUTO_HEIGHT = 540;
    int pw = window_size.x, ph = window_size.y;
    SDL_GetRendererOutputSize(renderer, &pw, &ph);
    int percent = ph * 100 / AUTO_HEIGHT;
    if (percent < 100)
      percent = 100;
    return Point2i(pw * 100 / percent, ph * 100 / percent);
  }
#endif
  if (scale_percent == 0 || !renderer)
    return window_size; // logical size: the desktop's scale factor applies

  // Real pixels (more than window_size on HiDPI screens) divided by the scale
  int pw = window_size.x, ph = window_size.y;
  SDL_GetRendererOutputSize(renderer, &pw, &ph);
  return Point2i(pw * 100 / (int)scale_percent, ph * 100 / (int)scale_percent);
}

bool Video::HasPendingResize() const
{
  if (window.IsNull())
    return false;
  Point2i target = TargetGameSize();
  int w = target.x < MIN_GAME_W ? MIN_GAME_W : target.x;
  int h = target.y < MIN_GAME_H ? MIN_GAME_H : target.y;
  return w != window.GetWidth() || h != window.GetHeight();
}

bool Video::ApplyPendingResize()
{
  if (!HasPendingResize())
    return false;
  Point2i target = TargetGameSize();
  if (!ResizeGameSurface(target.x, target.y))
    return false;
  Camera::GetInstance()->SetSize(window.GetWidth(), window.GetHeight());
  AppWarmux::GetInstance()->RefreshDisplay();
  return true;
}

bool Video::SetConfig(const int width, const int height, const bool _fullscreen)
{
  bool r;
  bool window_was_null = window.IsNull();

  if (!window_was_null
      && width == window.GetWidth()
      && height == window.GetHeight()
      && fullscreen == _fullscreen)
    return true; // nothing to change :-)

  int old_width, old_height;
  bool old_fullscreen;
  if (window_was_null) {
    old_width = available_configs.begin()->GetX();
    old_height = available_configs.begin()->GetY();
  } else {
    old_width = window.GetWidth();
    old_height = window.GetHeight();
  }
  old_fullscreen = fullscreen;

  r = __SetConfig(width, height, _fullscreen);
  if (!r) {
    fprintf(stderr,
            "WARNING: Fail to initialize main window with the following configuration:\n"
            " %dx%d, fullscreen: %d (%s)\n", width, height, _fullscreen, SDL_GetError());

    // Trying previous configuration
    if (! __SetConfig(old_width, old_height, old_fullscreen)) {
      // previous configuration fails !?!
      // let's have another try without fullscreen
      if (! __SetConfig(old_width, old_height, false)) {
        Error(Format("ERROR: Fail to initialize main window with the following configuration:\n"
                     " %dx%d, no fullscreen: %s\n", old_width, old_height, SDL_GetError()));
        exit(EXIT_FAILURE);
      }
    }
  }

  Camera::GetInstance()->SetSize(window.GetWidth(), window.GetHeight());

  // refresh all the map when switching to higher resolution
  if (!window_was_null)
    AppWarmux::GetInstance()->RefreshDisplay();

  return r;
}

void Video::ToggleFullscreen()
{
  // The resulting size change arrives as a window event
  if (SDL_SetWindowFullscreen(sdl_window, fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP) == 0)
    fullscreen = !fullscreen;
  Present();
}

void Video::SetWindowCaption(const std::string& caption) const
{
  if (sdl_window)
    SDL_SetWindowTitle(sdl_window, caption.c_str());
}

void Video::InitSDL()
{
  if (SDLReady)
    return;

  if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
    Error(Format("Unable to initialize SDL library: %s", SDL_GetError()));
    exit(EXIT_FAILURE);
  }

  SDLReady = true;
}

void Video::Flip()
{
  if (!texture || window.IsNull())
    return;
  SDL_UpdateTexture(texture, NULL, window.GetSurface()->pixels, window.GetSurface()->pitch);
  Present();
}

void Video::Present()
{
  if (!renderer || !texture)
    return;
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

static int SDLCALL InputFilter(void *data, SDL_Event *event)
{
  Video *video = static_cast<Video*>(data);

  if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
    // Keep side buttons from being mistaken for the wheel
    if (event->button.button == SDL_BUTTON_X1)
      event->button.button = 6;
    else if (event->button.button == SDL_BUTTON_X2)
      event->button.button = 7;
    return 1;
  }

  if (event->type == SDL_MOUSEWHEEL) {
    int dy = event->wheel.y;
    if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
      dy = -dy;
    if (!dy || !video->GetSDLWindow())
      return 0;

    int x, y;
    SDL_GetMouseState(&x, &y); // window coordinates; the renderer converts them
    SDL_Event click;
    SDL_zero(click);
    click.button.windowID = SDL_GetWindowID(video->GetSDLWindow());
    click.button.which = event->wheel.which;
    click.button.button = (dy > 0) ? WX_BUTTON_WHEELUP : WX_BUTTON_WHEELDOWN;
    click.button.clicks = 1;
    click.button.x = x;
    click.button.y = y;
    for (int n = (dy > 0) ? dy : -dy; n > 0; n--) {
      click.type = SDL_MOUSEBUTTONDOWN;
      click.button.state = SDL_PRESSED;
      SDL_PushEvent(&click);
      click.type = SDL_MOUSEBUTTONUP;
      click.button.state = SDL_RELEASED;
      SDL_PushEvent(&click);
    }
    return 0; // drop the original wheel event
  }
  return 1;
}

int Video::WindowEventWatch(void *data, SDL_Event *event)
{
  // Menus only redraw on input, so show the last frame again whenever the
  // window is uncovered or resized instead of leaving it black.
  if (event->type == SDL_WINDOWEVENT) {
    switch (event->window.event) {
    case SDL_WINDOWEVENT_SIZE_CHANGED:
      // Remember it; the game surface follows at the next safe point
      static_cast<Video*>(data)->window_size = Point2i(event->window.data1, event->window.data2);
      static_cast<Video*>(data)->Present();
      break;
    case SDL_WINDOWEVENT_EXPOSED:
      static_cast<Video*>(data)->Present();
      break;
    default:
      break;
    }
  }
  return 0;
}

Point2i Video::WindowToGame(int x, int y) const
{
  if (!renderer)
    return Point2i(x, y);
  float lx, ly;
  SDL_RenderWindowToLogical(renderer, x, y, &lx, &ly);
  return Point2i((int)lx, (int)ly);
}

Point2i Video::GameToWindow(int x, int y) const
{
  if (!renderer)
    return Point2i(x, y);
  int wx, wy;
  SDL_RenderLogicalToWindow(renderer, (float)x, (float)y, &wx, &wy);
  return Point2i(wx, wy);
}

bool Video::SaveScreenshot()
{
  int u = 0;
  char name[19];
  std::string path = Config::GetConstInstance()->GetPersonalDataDir();
  std::string filename;

  // Generate and try names
  do {
    snprintf(name, sizeof(name), "screenshot%04u.bmp", u);
    filename = path + name;
    u++;
  } while (DoesFileExist(filename) && u<10000);

  if (u == 10000)
    return false;
  if (window.ImgSave(filename, true)) {
    printf("Saved screenshot %s\n", filename.c_str());
    return true;
  }
  printf("Failed saving screenshot %s: %s\n", filename.c_str(), SDL_GetError());
  return false;
}


Uint32 GetGameMouseState(int *x, int *y)
{
  int wx, wy;
  Uint32 buttons = SDL_GetMouseState(&wx, &wy);
  Point2i p = AppWarmux::GetInstance()->video->WindowToGame(wx, wy);
  if (x) *x = p.x;
  if (y) *y = p.y;
  return buttons;
}

Surface& GetMainWindow()
{
  return AppWarmux::GetInstance()->video->window;
}

void SwapWindowClip(Rectanglei& r)
{
  AppWarmux::GetInstance()->video->window.SwapClipRect(r);
}
