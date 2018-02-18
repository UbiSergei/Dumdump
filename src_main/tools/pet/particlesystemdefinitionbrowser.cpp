// Copyright � 1996-2018, Valve Corporation, All rights reserved.
//
// Purpose: Singleton dialog that generates and presents the entity report.
//
//===========================================================================//

#include "particlesystemdefinitionbrowser.h"
#include "tier1/keyvalues.h"
#include "tier1/utlbuffer.h"
#include "iregistry.h"
#include "vgui/ivgui.h"
#include "vgui_controls/listpanel.h"
#include "vgui_controls/inputdialog.h"
#include "vgui_controls/messagebox.h"
#include "petdoc.h"
#include "pettool.h"
#include "datamodel/dmelement.h"
#include "vgui/keycode.h"
#include "dme_controls/dmecontrols_utils.h"
#include "dme_controls/particlesystempanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/include/memdbgon.h"

using namespace vgui;

	
//-----------------------------------------------------------------------------
// Sort by particle system definition name
//-----------------------------------------------------------------------------
static int __cdecl ParticleSystemNameSortFunc( vgui::ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	const char *string1 = item1.kv->GetString("name");
	const char *string2 = item2.kv->GetString("name");
	return Q_stricmp( string1, string2 );
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CParticleSystemDefinitionBrowser::CParticleSystemDefinitionBrowser( CPetDoc *pDoc, vgui::Panel* pParent, const char *pName )
	: BaseClass( pParent, pName ), m_pDoc( pDoc )
{
	SetKeyBoardInputEnabled( true );
	SetPaintBackgroundEnabled( true );

	m_pParticleSystemsDefinitions = new vgui::ListPanel( this, "ParticleSystems" );
	m_pParticleSystemsDefinitions->AddColumnHeader( 0, "name", "Name", 52, ListPanel::COLUMN_RESIZEWITHWINDOW );
	m_pParticleSystemsDefinitions->SetColumnSortable( 0, true );
	m_pParticleSystemsDefinitions->SetEmptyListText( "No Particle System Definitions" );
 	m_pParticleSystemsDefinitions->AddActionSignalTarget( this );
	m_pParticleSystemsDefinitions->SetSortFunc( 0, ParticleSystemNameSortFunc );
	m_pParticleSystemsDefinitions->SetSortColumn( 0 );

	LoadControlSettingsAndUserConfig( "resource/particlesystemdefinitionbrowser.res" );

	UpdateParticleSystemList();
}

CParticleSystemDefinitionBrowser::~CParticleSystemDefinitionBrowser()
{
	SaveUserConfig();
}


//-----------------------------------------------------------------------------
// Gets the ith selected particle system
//-----------------------------------------------------------------------------
CDmeParticleSystemDefinition* CParticleSystemDefinitionBrowser::GetSelectedParticleSystem( int i )
{
	int iSel = m_pParticleSystemsDefinitions->GetSelectedItem( i );
	KeyValues *kv = m_pParticleSystemsDefinitions->GetItem( iSel );
	return GetElementKeyValue< CDmeParticleSystemDefinition >( kv, "particleSystem" );
}


//-----------------------------------------------------------------------------
// Purpose: Deletes the marked objects.
//-----------------------------------------------------------------------------
void CParticleSystemDefinitionBrowser::DeleteParticleSystems()
{		
	int iSel = m_pParticleSystemsDefinitions->GetSelectedItem( 0 );
	int nRow = m_pParticleSystemsDefinitions->GetItemCurrentRow( iSel ) - 1;
	{
		// This is undoable
		CAppUndoScopeGuard guard( NOTIFY_SETDIRTYFLAG, "Delete Particle Systems", "Delete Particle Systems" );

		//
		// Build a list of objects to delete.
		//
		CUtlVector< CDmeParticleSystemDefinition* > itemsToDelete;
		int nCount = m_pParticleSystemsDefinitions->GetSelectedItemsCount();
		for (int i = 0; i < nCount; i++)
		{
			CDmeParticleSystemDefinition *pParticleSystem = GetSelectedParticleSystem( i );
			if ( pParticleSystem )
			{
				itemsToDelete.AddToTail( pParticleSystem );
			}
		}

		nCount = itemsToDelete.Count();
		for ( int i = 0; i < nCount; ++i )
		{
			m_pDoc->DeleteParticleSystemDefinition( itemsToDelete[i] );
		}
	}

	// Update the list box selection.
	if ( m_pParticleSystemsDefinitions->GetItemCount() > 0 )
	{
		if ( nRow < 0 )
		{
			nRow = 0;
		}
		else if ( nRow >= m_pParticleSystemsDefinitions->GetItemCount() ) 
		{
			nRow = m_pParticleSystemsDefinitions->GetItemCount() - 1;
		}

		iSel = m_pParticleSystemsDefinitions->GetItemIDFromRow( nRow );
		m_pParticleSystemsDefinitions->SetSingleSelectedItem( iSel );
	}
	else
	{
		m_pParticleSystemsDefinitions->ClearSelectedItems();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleSystemDefinitionBrowser::OnKeyCodeTyped( vgui::KeyCode code )
{
	if ( code == KEY_DELETE ) 
	{
		DeleteParticleSystems();
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}


//-----------------------------------------------------------------------------
// Called when the selection changes
//-----------------------------------------------------------------------------
void CParticleSystemDefinitionBrowser::UpdateParticleSystemSelection()
{
	if ( m_pParticleSystemsDefinitions->GetSelectedItemsCount() == 1 )
	{
		CDmeParticleSystemDefinition *pParticleSystem = GetSelectedParticleSystem( 0 );
		g_pPetTool->SetCurrentParticleSystem( pParticleSystem, false );
	}
	else
	{
		g_pPetTool->SetCurrentParticleSystem( NULL, false );
	}
}


//-----------------------------------------------------------------------------
// Item selection/deselection
//-----------------------------------------------------------------------------
void CParticleSystemDefinitionBrowser::OnItemSelected( void )
{
	UpdateParticleSystemSelection();
}

void CParticleSystemDefinitionBrowser::OnItemDeselected( void )
{
	UpdateParticleSystemSelection();
}


//-----------------------------------------------------------------------------
// Select a particular node
//-----------------------------------------------------------------------------
void CParticleSystemDefinitionBrowser::SelectParticleSystem( CDmeParticleSystemDefinition *pFind )
{
	m_pParticleSystemsDefinitions->ClearSelectedItems();
	for ( int nItemID = m_pParticleSystemsDefinitions->FirstItem(); nItemID != m_pParticleSystemsDefinitions->InvalidItemID(); nItemID = m_pParticleSystemsDefinitions->NextItem( nItemID ) )
	{
		KeyValues *kv = m_pParticleSystemsDefinitions->GetItem( nItemID );
		CDmeParticleSystemDefinition *pParticleSystem = GetElementKeyValue<CDmeParticleSystemDefinition>( kv, "particleSystem" );
		if ( pParticleSystem == pFind )
		{
			m_pParticleSystemsDefinitions->AddSelectedItem( nItemID );
			break;
		}
	}
}

	
//-----------------------------------------------------------------------------
// Called when buttons are clicked
//-----------------------------------------------------------------------------
void CParticleSystemDefinitionBrowser::OnInputCompleted( KeyValues *pKeyValues )
{
	const char *pText = pKeyValues->GetString( "text", NULL );
	if ( m_pDoc->IsParticleSystemDefined( pText ) )
	{
		char pBuf[1024];
		Q_snprintf( pBuf, sizeof(pBuf), "Particle System \"%s\" already exists!\n", pText ); 
		vgui::MessageBox *pMessageBox = new vgui::MessageBox( "Duplicate Particle System Name!\n", pBuf, g_pPetTool->GetRootPanel() );
		pMessageBox->DoModal( );
		return;
	}

	if ( pKeyValues->FindKey( "create" ) )
	{
		CDmeParticleSystemDefinition *pParticleSystem = m_pDoc->AddNewParticleSystemDefinition( pText );
		g_pPetTool->SetCurrentParticleSystem( pParticleSystem );
	}
	else
		if ( pKeyValues->FindKey( "copy" ) )
		{
			int nCount = m_pParticleSystemsDefinitions->GetSelectedItemsCount();
			if ( nCount )
			{
				CDmeParticleSystemDefinition *pParticleSystem = GetSelectedParticleSystem( 0 );
				CAppUndoScopeGuard guard( NOTIFY_SETDIRTYFLAG, "Copy Particle System",
										  "Copy Particle System" );
				CDmeParticleSystemDefinition * pNew =
					CastElement<CDmeParticleSystemDefinition>( pParticleSystem->Copy( ) );
				pNew->SetName( pText );
				m_pDoc->AddNewParticleSystemDefinition( pNew, guard );
			}
		}
}


//-----------------------------------------------------------------------------
// Copy to clipboard
//-----------------------------------------------------------------------------
void CParticleSystemDefinitionBrowser::CopyToClipboard( )
{
	int nCount = m_pParticleSystemsDefinitions->GetSelectedItemsCount();

	CUtlVector< KeyValues * > list;
	CUtlRBTree< CDmeParticleSystemDefinition* > defs( 0, 0, DefLessFunc( CDmeParticleSystemDefinition* ) );
	for ( int i = 0; i < nCount; ++i )
	{
		CDmeParticleSystemDefinition *pParticleSystem = GetSelectedParticleSystem( i );

		CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
		if ( g_pDataModel->Serialize( buf, "keyvalues2", "pcf", pParticleSystem->GetHandle() ) )
		{
			KeyValues *pData = new KeyValues( "Clipboard" );
			pData->SetString( "pcf", (char*)buf.Base() );
			list.AddToTail( pData );
		}
	}

	if ( list.Count() )
	{
		g_pDataModel->SetClipboardData( list );
	}
}


//-----------------------------------------------------------------------------
// Paste from clipboard
//-----------------------------------------------------------------------------
void CParticleSystemDefinitionBrowser::ReplaceDef_r( CUndoScopeGuard& guard, CDmeParticleSystemDefinition *pDef )
{
	if ( !pDef )
		return;

	m_pDoc->ReplaceParticleSystemDefinition( pDef );
	int nChildCount = pDef->GetParticleFunctionCount( FUNCTION_CHILDREN );
	for ( int i = 0; i < nChildCount; ++i )
	{
		CDmeParticleChild *pChildFunction = static_cast< CDmeParticleChild* >( pDef->GetParticleFunction( FUNCTION_CHILDREN, i ) );
		CDmeParticleSystemDefinition* pChild = pChildFunction->m_Child;
		ReplaceDef_r( guard, pChild );
	}
}

void CParticleSystemDefinitionBrowser::PasteFromClipboard( )
{
	// This is undoable
	CAppUndoScopeGuard guard( NOTIFY_SETDIRTYFLAG, "Paste From Clipboard", "Paste From Clipboard" );

	bool bRefreshAll = false;
	CUtlVector< KeyValues * > list;
	g_pDataModel->GetClipboardData( list );
	int nItems = list.Count();
	for ( int i = 0; i < nItems; ++i )
	{
		const char *pData = list[i]->GetString( "pcf" );
		if ( !pData )
			continue;

		int nLen = Q_strlen( pData );
		CUtlBuffer buf( pData, nLen, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY );

		DmElementHandle_t hRoot;
		if ( !g_pDataModel->Unserialize( buf, "keyvalues2", "pcf", NULL, "paste", CR_DELETE_OLD, hRoot ) )
			continue;

		CDmeParticleSystemDefinition *pDef = GetElement<CDmeParticleSystemDefinition>( hRoot );
		if ( !pDef )
			continue;

		ReplaceDef_r( guard, pDef );
		bRefreshAll = true;
	}

	guard.Release();

	if ( bRefreshAll )
	{
		m_pDoc->UpdateAllParticleSystems();
	}
}


//-----------------------------------------------------------------------------
// Called when buttons are clicked
//-----------------------------------------------------------------------------
void CParticleSystemDefinitionBrowser::OnCommand( const char *pCommand )
{
	if ( !Q_stricmp( pCommand, "create" ) )
	{
		vgui::InputDialog *pInputDialog = new vgui::InputDialog( g_pPetTool->GetRootPanel(), "Enter Particle System Name", "Name:", "" );
		pInputDialog->SetSmallCaption( true );
		pInputDialog->SetMultiline( false );
		pInputDialog->AddActionSignalTarget( this );
		pInputDialog->DoModal( new KeyValues("create") );
		return;
	}
	if ( !Q_stricmp( pCommand, "copy" ) )
	{
		vgui::InputDialog *pInputDialog = new vgui::InputDialog( g_pPetTool->GetRootPanel(), "Enter Particle System Name", "Name:", "" );
		pInputDialog->SetSmallCaption( true );
		pInputDialog->SetMultiline( false );
		pInputDialog->AddActionSignalTarget( this );
		pInputDialog->DoModal( new KeyValues("copy") );
		return;
	}

	if ( !Q_stricmp( pCommand, "delete" ) )
	{
		DeleteParticleSystems();
		return;
	}

	if ( !Q_stricmp( pCommand, "Save" ) )
	{
		g_pPetTool->Save();
		return;
	}

	if ( !Q_stricmp( pCommand, "SaveAndTest" ) )
	{
		g_pPetTool->SaveAndTest();
		return;
	}

	BaseClass::OnCommand( pCommand );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleSystemDefinitionBrowser::UpdateParticleSystemList(void)
{
	const CDmrParticleSystemList particleSystemList = m_pDoc->GetParticleSystemDefinitionList();
	if ( !particleSystemList.IsValid() )
		return;

	// Maintain selection if possible
	CUtlVector< CUtlString > selectedItems;
	int nCount = m_pParticleSystemsDefinitions->GetSelectedItemsCount();
	for ( int i = 0; i < nCount; ++i )
	{
		CDmeParticleSystemDefinition *pParticleSystem = GetSelectedParticleSystem( i );
		if ( pParticleSystem )
		{
			selectedItems.AddToTail( pParticleSystem->GetName() );
		}
	}

	m_pParticleSystemsDefinitions->RemoveAll();
	int nSelectedItemCount = selectedItems.Count();
	nCount = particleSystemList.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		CDmeParticleSystemDefinition *pParticleSystem = particleSystemList[i];
		if ( !pParticleSystem )
			continue;

		const char *pName = pParticleSystem->GetName();
		if ( !pName || !pName[0] )
		{
			pName = "<no name>";
		}

		KeyValues *kv = new KeyValues( "node" );
		kv->SetString( "name", pName ); 
		SetElementKeyValue( kv, "particleSystem", pParticleSystem );

		int nItemID = m_pParticleSystemsDefinitions->AddItem( kv, 0, false, false );

		for ( int j = 0; j < nSelectedItemCount; ++j )
		{
			if ( Q_stricmp( selectedItems[j], pName ) )
				continue;

			m_pParticleSystemsDefinitions->AddSelectedItem( nItemID );
			selectedItems.FastRemove(j);
			--nSelectedItemCount;
			break;
		}
	}
	m_pParticleSystemsDefinitions->SortList();
}

