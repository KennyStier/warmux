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

#include "ai/ai_stupid_player.h"
#include "graphic/video.h"
#include "gui/button.h"
#include "gui/flat_widgets.h"
#include "gui/label.h"
#include "gui/picture_widget.h"
#include "gui/spin_button_picture.h"
#include "gui/text_box.h"
#include "gui/vertical_box.h"
#include "menu/team_box.h"
#include "include/action_handler.h"
#include "network/network.h"
#include "team/team.h"
#include "team/custom_team.h"
#include "team/custom_teams_list.h"
#include "tool/resource_manager.h"

// Logo (38) + player type icon (30) + padding
int TeamBox::CompactHeight()
{
  return Flat::S(38) + 2 + Flat::S(30) + 2 * Flat::Unit();
}

// Cards scale their pictures with the menu (see Flat::Scale())
PictureWidget::ScalingType TeamBox::PictureType() const
{
  return compact ? PictureWidget::FIT_SCALING : PictureWidget::NO_SCALING;
}

TeamBox::TeamBox(const std::string& _player_name, const Point2i& _size, bool _compact) :
  HBox(_size.y, false, false, false),
  ai_name(NO_AI_NAME),
  dark_theme(false),
  compact(_compact)
{
  associated_team = NULL;

  SetMargin(compact ? Flat::Unit() : 2);
  if (compact)
    Widget::SetBorder(transparent_color, Flat::Unit()); // padding
  else
    SetNoBorder();
  Widget::SetBackgroundColor(transparent_color);

  Profile *res = GetResourceManager().LoadXMLProfile("graphism.xml", false);

  /********        Logos: team mascott, player type icon      ********/
  Box * tmp_logo_box = new VBox(W_UNDEF, false, false, false);
  tmp_logo_box->SetMargin(1);
  tmp_logo_box->SetNoBorder();

  const int logo = compact ? Flat::S(38) : 38;
  const int type_h = compact ? Flat::S(30) : 30;
  team_logo = new PictureWidget(Point2i(logo, logo));
  tmp_logo_box->AddWidget(team_logo);

  player_local_ai_surf = GetResourceManager().LoadImage(res, "menu/player_local_ai");
  player_local_human_surf = GetResourceManager().LoadImage(res, "menu/player_local_human");
  player_remote_ai_surf = GetResourceManager().LoadImage(res, "menu/player_remote_ai");
  player_remote_human_surf = GetResourceManager().LoadImage(res, "menu/player_remote_human");

  player_type =  new PictureWidget(Point2i(logo, type_h));
  player_type->SetSurface(player_local_ai_surf, PictureType());
  tmp_logo_box->AddWidget(player_type);

  AddWidget(tmp_logo_box);

  /********    Center box: team name, commander   *********/
  int width = compact ? _size.x - 2 * Flat::Unit() - logo - 4 - Flat::Unit()
                      : _size.x - (2*2+(38+2*2)+(110+2*2));
  Box * tmp_player_box = new VBox(_size.y, false, false, false);
  tmp_player_box->SetMargin(compact ? 2 : 0);
  tmp_player_box->SetNoBorder();

  previous_player_name = "team";
  team_name = new Label(previous_player_name, width,
                        compact ? Flat::F(Font::FONT_MEDIUM) : Font::FONT_MEDIUM, Font::FONT_BOLD);
  tmp_player_box->AddWidget(team_name);

  /********    Names: "Head commander" + text/custom team    *******/
  if (compact) {
    commander_label = NULL; // the name field speaks for itself
  } else {
    commander_label = new Label(_("Head commander"), width,
                                Font::FONT_SMALL, Font::FONT_BOLD);
    tmp_player_box->AddWidget(commander_label);
  }

  custom_team_list = GetCustomTeamsList().GetList();
  custom_team_current_id = 0;

  if (custom_team_list.empty()) {
    player_name = new TextBox(_player_name, width,
                              compact ? Flat::F(Font::FONT_SMALL) : Font::FONT_SMALL, Font::FONT_BOLD);
    tmp_player_box->AddWidget(player_name);

    next_custom_team = NULL;
    previous_custom_team = NULL;

  } else {
    next_custom_team = new Button(res, "menu/plus");
    previous_custom_team = new Button(res, "menu/minus");

    player_name = new TextBox(_player_name, width - 2 * (next_custom_team->GetSizeY() + 2),
                              Font::FONT_SMALL, Font::FONT_BOLD);

    Box * tmp_name_box = new HBox(player_name->GetSizeY(), false, false, false);
    tmp_name_box->SetNoBorder();
    tmp_name_box->SetMargin(2);

    tmp_name_box->AddWidget(previous_custom_team);
    tmp_name_box->AddWidget(player_name);
    tmp_name_box->AddWidget(next_custom_team);
    tmp_player_box->AddWidget(tmp_name_box);
  }
  if (compact) {
    player_name->SetBorder(Color(255, 255, 255, 50), 1);
    nb_characters = new FlatStepper(_("Characters"), width, 6, 1, 1, 10);
    tmp_player_box->AddWidget(nb_characters);
    tmp_player_box->Pack(); // boxes measure their children before packing them
    AddWidget(tmp_player_box);
    tmp_logo_box->Pack();
    Pack();
    return;
  }
  AddWidget(tmp_player_box);


  /**********     Number of characters        **********/
  nb_characters = new SpinButtonWithPicture(_("Number of characters"), "menu/ico_play",
                                            Point2i(110, 120), 6, 1, 1, 10);
  AddWidget(nb_characters);
}

