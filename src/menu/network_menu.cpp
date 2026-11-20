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
 * Game menu
 *****************************************************************************/
#include <WARMUX_index_server.h>
#include <WARMUX_team_config.h>
#include <WARMUX_distant_cpu.h>
#include <algorithm>

#include "menu/network_menu.h"

#include "menu/network_teams_selection_box.h"
#include "menu/map_selection_box.h"
#include "menu/team_box.h"
#include "game/config.h"
#include "game/game.h"
#include "game/game_mode.h"
#include "graphic/video.h"
#include "gui/flat_widgets.h"
#include "gui/label.h"
#include "gui/null_widget.h"
#include "gui/msg_box.h"
#include "gui/picture_widget.h"
#include "gui/scroll_box.h"
#include "gui/talk_box.h"
#include "gui/text_box.h"
#include "include/action_handler.h"
#include "include/app.h"
#include "include/constant.h"
#include "network/network.h"
#include "network/network_client.h"
#include "network/network_server.h"
#include "sound/jukebox.h"
#include "team/teams_list.h"
#include "team/team.h"
#include "tool/resource_manager.h"

// Players list: rows inside a scroll box that fills its panel
class PlayersListBox : public ScrollBox
{
public:
  PlayersListBox(const Point2i &size) : ScrollBox(size, false, false)
  {
    Widget::SetBorder(transparent_color, 0);
    vbox->SetMargin(Flat::Unit() / 2);
  }
  int RowWidth() const { return size.x - scrollbar_width; }
};

static Label *Title(const std::string &text, uint width)
{
  return new Label(text, width, Flat::F(Font::FONT_MEDIUM), Font::FONT_BOLD, Flat::TEXT);
}

