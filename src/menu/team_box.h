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
 *  Team box
 *****************************************************************************/

#ifndef TEAM_BOX_H
#define TEAM_BOX_H

#include <string>
#include <vector>
#include "gui/horizontal_box.h"
#include "gui/picture_widget.h"

class Button;
class CustomTeam;
class Team;
class SpinButtonWithPicture;
class Label;
class PictureWidget;
class TextBox;

class AbstractSpinButton;

class TeamBox : public HBox
{
  std::string previous_player_name; // only for network
  std::string ai_name;

  Surface player_local_ai_surf;
  Surface player_local_human_surf;
  Surface player_remote_ai_surf;
  Surface player_remote_human_surf;

  Team * associated_team;
  PictureWidget *team_logo, *player_type;
  Label * team_name;
  Label * commander_label;
  bool dark_theme;
  bool compact;
  TextBox * player_name;
  Button * next_custom_team;
  Button * previous_custom_team;
  AbstractSpinButton * nb_characters;

  std::vector<CustomTeam *> custom_team_list;
  unsigned custom_team_current_id;

  CustomTeam *GetCustomTeam() const;
  void UpdateTeam(const std::string& old_team_id) const;

  void UpdatePlayerNameColor();

 public:
  // White text, for dark backgrounds
  void SetDarkTheme();
  // compact: a card for the network lobby (small stepper, rounded background)
  TeamBox(const std::string& player_name, const Point2i &size, bool compact = false);
  static int CompactHeight();
  PictureWidget::ScalingType PictureType() const;
  // Click the logo to change team, the icon below to switch human / AI
  Rectanglei LogoRect() const;
  Rectanglei PlayerTypeRect() const;
  virtual void RedrawBackground(const Rectanglei &rect) const;

  void SetTeam(Team& _team, bool read_team_values=false);
  void SetAIName(const std::string name);
  void UpdatePlayerType();
  void ClearTeam();
  Team* GetTeam() const { return associated_team; }

  void ValidOptions() const;

  bool IsLocal() const;

  virtual bool Update(const Point2i &mousePosition,
                      const Point2i &lastMousePosition);

  virtual Widget* Click(const Point2i &mousePosition, uint button);
  virtual Widget* ClickUp(const Point2i &mousePosition, uint button);

  void SwitchPlayerType();
};

#endif