Rectanglei TeamBox::LogoRect() const
{
  return Rectanglei(team_logo->GetPosition(), team_logo->GetSize());
}

Rectanglei TeamBox::PlayerTypeRect() const
{
  return Rectanglei(player_type->GetPosition(), player_type->GetSize());
}

void TeamBox::RedrawBackground(const Rectanglei &rect) const
{
  if (!compact) {
    HBox::RedrawBackground(rect);
    return;
  }
  if (ct)
    ct->RedrawBackground(rect);
  if (IsVisible())
    Flat::RedrawRounded(*this, rect, Flat::Unit(), Flat::INSET);
}

void TeamBox::ClearTeam()
{
  associated_team = NULL;
  ai_name = NO_AI_NAME;

  NeedRedrawing();
}

CustomTeam* TeamBox::GetCustomTeam() const
{
  if (custom_team_list.empty()) {
    return NULL;
  }

  return GetCustomTeamsList().GetByName(player_name->GetText());
}

void TeamBox::UpdatePlayerNameColor()
{
  if (associated_team) {
    if (associated_team->IsLocal()) {
      if (GetCustomTeam()) {
        // player name is head commander of custom team
        player_name->SetColor(c_yellow);
      } else {
        player_name->SetColor(c_white);
      }
    } else {
      // remote team
      player_name->SetColor(light_gray_color);
    }
  }
}

bool TeamBox::Update(const Point2i &mousePosition,
                     const Point2i &lastMousePosition)
{
  UpdatePlayerNameColor();

  Box::Update(mousePosition, lastMousePosition);
  if (need_redrawing) {
    Draw(mousePosition);
  }

  if (associated_team) {
    WidgetList::Update(mousePosition, lastMousePosition);
    if (previous_player_name != player_name->GetText()) {
      previous_player_name = player_name->GetText();
      if (Network::GetInstance()->IsConnected()) {
        ValidOptions();
      }
    }
  } else {
    RedrawBackground(*this);
  }

  //SwapWindowClip(r);
  need_redrawing = false;
  return true;
}

Widget* TeamBox::ClickUp(const Point2i &mousePosition, uint button)
{
  if (associated_team) {

    if (!associated_team->IsLocal())
      return NULL; // it's not a local team, we can't configure it !!

    Widget* w = WidgetList::ClickUp(mousePosition, button);

    if (w == nb_characters) {
      if (Network::GetInstance()->IsConnected()) {
        ValidOptions();
      }
      return w;
    }
    if (w == player_name) {
      return w;
    }

    if (!w) {
      return w;
    }

    if (!custom_team_list.empty()) {

      if (w == next_custom_team) {
        player_name->SetText(custom_team_list[custom_team_current_id]->GetName());

        if (custom_team_current_id == custom_team_list.size()-1) {
          custom_team_current_id = 0;
        } else {
          custom_team_current_id++;
        }
      }

      if (w == previous_custom_team) {

        player_name->SetText(custom_team_list[custom_team_current_id]->GetName());

        if (custom_team_current_id == 0) {
          custom_team_current_id = custom_team_list.size()-1;
        } else {
          custom_team_current_id--;
        }
      }
    }
  }
  return NULL;
}

Widget* TeamBox::Click(const Point2i &/*mousePosition*/, uint /*button*/)
{
  return NULL;
}

