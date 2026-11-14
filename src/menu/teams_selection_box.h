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
 *  Teams selection box
 *****************************************************************************/

#ifndef TEAMS_SELECTION_BOX_H
#define TEAMS_SELECTION_BOX_H

#include <vector>
#include "gui/horizontal_box.h"
#include "gui/scroll_box.h"

// Forward declarations
class SpinButtonWithPicture;
class TeamBox;

const uint MAX_NB_TEAMS=8;

class AbstractSpinButton;

class TeamScrollBox : public ScrollBox
{
  // We need a real copy around for when we get destroyed
  std::vector<TeamBox*> teams;
  // Number of teams to be displayed
  uint  count;
  // Teams side by side, in rows of `cols`
  uint  cols;
  std::vector<HBox*> rows;
  void EmptyList();
public:
  TeamScrollBox(const std::vector<TeamBox*>& teams, const Point2i &size, uint cols = 1);
  ~TeamScrollBox();
  void SetNbTeams(uint nb);
};

// -----------------------------------------------
// -----------------------------------------------

class TeamsSelectionBox : public HBox
{
protected:
  TeamScrollBox* list_box;
  AbstractSpinButton *local_teams_nb;
  bool compact;
  std::vector<TeamBox*> teams_selections;

public:
  // White text, for dark backgrounds
  void SetDarkTheme();
  // compact: teams as cards in a grid, with a small "- n +" for their number
  TeamsSelectionBox(const Point2i &size, bool network, bool w_border, bool compact = false);

  virtual void Draw(const Point2i &mousePosition);

  virtual void ValidTeamsSelection() = 0;
  virtual Widget* ClickUp(const Point2i &mousePosition, uint button) = 0;
  virtual Widget* Click(const Point2i &mousePosition, uint button);
};

class LocalTeamsSelectionBox : public TeamsSelectionBox
{
private:
  void PrevTeam(int i);
  void NextTeam(int i);
  void SetNbTeams(uint nb_teams);

public:
  LocalTeamsSelectionBox(const Point2i &size, bool border);

  virtual void ValidTeamsSelection();
  virtual Widget* ClickUp(const Point2i &mousePosition, uint button);
};

#endif
