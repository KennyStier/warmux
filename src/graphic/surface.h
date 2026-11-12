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
 * Handle a SDL_Surface.
 *****************************************************************************/

#ifndef SURFACE_H
#define SURFACE_H

#include <string>
#include <list>
#include "color.h"
#include <WARMUX_base.h>
#include <WARMUX_point.h>
#include <vector>
#include <WARMUX_rectangle.h>

struct SDL_Surface;
struct SDL_PixelFormat;

// Surface options inherited from SDL 1.2, which SDL2 no longer has as flags.
// Pass them to the Surface constructor, SetAlpha() and SetColorKey().
#define WX_SRCALPHA    0x00010000  // blend using alpha (per-pixel or per-surface)
#define WX_SRCCOLORKEY 0x00001000  // treat the colour key as transparent

class Surface
{
private:
  SDL_Surface* surface;
  bool autoFree;
  // Pixels per coordinate unit; only the screen uses more than 1 (see
  // SetScale())
  int scale;
  // Full-resolution overlay for text (see SetHiResOverlay()), and where
  // to record the areas drawn on it
  SDL_Surface *hires;
  int hires_scale;
  std::vector<Rectanglei> *tracker;
  int Blit(const Surface& src, SDL_Rect *srcRect, SDL_Rect *dstRect);
  static SDL_Rect GetSDLRect(const Rectanglei &r);
  static SDL_Rect GetSDLRect(const Point2i &r);

public:
  /**
   * Default constructor.
   *
   * Build a null surface with autoFree at true.
   */
  explicit Surface() : surface(NULL), autoFree(true), scale(1), hires(NULL), hires_scale(1), tracker(NULL) { };
  /**
   * Constructor building a surface object using an existing SDL_Surface pointer.
   *
   * @param sdl_surface The existing sdl_surface.
   */
  explicit Surface(SDL_Surface *sdl_surface) : surface(sdl_surface), autoFree(true), scale(1), hires(NULL), hires_scale(1), tracker(NULL) { };
  /**
   * Constructor building a surface object using the NewSurface function.
   *
   * @param size
   * @param flags
   * @param useAlpha
   * @see NewSurface
   */
  explicit Surface(const Point2i &size, Uint32 flags, bool useAlpha = true)
  : surface(NULL), autoFree(true), scale(1), hires(NULL), hires_scale(1), tracker(NULL) { NewSurface(size, flags, useAlpha); }
  explicit Surface(const std::string &filename);
  Surface(const Surface &src);
  /**
   * Destructor of the surface.
   *
   * Will free the memory used by the surface if autoFree is set to true and if the counter of reference reach 0
   */
  ~Surface() { AutoFree(); };

  Surface &operator=(const Surface &src);

  void Free();
  void AutoFree() { if (autoFree) Free(); };
  /**
   * Set the auto free status of a surface.
   *
   * In general it should always be true for non-system surface.
   * @param newAutoFree the new autoFree status.
   */
  void SetAutoFree(bool newAutoFree) { autoFree = newAutoFree; };

  /**
   * Change the surface pointer.
   *
   * @param newSurface The new surface to use.
   * @param freePrevius Indicate if the old surface should be freed.
   */
  void SetSurface(SDL_Surface *newSurface, bool freePrevious = true){
    if (freePrevious)
      Free();

    surface = newSurface;
  }

  /**
   * Return the pointer of the SDL_Surface.
   *
   * Should be used carefully.
   */
  SDL_Surface *GetSurface() { return surface; };
  const SDL_Surface *GetSurface() const { return surface; };

  void NewSurface(const Point2i &size, Uint32 flags, bool useAlpha = true);
  int SetAlpha(Uint32 flags, Uint8 alpha);

  void Lock();
  void Unlock();

  void SwapClipRect(Rectanglei& rect);

  /**
   * Blit the whole surface src on the current surface.
   *
   * @param src The source surface.
   */
  int Blit(const Surface& src) { return Blit(src, NULL, NULL); };
  int Blit(const Surface& src, const Point2i& dst);
  int Blit(const Surface& src, const Rectanglei& srcRect, const Point2i &dstPoint);

  void MergeSurface(Surface &spr, const Point2i &position);

  int SetColorKey(Uint32 flag, Uint32 key);
  bool HasColorKey() const;
  Uint32 GetColorKey() const;

  // The 32-bit format of the screen; "display format" conversions target it
  static SDL_PixelFormat *GetDisplayFormat();

