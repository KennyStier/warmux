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

NetworkMenu::NetworkMenu()
  : Menu("menu/bg_network", vNo)
  , waiting_for_server(false)
  , last_ready(-1)
  , game_mode(0)
{
  const Surface& window = GetMainWindow();
  const int u = Flat::Unit();
  const int margin = 2 * u;
  const int pad = 2 * u;
  const int button_h = Flat::ButtonHeight();
  // The lobby is laid out in a block no bigger than this, centred
  const int W = std::min(window.GetWidth(), Flat::S(1400) + 2 * margin);
  const int H = std::min(window.GetHeight(), Flat::S(600) + 2 * margin);
  const int ox = (window.GetWidth() - W) / 2, oy = (window.GetHeight() - H) / 2;
  const bool master = Network::GetInstance()->IsGameMaster();

  // ---- Header: game name, who we are, Leave ----
  Label *title = new Label(Network::GetInstance()->GetGameName(), W * 2 / 3,
                           Flat::F(Font::FONT_BIG), Font::FONT_BOLD, Flat::TEXT,
                           Text::ALIGN_LEFT_TOP, true);
  title->SetPosition(ox + margin, oy + margin);
  widgets.AddWidget(title);
  subtitle = new Label(" ", W * 2 / 3, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, Flat::TEXT,
                       Text::ALIGN_LEFT_TOP, true);
  subtitle->SetPosition(ox + margin, oy + margin + title->GetSizeY());
  widgets.AddWidget(subtitle);

  leave = new FlatButton(_("Leave"), Point2i(12 * u, button_h), FlatButton::DARK);
  leave->SetPosition(ox + W - margin - leave->GetSizeX(), oy + margin);
  widgets.AddWidget(leave);

  const int top = margin + std::max(title->GetSizeY() + subtitle->GetSizeY(), button_h) + u;
  const int content_h = H - top - margin;
  const int right_w = std::min(Flat::S(380), std::max(W * 34 / 100, std::min(W / 2, Flat::S(300))));
  const int left_w = W - 2 * margin - right_w - 2 * u;
  const int right_x = ox + W - margin - right_w;
  const int inner_l = left_w - 2 * pad;
  const int inner_r = right_w - 2 * pad;
  const int title_h = Title("X", 10)->GetSizeY(); // measured once, leaked label is tiny

  // ---- Left: teams, then map ----
  // Room for the "- n +" line and a row of team cards
  const int card_rows = 1; // 3-4 cards per row; more scroll
  int teams_h = 3 * u + u + card_rows * (TeamBox::CompactHeight() + u);
  FlatPanel *teams = new FlatPanel(left_w, pad, u);
  teams->AddWidget(Title(_("Teams"), inner_l));
  teams->AddWidget(new Label(_("Click a team's picture to pick another team, the icon under it to "
                               "switch between human and computer."), inner_l,
                             Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, Flat::TEXT_DIM));
  team_box = new NetworkTeamsSelectionBox(Point2i(inner_l, teams_h), false, true);
  team_box->SetDarkTheme();
  teams->AddWidget(team_box);
  teams->SetPosition(ox + margin, oy + top);
  widgets.AddWidget(teams);
  widgets.Pack();

  int map_top = teams->GetPositionY() + teams->GetSizeY() + 2 * u;
  int map_h = oy + H - margin - map_top - 2 * pad - title_h - u;
  FlatPanel *map = new FlatPanel(left_w, pad, u);
  map->AddWidget(Title(_("Map"), inner_l));
  map_box = new MapSelectionBox(Point2i(inner_l, std::max(map_h, 100)), false, !master);
  map_box->SetDarkTheme();
  map->AddWidget(map_box);
  map->SetPosition(ox + margin, map_top);
  widgets.AddWidget(map);

  // ---- Right, bottom: game mode, play again, start ----
  FlatPanel *actions = new FlatPanel(right_w, pad, u);
  game_modes = GameMode::ListGameModes();
  const std::string &current = Config::GetInstance()->GetGameMode();
  for (size_t i = 0; i < game_modes.size(); i++)
    if (game_modes[i].first == current)
      game_mode = i;
  mode_button = new FlatButton("", Point2i(inner_r, button_h), FlatButton::SECONDARY,
                               Font::FONT_SMALL);
  actions->AddWidget(mode_button);
  play_in_loop = new FlatToggle(_("Play several times"), inner_r, true);
  actions->AddWidget(play_in_loop);
  action_button = new FlatButton(_("Ready"), Point2i(inner_r, button_h + u), FlatButton::PRIMARY,
                                 Font::FONT_MEDIUM);
  actions->AddWidget(action_button);
  actions->Pack();
  actions->SetPosition(right_x, oy + H - margin - actions->GetSizeY());
  widgets.AddWidget(actions);
  ShowGameMode();

  // ---- Right, top: players, chat ----
  // Short screens (phones): one row, the rest scrolls, so the chat has room
  const int players_h = window.GetHeight() < 640 ? button_h + 2 * u
                      : std::max(2 * button_h, std::min(4 * button_h, content_h * 20 / 100));
  FlatPanel *players = new FlatPanel(right_w, pad, u);
  players->AddWidget(Title(_("Players"), inner_r));
  players_status = new Label(" ", inner_r, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, Flat::TEXT_DIM);
  players->AddWidget(players_status);
  players_list = new PlayersListBox(Point2i(inner_r, players_h));
  players->AddWidget(players_list);
  players->SetPosition(right_x, oy + top);
  widgets.AddWidget(players);
  widgets.Pack();

  int chat_top = players->GetPositionY() + players->GetSizeY() + 2 * u;
  int chat_h = actions->GetPositionY() - 2 * u - chat_top - 2 * pad - title_h - u;
  FlatPanel *chat = new FlatPanel(right_w, pad, u);
  chat->AddWidget(Title(_("Chat"), inner_r));
  msg_box = new TalkBox(Point2i(inner_r, std::max(chat_h, 2 * button_h)),
                        Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, true);
  chat->AddWidget(msg_box);
  chat->SetPosition(right_x, chat_top);
  widgets.AddWidget(chat);

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
  const int text_w = row_w - 2 * pad - status_w - u;
  for (size_t i = 0; i < rows.size(); i++) {
    FlatRow *row = new FlatRow(0, pad, u);
    VBox *text = new VBox(text_w, false, false, false);
    text->SetNoBorder();
    text->SetMargin(0);
    text->AddWidget(new Label(rows[i].name, text_w, Flat::F(Font::FONT_MEDIUM), Font::FONT_BOLD, Flat::TEXT));
    text->AddWidget(new Label(rows[i].teams, text_w, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, Flat::TEXT_DIM));
    text->Pack(); // boxes measure their children before packing them
    row->AddWidget(text);
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
    for (it = team_list.begin(); it != team_list.end(); it++) {
      ActionHandler::GetInstance()->NewRequestTeamAction(*it);
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
  Network::Disconnect();
  return true;
}

void NetworkMenu::Draw(const Point2i& /*mousePosition*/)
{
  if (Network::GetInstance()->IsConnected())
    UpdatePlayers();
  else
    close_menu = true;
  ActionHandler * action_handler = ActionHandler::GetInstance();
  action_handler->ExecFrameLessActions();

  std::vector<std::pair<std::string, Color> > early = AppWarmux::GetInstance()->TakeEarlyMessages();
  for (size_t i = 0; i < early.size(); i++)
    ReceiveMsgCallback(early[i].first, early[i].second);

  // Developer aid: WARMUX_TEST_AUTOPLAY=1 hands our teams to the computer
  // and keeps pressing Ready / Start, so network games can be tested
  // without anyone at the keyboard
  static Uint32 autoplay_next = 0;
  static bool autoplay_ai = false;
  if (SDL_getenv("WARMUX_TEST_AUTOPLAY") && SDL_GetTicks() > autoplay_next
      && Network::GetInstance()->IsConnected()) {
    autoplay_next = SDL_GetTicks() + 3000;
    if (!autoplay_ai) {
      team_box->SetLocalTeamsAI();
      autoplay_ai = true;
    } else {
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
  } else if (evnt.type == SDL_MOUSEBUTTONUP) {

    int x=0, y=0;
    GetGameMouseState(&x, &y);
    Point2i mousePosition(x, y);

    if (leave->Contains(mousePosition))
      Menu::mouse_cancel();
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
