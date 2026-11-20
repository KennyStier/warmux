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
 *  Network teams selection box
 *****************************************************************************/

#ifndef NETWORK_TEAMS_SELECTION_BOX_H
#define NETWORK_TEAMS_SELECTION_BOX_H

#include <vector>
#include <WARMUX_team_config.h>
#include "menu/teams_selection_box.h"

class Team;

class NetworkTeamsSelectionBox : public TeamsSelectionBox
{
  void RemoveLocalTeam(uint i);
  void SetLocalTeam(uint i, Team& team);

  const std::string GetLocalPlayerName();

  bool IsSelected(Team * team);

  void UpdateNbTeams();

public:
  NetworkTeamsSelectionBox(const Point2i &size, bool w_border, bool compact = false);

  virtual void ValidTeamsSelection();
  virtual Widget* ClickUp(const Point2i &mousePosition, uint button);

  void AddTeamCallback(const std::string& team_id);
  void UpdateTeamCallback(const std::string& old_team_id, const std::string& team_id);
  void DelTeamCallback(const std::string& team_id);
  Team * FindUnusedTeam(const std::string default_team_id);
  bool HasOpenTeamSlot();
  void RequestTeam();
  // Developer aid (WARMUX_TEST_AUTOPLAY): let the computer play our teams
  void SetLocalTeamsAI();

  // For the phone lobby, which shows the teams its own way (this box
  // stays, hidden, to do the work)
  uint NbSlots() const { return teams_selections.size(); }
  TeamBox *Slot(uint i) const { return teams_selections.at(i); }
  void PrevTeam(uint i);
  void NextTeam(uint i);
  // Can we add a team, and remove (our last) one
  bool CanAddTeam();
  void RequestTeamRemoval();
};
#endif
