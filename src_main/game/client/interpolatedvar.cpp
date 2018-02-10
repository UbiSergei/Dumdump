// Copyright � 1996-2017, Valve Corporation, All rights reserved.
//
// Purpose:
//

#include "interpolatedvar.h"
#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

template class CInterpolatedVar<float>;
template class CInterpolatedVar<Vector>;
template class CInterpolatedVar<QAngle>;
template class CInterpolatedVar<C_AnimationLayer>;

CInterpolationContext *CInterpolationContext::s_pHead = NULL;
bool CInterpolationContext::s_bAllowExtrapolation = false;
float CInterpolationContext::s_flLastTimeStamp = 0;

float g_flLastPacketTimestamp = 0;

ConVar cl_extrapolate_amount(
    "cl_extrapolate_amount", "0.25", FCVAR_CHEAT,
    "Set how many seconds the client will extrapolate entities for.");