// Big screens: players on the left, teams in the middle, the map and the
// chat on the right, and along the bottom what we wait for and Ready /
// Start game. The map picker opens in the teams' place.
void NetworkMenu::BuildDesktopLayout()
{
  const Surface& window = GetMainWindow();
  const int u = Flat::Unit();
  const int margin = 2 * u;
  const int gap = 2 * u;
  const int pad = 2 * u;
  const int button_h = Flat::ButtonHeight();
  const bool master = Network::GetInstance()->IsGameMaster();
  // The lobby is laid out in a block no bigger than this, centred
  const int W = std::min(window.GetWidth(), Flat::S(1400) + 2 * margin);
  const int H = std::min(window.GetHeight(), Flat::S(760) + 2 * margin);
  const int ox = (window.GetWidth() - W) / 2, oy = (window.GetHeight() - H) / 2;
  const int x0 = ox + margin;
  const int full_w = W - 2 * margin;

  // ---- Header: Leave, the game's name and where it is ----
  FlatRow *header = new FlatRow(0, u, gap, Flat::PANEL);
  leave = new FlatButton(_("Leave"), Point2i(12 * u, button_h), FlatButton::SECONDARY);
  header->AddWidget(leave);
  const int names_w = full_w - 2 * u - leave->GetSizeX() - gap;
  VBox *names = new VBox(names_w, false, false, false);
  names->SetNoBorder();
  names->SetMargin(0);
  names->AddWidget(new Label(Network::GetInstance()->GetGameName(), names_w,
                             Flat::F(Font::FONT_BIG), Font::FONT_BOLD, Flat::TEXT));
  subtitle = new Label(" ", names_w, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, Flat::TEXT_DIM);
  names->AddWidget(subtitle);
  names->Pack();
  header->AddWidget(names);
  header->Pack();
  header->SetPosition(x0, oy + margin);
  widgets.AddWidget(header);

  // ---- Footer: what we wait for, and Ready / Start game ----
  FlatRow *footer = new FlatRow(0, u, gap, Flat::PANEL);
  const int action_w = Flat::S(260);
  action_button = new FlatButton(_("Ready"), Point2i(action_w, button_h + u), FlatButton::PRIMARY,
                                 Font::FONT_MEDIUM);
  const int status_w = full_w - 2 * u - gap - action_w;
  players_status = new Label(" ", status_w, Flat::F(Font::FONT_MEDIUM), Font::FONT_BOLD, Flat::TEXT);
  // Centred in the height of the button
  VBox *status_box = new VBox(status_w, false, false, false);
  status_box->SetNoBorder();
  status_box->SetMargin(0);
  status_box->AddWidget(new NullWidget(Point2i(status_w, std::max(0,
    (action_button->GetSizeY() - players_status->GetSizeY()) / 2))));
  status_box->AddWidget(players_status);
  status_box->Pack();
  footer->AddWidget(status_box);
  footer->AddWidget(action_button);
  footer->Pack();
  footer->SetPosition(x0, oy + H - margin - footer->GetSizeY());
  widgets.AddWidget(footer);

  const int top = header->GetPositionY() + header->GetSizeY() + gap;
  const int content_h = footer->GetPositionY() - gap - top;
  const int players_w = std::max(Flat::S(220), full_w * 19 / 100);
  const int right_w = std::max(Flat::S(300), full_w * 27 / 100);
  const int center_w = full_w - players_w - right_w - 2 * gap;
  const int center_x = x0 + players_w + gap;
  const int right_x = x0 + full_w - right_w;
  const int title_h = Title("X", 10)->GetSizeY(); // measured once, leaked label is tiny

  // ---- Left: players ----
  FlatPanel *players = new FlatPanel(players_w, pad, u);
  players->AddWidget(Title(_("Players"), players_w - 2 * pad));
  players_list = new PlayersListBox(Point2i(players_w - 2 * pad, content_h - 2 * pad - title_h - u));
  players->AddWidget(players_list);
  players->SetMinHeight(content_h);
  players->SetPosition(x0, top);
  widgets.AddWidget(players);

  // ---- Middle: teams ----
  const int inner_c = center_w - 2 * pad;
  FlatPanel *teams = new FlatPanel(center_w, pad, u);
  teams->AddWidget(Title(_("Teams"), inner_c));
  Label *hint = new Label(_("Click a team's picture to pick another team, the icon under it to "
                            "switch between human and computer."), inner_c,
                          Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, Flat::TEXT_DIM);
  teams->AddWidget(hint);
  team_box = new NetworkTeamsSelectionBox(
    Point2i(inner_c, content_h - 2 * pad - title_h - hint->GetSizeY() - 2 * u), false, true);
  team_box->SetDarkTheme();
  teams->AddWidget(team_box);
  teams->SetMinHeight(content_h);
  teams->SetPosition(center_x, top);
  widgets.AddWidget(teams);
  chat_panel = teams; // what the map picker takes the place of

  // ---- In its place: the map picker, game mode, play again ----
  map_panel = new FlatPanel(center_w, pad, u);
  map_panel->AddWidget(Title(_("Map"), inner_c));
  play_in_loop = new FlatToggle(_("Play several times"), inner_c, true);
  FlatRow *map_actions = new FlatRow(button_h, 0, u, transparent_color);
  const int done_w = Flat::S(180);
  mode_button = new FlatButton("", Point2i(inner_c - u - done_w, button_h), FlatButton::SECONDARY,
                               Font::FONT_SMALL);
  map_done = new FlatButton(_("Done"), Point2i(done_w, button_h), FlatButton::PRIMARY);
  map_actions->AddWidget(mode_button);
  map_actions->AddWidget(map_done);
  map_actions->Pack();
  const int map_h = std::min(content_h - 2 * pad - title_h - 3 * u - play_in_loop->GetSizeY()
                             - button_h, Flat::S(320));
  map_box = new MapSelectionBox(Point2i(inner_c, std::max(map_h, 100)), false, !master);
  map_box->SetDarkTheme();
  map_panel->AddWidget(map_box);
  map_panel->AddWidget(play_in_loop);
  map_panel->AddWidget(map_actions);
  map_panel->SetMinHeight(content_h, 1);
  map_panel->SetPosition(center_x, top);
  map_panel->SetVisible(false);
  widgets.AddWidget(map_panel);

  // ---- Right: the map (click it to change), then the chat ----
  const int inner_r = right_w - 2 * pad;
  FlatPanel *map = new FlatPanel(right_w, pad, u);
  map->AddWidget(Title(_("Map"), inner_r));
  map_preview = new PictureWidget(Point2i(inner_r, std::min(inner_r * 9 / 20, content_h / 4)));
  map->AddWidget(map_preview);
  map_card = new FlatButton(" ", Point2i(inner_r, button_h), FlatButton::SECONDARY,
                            Font::FONT_SMALL);
  map->AddWidget(map_card);
  map->Pack();
  map->SetPosition(right_x, top);
  widgets.AddWidget(map);

  const int chat_top = top + map->GetSizeY() + gap;
  const int chat_h = top + content_h - chat_top;
  FlatPanel *chat = new FlatPanel(right_w, pad, u);
  chat->AddWidget(Title(_("Chat"), inner_r));
  msg_box = new TalkBox(Point2i(inner_r, std::max(chat_h - 2 * pad - title_h - u, 2 * button_h)),
                        Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, true);
  chat->AddWidget(msg_box);
  chat->SetMinHeight(chat_h);
  chat->SetPosition(right_x, chat_top);
  widgets.AddWidget(chat);
  widgets.Pack();
  UpdateMapCard();
}

