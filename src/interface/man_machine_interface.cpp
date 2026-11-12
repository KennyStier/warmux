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
 * Virtual class to handle man/machine interaction
 *****************************************************************************/

#include "interface/man_machine_interface.h"
#include "interface/interface.h"
#include "character/character.h"
#include "game/config.h"
#include "game/game.h"
#include "game/game_mode.h"
#include "game/game_time.h"
#include "graphic/video.h"
#include "include/app.h"
#include "include/action_handler.h"
#include "map/camera.h"
#include "menu/options_menu.h"
#include "network/chat.h"
#include "network/network.h"
#include "object/objbox.h"
#include "replay/replay.h"
#include "team/teams_list.h"
#include "team/team.h"
#include "sound/jukebox.h"
#include "weapon/weapons_list.h"
#include <SDL_events.h>


void ManMachineInterface::Reset()
{
  for (int i = 0; i != 256; i++)
    PressedKeys[i] = false;
}

bool ManMachineInterface::IsRegistredEvent(uint32_t event_type)
{
  std::list<uint32_t>::iterator it;
  for (it = registred_event.begin(); it != registred_event.end(); it ++) {
    if (event_type == (*it))
      return true;
  }
  return false;
}

// Get the key associated to an action.
int ManMachineInterface::GetKeyAssociatedToAction(Key_t at) const
{
  std::map<int, std::vector<Key_t> >::const_iterator it;
  std::vector<Key_t>::const_iterator itv;
  for (it = layout.begin(); it != layout.end(); it++) {
    const std::vector<Key_t>& key_list = it->second;
    for (itv = key_list.begin(); itv != key_list.end(); itv++) {
      if (*itv == at) {
        return it->first;
      }
    }
  }
  return 0;
}

// Handle a pressed key
void ManMachineInterface::HandleKeyPressed(const Key_t &key)
{
  // Held keys are tracked here; ignore presses of an already-held key
  if (PressedKeys[key])
    return;

  switch (key) {
    case KEY_MOVE_CAMERA_RIGHT:
      Camera::GetInstance()->AddLRMoveIntention(INTENTION_MOVE_RIGHT);
      break;
    case KEY_MOVE_CAMERA_LEFT:
      Camera::GetInstance()->AddLRMoveIntention(INTENTION_MOVE_LEFT);
      break;
    case KEY_MOVE_CAMERA_UP:
      Camera::GetInstance()->AddUDMoveIntention(INTENTION_MOVE_UP);
      break;
    case KEY_MOVE_CAMERA_DOWN:
      Camera::GetInstance()->AddUDMoveIntention(INTENTION_MOVE_DOWN);
      break;
    default:
      break;
  }

  // Managing keys related to character moves
  // Available only when local
  if (!ActiveTeam().IsLocalHuman()) return;
  if (Game::GetInstance()->ReadState() == Game::END_TURN) return;
  if (ActiveCharacter().IsDead()) return;

  switch (key) {
    case KEY_MOVE_RIGHT:
      ActiveCharacter().HandleKeyPressed_MoveRight(false);
      break;
    case KEY_MOVE_RIGHT_SLOWLY:
      ActiveCharacter().HandleKeyPressed_MoveRight(true);
      break;
    case KEY_MOVE_LEFT:
      ActiveCharacter().HandleKeyPressed_MoveLeft(false);
      break;
    case KEY_MOVE_LEFT_SLOWLY:
      ActiveCharacter().HandleKeyPressed_MoveLeft(true);
      break;
    case KEY_UP:
      ActiveCharacter().HandleKeyPressed_Up(false);
      break;
    case KEY_UP_SLOWLY:
      ActiveCharacter().HandleKeyPressed_Up(true);
      break;
    case KEY_DOWN:
      ActiveCharacter().HandleKeyPressed_Down(false);
      break;
    case KEY_DOWN_SLOWLY:
      ActiveCharacter().HandleKeyPressed_Down(true);
      break;
    case KEY_JUMP:
      ActiveCharacter().HandleKeyPressed_Jump();
      break;
    case KEY_HIGH_JUMP:
      ActiveCharacter().HandleKeyPressed_HighJump();
      break;
    case KEY_BACK_JUMP:
      ActiveCharacter().HandleKeyPressed_BackJump();
      break;
    case KEY_SHOOT:
      // Shoot key is not accepted in HAS_PLAYED state
      if (Game::GetInstance()->ReadState() == Game::HAS_PLAYED)
        return;
      break;
    case KEY_SCREENSHOT:
      AppWarmux::GetInstance()->video->SaveScreenshot();
      return;
    default:
      // key not supported
      return;
  }
  if (Game::GetInstance()->ReadState() == Game::PLAYING) {
    switch (key) {
    case KEY_MOVE_RIGHT:
      ActiveTeam().AccessWeapon().HandleKeyPressed_MoveRight(false);
      break;
    case KEY_MOVE_RIGHT_SLOWLY:
      ActiveTeam().AccessWeapon().HandleKeyPressed_MoveRight(true);
      break;
    case KEY_MOVE_LEFT:
      ActiveTeam().AccessWeapon().HandleKeyPressed_MoveLeft(false);
      break;
    case KEY_MOVE_LEFT_SLOWLY:
      ActiveTeam().AccessWeapon().HandleKeyPressed_MoveLeft(true);
      break;
    case KEY_UP:
      ActiveTeam().AccessWeapon().HandleKeyPressed_Up(false);
      break;
    case KEY_UP_SLOWLY:
      ActiveTeam().AccessWeapon().HandleKeyPressed_Up(true);
      break;
    case KEY_DOWN:
      ActiveTeam().AccessWeapon().HandleKeyPressed_Down(false);
      break;
    case KEY_DOWN_SLOWLY:
      ActiveTeam().AccessWeapon().HandleKeyPressed_Down(true);
      break;
    case KEY_JUMP:
      ActiveTeam().AccessWeapon().HandleKeyPressed_Jump();
      break;
    case KEY_HIGH_JUMP:
      ActiveTeam().AccessWeapon().HandleKeyPressed_HighJump();
      break;
    case KEY_BACK_JUMP:
      ActiveTeam().AccessWeapon().HandleKeyPressed_BackJump();
      break;
    case KEY_SHOOT:
      ActiveTeam().AccessWeapon().HandleKeyPressed_Shoot();
      break;
    default:
      // key not supported
      return;
    }
  }

  PressedKeys[key] = true ;
}

