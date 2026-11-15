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
 ******************************************************************************/

#include <algorithm>
#include <cmath>
#include "gui/flat_widgets.h"
#include "gui/text_box.h"
#include "interface/mouse.h"
#include "tool/string_tools.h"
#include "graphic/surface.h"
#include "graphic/text.h"
#include "graphic/video.h"

namespace Flat
{
  const Color PANEL           (16,  22,  34, 210);
  const Color INSET           (255, 255, 255, 22);
  const Color INSET_HOVER     (255, 255, 255, 44);
  const Color PRIMARY         (46,  158,  88, 255);
  const Color PRIMARY_HOVER   (62,  184, 108, 255);
  const Color SECONDARY       (255, 255, 255, 38);
  const Color SECONDARY_HOVER (255, 255, 255, 70);
  const Color DANGER          (196,  62,  62, 255);
  const Color DANGER_HOVER    (222,  82,  82, 255);
  const Color DISABLED        (255, 255, 255, 18);
  const Color DARK_HOVER      (34,  46,  66, 230);
  const Color TEXT            (255, 255, 255, 255);
  const Color TEXT_DIM        (180, 192, 210, 255);
  const Color ACCENT          (110, 182, 255, 255);
  const Color GOOD            (110, 220, 140, 255);
  const Color WARN            (255, 140, 110, 255);

  // Sizes are in menu pixels, which already follow the display scale: they
  // stay the same whatever the window size (small windows get a bit less)
  float Scale()
  {
    const Surface &w = GetMainWindow();
    float r = std::min(w.GetWidth() / 1366.0f, w.GetHeight() / 768.0f);
    return r <= 1.0f ? 1.0f : powf(r, 0.8f);
  }

  int S(int pixels)
  {
    return (int)(pixels * Scale() + 0.5f);
  }

  Font::font_size_t F(Font::font_size_t size)
  {
    return (Font::font_size_t)S((int)size);
  }

  int Unit()
  {
    return GetMainWindow().GetHeight() < 480 ? 6 : S(8);
  }

  int ButtonHeight()
  {
    return 4 * Unit();
  }

  void RoundedBox(Surface &s, const Rectanglei &r, int radius, const Color &c)
  {
    const int x = r.GetPositionX(), y = r.GetPositionY();
    const int w = r.GetSizeX(), h = r.GetSizeY();
    radius = std::min(radius, std::min(w, h) / 2);
    if (radius <= 1) {
      s.BoxColor(r, c);
      return;
    }
    // Body: one full-width band in the middle, two bands between the corners
    s.BoxColor(Rectanglei(x, y + radius, w, h - 2 * radius), c);
    s.BoxColor(Rectanglei(x + radius, y, w - 2 * radius, radius), c);
    s.BoxColor(Rectanglei(x + radius, y + h - radius, w - 2 * radius, radius), c);
    // Corners: quarter discs
    const int r1 = radius - 1;
    s.FilledPieColor(x + radius - 1,     y + radius - 1,     r1, 180, 270, c);
    s.FilledPieColor(x + w - radius,     y + radius - 1,     r1, 270, 360, c);
    s.FilledPieColor(x + w - radius,     y + h - radius,     r1,   0,  90, c);
    s.FilledPieColor(x + radius - 1,     y + h - radius,     r1,  90, 180, c);
  }

  // Redraw `rect` of a rounded box on the main window (clipped to it)
  void RedrawRounded(const Rectanglei &box, const Rectanglei &rect,
                            int radius, const Color &c)
  {
    Rectanglei clip = rect;
    clip.Clip(box);
    Surface &s = GetMainWindow();
    // Stay inside the current clip too: outside it, the background under the
    // panel was not redrawn, and the translucent fill would add up
    Rectanglei old = clip;
    s.SwapClipRect(old);           // old = the current clip
    clip.Clip(old);
    if (clip.GetSizeX() > 0 && clip.GetSizeY() > 0) {
      Rectanglei tmp = clip;
      s.SwapClipRect(tmp);
      RoundedBox(s, box, radius, c);
    }
    s.SwapClipRect(old);           // restore it
  }
}

// --- FlatPanel --------------------------------------------------------------

FlatPanel::FlatPanel(uint width, uint padding, uint gap, const Color &_fill)
  : VBox(width, false, false, true)
  , fill(_fill)
  , radius(Flat::Unit() * 3 / 2)
{
  Widget::SetBorder(transparent_color, padding);
  SetMargin(gap);
  Widget::SetBackgroundColor(transparent_color);
}

void FlatPanel::RedrawBackground(const Rectanglei &rect) const
{
  if (ct)
    ct->RedrawBackground(rect);
  if (IsVisible())
    Flat::RedrawRounded(*this, rect, radius, fill);
}

// --- FlatRow ----------------------------------------------------------------

FlatRow::FlatRow(uint height, uint padding, uint gap, const Color &_fill)
  : HBox(height, false, false, false)
  , fill(_fill)
  , radius(Flat::Unit())
{
  Widget::SetBorder(transparent_color, padding);
  SetMargin(gap);
  Widget::SetBackgroundColor(transparent_color);
}