// Phones in landscape: the chat over a row of team chips on the left; the
// players, the map and Ready on the right. Changing a team, or the map,
// happens in the chat's place.
void NetworkMenu::BuildPhoneLayout()
{
  const Surface& window = GetMainWindow();
  const int W = window.GetWidth(), H = window.GetHeight();
  const int u = Flat::Unit();
  const int margin = 3 * u / 2;
  const int gap = 3 * u / 2;
  const int pad = 3 * u / 2;
  const int button_h = Flat::ButtonHeight();
  const bool master = Network::GetInstance()->IsGameMaster();
  const int right_w = std::max(Flat::S(230), (W - 2 * margin) * 26 / 100);
  const int left_w = W - 2 * margin - gap - right_w;
  const int x0 = margin;
  const int right_x = W - margin - right_w;
  const int inner_l = left_w - 2 * pad;
  const int inner_r = right_w - 2 * pad;
  chip_h = button_h + u;

  // ---- Header: Leave, the game's name and where it is ----
  FlatRow *header = new FlatRow(0, u / 2, u, Flat::PANEL);
  leave = new FlatButton(_("Leave"), Point2i(9 * u, button_h), FlatButton::SECONDARY);
  header->AddWidget(leave);
  const int names_w = left_w - u - leave->GetSizeX() - u;
  VBox *names = new VBox(names_w, false, false, false);
  names->SetNoBorder();
  names->SetMargin(0);
  names->AddWidget(new Label(Network::GetInstance()->GetGameName(), names_w,
                             Flat::F(Font::FONT_MEDIUM), Font::FONT_BOLD, Flat::TEXT));
  subtitle = new Label(" ", names_w, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, Flat::TEXT_DIM);
  names->AddWidget(subtitle);
  names->Pack();
  header->AddWidget(names);
  header->Pack();
  header->SetPosition(x0, margin);
  widgets.AddWidget(header);
  const int top = margin + header->GetSizeY() + gap;
  const int bottom = H - margin;

  // ---- The teams, done by the usual box, hidden: the chips show them ----
  team_box = new NetworkTeamsSelectionBox(Point2i(left_w, Flat::S(200)), false, true);
  team_box->SetDarkTheme();
  team_box->SetVisible(false);
  team_box->SetPosition(x0, top);

  // ---- Bottom left: the team chips ----
  chips_panel = new FlatPanel(left_w, u, 0);
  chips_row = new FlatRow(chip_h, 0, u, transparent_color);
  chips_panel->AddWidget(chips_row);
  chips_panel->Pack();
  const int chips_y = bottom - (chip_h + 2 * u);
  chips_panel->SetPosition(x0, chips_y);
  const int mid_h = chips_y - gap - top;

  // ---- Left: the chat ----
  chat_panel = new FlatPanel(left_w, pad, u);
  msg_box = new TalkBox(Point2i(inner_l, mid_h - 2 * pad), Flat::F(Font::FONT_SMALL),
                        Font::FONT_BOLD, true);
  chat_panel->AddWidget(msg_box);
  chat_panel->SetMinHeight(mid_h);
  chat_panel->SetPosition(x0, top);

  // ---- In its place: one of our teams ----
  team_panel = new FlatPanel(left_w, pad, u);
  team_title = Title(" ", inner_l);
  team_panel->AddWidget(team_title);
  FlatRow *pick = new FlatRow(button_h, 0, u, transparent_color);
  team_prev = new FlatButton("<", Point2i(button_h, button_h));
  team_flag = new PictureWidget(Point2i(button_h, button_h));
  team_next = new FlatButton(">", Point2i(button_h, button_h));
  pick->AddWidget(team_prev);
  pick->AddWidget(team_flag);
  pick->AddWidget(team_next);
  pick->AddWidget(new Label(_("Pick another team"), inner_l - 3 * button_h - 3 * u,
                            Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, Flat::TEXT_DIM));
  pick->Pack();
  team_panel->AddWidget(pick);
  team_chars = new FlatStepper(_("Characters"), inner_l, 4, 1, 1, 10);
  team_panel->AddWidget(team_chars);
  team_ai = new FlatToggle(_("The computer plays this team"), inner_l, false);
  team_panel->AddWidget(team_ai);
  FlatRow *team_actions = new FlatRow(button_h, 0, u, transparent_color);
  const int half = (inner_l - u) / 2;
  team_remove = new FlatButton(_("Remove team"), Point2i(half, button_h), FlatButton::DANGER);
  team_done = new FlatButton(_("Done"), Point2i(inner_l - u - half, button_h), FlatButton::PRIMARY);
  team_actions->AddWidget(team_remove);
  team_actions->AddWidget(team_done);
  team_actions->Pack();
  team_panel->AddWidget(team_actions);
  team_panel->SetMinHeight(mid_h, 1);
  team_panel->SetPosition(x0, top);
  team_panel->SetVisible(false);

  // ---- Or the map and game mode ----
  map_panel = new FlatPanel(left_w, pad, u);
  play_in_loop = new FlatToggle(_("Play several times"), inner_l, true);
  FlatRow *map_actions = new FlatRow(button_h, 0, u, transparent_color);
  const int done_w = std::min(half, Flat::S(160));
  mode_button = new FlatButton("", Point2i(inner_l - u - done_w, button_h), FlatButton::SECONDARY,
                               Font::FONT_SMALL);
  map_done = new FlatButton(_("Done"), Point2i(done_w, button_h), FlatButton::PRIMARY);
  map_actions->AddWidget(mode_button);
  map_actions->AddWidget(map_done);
  map_actions->Pack();
  const int map_h = mid_h - 2 * pad - 2 * u - play_in_loop->GetSizeY() - button_h;
  map_box = new MapSelectionBox(Point2i(inner_l, std::max(map_h, 100)), false, !master);
  map_box->SetDarkTheme();
  map_panel->AddWidget(map_box);
  map_panel->AddWidget(play_in_loop);
  map_panel->AddWidget(map_actions);
  map_panel->SetMinHeight(mid_h, 1);
  map_panel->SetPosition(x0, top);
  map_panel->SetVisible(false);

  // ---- Right: Ready / Start at the bottom, the map above, players on top ----
  action_button = new FlatButton(_("Ready"), Point2i(right_w, button_h + u), FlatButton::PRIMARY,
                                 Font::FONT_MEDIUM);
  action_button->SetPosition(right_x, bottom - action_button->GetSizeY());
  map_card = new FlatButton(" ", Point2i(right_w, button_h), FlatButton::DARK, Font::FONT_SMALL);
  map_card->SetPosition(right_x, action_button->GetPositionY() - u - button_h);

  const int players_h = map_card->GetPositionY() - u - top;
  FlatPanel *players = new FlatPanel(right_w, pad, u / 2);
  Label *players_title = Title(_("Players"), inner_r);
  players->AddWidget(players_title);
  players_status = new Label(" ", inner_r, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, Flat::TEXT_DIM);
  players->AddWidget(players_status);
  const int list_h = players_h - 2 * pad - players_title->GetSizeY() - players_status->GetSizeY() - u;
  players_list = new PlayersListBox(Point2i(inner_r, std::max(list_h, button_h)));
  players->AddWidget(players_list);
  players->SetMinHeight(players_h);
  players->SetPosition(right_x, top);

  widgets.AddWidget(chat_panel);
  widgets.AddWidget(team_panel);
  widgets.AddWidget(map_panel);
  widgets.AddWidget(chips_panel);
  widgets.AddWidget(players);
  widgets.AddWidget(map_card);
  widgets.AddWidget(action_button);
  widgets.AddWidget(team_box);
  widgets.Pack();
  UpdateMapCard();
}

