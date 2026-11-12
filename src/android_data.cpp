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
 * Android: the game reads its data through ordinary file paths, but on
 * Android it ships inside the APK. On first start (and after each update)
 * copy it to game/ in the app's internal storage, then run from there: the
 * game finds data/ relative to the current directory, and keeps its
 * settings in Warmux/ next to it.
 *
 * The subfolder matters: for a relative path SDL_RWFromFile tries the
 * internal storage folder itself before the APK, so extracting there would
 * make an update re-read the old copies instead of the new assets.
 *****************************************************************************/

#ifdef ANDROID

#include <SDL.h>
#include <android/log.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

#include "android_data.h"

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "warmux", __VA_ARGS__)

static bool ReadAsset(const char *name, std::string &out)
{
  SDL_RWops *rw = SDL_RWFromFile(name, "rb");
  if (!rw)
    return false;
  out.clear();
  char buf[64 * 1024];
  size_t n;
  while ((n = SDL_RWread(rw, buf, 1, sizeof(buf))) > 0)
    out.append(buf, n);
  SDL_RWclose(rw);
  return true;
}

static bool ReadFile(const std::string &name, std::string &out)
{
  FILE *f = fopen(name.c_str(), "rb");
  if (!f)
    return false;
  out.clear();
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
    out.append(buf, n);
  fclose(f);
  return true;
}

static void MakeParents(const std::string &path)
{
  for (size_t i = path.find('/'); i != std::string::npos; i = path.find('/', i + 1))
    mkdir(path.substr(0, i).c_str(), 0755);
}

static bool CopyAsset(const std::string &name)
{
  SDL_RWops *in = SDL_RWFromFile(name.c_str(), "rb");
  if (!in) {
    LOG("Missing asset %s: %s", name.c_str(), SDL_GetError());
    return false;
  }
  MakeParents(name);
  std::string tmp = name + ".part";
  FILE *out = fopen(tmp.c_str(), "wb");
  if (!out) {
    LOG("Cannot write %s: %s", tmp.c_str(), strerror(errno));
    SDL_RWclose(in);
    return false;
  }
  char buf[64 * 1024];
  size_t n;
  bool ok = true;
  while ((n = SDL_RWread(in, buf, 1, sizeof(buf))) > 0)
    ok &= fwrite(buf, 1, n, out) == n;
  SDL_RWclose(in);
  ok &= fclose(out) == 0;
  return ok && rename(tmp.c_str(), name.c_str()) == 0;
}

bool AndroidPrepareData()
{
  const char *storage = SDL_AndroidGetInternalStoragePath();
  if (!storage) {
    LOG("No internal storage: %s", SDL_GetError());
    return false;
  }
  std::string base = std::string(storage) + "/game";
  mkdir(base.c_str(), 0755);
  if (chdir(base.c_str()) != 0) {
    LOG("Cannot use %s: %s", base.c_str(), strerror(errno));
    return false;
  }

  std::string stamp, installed;
  if (!ReadAsset("data_stamp.txt", stamp)) {
    LOG("data_stamp.txt missing from the APK");
    return false;
  }
  if (ReadFile("data/.stamp", installed) && installed == stamp)
    return true;   // data already up to date

  std::string list;
  if (!ReadAsset("filelist.txt", list)) {
    LOG("filelist.txt missing from the APK");
    return false;
  }
  LOG("Copying game data to %s", base.c_str());
  unsigned count = 0;
  size_t start = 0;
  while (start < list.size()) {
    size_t end = list.find('\n', start);
    if (end == std::string::npos)
      end = list.size();
    std::string name = list.substr(start, end - start);
    start = end + 1;
    if (name.empty())
      continue;
    if (!CopyAsset(name))
      return false;
    count++;
  }

  FILE *f = fopen("data/.stamp", "wb");
  if (f) {
    fwrite(stamp.data(), 1, stamp.size(), f);
    fclose(f);
  }
  LOG("Copied %u data files", count);
  return true;
}

#endif // ANDROID