  void GetRGBA(Uint32 color, Uint8 &r, Uint8 &g, Uint8 &b, Uint8 &a) const;
  Uint32 MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) const;
  Color GetColor(Uint32 color) const;
  Uint32 MapColor(const Color& color) const;

  int BoxColor(const Rectanglei &rect, const Color &color);
  int RectangleColor(const Rectanglei &rect, const Color &color, const uint &border_size = 1);
  int VlineColor(const uint &x, const uint &y1, const uint &y2, const Color &color);
  int HlineColor(const uint &x1, const uint &x2, const uint &y, const Color &color);
  int LineColor(const uint &x1, const uint &x2, const uint &y1, const uint &y2, const Color &color);
  int AALineColor(const uint &x1, const uint &x2, const uint &y1, const uint &y2, const Color &color);
  int AAFadingLineColor(const uint &x1, const uint &x2, const uint &y1, const uint &y2, const Color &color1, const Color &color2);
  int CircleColor(const uint &x, const uint &y, const uint &rad, const Color &color);
  int FilledCircleColor(const uint &x, const uint &y, const uint &rad, const Color &color);
  int PieColor(const uint &x, const uint &y, const uint &rad, const int &start, const int &end, const Color &color);
  int FilledPieColor(const uint &x, const uint &y, const uint &rad, const int &start, const int &end, const Color &color);
  int AAPolygonColor(const Sint16 * vx, const Sint16 * vy, const int n, const Color & color);
  int AAPolygonColor(std::list<Point2i> polygon, const Color & color);
  int FilledPolygon(const Sint16 * vx, const Sint16 * vy, const int n, const Color & color);
  int FilledPolygon(std::list<Point2i> polygon, const Color & color);
  int TexturedPolygon(const Sint16 * vx, const Sint16 * vy, const int n, const Surface *texture, const int texture_dx, const int texture_dy);
  int TexturedPolygon(std::list<Point2i> polygon, const Surface *texture);

  int Fill(Uint32 color) const;
  int Fill(const Color &color) const;
  int FillRect(const Rectanglei &dstRect, Uint32 color) const;
  int FillRect(const Rectanglei &dstRect, const Color &color) const;

  int ImgLoad(const std::string& filename);
  bool ImgSave(const std::string& filename, bool bmp=false);
  Surface RotoZoom(Double angle, Double zoomx, Double zoomy);
  Surface DisplayFormatAlpha();
  Surface DisplayFormat();
  Uint32 GetPixel(int x, int y) const;
  void PutPixel(int x, int y, Uint32 pixel) const;

  bool IsNull() const { return surface == NULL; };
  Point2i GetSize() const { return Point2i( GetWidth(), GetHeight() ); };

  inline int GetWidth() const { return surface->w / scale; }
  inline int GetHeight() const { return surface->h / scale; }

  /**
   * High-resolution screen: the surface has `s` pixels per coordinate unit.
   * Sizes and every drawing call stay in coordinate units, so the game is
   * unaware; blits are stretched, shapes are drawn at full resolution and
   * Text draws itself with a larger font (BlitHiRes()).
   */
  void SetScale(int s) { scale = s < 1 ? 1 : s; }
  int GetScale() const { return scale; }
  /**
   * During a game frame the screen is drawn at game resolution and only
   * text goes to a full-resolution overlay `s` (k pixels per unit); the
   * areas drawn there are added to `t`. NULL: no overlay.
   */
  void SetHiResOverlay(SDL_Surface *s, int k, std::vector<Rectanglei> *t)
    { hires = s; hires_scale = k; tracker = t; }
  /** Pixels per unit available for sharp text: the scale or the overlay's */
  int HiResScale() const { return scale > 1 ? scale : (hires ? hires_scale : 1); }
  /** Blit a full-resolution image, at a position in pixels, onto this
   *  surface if scaled, else onto its overlay */
  int BlitHiRes(const Surface& src, const Point2i& pixel_pos);

  /** Return the length of a surface scanline in bytes. */
  Uint16 GetPitch() const { return surface->pitch; };
  /** Return the number of bytes used to represent each pixel
   *  in a surface. Usually one to four.
   */
  Uint8 GetBytesPerPixel() const { return surface->format->BytesPerPixel; };
  /** Return a pointer on the pixels data. */
  unsigned char *GetPixels() const { return (unsigned char *)surface->pixels; }

  static Surface DisplayFormatColorKey(const uint32_t* data, SDL_PixelFormat *fmt,
                                       int w, int h, int stride,
                                       uint8_t alpha_threshold, bool rle=false);
  Surface DisplayFormatColorKey(uint8_t alpha_threshold, bool rle=false);

  Surface Crop(const Rectanglei& area) const;

  Surface Mirror();
};

#endif