// Phones: the chat, or a team or the map in its place
void NetworkMenu::ShowLobbyView(lobby_view_t view)
{
  if (!map_panel)
    return;
  chat_panel->SetVisible(view == LOBBY_CHAT);
  if (team_panel)
    team_panel->SetVisible(view == LOBBY_TEAM);
  map_panel->SetVisible(view == LOBBY_MAP);
  if (view != LOBBY_TEAM)
    edited_slot = -1;
  shown_team.clear();
  if (view == LOBBY_TEAM)
    UpdateTeamPanel();
  RedrawMenu();
}

// The chips: every team, ours outlined in blue, rebuilt when one changed
void NetworkMenu::UpdateChips()
{
  if (!chips_panel)
    return;
  const bool can_add = team_box->CanAddTeam() && !waiting_for_server;
  std::vector<uint> slots;
  std::string key = can_add ? "+" : "-";
  for (uint i = 0; i < team_box->NbSlots(); i++) {
    TeamBox *box = team_box->Slot(i);
    Team *t = box->GetTeam();
    if (!t)
      continue;
    slots.push_back(i);
    key += Format("|%u:%s:%d:%d:%d:%s", i, t->GetId().c_str(), (int)box->IsLocal(),
                  (int)(box->IsLocal() ? box->IsAI() : t->IsAI()),
                  box->IsLocal() ? box->GetNbCharacters() : (int)t->GetNbCharacters(),
                  t->GetPlayerName().c_str());
  }
  if (key == shown_chips)
    return;
  shown_chips = key;

  chips_row->Clear();
  chips.clear();
  const int u = Flat::Unit();
  const int n = slots.size();
  const int avail = chips_panel->GetSizeX() - 2 * u;
  const int plus_w = can_add ? chip_h : 0;
  const int gaps = std::max(0, n - (can_add ? 0 : 1)) * u;
  const int chip_w = n ? std::min(Flat::S(210), (avail - plus_w - gaps) / n) : 0;
  const int icon = chip_h - u;
  const int text_w = std::max(chip_w - u - icon - u / 2, 10);

  for (int k = 0; k < n; k++) {
    TeamBox *box = team_box->Slot(slots[k]);
    Team *t = box->GetTeam();
    const bool mine = box->IsLocal();
    FlatRow *chip = new FlatRow(chip_h, u / 2, u / 2,
                                mine ? Color(110, 182, 255, 80) : Flat::INSET);
    PictureWidget *flag = new PictureWidget(Point2i(icon, icon));
    flag->SetSurface(t->GetFlag(), PictureWidget::FIT_SCALING);
    chip->AddWidget(flag);
    VBox *text = new VBox(text_w, false, false, false);
    text->SetNoBorder();
    text->SetMargin(0);
    text->AddWidget(new Label(t->GetName(), text_w, Flat::F(Font::FONT_MEDIUM), Font::FONT_BOLD,
                              Flat::TEXT));
    bool ai = mine ? box->IsAI() : t->IsAI();
    std::string who = ai ? _("Computer") : (mine ? _("you") : t->GetPlayerName());
    int nb = mine ? box->GetNbCharacters() : (int)t->GetNbCharacters();
    text->AddWidget(new Label(Format("%s  \xC2\xB7  %d", who.c_str(), nb), text_w,
                              Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, Flat::TEXT_DIM));
    text->Pack();
    chip->AddWidget(text);
    chip->Pack();
    chips_row->AddWidget(chip);
    chips.push_back(std::make_pair((Widget*)chip, (int)slots[k]));
  }
  if (can_add) {
    FlatButton *plus = new FlatButton("+", Point2i(chip_h, chip_h), FlatButton::SECONDARY,
                                      Font::FONT_BIG);
    chips_row->AddWidget(plus);
    chips.push_back(std::make_pair((Widget*)plus, -1));
  }
  if (chips.empty())
    chips_row->AddWidget(new Label(_("No team yet"), avail, Flat::F(Font::FONT_SMALL),
                                   Font::FONT_BOLD, Flat::TEXT_DIM));
  chips_row->Pack();
  chips_panel->Pack();
  chips_panel->NeedRedrawing();
}

// The team pop-up, from the team it is about
void NetworkMenu::UpdateTeamPanel()
{
  if (!team_panel || edited_slot < 0 || !team_panel->IsVisible())
    return;
  TeamBox *box = team_box->Slot(edited_slot);
  Team *t = box->GetTeam();
  if (!t || !box->IsLocal()) {  // gone meanwhile
    ShowLobbyView(LOBBY_CHAT);
    return;
  }
  // Our last team is the one that can be removed
  const std::list<ConfigTeam> &mine = Network::GetInstance()->GetPlayer().GetTeams();
  const bool last = !mine.empty() && mine.back().id == t->GetId();
  std::string key = Format("%s:%d:%d:%d", t->GetId().c_str(), box->GetNbCharacters(),
                           (int)box->IsAI(), (int)last);
  if (key == shown_team)
    return;
  shown_team = key;

  team_title->SetText(Format(_("%s Team"), t->GetName().c_str()));
  team_flag->SetSurface(t->GetFlag(), PictureWidget::FIT_SCALING);
  team_chars->SetMinValue(box->GetMinCharacters());
  team_chars->SetMaxValue(box->GetMaxCharacters());
  team_chars->SetValue(box->GetNbCharacters());
  team_ai->SetValue(box->IsAI());
  team_remove->SetEnabled(last);
  team_panel->NeedRedrawing();
}

