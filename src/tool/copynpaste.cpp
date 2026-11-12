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
 *****************************************************************************
 * Clipboard paste, through SDL2's cross-platform clipboard API.
 *****************************************************************************/

#include "copynpaste.h"
#include <cstring>
#include <SDL_clipboard.h>
#include <SDL_stdinc.h>

bool RetrieveBuffer(std::string& text, std::string::size_type& pos)
{
  if (!SDL_HasClipboardText())
    return false;

  char *data = SDL_GetClipboardText(); // UTF-8
  if (!data)
    return false;

  // Keep a single line: chat and text boxes don't handle newlines
  std::string paste(data);
  SDL_free(data);
  std::string::size_type eol = paste.find_first_of("\r\n");
  if (eol != std::string::npos)
    paste.erase(eol);
  if (paste.empty())
    return false;

  if (pos > text.size())
    pos = text.size();
  text.insert(pos, paste);
  pos += paste.size();
  return true;
}
