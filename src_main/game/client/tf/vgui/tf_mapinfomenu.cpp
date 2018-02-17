// Copyright � 1996-2018, Valve Corporation, All rights reserved.
//
// Purpose: 
//
// $NoKeywords: $


#include "cbase.h"

#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <game/client/iviewport.h>
#include <vgui/ILocalize.h>
#include "tier1/keyvalues.h"
#include <FileSystem.h>
#include "IGameUIFuncs.h" // for key bindings

#include "winerror.h"
#include "ixboxsystem.h"
#include "tf_gamerules.h"
#include "tf_controls.h"
#include "tf_shareddefs.h"
#include "tf_mapinfomenu.h"

using namespace vgui;

const char *GetMapDisplayName( const char *mapName );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFMapInfoMenu::CTFMapInfoMenu( IViewPort *pViewPort ) : Frame( NULL, PANEL_MAPINFO )
{
	m_pViewPort = pViewPort;

	// load the new scheme early!!
	SetScheme( "ClientScheme" );
	
	SetTitleBarVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );
	SetProportional( true );
	SetVisible( false );
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	m_pTitle = new CTFLabel( this, "MapInfoTitle", " " );

#ifdef _X360
	m_pFooter = new CTFFooter( this, "Footer" );
#else
	m_pContinue = new CTFButton( this, "MapInfoContinue", "#TF_Continue" );
	m_pBack = new CTFButton( this, "MapInfoBack", "#TF_Back" );
	m_pIntro = new CTFButton( this, "MapInfoWatchIntro", "#TF_WatchIntro" );
