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
 * Message Box
 *****************************************************************************/

#include <algorithm>
#include "gui/msg_box.h"
#include "gui/scroll_box.h"
#include "gui/label.h"

MsgBox::MsgBox(const Point2i& size, Font::font_size_t fsize,
               Font::font_style_t fstyle, uint max_lines)
  : ScrollBox(size)
  , font_size(fsize)
  , font_style(fstyle)
  , max_history(max_lines)
{
  Widget::SetBorder(defaultOptionColorRect, 2);
  Widget::SetBackgroundColor(defaultOptionColorBox);
}

MsgBox::~MsgBox()
{
}

void MsgBox::NewMessage(const std::string &msg, const Color& color)
{
  if (WidgetCount() >= max_history+1) {
    // Remove first lines
    RemoveFirstWidget();
  }
  AddWidget(new Label(msg, size.x-10, font_size, font_style, color));
  Pack();

  // Show the newest line: scroll to the end, measured once the new line is
  // in (guessing it beforehand scrolled past the end while all text fit)
  offset = std::max(0, GetMaxOffset());
  Pack();
  NeedRedrawing();
}
