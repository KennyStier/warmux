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
 ******************************************************************************/

#include <iostream> //cerr
#include "graphic/text.h"
#include "graphic/video.h"
#include "include/app.h"
#include "include/constant.h"
#include "interface/interface.h"
#include "map/camera.h"
#include "map/map.h"
#include "tool/xml_document.h"

Text::Text(const std::string & text,
           const Color & fontColor,
           uint fontSize,
           Font::font_style_t fontStyle,
           bool isShadowed,
           const Color & _shadowColor,
           bool _dummy) :
  surf(),
  background(),
  txt(text),
  color(fontColor),
  shadowed(isShadowed),
  dummy(_dummy),
  bg_offset(0),
  max_width(0),
  shadowColor(_shadowColor),
  font_size((Font::font_size_t)fontSize),
  font_style(fontStyle),
  offset(0),
  hi_scale(0)
{
  Init();
}

Text::Text() :
  surf(),
  background(),
  txt("No text"),
  color(black_color),
  shadowed(true),
  dummy(false),
  bg_offset(0),
  max_width(0),
  shadowColor(),
  font_size(Font::FONT_SMALL),
  font_style(Font::FONT_NORMAL),
  offset(0),
  hi_scale(0)
{
}

void Text::Init()
{
  if (shadowed) {
    int width = Font::GetInstance(font_size, font_style)->GetWidth("x");
    bg_offset = (uint)(width>>3); // shadow offset = 0.125ex
    if (bg_offset < 1) {
      bg_offset = 1;
    }
  }
  Render();
}

void Text::LoadXMLConfiguration(XmlReader * xmlFile,
                                const xmlNode * textNode)
{
  std::string xmlText("Text not found");
  xmlFile->ReadStringAttr(textNode, "text", xmlText);
  if ("%VERSION%" == xmlText) {
    xmlText = Constants::WARMUX_VERSION;
  } else if ("%WEB_SITE%" == xmlText) {
    xmlText = Constants::WEB_SITE;
  }

  Color textColor(0, 255);
  xmlFile->ReadHexColorAttr(textNode, "textColor", textColor);

  // Load the font size ... based on 72 DPI
  int fontSize = 12;
  float tmpValue;

  if (xmlFile->ReadPercentageAttr(textNode, "fontSize", tmpValue)) {
    fontSize = GetMainWindow().GetHeight() * tmpValue / 100;
  } else {
    xmlFile->ReadPixelAttr(textNode, "fontSize", fontSize);
  }

  std::string fontStyle;
  xmlFile->ReadStringAttr(textNode, "fontStyle", fontStyle);

  bool activeShadow = false;
  xmlFile->ReadBoolAttr(textNode, "shadow", activeShadow);
  Color shadowColor(255, 255);
  xmlFile->ReadHexColorAttr(textNode, "shadowColor", shadowColor);

  SetText(xmlText);
  SetFont(textColor,
          (Font::font_size_t)fontSize,
          DetectFontStyle(fontStyle),
          activeShadow,
          shadowColor);
  Init();
}

Font::font_style_t Text::DetectFontStyle(const std::string & fontStyle)
{
  if ("bold" == fontStyle) {
    return Font::FONT_BOLD;
  } else if ("italic" == fontStyle) {
    return Font::FONT_ITALIC;
  }
  return Font::FONT_NORMAL;
}

void Text::Render()
{
  hi_scale = 0; // the high-resolution copy is out of date
  RenderAt(1, surf, background);
}

// Render at k times the font size (and wrapping width) into s and bg
void Text::RenderAt(int k, Surface &s, Surface &bg) const
{
  if (!dummy) {
    if ("" == txt) {
      return;
    }

    if (max_width != 0 || txt.find_first_of('\n', 0)!=std::string::npos) {
      RenderMultiLinesAt(k, s, bg);
      return;
    }

    Font* font = Font::GetInstance((Font::font_size_t)(font_size * k), font_style);

    s = font->CreateSurface(txt, color);
    if (shadowed) {
      bg = font->CreateSurface(txt, shadowColor);
    }
  } else {
    s = Surface(Point2i(font_size * k, font_size * k), 0);
    if (shadowed) {
      bg = Surface(Point2i(font_size * k, font_size * k), 0);
    }
  }
}

void Text::RenderMultiLines()
{
  hi_scale = 0;
  RenderMultiLinesAt(1, surf, background);
}

