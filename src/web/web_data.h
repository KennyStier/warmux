/******************************************************************************
 *  Warmux is a convivial mass murder game.
 *  Copyright (C) 2001-2011 Warmux Team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 ******************************************************************************
 * Web build: game data downloaded when first used (see
 * packaging/web/split_data.sh). Elsewhere every call does nothing.
 *****************************************************************************/

#ifndef WEB_DATA_H
#define WEB_DATA_H

#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
// Read the list of files left out of the start-up download and put empty
// stand-ins in their place, so folder listings still find them
void WebDataInit();
// Download the file now if it was left out, waiting for it
void WebDataFetch(const std::string& path);
// The same for every left-out file in a folder, all at once
void WebDataFetchFolder(const std::string& folder);
// Whether a file is here (or was never left out)
bool WebDataReady(const std::string& path);
// Start downloading in the background, without waiting
void WebDataPrefetch(const std::string& path);
#else
inline void WebDataInit() {}
inline void WebDataFetch(const std::string&) {}
inline void WebDataFetchFolder(const std::string&) {}
inline bool WebDataReady(const std::string&) { return true; }
inline void WebDataPrefetch(const std::string&) {}
#endif

#endif
