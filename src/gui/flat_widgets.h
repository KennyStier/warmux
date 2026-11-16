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
 * Flat, rounded widgets for the redesigned menus: translucent dark panels,
 * large buttons that suit both mouse and touch, padded text fields.
 *****************************************************************************/

#ifndef FLAT_WIDGETS_H
#define FLAT_WIDGETS_H

#include <string>
#include "graphic/color.h"
#include "graphic/font.h"
#include "gui/horizontal_box.h"
#include "gui/vertical_box.h"
#include "gui/widget.h"
#include "gui/abstract_spin_button.h"

class Surface;
class Text;
class TextBox;

namespace Flat
{
  extern const Color PANEL;      // panel background
  extern const Color INSET;      // rows and fields inside a panel
  extern const Color INSET_HOVER;
  extern const Color PRIMARY;    // main action (green)
  extern const Color PRIMARY_HOVER;
  extern const Color SECONDARY;  // other actions (light, translucent)
  extern const Color SECONDARY_HOVER;
  extern const Color DANGER;
  extern const Color DANGER_HOVER;
  extern const Color DISABLED;
  extern const Color DARK_HOVER;
  extern const Color TEXT;
  extern const Color TEXT_DIM;
  extern const Color ACCENT;     // tags such as "Local network"
  extern const Color GOOD;       // "Ready"
  extern const Color WARN;       // errors and warnings

  // Base spacing unit, from the menu height (about 8 px at 720 lines)
  // Menus grow with the window: 1 up to 1366x768, then a little less than
  // proportionally (so big windows do not get giant buttons)
  float Scale();
  int S(int pixels);                          // a size, scaled
  Font::font_size_t F(Font::font_size_t size); // a font size, scaled
  int Unit();
  int ButtonHeight();
  // A rounded rectangle filled with `c` (corners do not overlap the body, so
  // translucent colours blend evenly)
  void RoundedBox(Surface &s, const Rectanglei &r, int radius, const Color &c);
  // For RedrawBackground(): redraw `rect` of the rounded `box` (clipped)
  void RedrawRounded(const Rectanglei &box, const Rectanglei &rect, int radius, const Color &c);
}

// A vertical stack of widgets on a rounded, translucent panel. `padding` is
// the space around the content, `gap` the space between widgets.
class FlatPanel : public VBox
{
  Color fill;
  int radius;
  int min_height;
  int bottom_widgets;

public:
  FlatPanel(uint width, uint padding, uint gap, const Color &fill = Flat::PANEL);
  void SetFill(const Color &c) { fill = c; NeedRedrawing(); }
  // At least this tall, whatever it holds (to fill a column); the last
  // `at_bottom` widgets then sit at the bottom (action buttons, in thumb
  // reach on phones)
  void SetMinHeight(int h, int at_bottom = 0) { min_height = h; bottom_widgets = at_bottom; }
  virtual void Pack();
  virtual void RedrawBackground(const Rectanglei &rect) const;
};

// A horizontal row on a rounded inset background
class FlatRow : public HBox
{
  Color fill;
  int radius;

public:
  FlatRow(uint height, uint padding, uint gap, const Color &fill = Flat::INSET);
  void SetFill(const Color &c) { fill = c; NeedRedrawing(); }
  virtual void RedrawBackground(const Rectanglei &rect) const;
};

class FlatButton : public Widget
{
public:
  typedef enum { PRIMARY, SECONDARY, DANGER, DARK } Style;

private:
  Text *text;
  Style style;
  bool enabled;

public:
  FlatButton(const std::string &label, const Point2i &size, Style style = SECONDARY,
             Font::font_size_t font_size = Font::FONT_MEDIUM);
  virtual ~FlatButton();

  void SetText(const std::string &label);
  void SetStyle(Style s) { style = s; NeedRedrawing(); }
  void SetEnabled(bool e) { enabled = e; NeedRedrawing(); }
  bool IsEnabled() const { return enabled; }

  virtual void Draw(const Point2i &mousePosition);
  virtual Widget *Click(const Point2i &, uint) { return enabled ? this : NULL; }
  virtual Widget *ClickUp(const Point2i &, uint) { return enabled ? this : NULL; }
  virtual void Pack() { }
};

// A thin rounded bar filled to `percent`, centred in its height
class FlatMeter : public Widget
{
  int percent;

public:
  FlatMeter(const Point2i &size, int percent) : Widget(size), percent(percent) { }
  virtual void Draw(const Point2i &mousePosition);
  virtual void Pack() { }
};

// A labelled on/off switch. With `info`, a round "i" follows the label: a
// click on it does not switch, InfoClicked() tells the menu to explain.
class FlatToggle : public Widget
{
  Text *text;
  Text *info_text;
  bool value;
  bool info_clicked;

  Rectanglei InfoRect() const;

public:
  FlatToggle(const std::string &label, uint width, bool value, bool info = false);
  bool InfoClicked() const { return info_clicked; }
  virtual ~FlatToggle();
  bool GetValue() const { return value; }
  void SetValue(bool v) { value = v; NeedRedrawing(); }

  virtual void Draw(const Point2i &mousePosition);
  virtual Widget *Click(const Point2i &, uint) { return this; }
  virtual Widget *ClickUp(const Point2i &, uint);
  virtual void Pack() { }
};

// "Label   (-) 6 (+)": a number with small round buttons
class FlatStepper : public AbstractSpinButton
{
  Text *label;
  Text *value_text;
  bool enabled;

  Rectanglei MinusRect() const;
  Rectanglei PlusRect() const;

public:
  FlatStepper(const std::string &label, uint width, int value, int step,
              int min_value, int max_value);
  virtual ~FlatStepper();
  void SetEnabled(bool e) { enabled = e; NeedRedrawing(); }

  virtual void Draw(const Point2i &mousePosition);
  virtual Widget *Click(const Point2i &, uint) { return NULL; }
  // Returns this only for a click on - or +
  virtual Widget *ClickUp(const Point2i &mousePosition, uint button);
  virtual void Pack() { }
  virtual void ValueHasChanged();
};

// A text box with padding on a rounded inset background; a click anywhere
// on it puts the cursor in the text box
class FlatField : public HBox
{
  TextBox *box;
  int radius;

public:
  // Takes ownership of `tb`, which is resized to fit `width`
  FlatField(TextBox *tb, uint width);
  TextBox *GetTextBox() const { return box; }
  virtual void RedrawBackground(const Rectanglei &rect) const;
  virtual Widget *ClickUp(const Point2i &mousePosition, uint button);
  virtual Widget *Click(const Point2i &mousePosition, uint button);
};

#endif
