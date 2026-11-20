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
 * The Network Game menu: finding a game (on the local network or through
 * the index server, in one list), hosting one, or joining one by address.
 *
 *   left   games found, refreshed on their own, each with a Join button
 *   right  "Host a game" (name, optional password, list online)
 *          "Join by address" (host[:port], password if the game has one)
 *
 * On short screens (phones in landscape) the right column only holds the
 * player's name and two buttons, which open Host or Join in the list's
 * place.
 *****************************************************************************/

#ifndef NETWORK_CONNECTION_MENU_H
#define NETWORK_CONNECTION_MENU_H

#include <map>
#include <string>
#include <vector>
#include <SDL_timer.h>
#include "menu.h"
#include "network/network.h"
#include <WARMUX_index_server.h> // for GameServerInfo

class FlatButton;
class FlatPanel;
class FlatToggle;
class GameListBox;
class Label;
class PasswordBox;
class TextBox;

class NetworkConnectionMenu : public Menu
{
public:
  typedef enum {
    NET_NOTHING,
    NET_HOST,
    NET_CONNECT
  } network_menu_action_t;

private:
  network_menu_action_t action;
  typedef enum { VIEW_GAMES, VIEW_HOST, VIEW_JOIN } view_t;
  bool compact;  // phone layout: one of the views at a time

  TextBox *nickname;
  FlatPanel *games_panel;
  FlatPanel *host_panel;
  FlatPanel *join_panel;
  FlatButton *open_host;    // compact layout only
  FlatButton *open_join;
  FlatButton *host_cancel;
  FlatButton *join_cancel;

  // Games
  GameListBox *games;
  Label *games_status;
  std::map<FlatButton*, GameServerInfo> join_buttons;
  std::vector<std::string> shown_games; // to rebuild the list only on change

  // Host a game
  TextBox *host_name;
  PasswordBox *host_pwd;
  FlatToggle *host_public;
  FlatButton *host_start;

  // Join by address
  TextBox *join_address;
  PasswordBox *join_pwd;
  FlatButton *join_go;
  Label *join_hint;

  FlatButton *back;

  SDL_TimerID refresh_timer;
  Uint32 last_search_end;

  virtual void OnClickUp(const Point2i &mousePosition, int button);
  virtual void HandleEvent(const SDL_Event& evnt);
  virtual void Draw(const Point2i &mousePosition);

  // Laid out for the window size: rebuilt (by main.cpp) when it changes
  virtual bool RebuildOnResize() const { return true; }
  virtual bool signal_ok();
  virtual bool signal_cancel();

  void ShowView(view_t view);
  void SaveNickname();
  void ShowGames();
  void ThreadRefreshList();
  void DisplayNetError(connection_state_t conn);

  bool Host();
  bool JoinGame(const GameServerInfo &game, const std::string &password);
  bool JoinAddress();
  bool Connect(const std::string& address, const std::string& port,
               const std::string& password);
  void PlayNetworkGame(); // the lobby, then back here

public:
  NetworkConnectionMenu(network_menu_action_t action);
  ~NetworkConnectionMenu();
};

#endif
