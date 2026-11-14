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

#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <SDL_timer.h>
#include <WARMUX_network.h>
#include "network/lan_discovery.h"
#include "include/constant.h"

#define LAN_MAGIC       "WARMUX-LAN 1"
#define LAN_PACKET_SIZE 512

static std::vector<std::string> SplitLines(const UDPpacket *p)
{
  std::vector<std::string> lines;
  std::string cur;
  for (int i = 0; i < p->len; i++) {
    char c = (char)p->data[i];
    if (c == '\n') {
      lines.push_back(cur);
      cur.clear();
    } else if (c != '\r' && c != '\0') {
      cur += c;
    }
  }
  if (!cur.empty())
    lines.push_back(cur);
  return lines;
}

static void SetData(UDPpacket *p, const std::string &s)
{
  size_t n = s.size() < (size_t)p->maxlen ? s.size() : (size_t)p->maxlen;
  memcpy(p->data, s.data(), n);
  p->len = (int)n;
}

static std::string AddressToString(const IPaddress &a)
{
  // IPaddress.host is in network byte order
  const Uint8 *b = (const Uint8 *)&a.host;
  char buf[16];
  snprintf(buf, sizeof(buf), "%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
  return buf;
}

// --- Host side --------------------------------------------------------------

LanResponder::LanResponder() : socket(NULL), packet(NULL) { }

LanResponder::~LanResponder()
{
  Close();
}

bool LanResponder::Open()
{
  Close();
  WNet::Init();
  socket = SDLNet_UDP_Open(LAN_DISCOVERY_PORT);
  if (!socket) {
    // Another game hosted on this machine already answers
    fprintf(stderr, "LAN discovery: cannot listen on UDP port %d: %s\n",
            LAN_DISCOVERY_PORT, SDLNet_GetError());
    return false;
  }
  packet = SDLNet_AllocPacket(LAN_PACKET_SIZE);
  if (!packet) {
    Close();
    return false;
  }
  return true;
}

void LanResponder::Close()
{
  if (packet)
    SDLNet_FreePacket(packet);
  packet = NULL;
  if (socket)
    SDLNet_UDP_Close(socket);
  socket = NULL;
}

void LanResponder::Poll(const std::string &game_name, int tcp_port, bool passworded)
{
  if (!socket)
    return;

  // A handful per call is plenty; the rest waits for the next one
  for (int n = 0; n < 8 && SDLNet_UDP_Recv(socket, packet) > 0; n++) {
    std::vector<std::string> l = SplitLines(packet);
    if (l.size() < 3 || l[0] != LAN_MAGIC || l[1] != "DISCOVER"
        || l[2] != Constants::WARMUX_VERSION)
      continue;

    std::ostringstream reply;
    reply << LAN_MAGIC << "\nGAME\n" << Constants::WARMUX_VERSION << "\n"
          << tcp_port << "\n" << (passworded ? 1 : 0) << "\n"
          << game_name.substr(0, 200) << "\n";
    // Straight back to the sender's address and port
    SetData(packet, reply.str());
    SDLNet_UDP_Send(socket, -1, packet);
  }
}

// --- Joining side -----------------------------------------------------------

std::list<GameServerInfo> LanDiscovery::Search(uint wait_ms)
{
  std::list<GameServerInfo> games;
  WNet::Init();

  // Prefer the discovery port so the answers get through a firewall that
  // allows it; it is taken if this machine is hosting too
  UDPsocket sock = SDLNet_UDP_Open(LAN_DISCOVERY_PORT);
  if (!sock)
    sock = SDLNet_UDP_Open(0);
  if (!sock) {
    fprintf(stderr, "LAN discovery: %s\n", SDLNet_GetError());
    return games;
  }
  UDPpacket *p = SDLNet_AllocPacket(LAN_PACKET_SIZE);
  if (!p) {
    SDLNet_UDP_Close(sock);
    return games;
  }

  IPaddress broadcast;
  broadcast.host = INADDR_BROADCAST;
  SDLNet_Write16(LAN_DISCOVERY_PORT, &broadcast.port);
  const std::string probe = std::string(LAN_MAGIC) + "\nDISCOVER\n"
                          + Constants::WARMUX_VERSION + "\n";

  const Uint32 start = SDL_GetTicks();
  Uint32 next_probe = start;
  int probes = 0;
  while (SDL_GetTicks() - start < wait_ms) {
    // A few probes, in case one gets lost (Wi-Fi drops broadcasts)
    if (probes < 3 && SDL_GetTicks() >= next_probe) {
      SetData(p, probe);
      p->address = broadcast;
      SDLNet_UDP_Send(sock, -1, p);
      probes++;
      next_probe = SDL_GetTicks() + wait_ms / 4;
    }

    while (SDLNet_UDP_Recv(sock, p) > 0) {
      std::vector<std::string> l = SplitLines(p);
      if (l.size() < 6 || l[0] != LAN_MAGIC || l[1] != "GAME"
          || l[2] != Constants::WARMUX_VERSION)
        continue;

      GameServerInfo info;
      info.ip_address = AddressToString(p->address);
      info.port = l[3];
      info.passworded = (l[4] == "1");
      info.game_name = l[5];

      bool known = false;
      for (std::list<GameServerInfo>::const_iterator it = games.begin();
           it != games.end(); ++it)
        if (it->ip_address == info.ip_address && it->port == info.port)
          known = true;
      if (!known)
        games.push_back(info);
    }
    SDL_Delay(20);
  }

  SDLNet_FreePacket(p);
  SDLNet_UDP_Close(sock);
  return games;
}