// Handle a released key
void ManMachineInterface::HandleKeyReleased(const Key_t &key)
{
  PressedKeys[key] = false;
  // Here we manage only actions which are activated on KEY_RELEASED event.

  { // Managing keys related to interface (no game interaction)
    // Always available
    switch(key){
      // Managing interface
    case KEY_HELP:
      Game::GetInstance()->UserAsksForHelpMenu();
      return;
    case KEY_QUIT:
    case KEY_PAUSE:
      Game::GetInstance()->UserAsksForMenu();
      return;
    case KEY_FULLSCREEN:
      AppWarmux::GetInstance()->video->ToggleFullscreen();
      return;
    case KEY_CHAT:
      if(Network::IsConnected())
        Game::GetInstance()->chatsession.ShowInput();
      return;
    case KEY_CENTER:
      Camera::GetInstance()->CenterOnActiveCharacter();
      return;
    case KEY_TOGGLE_WEAPONS_MENUS:
      // Same rules as the weapon button on the bottom bar
      if (ActiveTeam().IsLocalHuman() && !ActiveCharacter().IsDead()
          && Game::GetInstance()->ReadState() == Game::PLAYING
          && !Replay::GetConstInstance()->IsPlaying())
        Interface::GetInstance()->weapons_menu.SwitchDisplay();
      return;
    case KEY_TOGGLE_INTERFACE:
      Interface::GetInstance()->EnableDisplay(!Interface::GetInstance()->IsDisplayed());
      return;
    case KEY_MINIMAP_FROM_GAME:
      Interface::GetInstance()->ToggleMinimap();
      return;
    case KEY_MENU_OPTIONS_FROM_GAME: {
      OptionMenu options_menu;
      options_menu.Run();
      return;
    }
    case KEY_MOVE_CAMERA_RIGHT:
      Camera::GetInstance()->RemoveLRMoveIntention(INTENTION_MOVE_RIGHT);
      return;
    case KEY_MOVE_CAMERA_LEFT:
      Camera::GetInstance()->RemoveLRMoveIntention(INTENTION_MOVE_LEFT);
      return;
    case KEY_MOVE_CAMERA_UP:
      Camera::GetInstance()->RemoveUDMoveIntention(INTENTION_MOVE_UP);
      return;
    case KEY_MOVE_CAMERA_DOWN:
      Camera::GetInstance()->RemoveUDMoveIntention(INTENTION_MOVE_DOWN);
      return;
    case KEY_DECREASE_MINIMAP:
      Interface::GetInstance()->MinimapSizeDelta(1);
      return;
    case KEY_INCREASE_MINIMAP:
      Interface::GetInstance()->MinimapSizeDelta(-1);
      return;
    case KEY_DECREASE_VOLUME:
      {
        Config *cfg = Config::GetInstance();
        int volume = cfg->GetVolumeMusic() - 5;
        if (volume > 0) {
          cfg->SetVolumeMusic(volume);
        } else {
          cfg->SetVolumeMusic(0);
          JukeBox::GetInstance()->ActiveMusic(false);
        }
        volume = cfg->GetVolumeEffects() - 5;
        if (volume > 0) {
          cfg->SetVolumeEffects(volume);
        } else {
          cfg->SetVolumeEffects(0);
          cfg->SetSoundEffects(false);
        }
        return;
      }
    case KEY_INCREASE_VOLUME:
      {
        Config *cfg = Config::GetInstance();
        int max_volume = cfg->GetMaxVolume();
        int volume = cfg->GetVolumeMusic() + 5;
        if (volume < max_volume) {
          cfg->SetVolumeMusic(volume);
        } else {
          cfg->SetVolumeMusic(max_volume);
        }
        JukeBox::GetInstance()->ActiveMusic(true);

        volume = cfg->GetVolumeEffects() + 5;
        if (volume < max_volume) {
          cfg->SetVolumeEffects(volume);
        } else {
          cfg->SetVolumeEffects(max_volume);
        }
        cfg->SetSoundEffects(true);
        return;
      }
    default:
      break;
    }
  }

  // Managing shoot key
  // Drop bonus box or medkit when outside a turn
  // Shoot when in turn
  if (key == KEY_SHOOT) {

    if (Game::GetInstance()->ReadState() == Game::END_TURN) {
      Game::GetInstance()->RequestBonusBoxDrop();
    } else if (Game::GetInstance()->ReadState() == Game::PLAYING &&
               ActiveTeam().IsLocalHuman() &&
               !ActiveCharacter().IsDead()) {
      ActiveTeam().AccessWeapon().HandleKeyReleased_Shoot();
    }
    return;
  }

  { // Managing keys related to character moves
    // Available only when local
    if (!ActiveTeam().IsLocalHuman()) return;
    if (ActiveCharacter().IsDead()) return;
    if (Game::GetInstance()->ReadState() == Game::END_TURN) return;

    switch (key) {
      case KEY_MOVE_RIGHT:
        ActiveCharacter().HandleKeyReleased_MoveRight(false);
        break;
      case KEY_MOVE_RIGHT_SLOWLY:
        ActiveCharacter().HandleKeyReleased_MoveRight(true);
        break;
      case KEY_MOVE_LEFT:
        ActiveCharacter().HandleKeyReleased_MoveLeft(false);
        break;
      case KEY_MOVE_LEFT_SLOWLY:
        ActiveCharacter().HandleKeyReleased_MoveLeft(true);
        break;
      case KEY_UP:
        ActiveCharacter().HandleKeyReleased_Up(false);
        break;
      case KEY_UP_SLOWLY:
        ActiveCharacter().HandleKeyReleased_Up(true);
        break;
      case KEY_DOWN:
        ActiveCharacter().HandleKeyReleased_Down(false);
        break;
      case KEY_DOWN_SLOWLY:
        ActiveCharacter().HandleKeyReleased_Down(true);
        break;
      case KEY_JUMP:
        ActiveCharacter().HandleKeyReleased_Jump();
        break;
      case KEY_HIGH_JUMP:
        ActiveCharacter().HandleKeyReleased_HighJump();
        break;
      case KEY_BACK_JUMP:
        ActiveCharacter().HandleKeyReleased_BackJump();
        break;
      default:
        if (Game::GetInstance()->ReadState() == Game::HAS_PLAYED)
          return;
        break;
    }
    if (Game::GetInstance()->ReadState() == Game::PLAYING) {
      switch (key) {

      case KEY_MOVE_RIGHT:
        ActiveTeam().AccessWeapon().HandleKeyReleased_MoveRight(false);
        return;
      case KEY_MOVE_RIGHT_SLOWLY:
        ActiveTeam().AccessWeapon().HandleKeyReleased_MoveRight(true);
        return;
      case KEY_MOVE_LEFT:
        ActiveTeam().AccessWeapon().HandleKeyReleased_MoveLeft(false);
        return;
      case KEY_MOVE_LEFT_SLOWLY:
        ActiveTeam().AccessWeapon().HandleKeyReleased_MoveLeft(true);
        return;
      case KEY_UP:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Up(false);
        return;
      case KEY_UP_SLOWLY:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Up(true);
        return;
      case KEY_DOWN:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Down(false);
        return;
      case KEY_DOWN_SLOWLY:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Down(true);
        return;
      case KEY_JUMP:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Jump();
        return;
      case KEY_HIGH_JUMP:
        ActiveTeam().AccessWeapon().HandleKeyReleased_HighJump();
        return;
      case KEY_BACK_JUMP:
        ActiveTeam().AccessWeapon().HandleKeyReleased_BackJump();
        return;

        // Shoot key
      case KEY_SHOOT:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Shoot();
        return;

        // Other keys usefull for weapons
      case KEY_WEAPON_1:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Num1();
        return;
      case KEY_WEAPON_2:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Num2();
        return;
      case KEY_WEAPON_3:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Num3();
        return;
      case KEY_WEAPON_4:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Num4();
        return;
      case KEY_WEAPON_5:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Num5();
        return;
      case KEY_WEAPON_6:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Num6();
        return;
      case KEY_WEAPON_7:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Num7();
        return;
      case KEY_WEAPON_8:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Num8();
        return;
      case KEY_WEAPON_9:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Num9();
        return;
      case KEY_WEAPON_LESS:
        ActiveTeam().AccessWeapon().HandleKeyReleased_Less();
        return;
      case KEY_WEAPON_MORE:
        ActiveTeam().AccessWeapon().HandleKeyReleased_More();
        return;
      default:
        break;
      }
    }
  }

  { // Managing keys related to change of character or weapon

    if (Game::GetInstance()->ReadState() != Game::PLAYING ||
        !ActiveTeam().GetWeapon().CanChangeWeapon() ||
        !ActiveTeam().IsLocalHuman())
      return;

    Weapon::category_t weapon_sort = Weapon::INVALID;

    switch(key) {

    case KEY_NEXT_CHARACTER:
      if (GameMode::GetInstance()->AllowCharacterSelection()) {
        SDL_Keymod mod = SDL_GetModState();
        if (mod & KMOD_CTRL) {
          ActiveTeam().PreviousCharacter();
        } else {
          ActiveTeam().NextCharacter();
        }
        ActionHandler::GetInstance()->NewActionActiveCharacter();
      }
      return;

    case KEY_WEAPONS1:
      weapon_sort = Weapon::HEAVY;
      break;
    case KEY_WEAPONS2:
      weapon_sort = Weapon::RIFLE;
      break;
    case KEY_WEAPONS3:
      weapon_sort = Weapon::THROW;
      break;
    case KEY_WEAPONS4:
      weapon_sort = Weapon::SPECIAL;
      break;
    case KEY_WEAPONS5:
      weapon_sort = Weapon::DUEL;
      break;
    case KEY_WEAPONS6:
      weapon_sort = Weapon::MOVE;
      break;
    case KEY_WEAPONS7:
      weapon_sort = Weapon::TOOL;
      break;
    default:
      // Key not supported
      return;
    }

    if (weapon_sort != Weapon::INVALID && ActiveTeam().GetWeapon().CanChangeWeapon()) {
      Weapon::Weapon_type weapon;
      WeaponsList * weapons_list = Game::GetInstance()->GetWeaponsList();
      if (weapons_list->GetWeaponBySort(weapon_sort, weapon)) {
        ASSERT (weapon >= Weapon::FIRST && weapon <= Weapon::LAST);
        ActionHandler::GetInstance()->NewAction(new Action(Action::ACTION_PLAYER_CHANGE_WEAPON, weapon));
      }
    }
  }
}

