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
 * The Network Game menu (see network_connection_menu.h)
 *****************************************************************************/

#include <algorithm>
#include <SDL_thread.h>
#include "game/config.h"
#include "graphic/video.h"
#include "gui/flat_widgets.h"
#include "gui/question.h"
#include "gui/label.h"
#include "gui/null_widget.h"
#include "gui/picture_widget.h"
#include "gui/scroll_box.h"
#include "gui/text_box.h"
#include "include/app.h"
#include "include/constant.h"
#include "network/lan_discovery.h"
#include "network/net_error_msg.h"
#include "menu/network_connection_menu.h"
#include "menu/network_menu.h"
#include "team/teams_list.h"
#include "tool/string_tools.h"

// How long to wait between two searches for games
#define SEARCH_INTERVAL_MS 2500

// --- Searching for games, in a thread ----------------------------------------

struct shared_net_info {
  SDL_Thread* thread_refresh;
  SDL_sem* lock;
  std::list<GameServerInfo> lst_games;   // from the index server
  std::list<GameServerInfo> lan_games;   // found on the local network
  connection_state_t index_conn_state;
  bool searching_index;  // LAN results are in, the index server's are not
  bool todisplay;
  bool finished;
} net_info;

static void InitNetInfo()
{
  net_info.thread_refresh = NULL;
  net_info.index_conn_state = CONNECTED;
  net_info.searching_index = false;
  net_info.lock = SDL_CreateSemaphore(1);
  net_info.todisplay = false;
  net_info.finished = true;
}

// Wait for a running search to end
static void WaitForSearch()
{
  int status;
  if (net_info.thread_refresh)
    SDL_WaitThread(net_info.thread_refresh, &status);
  net_info.thread_refresh = NULL;
  net_info.finished = true;
}

static void CleanNetInfo()
{
  // The search takes the lock when it is done: it must not be held (or
  // destroyed) while waiting for it
  WaitForSearch();
  SDL_DestroySemaphore(net_info.lock);
  net_info.lst_games.clear();
  net_info.lan_games.clear();
}

static int RefreshNetInfo(void *)
{
  MSG_DEBUG("network.refresh_games_list", "Begin");

  // Games on the local network first: quick, and needs no index server
  std::list<GameServerInfo> lan = LanDiscovery::Search();
  SDL_SemWait(net_info.lock);
  net_info.lan_games = lan;
  net_info.searching_index = true;
  net_info.todisplay = true;
  SDL_SemPost(net_info.lock);
  Menu::WakeUpOnCallback();

  // Then the public games from the index server
  std::list<GameServerInfo> lst;
  connection_state_t conn = IndexServer::GetInstance()->Connect(Constants::WARMUX_VERSION);
  if (conn == CONNECTED) {
    lst = IndexServer::GetInstance()->GetHostList(false);
    IndexServer::GetInstance()->Disconnect();
    // A LAN game that is also public shows up once, as a LAN game
    for (std::list<GameServerInfo>::iterator it = lst.begin(); it != lst.end(); ) {
      bool dup = false;
      for (std::list<GameServerInfo>::const_iterator l = lan.begin(); l != lan.end(); ++l)
        if (l->game_name == it->game_name && l->port == it->port)
          dup = true;
      it = dup ? lst.erase(it) : ++it;
    }
  }

  SDL_SemWait(net_info.lock);
  net_info.lan_games = lan;
  net_info.lst_games = lst;
  net_info.index_conn_state = conn;
  net_info.searching_index = false;
  net_info.todisplay = true;
  net_info.finished = true;
  SDL_SemPost(net_info.lock);

  Menu::WakeUpOnCallback();
  MSG_DEBUG("network.refresh_games_list", "End");
  return 0;
}

// Wakes the menu up regularly, so the list refreshes without any input
static Uint32 SDLCALL WakeUp(Uint32 interval, void *)
{
  Menu::WakeUpOnCallback();
  return interval;
}

// --- The list of games ----------------------------------------------------

class GameListBox : public ScrollBox
{
public:
  GameListBox(const Point2i &size)
    : ScrollBox(size, false, false)
  {
    Widget::SetBorder(transparent_color, 0);
    vbox->SetMargin(Flat::Unit());
  }
  // Rows fill the list, leaving room for the scroll bar
  int RowWidth() const { return size.x - scrollbar_width; }
};

