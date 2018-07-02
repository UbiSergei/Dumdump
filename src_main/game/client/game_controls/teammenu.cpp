// Copyright � 1996-2018, Valve Corporation, All rights reserved.

#include "cbase.h"

#include <cdll_client_int.h>

#include "teammenu.h"

#include <FileSystem.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui_controls/ImageList.h>
#include "tier1/keyvalues.h"

#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>

#include <game/client/iviewport.h>
#include <igameresources.h>
#include <cstdio>
#include <cstdlib>         // SOURCE_MAX_PATH define
#include "IGameUIFuncs.h"  // for key bindings
#include "tier1/byteswap.h"

#include "tier0/include/memdbgon.h"

extern IGameUIFuncs *gameuifuncs;  // for key binding details

using namespace vgui;

void UpdateCursorState();
// void DuckMessage(const char *str);

// helper function
const char *GetStringTeamColor(int i) {
  switch (i) {
    case 0:
      return "team0";

    case 1:
      return "team1";

    case 2:
      return "team2";

    case 3:
      return "team3";

    case 4:
    default:
      return "team4";
  }
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTeamMenu::CTeamMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_TEAM) {
  m_pViewPort = pViewPort;
  m_iJumpKey = BUTTON_CODE_INVALID;        // this is looked up in Activate()
  m_iScoreBoardKey = BUTTON_CODE_INVALID;  // this is looked up in Activate()

  // initialize dialog
  SetTitle("", true);

  // load the new scheme early!!
  SetScheme("ClientScheme");
  SetMoveable(false);
  SetSizeable(false);

  // hide the system buttons
  SetTitleBarVisible(false);
  SetProportional(true);

  // info window about this map
  m_pMapInfo = new RichText(this, "MapInfo");

#if defined(ENABLE_HTML_WINDOW)
  m_pMapInfoHTML = new HTML(this, "MapInfoHTML");
#endif

  LoadControlSettings("Resource/UI/TeamMenu.res");
  InvalidateLayout();

  m_szMapName[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTeamMenu::~CTeamMenu() {}

//-----------------------------------------------------------------------------
// Purpose: sets the text color of the map description field
//-----------------------------------------------------------------------------
void CTeamMenu::ApplySchemeSettings(IScheme *pScheme) {
  BaseClass::ApplySchemeSettings(pScheme);
  m_pMapInfo->SetFgColor(
      pScheme->GetColor("MapDescriptionText", Color(255, 255, 255, 0)));

  if (*m_szMapName) {
    LoadMapPage(
        m_szMapName);  // reload the map description to pick up the color
  }
}

//-----------------------------------------------------------------------------
// Purpose: makes the user choose the auto assign option
//-----------------------------------------------------------------------------
void CTeamMenu::AutoAssign() {
  engine->ClientCmd("jointeam 0");
  OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: shows the team menu
//-----------------------------------------------------------------------------
void CTeamMenu::ShowPanel(bool bShow) {
  if (BaseClass::IsVisible() == bShow) return;

  if (bShow) {
    Activate();

    SetMouseInputEnabled(true);

    // get key bindings if shown

    if (m_iJumpKey == BUTTON_CODE_INVALID)  // you need to lookup the jump key
                                            // AFTER the engine has loaded
    {
      m_iJumpKey = gameuifuncs->GetButtonCodeForBind("jump");
    }

    if (m_iScoreBoardKey == BUTTON_CODE_INVALID) {
      m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind("showscores");
    }
  } else {
    SetVisible(false);
    SetMouseInputEnabled(false);
  }

  m_pViewPort->ShowBackGround(bShow);
}

//-----------------------------------------------------------------------------
// Purpose: updates the UI with a new map name and map html page, and sets up
// the team buttons
//-----------------------------------------------------------------------------
void CTeamMenu::Update() {
  char mapname[MAX_MAP_NAME];

  Q_FileBase(engine->GetLevelName(), mapname, sizeof(mapname));

  SetLabelText("mapname", mapname);

  LoadMapPage(mapname);
}

//-----------------------------------------------------------------------------
// Purpose: chooses and loads the text page to display that describes mapName
// map
//-----------------------------------------------------------------------------
void CTeamMenu::LoadMapPage(const char *mapName) {
  // Save off the map name so we can re-load the page in ApplySchemeSettings().
  Q_strncpy(m_szMapName, mapName, strlen(mapName) + 1);

  char mapRES[SOURCE_MAX_PATH];

  char uilanguage[64];
  engine->GetUILanguage(uilanguage, sizeof(uilanguage));

  Q_snprintf(mapRES, sizeof(mapRES), "resource/maphtml/%s_%s.html", mapName,
             uilanguage);

  bool bFoundHTML = false;

  if (!g_pFullFileSystem->FileExists(mapRES)) {
    // try english
    Q_snprintf(mapRES, sizeof(mapRES), "resource/maphtml/%s_english.html",
               mapName);
  } else {
    bFoundHTML = true;
  }

  if (bFoundHTML || g_pFullFileSystem->FileExists(mapRES)) {
    // it's a local HTML file
    char localURL[SOURCE_MAX_PATH + 7];
    Q_strncpy(localURL, "file://", sizeof(localURL));

    char pPathData[SOURCE_MAX_PATH];
    g_pFullFileSystem->GetLocalPath(mapRES, pPathData, sizeof(pPathData));
    Q_strncat(localURL, pPathData, sizeof(localURL), COPY_ALL_CHARACTERS);

    // force steam to dump a local copy
    g_pFullFileSystem->GetLocalCopy(pPathData);

    m_pMapInfo->SetVisible(false);

#if defined(ENABLE_HTML_WINDOW)
    m_pMapInfoHTML->SetVisible(true);
    m_pMapInfoHTML->OpenURL(localURL);
#endif
    InvalidateLayout();
    Repaint();

    return;
  } else {
    m_pMapInfo->SetVisible(true);

#if defined(ENABLE_HTML_WINDOW)
    m_pMapInfoHTML->SetVisible(false);
#endif
  }

  Q_snprintf(mapRES, sizeof(mapRES), "maps/%s.txt", mapName);

  // if no map specific description exists, load default text
  if (!g_pFullFileSystem->FileExists(mapRES)) {
    if (g_pFullFileSystem->FileExists("maps/default.txt")) {
      Q_snprintf(mapRES, sizeof(mapRES), "maps/default.txt");
    } else {
      m_pMapInfo->SetText("");
      return;
    }
  }

  FileHandle_t f = g_pFullFileSystem->Open(mapRES, "r");

  // read into a memory block
  int fileSize = g_pFullFileSystem->Size(f);
  int dataSize = fileSize + sizeof(wchar_t);
  if (dataSize % 2) ++dataSize;
  wchar_t *memBlock = (wchar_t *)malloc(dataSize);
  memset(memBlock, 0x0, dataSize);
  int bytesRead = g_pFullFileSystem->Read(memBlock, fileSize, f);
  if (bytesRead < fileSize) {
    // NULL-terminate based on the length read in, since Read() can transform
    // \r\n to \n and return fewer bytes than we were expecting.
    char *data = reinterpret_cast<char *>(memBlock);
    data[bytesRead] = 0;
    data[bytesRead + 1] = 0;
  }

  // 0-terminate the stream (redundant, since we memset & then trimmed the
  // transformed buffer already)
  memBlock[dataSize / sizeof(wchar_t) - 1] = 0x0000;

  // ensure little-endian unicode reads correctly on all platforms
  CByteswap byteSwap;
  byteSwap.SetTargetBigEndian(false);
  byteSwap.SwapBufferToTargetEndian(memBlock, memBlock,
                                    dataSize / sizeof(wchar_t));

  // check the first character, make sure this a little-endian unicode file
  if (memBlock[0] != 0xFEFF) {
    // its a ascii char file
    m_pMapInfo->SetText(reinterpret_cast<char *>(memBlock));
  } else {
    m_pMapInfo->SetText(memBlock + 1);
  }
  // go back to the top of the text buffer
  m_pMapInfo->GotoTextStart();

  g_pFullFileSystem->Close(f);
  heap_free(memBlock);

  InvalidateLayout();
  Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CTeamMenu::SetLabelText(const char *textEntryName, const char *text) {
  Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
  if (entry) {
    entry->SetText(text);
  }
}

void CTeamMenu::OnKeyCodePressed(KeyCode code) {
  if (m_iJumpKey != BUTTON_CODE_INVALID && m_iJumpKey == code) {
    AutoAssign();
  } else if (m_iScoreBoardKey != BUTTON_CODE_INVALID &&
             m_iScoreBoardKey == code) {
    gViewPortInterface->ShowPanel(PANEL_SCOREBOARD, true);
    gViewPortInterface->PostMessageToPanel(
        PANEL_SCOREBOARD, new KeyValues("PollHideCode", "code", code));
  } else {
    BaseClass::OnKeyCodePressed(code);
  }
}
