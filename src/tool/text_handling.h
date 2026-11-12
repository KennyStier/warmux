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
 * Text typing, deleting, editing, ...
 *****************************************************************************/

#ifndef TEXT_HANDLING_H
#define TEXT_HANDLING_H

#include <string>
#include <SDL_keyboard.h>

// Handle an editing key (cursor movement, deletion, paste). Returns true if
// the key belongs to the text field.
bool TextHandle(std::string& text, std::string::size_type& pos, const SDL_Keysym& key);
// Insert UTF-8 text from an SDL_TEXTINPUT event at the cursor
bool TextInsert(std::string& text, std::string::size_type& pos, const std::string& utf8);

#endif //TEXT_HANDLING_H