void ManMachineInterface::ClearKeyAction(Key_t at)
{
  int key = GetKeyAssociatedToAction(at);
  if (key)
    layout[key].clear();
}

// Key names as stored in config.xml (inherited from SDL 1.2 keysym names)
// mapped to SDL2 scancodes, which are used as internal key codes.
// Keys that only exist as shifted symbols or SDL 1.2 'world' keys have no
// physical-key equivalent and are not listed.
static const struct { const char *name; SDL_Scancode scancode; } key_names[] = {
  { "unknown", SDL_SCANCODE_UNKNOWN },
  { "backspace", SDL_SCANCODE_BACKSPACE },
  { "tab", SDL_SCANCODE_TAB },
  { "clear", SDL_SCANCODE_CLEAR },
  { "return", SDL_SCANCODE_RETURN },
  { "pause", SDL_SCANCODE_PAUSE },
  { "escape", SDL_SCANCODE_ESCAPE },
  { "space", SDL_SCANCODE_SPACE },
  { "quote", SDL_SCANCODE_APOSTROPHE },
  { "comma", SDL_SCANCODE_COMMA },
  { "minus", SDL_SCANCODE_MINUS },
  { "period", SDL_SCANCODE_PERIOD },
  { "slash", SDL_SCANCODE_SLASH },
  { "0", SDL_SCANCODE_0 },
  { "1", SDL_SCANCODE_1 },
  { "2", SDL_SCANCODE_2 },
  { "3", SDL_SCANCODE_3 },
  { "4", SDL_SCANCODE_4 },
  { "5", SDL_SCANCODE_5 },
  { "6", SDL_SCANCODE_6 },
  { "7", SDL_SCANCODE_7 },
  { "8", SDL_SCANCODE_8 },
  { "9", SDL_SCANCODE_9 },
  { "semicolon", SDL_SCANCODE_SEMICOLON },
  { "equals", SDL_SCANCODE_EQUALS },
  { "leftbracket", SDL_SCANCODE_LEFTBRACKET },
  { "backslash", SDL_SCANCODE_BACKSLASH },
  { "rightbracket", SDL_SCANCODE_RIGHTBRACKET },
  { "backquote", SDL_SCANCODE_GRAVE },
  { "a", SDL_SCANCODE_A },
  { "b", SDL_SCANCODE_B },
  { "c", SDL_SCANCODE_C },
  { "d", SDL_SCANCODE_D },
  { "e", SDL_SCANCODE_E },
  { "f", SDL_SCANCODE_F },
  { "g", SDL_SCANCODE_G },
  { "h", SDL_SCANCODE_H },
  { "i", SDL_SCANCODE_I },
  { "j", SDL_SCANCODE_J },
  { "k", SDL_SCANCODE_K },
  { "l", SDL_SCANCODE_L },
  { "m", SDL_SCANCODE_M },
  { "n", SDL_SCANCODE_N },
  { "o", SDL_SCANCODE_O },
  { "p", SDL_SCANCODE_P },
  { "q", SDL_SCANCODE_Q },
  { "r", SDL_SCANCODE_R },
  { "s", SDL_SCANCODE_S },
  { "t", SDL_SCANCODE_T },
  { "u", SDL_SCANCODE_U },
  { "v", SDL_SCANCODE_V },
  { "w", SDL_SCANCODE_W },
  { "x", SDL_SCANCODE_X },
  { "y", SDL_SCANCODE_Y },
  { "z", SDL_SCANCODE_Z },
  { "delete", SDL_SCANCODE_DELETE },
  { "kp0", SDL_SCANCODE_KP_0 },
  { "kp1", SDL_SCANCODE_KP_1 },
  { "kp2", SDL_SCANCODE_KP_2 },
  { "kp3", SDL_SCANCODE_KP_3 },
  { "kp4", SDL_SCANCODE_KP_4 },
  { "kp5", SDL_SCANCODE_KP_5 },
  { "kp6", SDL_SCANCODE_KP_6 },
  { "kp7", SDL_SCANCODE_KP_7 },
  { "kp8", SDL_SCANCODE_KP_8 },
  { "kp9", SDL_SCANCODE_KP_9 },
  { "kp_period", SDL_SCANCODE_KP_PERIOD },
  { "kp_divide", SDL_SCANCODE_KP_DIVIDE },
  { "kp_multiply", SDL_SCANCODE_KP_MULTIPLY },
  { "kp_minus", SDL_SCANCODE_KP_MINUS },
  { "kp_plus", SDL_SCANCODE_KP_PLUS },
  { "kp_enter", SDL_SCANCODE_KP_ENTER },
  { "kp_equals", SDL_SCANCODE_KP_EQUALS },
  { "up", SDL_SCANCODE_UP },
  { "down", SDL_SCANCODE_DOWN },
  { "right", SDL_SCANCODE_RIGHT },
  { "left", SDL_SCANCODE_LEFT },
  { "insert", SDL_SCANCODE_INSERT },
  { "home", SDL_SCANCODE_HOME },
  { "end", SDL_SCANCODE_END },
  { "pageup", SDL_SCANCODE_PAGEUP },
  { "pagedown", SDL_SCANCODE_PAGEDOWN },
  { "f1", SDL_SCANCODE_F1 },
  { "f2", SDL_SCANCODE_F2 },
  { "f3", SDL_SCANCODE_F3 },
  { "f4", SDL_SCANCODE_F4 },
  { "f5", SDL_SCANCODE_F5 },
  { "f6", SDL_SCANCODE_F6 },
  { "f7", SDL_SCANCODE_F7 },
  { "f8", SDL_SCANCODE_F8 },
  { "f9", SDL_SCANCODE_F9 },
  { "f10", SDL_SCANCODE_F10 },
  { "f11", SDL_SCANCODE_F11 },
  { "f12", SDL_SCANCODE_F12 },
  { "f13", SDL_SCANCODE_F13 },
  { "f14", SDL_SCANCODE_F14 },
  { "F15", SDL_SCANCODE_F15 },
  { "numlock", SDL_SCANCODE_NUMLOCKCLEAR },
  { "capslock", SDL_SCANCODE_CAPSLOCK },
  { "scrollock", SDL_SCANCODE_SCROLLLOCK },
  { "rshift", SDL_SCANCODE_RSHIFT },
  { "lshift", SDL_SCANCODE_LSHIFT },
  { "rctrl", SDL_SCANCODE_RCTRL },
  { "lctrl", SDL_SCANCODE_LCTRL },
  { "ralt", SDL_SCANCODE_RALT },
  { "lalt", SDL_SCANCODE_LALT },
  { "rmeta", SDL_SCANCODE_RGUI },
  { "lmeta", SDL_SCANCODE_LGUI },
  { "lsuper", SDL_SCANCODE_LGUI },
  { "rsuper", SDL_SCANCODE_RGUI },
  { "mode", SDL_SCANCODE_MODE },
  { "compose", SDL_SCANCODE_APPLICATION },
  { "help", SDL_SCANCODE_HELP },
  { "print", SDL_SCANCODE_PRINTSCREEN },
  { "sysreq", SDL_SCANCODE_SYSREQ },
  { "menu", SDL_SCANCODE_MENU },
  { "power", SDL_SCANCODE_POWER },
  { "undo", SDL_SCANCODE_UNDO },
};

