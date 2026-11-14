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
 * Finding games on the local network, without an index server.
 *
 * The joining side broadcasts a probe on UDP port 3826 (the same number as
 * the game's TCP port, so one firewall rule covers both); every host still
 * waiting for players answers it directly with its game name, TCP port and
 * whether it needs a password. The probe is sent from port 3826 when it is
 * free, so the answer arrives on a port the firewall rule already allows.
 *
 * Packets are short text, one field per line:
 *   probe:  WARMUX-LAN 1 / DISCOVER / <version>
 *   answer: WARMUX-LAN 1 / GAME / <version> / <tcp port> / <0|1> / <name>
 * Hosts only answer probes from the same version, since the game refuses
 * to connect different versions anyway.
 *****************************************************************************/

#ifndef LAN_DISCOVERY_H
#define LAN_DISCOVERY_H

#include <list>
#include <string>
#include <SDL_net.h>
#include <WARMUX_index_server.h> // GameServerInfo

#define LAN_DISCOVERY_PORT 3826

// Host side: answers probes while the game is open to new players
class LanResponder
{
  UDPsocket socket;
  UDPpacket *packet;

public:
  LanResponder();
  ~LanResponder();

  bool Open();
  void Close();
  // Answer any waiting probes (non-blocking; call regularly)
  void Poll(const std::string &game_name, int tcp_port, bool passworded,
            int nb_players, int max_players);
};

namespace LanDiscovery
{
  // Broadcast probes and collect the answers for about `wait_ms`
  std::list<GameServerInfo> Search(uint wait_ms = 1000);
}

#endif