static std::string DefaultPort()
{
  return int2str(WARMUX_NETWORK_PORT_INT);
}

// "host" or "host:port" (IPv6 addresses, with several colons, have no port)
static void SplitAddress(const std::string &text, std::string &host, std::string &port)
{
  std::string s = text;
  s.erase(0, s.find_first_not_of(" \t"));
  s.erase(s.find_last_not_of(" \t") + 1);
  host = s;
  port = DefaultPort();
  std::string::size_type colon = s.rfind(':');
  if (colon != std::string::npos && s.find(':') == colon) {
    host = s.substr(0, colon);
    if (colon + 1 < s.size())
      port = s.substr(colon + 1);
  }
}

// --- The menu ---------------------------------------------------------------

static Label *Caption(const std::string &text, uint width)
{
  return new Label(text, width, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, Flat::TEXT_DIM);
}

static Label *Title(const std::string &text, uint width)
{
  return new Label(text, width, Flat::F(Font::FONT_MEDIUM), Font::FONT_BOLD, Flat::TEXT);
}

NetworkConnectionMenu::NetworkConnectionMenu(network_menu_action_t _action)
  : Menu("menu/bg_network", vNo)
  , action(_action)
  , refresh_timer(0)
  , last_search_end(0)
{
  const Surface &window = GetMainWindow();
  const int W = window.GetWidth(), H = window.GetHeight();
  const int u = Flat::Unit();
  const int margin = 2 * u;
  // Less padding in short windows, so Host and Join fit stacked
  const int pad = H < 700 ? 3 * u / 2 : 2 * u;
  const int button_h = Flat::ButtonHeight();

  // Everything sits in one block, centred, no bigger than it needs to be.
  // Host and Join are stacked on the right, or side by side when the window
  // is too short for that (phones in landscape)
  const bool three_columns = H < 640 && W >= 900;
  const int block_w = std::min(W - 2 * margin, Flat::S(three_columns ? 1240 : 1040));
  const int right_w = three_columns ? (block_w - 4 * u) / 3
                                    : std::min(Flat::S(380), block_w * 45 / 100);
  const int left_w = three_columns ? right_w : block_w - right_w - 2 * u;
  const int inner_r = right_w - 2 * pad;
  const int inner_l = left_w - 2 * pad;

  Label *title = new Label(_("Network game"), block_w / 2, Flat::F(Font::FONT_BIG), Font::FONT_BOLD,
                           Flat::TEXT, Text::ALIGN_LEFT_TOP, true);
  back = new FlatButton(_("Back"), Point2i(12 * u, button_h), FlatButton::DARK);
  const int header_h = std::max(title->GetSizeY(), button_h);

  // ---- Host a game ----
  FlatPanel *host = new FlatPanel(right_w, pad, u);
  host->AddWidget(Title(_("Host a game"), inner_r));
  host->AddWidget(Caption(_("Game name"), inner_r));
  host_name = new TextBox(Config::GetInstance()->GetNetworkServerGameName(), 0,
                          Flat::F(Font::FONT_MEDIUM));
  host_name->SetMaxNbChars(20);
  host->AddWidget(new FlatField(host_name, inner_r));
  host->AddWidget(Caption(_("Password (optional)"), inner_r));
  host_pwd = new PasswordBox("", 0, Flat::F(Font::FONT_MEDIUM));
  host_pwd->SetMaxNbChars(15);
  host->AddWidget(new FlatField(host_pwd, inner_r));
  host_public = new FlatToggle(_("List it on the internet"), inner_r,
                               Config::GetInstance()->GetNetworkServerPublic(), true);
  host->AddWidget(host_public);
  host_start = new FlatButton(_("Start hosting"), Point2i(inner_r, button_h),
                              FlatButton::PRIMARY);
  host->AddWidget(host_start);
  host->Pack();

  // ---- Join by address ----
  FlatPanel *join = new FlatPanel(right_w, pad, u);
  join->AddWidget(Title(_("Join by address"), inner_r));
  std::string address = Config::GetInstance()->GetNetworkClientHost();
  if (!address.empty() && Config::GetInstance()->GetNetworkClientPort() != DefaultPort())
    address += ":" + Config::GetInstance()->GetNetworkClientPort();
  join->AddWidget(Caption(_("Address, or address:port"), inner_r));
  join_address = new TextBox(address, 0, Flat::F(Font::FONT_MEDIUM));
  join->AddWidget(new FlatField(join_address, inner_r));
  join->AddWidget(Caption(_("Password, if the game has one"), inner_r));
  join_pwd = new PasswordBox("", 0, Flat::F(Font::FONT_MEDIUM));
  join->AddWidget(new FlatField(join_pwd, inner_r));
  // The message (e.g. what's wrong with the address) under the fields, so
  // the button can line up with Start hosting
  join_hint = new Label(" ", inner_r, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, Flat::WARN);
  join->AddWidget(join_hint);
  join_go = new FlatButton(_("Join"), Point2i(inner_r, button_h));
  join->AddWidget(join_go);
  join->Pack();

  // ---- Games: as tall as the right column, within the window ----
  const int right_h = three_columns ? std::max(host->GetSizeY(), join->GetSizeY())
                                    : host->GetSizeY() + 2 * u + join->GetSizeY();
  const int avail_h = H - 2 * margin - header_h - 2 * u;
  // On short screens (phones) the block takes the whole height, the games
  // list getting what's left; elsewhere it stays as big as it needs to be
  const int block_h = three_columns ? std::max(avail_h, right_h)
                    : std::min(std::max(right_h, 12 * button_h), avail_h);
  FlatPanel *left = new FlatPanel(left_w, pad, u);
  Label *games_title = Title(_("Games"), inner_l);
  left->AddWidget(games_title);
  games_status = Caption(_("Looking for games on your network..."), inner_l);
  left->AddWidget(games_status);
  int list_h = block_h - 2 * pad - games_title->GetSizeY() - games_status->GetSizeY() - 2 * u;
  games = new GameListBox(Point2i(inner_l, std::max(list_h, 3 * button_h)));
  left->AddWidget(games);

  // ---- Place the block ----
  if (three_columns) {  // all three columns down to the bottom
    left->SetMinHeight(block_h);
    host->SetMinHeight(block_h, 1);  // Start hosting
    join->SetMinHeight(block_h, 1);  // Join
  }
  const int x0 = (W - block_w) / 2;
  const int y0 = std::max(margin, (H - header_h - 2 * u - block_h) / 2);
  title->SetPosition(x0, y0 + (header_h - title->GetSizeY()) / 2);
  back->SetPosition(x0 + block_w - back->GetSizeX(), y0 + (header_h - button_h) / 2);
  const int top = y0 + header_h + 2 * u;
  left->SetPosition(x0, top);
  if (three_columns) {
    host->SetPosition(x0 + left_w + 2 * u, top);
    join->SetPosition(x0 + block_w - right_w, top);
  } else {
    host->SetPosition(x0 + block_w - right_w, top);
    join->SetPosition(x0 + block_w - right_w, top + host->GetSizeY() + 2 * u);
  }
  widgets.AddWidget(title);
  widgets.AddWidget(back);
  widgets.AddWidget(left);
  widgets.AddWidget(host);
  widgets.AddWidget(join);
  widgets.Pack();

  // ************************************************************************
  InitNetInfo();
  refresh_timer = SDL_AddTimer(1000, WakeUp, NULL);
  ThreadRefreshList();
}