void NetworkMenu::UpdateMapCard()
{
  if (!map_card)
    return;
  std::string text = Format(_("Map: %s"), map_box->GetMapName().c_str());
  if (text != shown_map) {
    shown_map = text;
    map_card->SetText(text);
    if (map_preview) {
      map_preview->SetSurface(map_box->GetPreview(), PictureWidget::FIT_SCALING);
      map_preview->NeedRedrawing();
    }
  }
}

// Phones: clicks on the chips, the team pop-up and the map card
bool NetworkMenu::PhoneClick(Widget *w, const Point2i &mousePosition)
{
  for (size_t i = 0; i < chips.size(); i++) {
    if (!PressedOn(chips[i].first, mousePosition))
      continue;
    int slot = chips[i].second;
    if (slot < 0) {
      team_box->RequestTeam();
    } else if (team_box->Slot(slot)->IsLocal()) {
      edited_slot = slot;
      ShowLobbyView(LOBBY_TEAM);
    }
    return true;
  }
  if (!w)
    return false;

  TeamBox *box = edited_slot >= 0 ? team_box->Slot(edited_slot) : NULL;
  if (w == map_card) {
    ShowLobbyView(LOBBY_MAP);
  } else if (w == map_done || w == team_done) {
    ShowLobbyView(LOBBY_CHAT);
  } else if (box && w == team_prev) {
    team_box->PrevTeam(edited_slot);
  } else if (box && w == team_next) {
    team_box->NextTeam(edited_slot);
  } else if (box && w == team_chars) {
    box->SetNbCharacters(team_chars->GetValue());
  } else if (box && w == team_ai) {
    if (team_ai->GetValue() != box->IsAI())
      box->SwitchPlayerType();
  } else if (box && w == team_remove) {
    team_box->RequestTeamRemoval();
    ShowLobbyView(LOBBY_CHAT);
  } else {
    return false;
  }
  UpdateTeamPanel();
  return true;
}

NetworkMenu::NetworkMenu()
  : Menu("menu/bg_network", vNo)
  , waiting_for_server(false)
  , last_ready(-1)
  , game_mode(0)
  , compact(GetMainWindow().GetHeight() < 640)
  , chat_panel(NULL)
  , team_panel(NULL)
  , map_panel(NULL)
  , chips_panel(NULL)
  , chips_row(NULL)
  , map_card(NULL)
  , map_done(NULL)
  , team_done(NULL)
  , team_remove(NULL)
  , team_prev(NULL)
  , team_next(NULL)
  , team_title(NULL)
  , team_flag(NULL)
  , team_chars(NULL)
  , team_ai(NULL)
  , map_preview(NULL)
  , edited_slot(-1)
{
  const bool master = Network::GetInstance()->IsGameMaster();
  game_modes = GameMode::ListGameModes();
  const std::string &current = Config::GetInstance()->GetGameMode();
  for (size_t i = 0; i < game_modes.size(); i++)
    if (game_modes[i].first == current)
      game_mode = i;

  if (compact)
    BuildPhoneLayout();
  else
    BuildDesktopLayout();
  ShowGameMode();

  if (!Network::GetInstance()->IsServer()) {
    // First %s will be replaced with the name of the network game,
    // second %s by the server hostname
    msg_box->NewMessage(Format(_("Welcome to %s on %s!"),
                               Network::GetInstance()->GetGameName().c_str(),
                               ((NetworkClient*)Network::GetInstance())->GetServerAddress().c_str())
                        , Flat::WARN);
  }

  if (!master) {
    msg_box->NewMessage(_("Pick your team(s), then press Ready."), Flat::ACCENT);
  } else if (!Network::GetInstance()->IsServer()) {
    // The first player to connect to a headless server assumes the game master role
    SetGameMasterCallback();
  }
  UpdateSubtitle();
  UpdateActionButton();
  widgets.Pack();
}

void NetworkMenu::ShowGameMode()
{
  bool master = Network::GetInstance()->IsGameMaster();
  mode_button->SetEnabled(master && !game_modes.empty());
  if (!master)
    mode_button->SetText(_("Game mode: chosen by the host"));
  else if (!game_modes.empty())
    mode_button->SetText(Format(_("Game mode: %s"), game_modes[game_mode].second.c_str()));
}

void NetworkMenu::UpdateSubtitle()
{
  Network *net = Network::GetInstance();
  std::string text;
  if (net->IsServer())
    text = _("You are hosting this game");
  else if (net->IsGameMaster())
    text = Format(_("Connected to %s  \xC2\xB7  you lead the game"),
                  ((NetworkClient*)net)->GetServerAddress().c_str());
  else
    text = Format(_("Connected to %s"), ((NetworkClient*)net)->GetServerAddress().c_str());
  subtitle->SetText(text);
}