void Text::RenderMultiLinesAt(int k, Surface &s, Surface &bg) const
{
  const uint max_width = this->max_width * k;
  if ("" == txt) {
    return;
  }

  Font* font = Font::GetInstance((Font::font_size_t)(font_size * k), font_style);

  // Make a first try
  if (font->GetWidth(txt) < int(max_width)) {
    s = font->CreateSurface(txt, color);
    if (shadowed) {
      bg = font->CreateSurface(txt, shadowColor);
    }
    return;
  }

  // Cut the text on space
  std::vector<std::string> ret_lines;
  uint line_width = 0;
  uint max_line_width = 0;
  std::string::size_type ret_old_pos = 0;
  while (ret_old_pos < txt.size()) {
    std::string::size_type ret_current_pos = txt.find_first_of('\n', ret_old_pos);
    std::string line = (ret_current_pos==std::string::npos)
                     ? txt.substr(ret_old_pos, std::string::npos)
                     : txt.substr(ret_old_pos, ret_current_pos-ret_old_pos);
    std::string::size_type old_pos = 0, current_pos = 0;
    std::vector<std::string> tokens;
    uint index_word = 0;
    uint index_lines = 0;

    while (old_pos < line.size() &&
           (current_pos = line.find_first_of(' ', old_pos)) != std::string::npos) {
      std::string tmp = line.substr(old_pos, current_pos-old_pos);
      if (tmp != " " && tmp != "") {
        tokens.push_back(tmp);
      }
      old_pos = current_pos+1;
    }
    tokens.push_back(line.substr(old_pos));

    // Compute size
    std::vector<std::string> lines;
    while (index_word < tokens.size()) {
      if (lines.size() == index_lines) {
        // first word of a line
        lines.push_back(tokens.at(index_word));

        // compute current line size
        line_width = font->GetWidth(tokens.at(index_word));
        if (line_width > max_line_width) {
          max_line_width = line_width;
        }

      } else {
        line_width = font->GetWidth(lines.at(index_lines)+" "+tokens.at(index_word));

        if ( line_width > max_width ) {

          // line will be too long : prepare next line!
          index_lines++;
          index_word--;

        } else {
          lines.at(index_lines) += " " + tokens.at(index_word);

          // this is the longest line
          if (line_width > max_line_width) {
            max_line_width = line_width;
          }
        }

      }

      index_word++;
    }
    std::vector<std::string>::iterator it = lines.begin();
    while (it != lines.end()) {
      ret_lines.push_back(std::string(*it));
      ++it;
    }

    if (ret_current_pos == std::string::npos)
      break;
    ret_old_pos = ret_current_pos+1;
  }

  // really Render !

  // First, creating a destination surface
  if (max_line_width == 0) {
    max_line_width = max_width;
  }

  // We reduce interline space by using GetLineHeight,
  // but we still want the last line to be properly displayed
  Point2i size(max_line_width,
               GetLineHeight(font)*(ret_lines.size()-1)+font->GetHeight());
  s = Surface(size, SDL_SWSURFACE|WX_SRCALPHA, true);

  // for each line
  for (uint i = 0; i < ret_lines.size(); i++) {
    Surface tmp = font->CreateSurface(ret_lines[i], color);
    s.MergeSurface(tmp, Point2i(0, GetLineHeight(font)*i));
  }

  // Render the shadow !
  if (!shadowed)
    return;

  bg = Surface(size, SDL_SWSURFACE|WX_SRCALPHA, true);

  // Putting pixels of each image in destination surface
  // for each lines
  for (uint i = 0; i < ret_lines.size(); i++) {
    Surface tmp = font->CreateSurface(ret_lines[i], black_color);
    bg.MergeSurface(tmp, Point2i(0, GetLineHeight(font)*i));
  }
}

// On a high-resolution screen, draw a copy rendered with a font that much
// larger instead of stretching this one: sharp text
bool Text::DrawHiRes(const Point2i &pos) const
{
  Surface& window = GetMainWindow();
  int k = window.HiResScale();
  if (k <= 1 || dummy)
    return false;
  if (hi_scale != k) {
    RenderAt(k, surf_hi, background_hi);
    hi_scale = k;
  }
  if (surf_hi.IsNull())
    return false;
  if (shadowed && !background_hi.IsNull())
    window.BlitHiRes(background_hi, Point2i((pos.x + bg_offset) * k, (pos.y + bg_offset) * k));
  window.BlitHiRes(surf_hi, Point2i(pos.x * k, pos.y * k));
  return true;
}