NetworkConnectionMenu::~NetworkConnectionMenu()
{
  if (refresh_timer)
    SDL_RemoveTimer(refresh_timer);
  CleanNetInfo();
}

void NetworkConnectionMenu::ThreadRefreshList()
{
  SDL_SemWait(net_info.lock);
  bool running = net_info.thread_refresh && !net_info.finished;
  SDL_SemPost(net_info.lock);
  if (running)
    return;

  WaitForSearch(); // collect the previous, finished thread
  net_info.finished = false;
  net_info.thread_refresh = SDL_CreateThread(RefreshNetInfo, "net-info", NULL);
}

// Show the results of the last search, rebuilding the list if it changed
void NetworkConnectionMenu::ShowGames()
{
  SDL_SemWait(net_info.lock);
  if (!net_info.todisplay) {
    SDL_SemPost(net_info.lock);
    return;
  }
  net_info.todisplay = false;
  std::list<GameServerInfo> lan = net_info.lan_games;
  std::list<GameServerInfo> online = net_info.lst_games;
  bool searching = net_info.searching_index;
  SDL_SemPost(net_info.lock);

  if (!searching)
    last_search_end = SDL_GetTicks();

  // Games as one list, LAN first
  std::vector<GameServerInfo> all(lan.begin(), lan.end());
  size_t nb_lan = all.size();
  all.insert(all.end(), online.begin(), online.end());

  size_t n = all.size();
  if (n == 0)
    games_status->SetText(searching && shown_games.empty()
                          ? _("Looking for games on your network...")
                          : _("No games found yet. Host one, or join by address."));
  else
    games_status->SetText(Format(ngettext("%i game found", "%i games found", (int)n), (int)n));

  std::vector<std::string> keys;
  for (size_t i = 0; i < n; i++)
    keys.push_back(all[i].ip_address + ":" + all[i].port + "/" + all[i].game_name
                   + (all[i].passworded ? "/p" : "") + (i < nb_lan ? "/l" : "")
                   + Format("/%d/%d", all[i].nb_players, all[i].max_players));
  for (size_t i = 0; i < n; i++)
    for (size_t b = 0; b < all[i].battles.size(); b++)
      keys.push_back(Format("%zu/battle/%d/%d", i, all[i].battles[b].nb_players,
                            all[i].battles[b].progress));
  if (keys == shown_games)
    return;
  shown_games = keys;

  // Rebuild the rows
  games->Clear();
  join_buttons.clear();

  const int u = Flat::Unit();
  const int row_w = games->RowWidth();
  const int pad = u;
  const int join_w = 9 * u;
  const int join_h = Flat::ButtonHeight() - u / 2;
  const int icon = 2 * u;
  const int count_w = 6 * u;
  const int text_w = row_w - 2 * pad - join_w - icon - count_w - 3 * u;

  for (size_t i = 0; i < n; i++) {
    const GameServerInfo &g = all[i];
    FlatRow *row = new FlatRow(0, pad, u);

    if (g.passworded)
      row->AddWidget(new PictureWidget(Point2i(icon, icon), "menu/password_lock",
                                       PictureWidget::FIT_SCALING));
    else
      row->AddWidget(new NullWidget(Point2i(icon, icon)));

    VBox *text = new VBox(text_w, false, false, false);
    text->SetNoBorder();
    text->SetMargin(0);
    text->AddWidget(new Label(g.game_name, text_w, Flat::F(Font::FONT_MEDIUM), Font::FONT_BOLD,
                              Flat::TEXT));
    std::string where = (i < nb_lan) ? _("LAN") : _("Online");
    std::string host = (i >= nb_lan && !g.dns_address.empty()) ? g.dns_address : g.ip_address;
    if (g.port != DefaultPort())
      host += ":" + g.port;
    text->AddWidget(new Label(where + "  \xC2\xB7  " + host, text_w, Flat::F(Font::FONT_SMALL),
                              Font::FONT_BOLD, i < nb_lan ? Flat::ACCENT : Flat::TEXT_DIM));
    text->Pack(); // boxes measure their children before packing them
    row->AddWidget(text);

    // Players: "3/8", or nothing for hosts that do not say
    const bool full = g.max_players > 0 && g.nb_players >= g.max_players;
    VBox *count = new VBox(count_w, false, false, false);
    count->SetNoBorder();
    count->SetMargin(0);
    if (g.max_players > 0) {
      count->AddWidget(new Label(Format("%d/%d", g.nb_players, g.max_players), count_w,
                                 Flat::F(Font::FONT_MEDIUM), Font::FONT_BOLD,
                                 full ? Flat::WARN : Flat::TEXT, Text::ALIGN_RIGHT_TOP));
      count->AddWidget(new Label(full ? _("full") : _("players"), count_w,
                                 Flat::F(Font::FONT_SMALL), Font::FONT_BOLD, Flat::TEXT_DIM,
                                 Text::ALIGN_RIGHT_TOP));
    } else {
      count->AddWidget(new NullWidget(Point2i(count_w, 1)));
    }
    count->Pack();
    row->AddWidget(count);

    FlatButton *join = new FlatButton(_("Join"), Point2i(join_w, join_h), FlatButton::PRIMARY);
    join->SetEnabled(!full);
    row->AddWidget(join);
    join_buttons[join] = g;

    row->Pack();
    games->AddWidget(row);

    // Battles being played on this server: not joinable, but they show
    // there are people around (and the lobby above is where to wait)
    for (size_t b = 0; b < g.battles.size(); b++) {
      const BattleInfo &battle = g.battles[b];
      FlatRow *brow = new FlatRow(0, pad, u);
      brow->AddWidget(new NullWidget(Point2i(icon, icon)));

      VBox *btext = new VBox(text_w, false, false, false);
      btext->SetNoBorder();
      btext->SetMargin(0);
      btext->AddWidget(new Label(_("Battle in progress"), text_w, Flat::F(Font::FONT_MEDIUM),
                                 Font::FONT_BOLD, Flat::TEXT_DIM));
      btext->AddWidget(new Label(Format(ngettext("%i player", "%i players", battle.nb_players),
                                        battle.nb_players)
                                 + "  \xC2\xB7  "
                                 + Format(_("%i%% done"), battle.progress),
                                 text_w, Flat::F(Font::FONT_SMALL), Font::FONT_BOLD,
                                 Flat::TEXT_DIM));
      btext->Pack();
      brow->AddWidget(btext);
      brow->AddWidget(new FlatMeter(Point2i(count_w + u + join_w, join_h), battle.progress));
      brow->Pack();
      games->AddWidget(brow);
    }
  }
  games->Pack();
  games->NeedRedrawing();

}