void NetworkMenu::UpdateActionButton()
{
  if (Network::GetInstance()->IsGameMaster()) {
    action_button->SetText(_("Start game"));
    action_button->SetEnabled(true);
  } else if (waiting_for_server) {
    action_button->SetText(_("Waiting for the host..."));
    action_button->SetEnabled(false);
  } else {
    action_button->SetText(_("Ready"));
    action_button->SetEnabled(true);
  }
}

static std::string TeamNames(const std::list<ConfigTeam> &teams)
{
  std::string names;
  for (std::list<ConfigTeam>::const_iterator t = teams.begin(); t != teams.end(); ++t) {
    int pos;
    Team *team = GetTeamsList().FindById(t->id, pos);
    if (!names.empty())
      names += ", ";
    names += team ? team->GetName() : t->id;
  }
  return names.empty() ? _("No team yet") : names;
}

// Rebuild the players list when something changed
void NetworkMenu::UpdatePlayers()
{
  Network *net = Network::GetInstance();
  const bool master = net->IsGameMaster();

  struct Row { std::string name, teams, status; Color color; };
  std::vector<Row> rows;

  Row me;
  me.name = net->GetPlayer().GetNickname() + " (" + _("you") + ")";
  me.teams = TeamNames(net->GetPlayer().GetTeams());
  if (master) {
    me.status = _("Host");
    me.color = Flat::ACCENT;
  } else if (net->GetState() == WNet::NETWORK_MENU_OK) {
    me.status = _("Ready");
    me.color = Flat::GOOD;
  } else {
    me.status = _("Not ready");
    me.color = Flat::TEXT_DIM;
  }
  rows.push_back(me);

  int ready = 0, others = 0;
  const std::list<DistantComputer*>& hosts = net->LockRemoteHosts();
  for (std::list<DistantComputer*>::const_iterator h = hosts.begin(); h != hosts.end(); ++h) {
    const std::list<Player>& players = (*h)->GetPlayers();
    for (std::list<Player>::const_iterator p = players.begin(); p != players.end(); ++p) {
      Row r;
      r.name = p->GetNickname().empty() ? _("Player") : p->GetNickname();
      r.teams = TeamNames(p->GetTeams());
      others++;
      if (master) {
        // Only the game master is told who is ready
        bool is_ready = p->GetState() == Player::STATE_INITIALIZED
                     || p->GetState() == Player::STATE_READY;
        if (is_ready)
          ready++;
        r.status = is_ready ? _("Ready") : _("Not ready");
        r.color = is_ready ? Flat::GOOD : Flat::TEXT_DIM;
      } else {
        r.status = " "; // unknown here (and empty labels cannot be drawn)
        r.color = Flat::TEXT_DIM;
      }
      rows.push_back(r);
    }
  }
  net->UnlockRemoteHosts();

  // Status line
  std::string status;
  if (others == 0)
    status = _("Waiting for players to join...");
  else if (master)
    status = Format(ngettext("%i of %i player ready", "%i of %i players ready", others),
                    ready, others);
  else
    status = Format(ngettext("%i player", "%i players", others + 1), others + 1);
  if (players_status->GetText() != status)
    players_status->SetText(status);

  // Chat notices when players get ready (host)
  if (master && ready != last_ready) {
    if (last_ready >= 0 && others > 0 && ready == others) {
      msg_box->NewMessage(_("Everybody is ready: you can start the game!"), Flat::GOOD);
      JukeBox::GetInstance()->Play("default", "menu/newcomer");
    }
    last_ready = ready;
  }

  std::string key;
  for (size_t i = 0; i < rows.size(); i++)
    key += rows[i].name + "|" + rows[i].teams + "|" + rows[i].status + "\n";
  if (key == shown_players)
    return;
  shown_players = key;

  players_list->Clear();
  const int u = Flat::Unit();
  const int row_w = ((PlayersListBox*)players_list)->RowWidth();
  const int pad = u;
  const int status_w = std::max(10 * u, 70);
  // Narrow lists: the status on a line of its own, under the name
  const bool stacked = row_w - 2 * pad - status_w - u < Flat::S(170);
  const int text_w = stacked ? row_w - 2 * pad : row_w - 2 * pad - status_w - u;
  for (size_t i = 0; i < rows.size(); i++) {
    FlatRow *row = new FlatRow(0, pad, u);
    VBox *text = new VBox(text_w, false, false, false);
    text->SetNoBorder();
    text->SetMargin(0);
    text->AddWidget(new Label(rows[i].name, text_w, Flat::F(Font::FONT_MEDIUM), Font::FONT_BOLD, Flat::TEXT));
    if (stacked && rows[i].status != " ")
      text->AddWidget(new Label(rows[i].status, text_w, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD,
                                rows[i].color));
    text->AddWidget(new Label(rows[i].teams, text_w, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, Flat::TEXT_DIM));
    text->Pack(); // boxes measure their children before packing them
    row->AddWidget(text);
    if (!stacked)
      row->AddWidget(new Label(rows[i].status, status_w, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD,
                               rows[i].color, Text::ALIGN_RIGHT_TOP));
    row->Pack();
    players_list->AddWidget(row);
  }
  players_list->Pack();
  players_list->NeedRedrawing();
}

void NetworkMenu::OnClickUp(const Point2i &mousePosition, int button)
{
  Widget *w = widgets.ClickUp(mousePosition, button);
  if (map_panel && PhoneClick(w, mousePosition))
    return;
  if (!w)
    return;
  if (w == leave) {
    mouse_cancel();
  } else if (w == action_button) {
    mouse_ok();
  } else if (w == mode_button && !game_modes.empty()) {
    game_mode = (game_mode + 1) % game_modes.size();
    ShowGameMode();
  }
}

