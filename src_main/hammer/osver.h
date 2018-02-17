// Copyright � 1996-2018, Valve Corporation, All rights reserved.
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
typedef enum eOSVersion
{
    eUninitialized,
    eUnknown,
    eWin9x,
    eWinNT,
};

extern void       initOSVersion();
extern eOSVersion getOSVersion();