void FlatRow::RedrawBackground(const Rectanglei &rect) const
{
  if (ct)
    ct->RedrawBackground(rect);
  if (IsVisible())
    Flat::RedrawRounded(*this, rect, radius, fill);
}

// --- FlatButton -------------------------------------------------------------

FlatButton::FlatButton(const std::string &label, const Point2i &_size, Style _style,
                       Font::font_size_t font_size)
  : Widget(_size)
  , text(new Text(label, Flat::TEXT, Flat::F(font_size), Font::FONT_BOLD, false))
  , style(_style)
  , enabled(true)
{
}

FlatButton::~FlatButton()
{
  delete text;
}

void FlatButton::SetText(const std::string &label)
{
  text->SetText(label);
  NeedRedrawing();
}

void FlatButton::Draw(const Point2i &mousePosition)
{
  bool hover = enabled && (Contains(mousePosition) || IsHighlighted());
  Color c;
  if (!enabled)
    c = Flat::DISABLED;
  else if (style == PRIMARY)
    c = hover ? Flat::PRIMARY_HOVER : Flat::PRIMARY;
  else if (style == DANGER)
    c = hover ? Flat::DANGER_HOVER : Flat::DANGER;
  else if (style == DARK)
    c = hover ? Flat::DARK_HOVER : Flat::PANEL;
  else
    c = hover ? Flat::SECONDARY_HOVER : Flat::SECONDARY;

  Flat::RoundedBox(GetMainWindow(), *this, Flat::Unit(), c);
  text->SetColor(enabled ? Flat::TEXT : Flat::TEXT_DIM);
  text->DrawCenter(position + size / 2);
}

// --- FlatToggle ------------------------------------------------------------

FlatToggle::FlatToggle(const std::string &label, uint width, bool _value, bool info)
  : Widget(Point2i(width, 3 * Flat::Unit()))
  , text(new Text(label, Flat::TEXT, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, false))
  , info_text(info ? new Text("i", Flat::TEXT, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, false)
                   : NULL)
  , value(_value)
  , info_clicked(false)
{
}

FlatToggle::~FlatToggle()
{
  delete text;
  delete info_text;
}

Rectanglei FlatToggle::InfoRect() const
{
  // A circle the height of the text, a little after it
  const int d = text->GetHeight() + 2;
  return Rectanglei(position.x + text->GetWidth() + Flat::Unit() / 2,
                    position.y + (size.y - d) / 2, d, d);
}

Widget *FlatToggle::ClickUp(const Point2i &mousePosition, uint)
{
  // The "i" is small: accept clicks a little around it (fingers)
  Rectanglei hit = InfoRect();
  const int slack = Flat::Unit() / 2;
  hit = Rectanglei(hit.GetPositionX() - slack, position.y,
                   hit.GetSizeX() + 2 * slack, size.y);
  info_clicked = info_text && hit.Contains(mousePosition);
  if (!info_clicked) {
    value = !value;
    NeedRedrawing();
  }
  return this;
}

void FlatToggle::Draw(const Point2i &mousePosition)
{
  Surface &s = GetMainWindow();
  const int h = size.y;
  const int track_w = h * 2 - h / 4;
  const Rectanglei track(position.x + size.x - track_w, position.y, track_w, h);
  bool hover = Contains(mousePosition) || IsHighlighted();

  if (info_text) {
    const Rectanglei r = InfoRect();
    const bool info_hover = r.Contains(mousePosition);
    Flat::RoundedBox(s, r, r.GetSizeX() / 2, info_hover ? Flat::SECONDARY_HOVER : Flat::SECONDARY);
    info_text->DrawCenter(r.GetPosition() + r.GetSize() / 2);
    if (info_hover)
      hover = false; // the switch does not react to the "i"
  }

  Color c = value ? (hover ? Flat::PRIMARY_HOVER : Flat::PRIMARY)
                  : (hover ? Flat::SECONDARY_HOVER : Flat::SECONDARY);
  Flat::RoundedBox(s, track, h / 2, c);
  const int knob_r = h / 2 - std::max(2, h / 8);
  const int knob_x = value ? track.GetPositionX() + track_w - h / 2
                           : track.GetPositionX() + h / 2;
  s.FilledCircleColor(knob_x, position.y + h / 2, knob_r, Flat::TEXT);

  text->DrawLeftTop(Point2i(position.x, position.y + (h - text->GetHeight()) / 2));
}

// --- FlatStepper -----------------------------------------------------------

FlatStepper::FlatStepper(const std::string &_label, uint width, int value, int step,
                         int min_value, int max_value)
  : AbstractSpinButton(value, step, min_value, max_value)
  , label(new Text(_label, Flat::TEXT_DIM, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, false))
  , value_text(new Text(int2str(value), Flat::TEXT, Flat::F(Font::FONT_MEDIUM), Font::FONT_BOLD, false))
  , enabled(true)
{
  size = Point2i(width, 3 * Flat::Unit());
}

FlatStepper::~FlatStepper()
{
  delete label;
  delete value_text;
}