void Text::DrawLeftTop(const Point2i &position) const
{
  if(txt == "" && !dummy) return;

  Rectanglei dst_rect(position + Point2i(offset, 0), surf.GetSize());
  Surface& window = GetMainWindow();

  if (DrawHiRes(dst_rect.GetPosition())) {
    GetWorld().ToRedrawOnScreen(Rectanglei(dst_rect.GetPosition(),
                                           dst_rect.GetSize() + bg_offset));
    return;
  }

  if(shadowed){
    Rectanglei shad_rect;

    shad_rect.SetPosition(dst_rect.GetPosition() + bg_offset);
    shad_rect.SetSize(background.GetWidth(), background.GetHeight() );

    window.Blit(background, shad_rect.GetPosition());
    window.Blit(surf, dst_rect.GetPosition());

    GetWorld().ToRedrawOnScreen(Rectanglei(dst_rect.GetPosition(),
                                           shad_rect.GetSize() + bg_offset));
  }else{
    window.Blit(surf, dst_rect.GetPosition());
    GetWorld().ToRedrawOnScreen(dst_rect);
  }
}


void Text::DrawCenterTopOnMap(const Point2i &pos) const
{
  if (GetMainWindow().HiResScale() > 1 && txt != "") {
    // Same as AbsoluteDraw(), with the sharp copy
    Rectanglei on_map(Point2i(pos.x - surf.GetWidth() / 2, pos.y),
                      surf.GetSize() + bg_offset);
    if (!on_map.Intersect(*Camera::GetInstance()))
      return;
    GetWorld().ToRedrawOnMap(on_map);
    if (DrawHiRes(on_map.GetPosition() - Camera::GetInstance()->GetPosition()))
      return;
  }
  if(shadowed)
    AbsoluteDraw(background, Point2i(bg_offset + pos.x - surf.GetWidth() / 2,
                                     bg_offset + pos.y) );
  AbsoluteDraw(surf, Point2i(pos.x - surf.GetWidth() / 2, pos.y) );
}

void Text::DrawCursor(const Point2i &text_pos, std::string::size_type cursor_pos, int real_width)
{
  // the cursor position is expressed in number of bytes, taking care of UTF8 character

  //sort of a hacky way to get the cursor pos, but I couldn't find anything better...
  int txt_width = 1;
  if (GetText() != "") {
    Text txt_before_cursor(*this);
    txt_before_cursor.SetText(GetText().substr(0, cursor_pos));
    txt_width = txt_before_cursor.GetWidth();
    if (txt_width > real_width)
      offset = real_width - 2 - txt_width;
    if (txt_width+offset < 0)
      offset = -txt_width;
  }
  GetMainWindow().VlineColor(text_pos.GetX()+txt_width+offset,
                             text_pos.GetY(),
                             text_pos.GetY()+GetHeight()-2, c_white);
}

int Text::GetHeight() const
{
  Font* font = Font::GetInstance(font_size, font_style);
  if (txt=="" || dummy) {
    return font->GetHeight() + bg_offset;
  }
  return std::max(surf.GetHeight(), font->GetHeight()) + bg_offset;
}

void DrawTmpBoxText(Font& font, Point2i pos,
                    const std::string& txt, uint space,
                    const Color& boxColor, const Color& rectColor)
{
  Point2i size = font.GetSize(txt) + Point2i(space, space)*2;

  Rectanglei rect( pos - size/2, size);

  AppWarmux * app = AppWarmux::GetInstance();

  app->video->window.BoxColor(rect, boxColor);
  app->video->window.RectangleColor(rect, rectColor);

  GetWorld().ToRedrawOnScreen( rect );

  pos.y += font.GetHeight(txt)/2;
  font.WriteCenter( pos, txt, white_color);
}

void Text::SetFont(const Color &_font_color,
                   const Font::font_size_t _font_size,
                   const Font::font_style_t _font_style,
                   bool _font_shadowed,
                   const Color & _shadowColor)
{
  color = _font_color;
  font_size = _font_size;
  font_style = _font_style;
  shadowColor = _shadowColor;

  if (shadowed != _font_shadowed) {
    // recompute shadow offset
    shadowed = _font_shadowed;
    Init();
  }
}