void NetworkConnectionMenu::Draw(const Point2i &/*mousePosition*/)
{
  ShowGames();

  // Search again a little after the last search ended
  SDL_SemWait(net_info.lock);
  bool finished = net_info.finished;
  SDL_SemPost(net_info.lock);
  if (finished && SDL_GetTicks() - last_search_end > SEARCH_INTERVAL_MS)
    ThreadRefreshList();
}

void NetworkConnectionMenu::HandleEvent(const SDL_Event& evnt)
{
  ShowGames();
  Menu::HandleEvent(evnt);
}

void NetworkConnectionMenu::OnClickUp(const Point2i &mousePosition, int button)
{
  Widget *w = widgets.ClickUp(mousePosition, button);
  if (!w)
    return;

  if (w == back) {
    mouse_cancel();
  } else if (w == host_public) {
    if (host_public->InfoClicked()) {
      std::string port = Config::GetInstance()->GetNetworkServerPort();
      if (port.empty())
        port = DefaultPort();
      Question info;
      // (no empty lines: an empty line of text cannot be drawn)
      info.Set(Format(_("Anyone online can find your game, but can join only if your "
                        "router forwards TCP port %s to this device.\n"
                        "Public Battle needs no port forwarding."),
                      port.c_str()), true, 0);
      info.Ask();
      Menu::RedrawMenu();
      return;
    }
    // Kept for this session, so a resize (which rebuilds the menu) keeps it
    Config::GetInstance()->SetNetworkServerPublic(host_public->GetValue());
  } else if (w == host_start) {
    if (Host())
      PlayNetworkGame();
  } else if (w == join_go) {
    if (JoinAddress())
      PlayNetworkGame();
  } else {
    std::map<FlatButton*, GameServerInfo>::iterator it =
      join_buttons.find(static_cast<FlatButton*>(w));
    if (it == join_buttons.end())
      return;
    GameServerInfo game = it->second;  // the list may be rebuilt meanwhile
    if (game.passworded && join_pwd->GetPassword().empty()) {
      // Ask for the password on the Join by address panel
      join_address->SetText(game.port == DefaultPort() ? game.ip_address
                                                       : game.ip_address + ":" + game.port);
      join_hint->SetText(Format(_("\"%s\" needs a password: type it, then press Join."),
                                game.game_name.c_str()));
      widgets.SetFocusOn(join_pwd);
#ifdef ANDROID
      SDL_StartTextInput();
#endif
      return;
    }
    if (JoinGame(game, join_pwd->GetPassword()))
      PlayNetworkGame();
  }
}