void NetworkMenu::signal_begin_run()
{
  ActionHandler::GetInstance()->ExecFrameLessActions();
  RequestSavedTeams();
}

void NetworkMenu::RequestSavedTeams()
{
  const std::list<ConfigTeam> & team_list = Config::GetInstance()->AccessNetworkTeamsList();
  std::list<ConfigTeam>::const_iterator it;

  if (team_list.size() > 0) {
    const std::string &nick = Config::GetInstance()->GetNetworkNickname();
    for (it = team_list.begin(); it != team_list.end(); it++) {
      ConfigTeam team = *it;
      if (!nick.empty())  // the name typed in the Network Game menu
        team.player_name = nick;
      ActionHandler::GetInstance()->NewRequestTeamAction(team);
    }
  } else {
    team_box->RequestTeam();
  }
}

void NetworkMenu::SaveOptions()
{
  // map
  map_box->ValidMapSelection();

  // team
  team_box->ValidTeamsSelection();

  //Save options in XML
//  Config::GetInstance()->Save();

  if (Network::GetInstance()->IsGameMaster() && !game_modes.empty()) {
    Config::GetInstance()->SetGameMode(game_modes[game_mode].first);
  }
}

void NetworkMenu::PrepareForNewGame()
{
  msg_box->Clear();
  UpdateActionButton();

  Network::GetInstance()->SetState(WNet::NETWORK_NEXT_GAME);
  Network::GetInstance()->SendNetworkState();

  // to choose another random map
  if (Network::GetInstance()->IsGameMaster())
    map_box->ChangeMapDelta(0);

  RedrawMenu();
}

bool NetworkMenu::signal_ok()
{
  if (!Network::GetInstance()->IsGameMaster()) {
    // Check the user have selected a team:
    bool found = false;

    for (std::vector<Team*>::iterator team = GetTeamsList().playing_list.begin();
         team != GetTeamsList().playing_list.end();
         team++) {
      if ((*team)->IsLocalHuman()
          || ((*team)->IsLocal() && SDL_getenv("WARMUX_TEST_AUTOPLAY"))) {
        found = true;
        break;
      }
    }

    if (!found) {
      msg_box->NewMessage(_("You won't be able to play before selecting a team!"));
      goto error;
    }

    // Wait for the game master, and stay in the menu map / team can still be changed
    WaitingForGameMaster();

    if (Network::GetInstance()->IsGameMaster()) {
      goto error;
    }

  } else {
    if (GetTeamsList().playing_list.size() <= 1) {
      msg_box->NewMessage(Format(ngettext("There is only %i team.",
                                          "There are only %i teams.",
                                          GetTeamsList().playing_list.size()),
                                 GetTeamsList().playing_list.size()), Flat::WARN);
      goto error;
    }
    if (Network::GetInstance()->GetNbPlayersConnected() == 0) {
      msg_box->NewMessage(_("You are alone. :-/"), Flat::WARN);
      goto error;
    }
    if (Network::GetInstance()->GetNbPlayersConnected() != Network::GetInstance()->GetNbPlayersWithState(Player::STATE_INITIALIZED)) {
      int nbr = Network::GetInstance()->GetNbPlayersConnected() - Network::GetInstance()->GetNbPlayersWithState(Player::STATE_INITIALIZED);
      std::string pl = Format(ngettext("Wait! %i player is not ready yet!", "Wait! %i players are not ready yet!", nbr), nbr);
      msg_box->NewMessage(pl, Flat::WARN);
      goto error;
    }
  }

  // Disconnection :-/
  if (!Network::IsConnected()) {
    Network::GetInstance()->network_menu = NULL;
    return true;

  } else {

    // Starting the game :-)
    SaveOptions();
    play_ok_sound();

    if (Network::GetInstance()->IsServer())
      IndexServer::GetInstance()->Disconnect();

    if (Network::GetInstance()->IsGameMaster())
      GameMode::GetInstance()->Load();

    Game::GetInstance()->Start();

    if (Network::GetInstance()->IsConnected()
        && Network::GetInstance()->GetNbPlayersConnected() != 0
        && play_in_loop->GetValue()) {
      PrepareForNewGame();
      return false;
    }

    Network::GetInstance()->network_menu = NULL;
  }

  Network::Disconnect();
  return true;

 error:
  play_error_sound();
  return false;
}

void NetworkMenu::key_ok()
{
  // return was pressed while chat texbox still had focus (player wants to send his msg)
  if (msg_box->TextHasFocus()) {
    msg_box->SendChatMsg();
    return;
  }

  IndexServer::GetInstance()->Disconnect();

  Menu::key_ok();
}

bool NetworkMenu::signal_cancel()
{
  // Phones: Back closes a team or the map first
  if (map_panel && !chat_panel->IsVisible()) {
    ShowLobbyView(LOBBY_CHAT);
    return false;
  }
  Network::Disconnect();
  return true;
}