int ManMachineInterface::GetKeyFromKeyName(const std::string &name) const
{
  for (size_t i = 0; i < sizeof(key_names)/sizeof(key_names[0]); i++)
    if (name == key_names[i].name)
      return key_names[i].scancode;
  return SDL_SCANCODE_UNKNOWN;
}

std::string ManMachineInterface::GetKeyNameFromKey(int key) const
{
  for (size_t i = 0; i < sizeof(key_names)/sizeof(key_names[0]); i++)
    if (key == key_names[i].scancode)
      return key_names[i].name;
  return "unknown";
}

ManMachineInterface::Key_t ManMachineInterface::GetActionFromActionName(const std::string &name) const
{
  if(name == "quit") return KEY_QUIT;
  if(name == "weapons1") return KEY_WEAPONS1;
  if(name == "weapons2") return KEY_WEAPONS2;
  if(name == "weapons3") return KEY_WEAPONS3;
  if(name == "weapons4") return KEY_WEAPONS4;
  if(name == "weapons5") return KEY_WEAPONS5;
  if(name == "weapons6") return KEY_WEAPONS6;
  if(name == "weapons7") return KEY_WEAPONS7;
  if(name == "weapons8") return KEY_WEAPONS8;
  if(name == "PAUSE") return KEY_PAUSE;
  if(name == "fullscreen") return KEY_FULLSCREEN;
  if(name == "toggle_interface") return KEY_TOGGLE_INTERFACE;
  if(name == "center") return KEY_CENTER;
  if(name == "toggle_weapons_menus") return KEY_TOGGLE_WEAPONS_MENUS;
  if(name == "chat") return KEY_CHAT;
  if(name == "move_left") return KEY_MOVE_LEFT;
  if(name == "move_left_slowly") return KEY_MOVE_LEFT_SLOWLY;
  if(name == "move_right") return KEY_MOVE_RIGHT;
  if(name == "move_right_slowly") return KEY_MOVE_RIGHT_SLOWLY;
  if(name == "up") return KEY_UP;
  if(name == "up_slowly") return KEY_UP_SLOWLY;
  if(name == "down") return KEY_DOWN;
  if(name == "down_slowly") return KEY_DOWN_SLOWLY;
  if(name == "move_camera_left") return KEY_MOVE_CAMERA_LEFT;
  if(name == "move_camera_right") return KEY_MOVE_CAMERA_RIGHT;
  if(name == "move_camera_up") return KEY_MOVE_CAMERA_UP;
  if(name == "move_camera_down") return KEY_MOVE_CAMERA_DOWN;
  if(name == "jump") return KEY_JUMP;
  if(name == "high_jump") return KEY_HIGH_JUMP;
  if(name == "help") return KEY_HELP;
  if(name == "back_jump") return KEY_BACK_JUMP;
  if(name == "shoot") return KEY_SHOOT;
  if(name == "change_weapon") return KEY_CHANGE_WEAPON;
  if(name == "weapon_1") return KEY_WEAPON_1;
  if(name == "weapon_2") return KEY_WEAPON_2;
  if(name == "weapon_3") return KEY_WEAPON_3;
  if(name == "weapon_4") return KEY_WEAPON_4;
  if(name == "weapon_5") return KEY_WEAPON_5;
  if(name == "weapon_6") return KEY_WEAPON_6;
  if(name == "weapon_7") return KEY_WEAPON_7;
  if(name == "weapon_8") return KEY_WEAPON_8;
  if(name == "weapon_9") return KEY_WEAPON_9;
  if(name == "weapon_less") return KEY_WEAPON_LESS;
  if(name == "weapon_more") return KEY_WEAPON_MORE;
  if(name == "next_character") return KEY_NEXT_CHARACTER;
  if(name == "menu_options_from_game") return KEY_MENU_OPTIONS_FROM_GAME;
  if(name == "minimap_from_game") return KEY_MINIMAP_FROM_GAME;
  if(name == "decrease_minimap") return KEY_DECREASE_MINIMAP;
  if(name == "increase_minimap") return KEY_INCREASE_MINIMAP;
  if(name == "decrease_volume") return KEY_DECREASE_VOLUME;
  if(name == "increase_volume") return KEY_INCREASE_VOLUME;
  if(name == "screenshot") return KEY_SCREENSHOT;

  return KEY_NONE;
}