void NetworkConnectionMenu::DisplayNetError(connection_state_t conn)
{
  std::string error_msg;

  if (conn == CONN_WRONG_VERSION) {
    error_msg = Format(_("Sorry, your version is not supported anymore. "
                         "Supported versions are %s. "
                         "You can download an updated version "
                         "from https://github.com/KennyStier/warmux"),
                       IndexServer::GetInstance()->GetSupportedVersions().c_str());
  } else {
    error_msg = NetworkErrorToString(conn);
  }

  Menu::DisplayError(error_msg);
}

bool NetworkConnectionMenu::Host()
{
  // Hack: force loading of teams before creating threads.
  GetTeamsList();
  WaitForSearch(); // the search uses the index server too

  std::string name = host_name->GetText();
  if (name.empty())
    name = _("Warmux party");
  const std::string port = Config::GetInstance()->GetNetworkServerPort();
  const bool internet = host_public->GetValue();
  connection_state_t conn;
  int net_port;

  Config::GetInstance()->SetNetworkServerGameName(name);
  Config::GetInstance()->SetNetworkServerPublic(internet);

  if (!internet)
    IndexServer::GetInstance()->SetHiddenServer();
  else {
    conn = IndexServer::GetInstance()->Connect(Constants::WARMUX_VERSION);
    if (conn != CONNECTED) {
      DisplayNetError(conn);
      return false;
    }
  }

  conn = Network::ServerStart(port, name, host_pwd->GetPassword());
  if (conn != CONNECTED) {
    DisplayNetError(conn);
    return false;
  }

  if (!str2int(port, net_port)) {
    DisplayNetError(CONN_BAD_PORT);
    Network::Disconnect();
    return false;
  }

  if (internet && !IndexServer::GetInstance()->SendServerStatus(name, !host_pwd->GetPassword().empty(), net_port)) {
    Menu::DisplayError(Format(_("Your game is not reachable from the internet: TCP port %s "
                                "must accept connections (check your firewall, and your "
                                "router's port forwarding)."), port.c_str()));
    Network::Disconnect();
    return false;
  }

  return Network::GetInstance()->IsConnected();
}