void NetworkMenu::Draw(const Point2i& /*mousePosition*/)
{
  if (Network::GetInstance()->IsConnected())
    UpdatePlayers();
  else
    close_menu = true;
  if (!close_menu) {
    UpdateChips();
    UpdateTeamPanel();
    UpdateMapCard();
  }
  ActionHandler * action_handler = ActionHandler::GetInstance();
  action_handler->ExecFrameLessActions();

  std::vector<std::pair<std::string, Color> > early = AppWarmux::GetInstance()->TakeEarlyMessages();
  for (size_t i = 0; i < early.size(); i++)
    ReceiveMsgCallback(early[i].first, early[i].second);

  // Developer aid: WARMUX_TEST_AUTOPLAY=1 hands our teams to the computer
  // and keeps pressing Ready / Start, so network games can be tested
  // without anyone at the keyboard. WARMUX_TEST_AUTOPLAY=lobby only waits
  // in the lobby (to test joining and leaving it).
  static Uint32 autoplay_next = 0;
  static bool autoplay_ai = false;
  const char *autoplay = SDL_getenv("WARMUX_TEST_AUTOPLAY");
  if (autoplay && SDL_GetTicks() > autoplay_next
      && Network::GetInstance()->IsConnected()) {
    autoplay_next = SDL_GetTicks() + 3000;
    if (!autoplay_ai) {
      team_box->SetLocalTeamsAI();
      autoplay_ai = true;
    } else if (std::string(autoplay) != "lobby") {
      mouse_ok();
    }
  }
}

void NetworkMenu::DelTeamCallback(const std::string& team_id)
{
  if (close_menu)
    return;

  // Called from the action handler
  team_box->DelTeamCallback(team_id);
}

void NetworkMenu::AddTeamCallback(const std::string& team_id)
{
  if (close_menu)
    return;

  team_box->AddTeamCallback(team_id);
}

void NetworkMenu::UpdateTeamCallback(const std::string& old_team_id, const std::string& team_id)
{
  if (close_menu)
    return;

  team_box->UpdateTeamCallback(old_team_id, team_id);
}

void NetworkMenu::ChangeMapCallback()
{
  if (close_menu)
    return;

  map_box->ChangeMapCallback();
}

void NetworkMenu::SetGameMasterCallback()
{
  // We are becoming game master, updating the menu...
  AppWarmux::GetInstance()->video->SetWindowCaption(std::string("WarMUX ") +
                                                    Constants::WARMUX_VERSION + " - " +
                                                    _("Master mode"));
  map_box->AllowSelection();
  waiting_for_server = false;
  ShowGameMode();
  UpdateSubtitle();
  UpdateActionButton();
  shown_players.clear();

  widgets.Pack();
  RedrawMenu();

  msg_box->NewMessage(_("You are the new turn master! Learn about your new powers by typing /help"), Flat::WARN);
  msg_box->NewMessage(_("Wait until some opponent(s) connect!"), Flat::WARN);
}

void NetworkMenu::ReceiveMsgCallback(const std::string& msg, const Color& color)
{
  // Notices come in dark red, made for the old light background
  msg_box->NewMessage(msg, color == c_red ? Flat::WARN : color);
}

void NetworkMenu::SetMapsCallback(const std::vector<uint>& list)
{
  map_box->ChangeMapListCallback(list);
}

Team * NetworkMenu::FindUnusedTeam(const std::string default_team_id)
{
  return team_box->FindUnusedTeam(default_team_id);
}

bool NetworkMenu::HasOpenTeamSlot()
{
  return team_box->HasOpenTeamSlot();
}

// to be call from NetworkMenu::WaitingForGameMaster()
void NetworkMenu::HandleEvent(const SDL_Event& evnt)
{
  if (!waiting_for_server) {
    Menu::HandleEvent(evnt);
    return;
  }

  if (evnt.type == SDL_QUIT) {
    Menu::mouse_cancel();
  } else if (evnt.type == SDL_TEXTINPUT) {
    widgets.SendText(evnt.text.text);
  } else if (evnt.type == SDL_KEYDOWN) {
    switch (evnt.key.keysym.sym) {
    case SDLK_ESCAPE:
      Menu::mouse_cancel();
      break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      msg_box->SendChatMsg();
      break;
    case SDLK_F10:
      AppWarmux::GetInstance()->video->ToggleFullscreen();
      break;
    default:
      widgets.SendKey(evnt.key.keysym);
      break;
    }
  } else if (evnt.type == SDL_MOUSEBUTTONDOWN) {
    press_position = Point2i(evnt.button.x, evnt.button.y);
    pressed = true;
  } else if (evnt.type == SDL_MOUSEBUTTONUP) {

    int x=0, y=0;
    GetGameMouseState(&x, &y);
    Point2i mousePosition(x, y);

    if (PressedOn(leave, mousePosition))
      Menu::mouse_cancel();
    pressed = false;
  }
}

void NetworkMenu::WaitingForGameMaster()
{
  Network::GetInstance()->SetState(WNet::NETWORK_MENU_OK);

  // warn the server that we have validated the menu
  Network::GetInstance()->SendNetworkState();

  waiting_for_server = true;
  UpdateActionButton();
  shown_players.clear();

  msg_box->NewMessage(_("Waiting for the server. All you can do is cancel or chat!"), Flat::WARN);

  widgets.SetFocusOn(msg_box->GetTextBox());

  do {
    HandleEvents();

    int x=0, y=0;
    GetGameMouseState(&x, &y);
    Point2i mousePosition(x, y);

    Display(mousePosition);
    Menu::Display(mousePosition);

  } while (Network::GetInstance()->IsConnected() &&
           Network::GetInstance()->GetState() == WNet::NETWORK_MENU_OK);

  waiting_for_server = false;
  UpdateActionButton();
}