#endif

	// info window about this map
	m_pMapInfo = new CTFRichText( this, "MapInfoText" );
	m_pMapImage = new ImagePanel( this, "MapImage" );

	m_szMapName[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFMapInfoMenu::~CTFMapInfoMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings("Resource/UI/MapInfoMenu.res");

	CheckIntroState();
	CheckBackContinueButtons();

	char mapname[MAX_MAP_NAME];

	Q_FileBase( engine->GetLevelName(), mapname, sizeof(mapname) );

	// Save off the map name so we can re-load the page in ApplySchemeSettings().
	Q_strncpy( m_szMapName, mapname, sizeof( m_szMapName ) );
	Q_strupr( m_szMapName );

#ifdef _X360
	char *pExt = Q_stristr( m_szMapName, ".360" );
	if ( pExt )
	{
		*pExt = '\0';
	}
#endif

	LoadMapPage( m_szMapName );
	SetMapTitle();

#ifndef _X360
	if ( m_pContinue )
	{
		m_pContinue->RequestFocus();
	}
#endif

	if ( IsX360() )
	{
		SetDialogVariable( "gamemode", g_pVGuiLocalize->Find( GetMapType( m_szMapName ) ) );
	}
	else
	{
		if ( GameRules() )
		{
			SetDialogVariable( "gamemode", g_pVGuiLocalize->Find( GameRules()->GetGameTypeName() ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::ShowPanel( bool bShow )
{
	if ( IsVisible() == bShow )
		return;

	m_KeyRepeat.Reset();

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );
		CheckIntroState();
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMapInfoMenu::CheckForIntroMovie()
{
	if ( g_pFullFileSystem->FileExists( TFGameRules()->GetVideoFileForMap() ) )
		return true;

	return false;
}

const char *COM_GetModDirectory();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMapInfoMenu::HasViewedMovieForMap()
{
	return ( UTIL_GetMapKeyCount( "viewed" ) > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::CheckIntroState()
{
	if ( CheckForIntroMovie() && HasViewedMovieForMap() )
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "intro", true );
		}
#else
		if ( m_pIntro && !m_pIntro->IsVisible() )
		{
			m_pIntro->SetVisible( true );
		}
#endif
	}
	else
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "intro", false );
		}
#else
		if ( m_pIntro && m_pIntro->IsVisible() )
		{
			m_pIntro->SetVisible( false );
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::CheckBackContinueButtons()
{
#ifndef _X360
	if ( m_pBack && m_pContinue )
	{
		if ( GetLocalPlayerTeam() == TEAM_UNASSIGNED )
		{
			m_pBack->SetVisible( true );
			m_pContinue->SetText( "#TF_Continue" );
		}
		else
		{
			m_pBack->SetVisible( false );
			m_pContinue->SetText( "#TF_Close" );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::OnCommand( const char *command )
{
	m_KeyRepeat.Reset();

	if ( !Q_strcmp( command, "back" ) )
	{
		 // only want to go back to the Welcome menu if we're not already on a team
		if ( !IsX360() && ( GetLocalPlayerTeam() == TEAM_UNASSIGNED ) )
		{
			m_pViewPort->ShowPanel( this, false );
			m_pViewPort->ShowPanel( PANEL_INFO, true );
		}
	}
	else if ( !Q_strcmp( command, "continue" ) )
	{
		m_pViewPort->ShowPanel( this, false );

		if ( CheckForIntroMovie() && !HasViewedMovieForMap() )
		{
			m_pViewPort->ShowPanel( PANEL_INTRO, true );

			UTIL_IncrementMapKey( "viewed" );
		}
		else
		{
			// On console, we may already have a team due to the lobby assigning us one.
			// We tell the server we're done with the map info menu, and it decides what to do with us.
			if ( IsX360() )
			{
				engine->ClientCmd( "closedwelcomemenu" );
			}
			else if ( GetLocalPlayerTeam() == TEAM_UNASSIGNED )
			{
				m_pViewPort->ShowPanel( PANEL_TEAM, true );
			}

			UTIL_IncrementMapKey( "viewed" );
		}
	}
	else if ( !Q_strcmp( command, "intro" ) )
	{
		m_pViewPort->ShowPanel( this, false );

		if ( CheckForIntroMovie() )
		{
			m_pViewPort->ShowPanel( PANEL_INTRO, true );
		}
		else
		{
			m_pViewPort->ShowPanel( PANEL_TEAM, true );
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::Update()
{ 
	InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: chooses and loads the text page to display that describes mapName map
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::LoadMapPage( const char *mapName )
{
	// load the map image (if it exists for the current map)
	char szMapImage[ MAX_PATH ];
	Q_snprintf( szMapImage, sizeof( szMapImage ), "VGUI/maps/menu_photos_%s", mapName );
	Q_strlower( szMapImage );

	IMaterial *pMapMaterial = materials->FindMaterial( szMapImage, TEXTURE_GROUP_VGUI, false );
	if ( pMapMaterial && !IsErrorMaterial( pMapMaterial ) )
	{
		if ( m_pMapImage )
		{
			if ( !m_pMapImage->IsVisible() )
			{
				m_pMapImage->SetVisible( true );
			}

			// take off the vgui/ at the beginning when we set the image
			Q_snprintf( szMapImage, sizeof( szMapImage ), "maps/menu_photos_%s", mapName );
			Q_strlower( szMapImage );
			
			m_pMapImage->SetImage( szMapImage );
		}
	}
	else
	{
		if ( m_pMapImage && m_pMapImage->IsVisible() )
		{
			m_pMapImage->SetVisible( false );
		}
	}

	// load the map description files
	char mapRES[ MAX_PATH ];

	char uilanguage[ 64 ];
	engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );

	Q_snprintf( mapRES, sizeof( mapRES ), "maps/%s_%s.txt", mapName, uilanguage );

	// try English if the file doesn't exist for our language
	if( !g_pFullFileSystem->FileExists( mapRES, "GAME" ) )
	{
		Q_snprintf( mapRES, sizeof( mapRES ), "maps/%s_english.txt", mapName );

		// if the file doesn't exist for English either, try the filename without any language extension
		if( !g_pFullFileSystem->FileExists( mapRES, "GAME" ) )
		{
			Q_snprintf( mapRES, sizeof( mapRES ), "maps/%s.txt", mapName );
		}
	}

	// if no map specific description exists, load default text
	if( !g_pFullFileSystem->FileExists( mapRES, "GAME" ) )
	{
		const char *pszDefault = "maps/default.txt";

		if ( TFGameRules() )
		{
			if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CTF )
			{
				pszDefault = "maps/default_ctf.txt";
			}
			else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP )
			{
				pszDefault = "maps/default_cp.txt";
			}
		}

		if ( g_pFullFileSystem->FileExists( pszDefault ) )
		{
			Q_snprintf ( mapRES, sizeof( mapRES ), pszDefault );
		}
		else
		{
			m_pMapInfo->SetText( "" );

			// we haven't loaded a valid map image for the current map
			if ( m_pMapImage && !m_pMapImage->IsVisible() )
			{
				if ( m_pMapInfo )
				{
					m_pMapInfo->SetWide( m_pMapInfo->GetWide() + ( m_pMapImage->GetWide() * 0.75 ) ); // add in the extra space the images would have taken 
				}
			}

			return; 
		}
	}

	FileHandle_t f = g_pFullFileSystem->Open( mapRES, "rb" );

	// read into a memory block
	int fileSize = g_pFullFileSystem->Size(f);
	int dataSize = fileSize + sizeof( wchar_t );
	if ( dataSize % 2 )
		++dataSize;
	wchar_t *memBlock = (wchar_t *)malloc(dataSize);
	memset( memBlock, 0x0, dataSize);
	int bytesRead = g_pFullFileSystem->Read(memBlock, fileSize, f);
	if ( bytesRead < fileSize )
	{
		// NULL-terminate based on the length read in, since Read() can transform \r\n to \n and
		// return fewer bytes than we were expecting.
		char *data = reinterpret_cast<char *>( memBlock );
		data[ bytesRead ] = 0;
		data[ bytesRead+1 ] = 0;
	}

	// 0-terminate the stream (redundant, since we memset & then trimmed the transformed buffer already)
	memBlock[dataSize / sizeof(wchar_t) - 1] = 0x0000;

	// check the first character, make sure this a little-endian unicode file

#if defined( _X360 )
	if ( memBlock[0] != 0xFFFE )
#else
	if ( memBlock[0] != 0xFEFF )
#endif
	{
		// its a ascii char file
		m_pMapInfo->SetText( reinterpret_cast<char *>( memBlock ) );
	}
	else
	{
		// ensure little-endian unicode reads correctly on all platforms
		CByteswap byteSwap;
		byteSwap.SetTargetBigEndian( false );
		byteSwap.SwapBufferToTargetEndian( memBlock, memBlock, dataSize/sizeof(wchar_t) );

		m_pMapInfo->SetText( memBlock+1 );
	}
	// go back to the top of the text buffer
	m_pMapInfo->GotoTextStart();

	g_pFullFileSystem->Close( f );
	free(memBlock);

	// we haven't loaded a valid map image for the current map
	if ( m_pMapImage && !m_pMapImage->IsVisible() )
	{
		if ( m_pMapInfo )
		{
			m_pMapInfo->SetWide( m_pMapInfo->GetWide() + ( m_pMapImage->GetWide() * 0.75 ) ); // add in the extra space the images would have taken 
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::SetMapTitle()
{
	SetDialogVariable( "mapname", GetMapDisplayName( m_szMapName ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::OnKeyCodePressed( KeyCode code )
{
	m_KeyRepeat.KeyDown( code );

	if ( code == KEY_XBUTTON_A )
	{
		OnCommand( "continue" );
	}
	else if ( code == KEY_XBUTTON_Y )
	{
		OnCommand( "intro" );
	}
	else if( code == KEY_XBUTTON_UP || code == KEY_XSTICK1_UP )
	{
		// Scroll class info text up
		if ( m_pMapInfo )
		{
			PostMessage( m_pMapInfo, new KeyValues("MoveScrollBarDirect", "delta", 1) );
		}
	}
	else if( code == KEY_XBUTTON_DOWN || code == KEY_XSTICK1_DOWN )
	{
		// Scroll class info text up
		if ( m_pMapInfo )
		{
			PostMessage( m_pMapInfo, new KeyValues("MoveScrollBarDirect", "delta", -1) );
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::OnKeyCodeReleased( vgui::KeyCode code )
{
	m_KeyRepeat.KeyUp( code );

	BaseClass::OnKeyCodeReleased( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::OnThink()
{
	vgui::KeyCode code = m_KeyRepeat.KeyRepeated();
	if ( code )
	{
		OnKeyCodePressed( code );
	}

	BaseClass::OnThink();
}

struct s_MapInfo
{
	const char	*pDiskName;
	const char	*pDisplayName;
	const char	*pGameType;
};

static s_MapInfo s_Maps[] = {
	"ctf_2fort",	"2Fort",		"#Gametype_CTF",
	"cp_dustbowl",	"Dustbowl",		"#TF_AttackDefend",
	"cp_granary",	"Granary",		"#Gametype_CP",
	"cp_well",		"Well (CP)",	"#Gametype_CP",
	"cp_gravelpit", "Gravel Pit",	"#TF_AttackDefend",
	"tc_hydro",		"Hydro",		"#TF_TerritoryControl",
	"ctf_well",		"Well (CTF)",	"#Gametype_CTF",
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *GetMapDisplayName( const char *mapName )
{
	static char szDisplayName[256];
	char szTempName[256];
	const char *pszSrc = NULL;

	szDisplayName[0] = '\0';

	if ( !mapName )
		return szDisplayName;
/*
	// check the worldspawn entity to see if the map author has specified a name
	if ( GetClientWorldEntity() )
	{
		const char *pszMapDescription = GetClientWorldEntity()->m_iszMapDescription;
		if ( Q_strlen( pszMapDescription ) > 0 )
		{
			Q_strncpy( szDisplayName, pszMapDescription, sizeof( szDisplayName ) );
			Q_strupr( szDisplayName );
			
			return szDisplayName;
		}
	}
*/
	// check our lookup table
	Q_strncpy( szTempName, mapName, sizeof( szTempName ) );
	Q_strlower( szTempName );

	for ( int i = 0; i < ARRAYSIZE( s_Maps ); ++i )
	{
		if ( !Q_stricmp( s_Maps[i].pDiskName, szTempName ) )
		{
			return s_Maps[i].pDisplayName;
		}
	}

	// we haven't found a "friendly" map name, so let's just clean up what we have
	if ( !Q_strncmp( szTempName, "cp_", 3 ) ||
		 !Q_strncmp( szTempName, "tc_", 3 ) ||
		 !Q_strncmp( szTempName, "ad_", 3 ) )
	{
		pszSrc = szTempName + 3;
	}
	else if ( !Q_strncmp( szTempName, "ctf_", 4 ) )
	{
		pszSrc = szTempName + 4;
	}
	else
	{
		pszSrc = szTempName;
	}

	Q_strncpy( szDisplayName, pszSrc, sizeof( szDisplayName ) );
	Q_strupr( szDisplayName );

	return szDisplayName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFMapInfoMenu::GetMapType( const char *mapName )
{
	if ( IsX360() && mapName )
	{
		for ( int i = 0; i < ARRAYSIZE( s_Maps ); ++i )
		{
			if ( !Q_stricmp( s_Maps[i].pDiskName, mapName ) )
			{
				return s_Maps[i].pGameType;
			}
		}
	}

	return "";
}