bool NetworkConnectionMenu::Connect(const std::string& address, const std::string& port,
                                    const std::string& password)
{
  // Hack: force loading of teams before creating threads.
  GetTeamsList();
  WaitForSearch();

  join_hint->SetText(" ");
  connection_state_t conn = Network::ClientStart(address, port, password);
  if (Network::IsConnected() && conn == CONNECTED)
    return true;

  if (conn == CONN_WRONG_PASSWORD) {
    join_hint->SetText(password.empty() ? _("This game needs a password.")
                                        : _("Wrong password."));
    join_address->SetText(port == DefaultPort() ? address : address + ":" + port);
    widgets.SetFocusOn(join_pwd);
  } else {
    DisplayNetError(conn);
    // translators: %s:%s will expand to something like "example.org:9999"
    join_hint->SetText(Format(_("Unable to connect to %s:%s"), address.c_str(), port.c_str()));
  }
  Network::Disconnect();
  return false;
}

bool NetworkConnectionMenu::JoinGame(const GameServerInfo &game, const std::string &password)
{
  return Connect(game.ip_address, game.port, password);
}

bool NetworkConnectionMenu::JoinAddress()
{
  std::string host, port;
  SplitAddress(join_address->GetText(), host, port);
  if (host.empty()) {
    join_hint->SetText(_("Type the address of the computer hosting the game."));
    widgets.SetFocusOn(join_address);
    return false;
  }
  if (!Connect(host, port, join_pwd->GetPassword()))
    return false;

  Config::GetInstance()->SetNetworkClientHost(host);
  Config::GetInstance()->SetNetworkClientPort(port);
  return true;
}

void NetworkConnectionMenu::PlayNetworkGame()
{
  play_ok_sound();

  NetworkMenu nm;
  Network::GetInstance()->network_menu = &nm;
  nm.Run();
  Network::GetInstance()->network_menu = NULL;
  IndexServer::GetInstance()->Disconnect();
  Network::Disconnect();

  // Back here: the window may have been resized, the list is out of date
  join_pwd->SetText("");
  join_hint->SetText(" ");
  shown_games.clear();
  Menu::RedrawMenu();
  last_search_end = 0;
  ThreadRefreshList();
}

// Enter, or the --server / --client command line options
bool NetworkConnectionMenu::signal_ok()
{
  network_menu_action_t start = action;
  action = NET_NOTHING;

  bool join = (start == NET_CONNECT) || join_address->HasFocus() || join_pwd->HasFocus();
  bool host = (start == NET_HOST) || host_name->HasFocus() || host_pwd->HasFocus();

  if (join && JoinAddress())
    PlayNetworkGame();
  else if (host && !join && Host())
    PlayNetworkGame();
  return false; // stay in this menu
}

bool NetworkConnectionMenu::signal_cancel()
{
  IndexServer::GetInstance()->Disconnect();
  Network::Disconnect();
  return true;
}
