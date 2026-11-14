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
 * The lobby of a network game, once hosted or joined:
 *
 *   left   your teams, the map
 *   right  players (with who is ready, on the host), chat,
 *          game mode and "Start game" (host) or "Ready" (others)
 *****************************************************************************/

#ifndef NETWORK_MENU_H
#define NETWORK_MENU_H

#include <string>
#include <utility>
#include <vector>
#include "menu.h"

class FlatButton;
class FlatPanel;
class FlatToggle;
class Label;
class MapSelectionBox;
class NetworkTeamsSelectionBox;
class ScrollBox;
class TalkBox;
class Team;

class NetworkMenu : public Menu
{
  bool waiting_for_server;

  Label *subtitle;
  FlatButton *leave;

  /* Players */
  Label *players_status;
  ScrollBox *players_list;
  std::string shown_players;   // to rebuild the list only on change
  int last_ready;              // players ready, for the chat notices (host)

  /* Options and start */
  FlatButton *mode_button;     // host only
  std::vector<std::pair<std::string, std::string> > game_modes;
  size_t game_mode;
  FlatToggle *play_in_loop;
  FlatButton *action_button;   // "Start game" or "Ready"

  /* Chat controller */
  TalkBox* msg_box;

  /* Team controllers */
  NetworkTeamsSelectionBox *team_box;

  /* Map controllers */
  MapSelectionBox *map_box;

  void PrepareForNewGame();
  void SaveOptions();
  void WaitingForGameMaster();

  void ShowGameMode();
  void UpdateSubtitle();
  void UpdatePlayers();
  void UpdateActionButton();

  virtual void Draw(const Point2i &mousePosition);
  virtual void HandleEvent(const SDL_Event& evnt);
  virtual void OnClickUp(const Point2i &mousePosition, int button);

  virtual void key_ok();
  virtual bool signal_ok();
  virtual bool signal_cancel();
  virtual void signal_begin_run();

  void RequestSavedTeams();

public:
  NetworkMenu();

  void AddTeamCallback(const std::string& team_id);
  void UpdateTeamCallback(const std::string& old_team_id, const std::string& team_id);
  void DelTeamCallback(const std::string& team_id);
  void ChangeMapCallback();
  void SetGameMasterCallback();
  Team * FindUnusedTeam(const std::string default_team_id);
  bool HasOpenTeamSlot();

  void ReceiveMsgCallback(const std::string& msg, const Color& color);
  void SetMapsCallback(const std::vector<uint>& list);
};

#endif
