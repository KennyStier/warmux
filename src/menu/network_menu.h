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
 *
 * On short screens (phones in landscape):
 *   left   the chat, and below it every team as a small chip: tap one of
 *          yours to change it (team, characters, human or computer), in
 *          the chat's place; "+" adds a team
 *   right  players, the map (tap: map and game mode, in the chat's place)
 *          and "Ready" / "Start game"
 *****************************************************************************/

#ifndef NETWORK_MENU_H
#define NETWORK_MENU_H

#include <string>
#include <utility>
#include <vector>
#include "menu.h"

class FlatButton;
class FlatPanel;
class FlatRow;
class FlatStepper;
class FlatToggle;
class PictureWidget;
class Widget;
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

  /* Panels shown in another's place (phones: team, map; big screens: map) */
  typedef enum { LOBBY_CHAT, LOBBY_TEAM, LOBBY_MAP } lobby_view_t;
  bool compact;
  FlatPanel *chat_panel;
  FlatPanel *team_panel;   // the team pop-up, in the chat's place
  FlatPanel *map_panel;
  FlatPanel *chips_panel;
  FlatRow *chips_row;
  std::vector<std::pair<Widget*, int> > chips; // and their slot (-1: "+")
  std::string shown_chips;
  std::string shown_team;
  std::string shown_map;
  int chip_h;
  FlatButton *map_card;
  FlatButton *map_done;
  FlatButton *team_done;
  FlatButton *team_remove;
  FlatButton *team_prev;
  FlatButton *team_next;
  Label *team_title;
  PictureWidget *team_flag;
  FlatStepper *team_chars;
  FlatToggle *team_ai;
  PictureWidget *map_preview; // big screens
  ScrollBox *map_grid;        // big screens: every map, as a grid
  std::vector<std::pair<FlatPanel*, uint> > map_tiles;
  std::string shown_map_list;
  int shown_map_choice;
  int edited_slot;         // in team_box

  void BuildDesktopLayout();
  void BuildPhoneLayout();
  void ShowLobbyView(lobby_view_t view);
  void UpdateChips();
  void UpdateTeamPanel();
  void UpdateMapCard();
  void UpdateMapGrid();
  bool PhoneClick(Widget *w, const Point2i &mousePosition);

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