void TeamBox::SetTeam(Team& _team, bool read_team_values)
{
  Team* old_team = associated_team;

  associated_team = &_team;

  if (_team.IsRemote()) {
    team_name->SetFont(dark_theme ? Color(180, 192, 210, 255) : dark_gray_color,
                       compact ? Flat::F(Font::FONT_MEDIUM) : Font::FONT_MEDIUM,
                       Font::FONT_BOLD, false);

    // translators: this is the team listing and will expand in a context like "OOo team - Remote"
    team_name->SetText(compact ? Format(_("%s Team"), _team.GetName().c_str())
                               : Format(_("%s Team - Remote"), _team.GetName().c_str()));
    if (compact)
      static_cast<FlatStepper*>(nb_characters)->SetEnabled(false);

    if (previous_custom_team) {
      previous_custom_team->SetVisible(false);
      next_custom_team->SetVisible(false);
    }
  } else {
    team_name->SetFont(dark_theme ? white_color : primary_red_color,
                       compact ? Flat::F(Font::FONT_MEDIUM) : Font::FONT_MEDIUM,
                       Font::FONT_BOLD, true);
    team_name->SetText(Format(_("%s Team"), _team.GetName().c_str()));
    if (compact)
      static_cast<FlatStepper*>(nb_characters)->SetEnabled(true);

    if (previous_custom_team) {
      previous_custom_team->SetVisible(true);
      next_custom_team->SetVisible(true);
    }
  }
  UpdatePlayerType();
  team_logo->SetSurface(_team.GetFlag(), PictureType());

  if (read_team_values) {
    player_name->SetText(_team.GetPlayerName());
    nb_characters->SetValue(_team.GetNbCharacters());
    SetAIName(_team.GetAIName());
  } else if (old_team) {
    UpdateTeam(old_team->GetId());
  }
  previous_player_name = player_name->GetText();

  NeedRedrawing();
}

void TeamBox::SetAIName(const std::string name)
{
  ai_name = name;
  UpdatePlayerType();
}

void TeamBox::UpdatePlayerType()
{
  bool is_human = ai_name == NO_AI_NAME;
  if (associated_team->IsLocal()) {
    if (is_human) {
      player_type->SetSurface(player_local_human_surf, PictureType());
    } else {
      player_type->SetSurface(player_local_ai_surf, PictureType());
    }
  } else {
    if (is_human) {
      player_type->SetSurface(player_remote_human_surf, PictureType());
    } else {
      player_type->SetSurface(player_remote_ai_surf, PictureType());
    }
  }
}

void TeamBox::UpdateTeam(const std::string& old_team_id) const
{
  // set the number of characters
  associated_team->SetNbCharacters(uint(nb_characters->GetValue()));

  // set the player name
  associated_team->SetPlayerName(player_name->GetText());

  associated_team->SetAIName(ai_name);

  // change only for local teams...
  if (associated_team->IsLocal()) {

    CustomTeam* custom = GetCustomTeam();
    if (custom) {
      associated_team->SetCustomCharactersNames(custom->GetCharactersNameList());
    } else {
      associated_team->ClearCustomCharactersNames();
    }

    // send team configuration to the remote clients
    if (Network::GetInstance()->IsConnected()) {
      Action* a = new Action(Action::ACTION_GAME_UPDATE_TEAM);
      a->Push(int(Network::GetInstance()->GetPlayer().GetId()));
      a->Push(old_team_id);
      a->Push(associated_team->GetId());
      a->Push(associated_team->GetPlayerName());
      a->Push(int(associated_team->GetNbCharacters()));
      a->Push(associated_team->GetAIName());
      associated_team->PushCustomCharactersNamesIntoAction(a);
      ActionHandler::GetInstance()->NewAction (a);
    }
  }
}

void TeamBox::ValidOptions() const
{
  UpdateTeam(associated_team->GetId());
}

bool TeamBox::IsLocal() const
{
  if (associated_team != NULL && associated_team->IsLocal()) {
    return true;
  }

  return false;
}

void TeamBox::SwitchPlayerType()
{
  if (!associated_team)
    return;

  SetAIName((ai_name == NO_AI_NAME) ? DEFAULT_AI_NAME : NO_AI_NAME);
  if (Network::GetInstance()->IsConnected()) {
    ValidOptions();
  }
}

void TeamBox::SetDarkTheme()
{
  dark_theme = true;
  team_name->SetColor(white_color);
  if (commander_label)
    commander_label->SetColor(Color(180, 192, 210, 255));
  if (!compact)
    static_cast<SpinButtonWithPicture*>(nb_characters)->SetLabelColor(Color(180, 192, 210, 255));
  NeedRedrawing();
}