Rectanglei FlatStepper::PlusRect() const
{
  const int h = size.y;
  return Rectanglei(position.x + size.x - h, position.y, h, h);
}

Rectanglei FlatStepper::MinusRect() const
{
  const int h = size.y;
  return Rectanglei(position.x + size.x - 3 * h - h / 2, position.y, h, h);
}

void FlatStepper::ValueHasChanged()
{
  value_text->SetText(int2str(GetValue()));
  NeedRedrawing();
}

Widget *FlatStepper::ClickUp(const Point2i &mousePosition, uint button)
{
  if (!enabled || !Mouse::IS_CLICK_BUTTON(button))
    return NULL;
  if (MinusRect().Contains(mousePosition)) {
    DecValue();
    return this;
  }
  if (PlusRect().Contains(mousePosition)) {
    IncValue();
    return this;
  }
  return NULL;
}

void FlatStepper::Draw(const Point2i &mousePosition)
{
  Surface &s = GetMainWindow();
  const int h = size.y;
  const Rectanglei minus = MinusRect(), plus = PlusRect();
  const bool can_dec = enabled && GetValue() > GetMinValue();
  const bool can_inc = enabled && GetValue() < GetMaxValue();

  Flat::RoundedBox(s, minus, h / 2, !can_dec ? Flat::DISABLED
                   : minus.Contains(mousePosition) ? Flat::SECONDARY_HOVER : Flat::SECONDARY);
  Flat::RoundedBox(s, plus, h / 2, !can_inc ? Flat::DISABLED
                   : plus.Contains(mousePosition) ? Flat::SECONDARY_HOVER : Flat::SECONDARY);
  // - and + drawn as bars, crisper than glyphs at this size
  const int bar = h / 3, t = std::max(1, h / 12);
  Point2i mc = minus.GetPosition() + minus.GetSize() / 2;
  Point2i pc = plus.GetPosition() + plus.GetSize() / 2;
  s.BoxColor(Rectanglei(mc.x - bar, mc.y - t, 2 * bar, 2 * t), can_dec ? Flat::TEXT : Flat::TEXT_DIM);
  s.BoxColor(Rectanglei(pc.x - bar, pc.y - t, 2 * bar, 2 * t), can_inc ? Flat::TEXT : Flat::TEXT_DIM);
  s.BoxColor(Rectanglei(pc.x - t, pc.y - bar, 2 * t, 2 * bar), can_inc ? Flat::TEXT : Flat::TEXT_DIM);

  value_text->DrawCenter(Point2i((minus.GetPositionX() + minus.GetSizeX() + plus.GetPositionX()) / 2,
                                 position.y + h / 2));
  label->DrawLeftTop(Point2i(position.x, position.y + (h - label->GetHeight()) / 2));
}

// --- FlatField --------------------------------------------------------------

FlatField::FlatField(TextBox *tb, uint width)
  : HBox(0, false, false, false)
  , box(tb)
  , radius(Flat::Unit())
{
  const int pad = Flat::Unit();
  Widget::SetBorder(transparent_color, pad);
  SetMargin(0);
  Widget::SetBackgroundColor(transparent_color);

  box->SetBorder(transparent_color, 0);
  box->SetHighlightBgColor(transparent_color);
  box->SetSizeX(width - 2 * pad);
  box->SetCursorOnlyWhenFocused(true);
  AddWidget(box);
  Pack(); // know our size right away: boxes measure before packing
}

void FlatField::RedrawBackground(const Rectanglei &rect) const
{
  if (ct)
    ct->RedrawBackground(rect);
  if (IsVisible())
    Flat::RedrawRounded(*this, rect, radius, Flat::INSET);
}

Widget *FlatField::Click(const Point2i &mousePosition, uint button)
{
  Point2i inside = mousePosition;
  // Anywhere in the padding counts as a click in the text box
  inside.x = std::max(box->GetPositionX(), std::min(inside.x, box->GetPositionX() + box->GetSizeX() - 1));
  inside.y = box->GetPositionY() + box->GetSizeY() / 2;
  return box->Click(inside, button);
}

Widget *FlatField::ClickUp(const Point2i &mousePosition, uint button)
{
  Point2i inside = mousePosition;
  inside.x = std::max(box->GetPositionX(), std::min(inside.x, box->GetPositionX() + box->GetSizeX() - 1));
  inside.y = box->GetPositionY() + box->GetSizeY() / 2;
  Widget *w = box->ClickUp(inside, button);
  NeedRedrawing();
  return w ? w : box;
}

void FlatMeter::Draw(const Point2i &/*mousePosition*/)
{
  Surface &s = GetMainWindow();
  int h = std::max(4, Flat::Unit());
  Rectanglei track(position.x, position.y + (size.y - h) / 2, size.x, h);
  Flat::RoundedBox(s, track, h / 2, Flat::PANEL);
  int p = std::max(0, std::min(100, percent));
  int w = track.GetSizeX() * p / 100;
  if (w >= h) // narrower than its height, a rounded bar is just a blob
    Flat::RoundedBox(s, Rectanglei(track.GetPositionX(), track.GetPositionY(), w, h), h / 2,
                     Flat::PRIMARY);
}