std::string ManMachineInterface::GetActionNameFromAction(ManMachineInterface::Key_t key) const
{
  if(key == KEY_QUIT) return "quit";
  if(key == KEY_WEAPONS1) return "weapons1";
  if(key == KEY_WEAPONS2) return "weapons2";
  if(key == KEY_WEAPONS3) return "weapons3";
  if(key == KEY_WEAPONS4) return "weapons4";
  if(key == KEY_WEAPONS5) return "weapons5";
  if(key == KEY_WEAPONS6) return "weapons6";
  if(key == KEY_WEAPONS7) return "weapons7";
  if(key == KEY_WEAPONS8) return "weapons8";
  if(key == KEY_PAUSE) return "pause";
  if(key == KEY_FULLSCREEN) return "fullscreen";
  if(key == KEY_TOGGLE_INTERFACE) return "toggle_interface";
  if(key == KEY_CENTER) return "center";
  if(key == KEY_TOGGLE_WEAPONS_MENUS) return "toggle_weapons_menus";
  if(key == KEY_CHAT) return "chat";
  if(key == KEY_MOVE_LEFT) return "move_left";
  if(key == KEY_MOVE_LEFT_SLOWLY) return "move_left_slowly";
  if(key == KEY_MOVE_RIGHT) return "move_right";
  if(key == KEY_MOVE_RIGHT_SLOWLY) return "move_right_slowly";
  if(key == KEY_UP) return "up";
  if(key == KEY_UP_SLOWLY) return "up_slowly";
  if(key == KEY_DOWN) return "down";
  if(key == KEY_DOWN_SLOWLY) return "down_slowly";
  if(key == KEY_MOVE_CAMERA_LEFT) return "move_camera_left";
  if(key == KEY_MOVE_CAMERA_RIGHT) return "move_camera_right";
  if(key == KEY_MOVE_CAMERA_UP) return "move_camera_up";
  if(key == KEY_MOVE_CAMERA_DOWN) return "move_camera_down";
  if(key == KEY_JUMP) return "jump";
  if(key == KEY_HIGH_JUMP) return "high_jump";
  if(key == KEY_HELP) return "help";
  if(key == KEY_BACK_JUMP) return "back_jump";
  if(key == KEY_SHOOT) return "shoot";
  if(key == KEY_CHANGE_WEAPON) return "change_weapon";
  if(key == KEY_WEAPON_1) return "weapon_1";
  if(key == KEY_WEAPON_2) return "weapon_2";
  if(key == KEY_WEAPON_3) return "weapon_3";
  if(key == KEY_WEAPON_4) return "weapon_4";
  if(key == KEY_WEAPON_5) return "weapon_5";
  if(key == KEY_WEAPON_6) return "weapon_6";
  if(key == KEY_WEAPON_7) return "weapon_7";
  if(key == KEY_WEAPON_8) return "weapon_8";
  if(key == KEY_WEAPON_9) return "weapon_9";
  if(key == KEY_WEAPON_LESS) return "weapon_less";
  if(key == KEY_WEAPON_MORE) return "weapon_more";
  if(key == KEY_NEXT_CHARACTER) return "next_character";
  if(key == KEY_MENU_OPTIONS_FROM_GAME) return "menu_options_from_game";
  if(key == KEY_MINIMAP_FROM_GAME) return "minimap_from_game";
  if(key == KEY_DECREASE_MINIMAP) return "decrease_minimap";
  if(key == KEY_INCREASE_MINIMAP) return "increase_minimap";
  if(key == KEY_DECREASE_VOLUME) return "decrease_volume";
  if(key == KEY_INCREASE_VOLUME) return "increase_volume";
  if(key == KEY_SCREENSHOT) return "screenshot";

  return "none";
}

std::string ManMachineInterface::GetHumanReadableActionName(Key_t key) const
{
  if(key == KEY_QUIT) return _("Quit");
  if(key == KEY_WEAPONS1) return _("'Heavy' weapon category");
  if(key == KEY_WEAPONS2) return _("'Rifle' weapon category");
  if(key == KEY_WEAPONS3) return _("'Throwing' weapon category");
  if(key == KEY_WEAPONS4) return _("'Special' weapon category");
  if(key == KEY_WEAPONS5) return _("'Duel' weapon category");
  if(key == KEY_WEAPONS6) return _("'Move' weapon category");
  if(key == KEY_WEAPONS7) return _("'Tool' weapon category");
  if(key == KEY_WEAPONS8) return _("Weapon category 8");
  if(key == KEY_PAUSE) return _("Pause");
  if(key == KEY_FULLSCREEN) return _("Toggle fullscreen");
  if(key == KEY_TOGGLE_INTERFACE) return _("Toggle interface");
  if(key == KEY_CENTER) return _("Center on active character");
  if(key == KEY_TOGGLE_WEAPONS_MENUS) return _("Toggle weapons menu");
  if(key == KEY_CHAT) return _("Chat");
  if(key == KEY_MOVE_LEFT) return _("Move left");
  if(key == KEY_MOVE_LEFT_SLOWLY) return _("Move left slowly");
  if(key == KEY_MOVE_RIGHT) return _("Move right");
  if(key == KEY_MOVE_RIGHT_SLOWLY) return _("Move right slowly");
  if(key == KEY_UP) return _("Up");
  if(key == KEY_UP_SLOWLY) return _("Up slowly");
  if(key == KEY_DOWN) return _("Down");
  if(key == KEY_DOWN_SLOWLY) return _("Down slowly");
  if(key == KEY_MOVE_CAMERA_LEFT) return _("Move camera left");
  if(key == KEY_MOVE_CAMERA_RIGHT) return _("Move camera right");
  if(key == KEY_MOVE_CAMERA_UP) return _("Move camera up");
  if(key == KEY_MOVE_CAMERA_DOWN) return _("Move camera down");
  if(key == KEY_JUMP) return _("Jump");
  if(key == KEY_HIGH_JUMP) return _("High jump");
  if(key == KEY_HELP) return _("Help");
  if(key == KEY_BACK_JUMP) return _("Back jump");
  if(key == KEY_SHOOT) return _("Shoot");
  if(key == KEY_CHANGE_WEAPON) return _("Change weapon");
  if(key == KEY_WEAPON_1) return Format(_("Explosion delay of %u"), 1);
  if(key == KEY_WEAPON_2) return Format(_("Explosion delay of %u"), 2);
  if(key == KEY_WEAPON_3) return Format(_("Explosion delay of %u"), 3);
  if(key == KEY_WEAPON_4) return Format(_("Explosion delay of %u"), 4);
  if(key == KEY_WEAPON_5) return Format(_("Explosion delay of %u"), 5);
  if(key == KEY_WEAPON_6) return Format(_("Explosion delay of %u"), 6);
  if(key == KEY_WEAPON_7) return Format(_("Explosion delay of %u"), 7);
  if(key == KEY_WEAPON_8) return Format(_("Explosion delay of %u"), 8);
  if(key == KEY_WEAPON_9) return Format(_("Explosion delay of %u"), 9);
  if(key == KEY_WEAPON_LESS) return _("Decrease explosion delay");
  if(key == KEY_WEAPON_MORE) return _("Increase explosion delay");
  if(key == KEY_NEXT_CHARACTER) return _("Next character");
  if(key == KEY_MENU_OPTIONS_FROM_GAME) return _("Open options menu");
  if(key == KEY_MINIMAP_FROM_GAME) return _("Toggle minimap");
  if(key == KEY_DECREASE_MINIMAP) return _("Decrease minimap size");
  if(key == KEY_INCREASE_MINIMAP) return _("Increase minimap size");
  if(key == KEY_DECREASE_VOLUME) return _("Decrease sound volume");
  if(key == KEY_INCREASE_VOLUME) return _("Increase sound volume");
  if(key == KEY_SCREENSHOT) return _("Take screenshot");

  return _("None");
}
