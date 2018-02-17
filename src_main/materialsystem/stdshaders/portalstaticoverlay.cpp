// Copyright � 1996-2018, Valve Corporation, All rights reserved.
//
// Purpose: 
//
// $NoKeywords: $


#include "BaseVSShader.h"
#include "tier1/convar.h"
#include "portalstaticoverlay_vs20.inc"
#include "portalstaticoverlay_ps20.inc"
#include "portalstaticoverlay_ps20b.inc"
#include "cpp_shader_constant_register_map.h"

BEGIN_VS_SHADER( PortalStaticOverlay, 
				"Help for PortalStaticOverlay shader" )

				BEGIN_SHADER_PARAMS
				SHADER_PARAM_OVERRIDE( COLOR, SHADER_PARAM_TYPE_COLOR, "{255 255 255}", "unused", SHADER_PARAM_NOT_EDITABLE )
				SHADER_PARAM_OVERRIDE( ALPHA, SHADER_PARAM_TYPE_FLOAT, "1.0", "unused", SHADER_PARAM_NOT_EDITABLE )
				SHADER_PARAM( STATICAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0.0", "Amount of the static blend texture to blend into the base texture" )
				SHADER_PARAM( STATICBLENDTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "When adding static, this is the texture that gets blended in" )
				SHADER_PARAM( STATICBLENDTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
				SHADER_PARAM( ALPHAMASKTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "An alpha mask for odd shaped portals" )
				SHADER_PARAM( ALPHAMASKTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
				SHADER_PARAM( NOCOLORWRITE, SHADER_PARAM_TYPE_INTEGER, "0", "" )
				END_SHADER_PARAMS


SHADER_INIT_PARAMS()
{
	SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );
}

SHADER_FALLBACK
{
	if( !g_pHardwareConfig->SupportsVertexAndPixelShaders() )
		return "PortalStaticOverlay_DX60";

	return 0;
}

SHADER_INIT
{
	if( params[STATICBLENDTEXTURE]->IsDefined() )
		LoadTexture( STATICBLENDTEXTURE );
	if( params[ALPHAMASKTEXTURE]->IsDefined() )
		LoadTexture( ALPHAMASKTEXTURE );

	if( !params[STATICAMOUNT]->IsDefined() )
		params[STATICAMOUNT]->SetFloatValue( 0.0f );

	if( !params[STATICBLENDTEXTURE]->IsDefined() )
		params[STATICBLENDTEXTURE]->SetIntValue( 0 );
	if( !params[STATICBLENDTEXTUREFRAME]->IsDefined() )
		params[STATICBLENDTEXTUREFRAME]->SetIntValue( 0 );

	if( !params[ALPHAMASKTEXTURE]->IsDefined() )
		params[ALPHAMASKTEXTURE]->SetIntValue( 0 );
	if( !params[ALPHAMASKTEXTUREFRAME]->IsDefined() )
		params[ALPHAMASKTEXTUREFRAME]->SetIntValue( 0 );

	if( !params[NOCOLORWRITE]->IsDefined() )
		params[NOCOLORWRITE]->SetIntValue( 0 );
}

SHADER_DRAW
{
	bool bStaticBlendTexture = params[STATICBLENDTEXTURE]->IsTexture();
	bool bAlphaMaskTexture = params[ALPHAMASKTEXTURE]->IsTexture(); //must support 2 texture stages to use a mask

	bool bIsModel = IS_FLAG_SET( MATERIAL_VAR_MODEL );
	bool bColorWrites = params[NOCOLORWRITE]->GetIntValue() == 0;

	SHADOW_STATE
	{
		SetInitialShadowState();
		FogToFogColor();

		//pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL ); //a portal is effectively a decal on top of a wall
		pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_NEAREROREQUAL );

		pShaderShadow->EnableDepthWrites( true );

		if( g_pHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
		{
			pShaderShadow->EnableSRGBWrite( true );
		}

		pShaderShadow->EnableBlending( true );
		pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

		pShaderShadow->EnableAlphaTest( true );
		pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GREATER, 0.0f );

		pShaderShadow->EnableColorWrites( bColorWrites );

		if( g_pHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
			pShaderShadow->EnableSRGBWrite( true );

		if( bStaticBlendTexture || bAlphaMaskTexture )
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		if( bStaticBlendTexture && bAlphaMaskTexture )
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

		int fmt = VERTEX_POSITION | VERTEX_NORMAL;
		int userDataSize = 0;
		if( bIsModel )
		{
			userDataSize = 4;
		}
		else
		{
			fmt |= VERTEX_TANGENT_S | VERTEX_TANGENT_T;
		}
		pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, userDataSize );

		DECLARE_STATIC_VERTEX_SHADER( portalstaticoverlay_vs20 );
		SET_STATIC_VERTEX_SHADER_COMBO( MODEL,  bIsModel );
		SET_STATIC_VERTEX_SHADER( portalstaticoverlay_vs20 );

		//avoid setting a pixel shader when only doing depth/stencil operations, as recommended by PIX
		if( bColorWrites || bAlphaMaskTexture )
		{
			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( portalstaticoverlay_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( HASALPHAMASK, bAlphaMaskTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( HASSTATICTEXTURE, bStaticBlendTexture );
				SET_STATIC_PIXEL_SHADER( portalstaticoverlay_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( portalstaticoverlay_ps20 );
				SET_STATIC_PIXEL_SHADER_COMBO( HASALPHAMASK, bAlphaMaskTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( HASSTATICTEXTURE, bStaticBlendTexture );
				SET_STATIC_PIXEL_SHADER( portalstaticoverlay_ps20 );
			}
		}
	}
	DYNAMIC_STATE
	{
		pShaderAPI->SetDefaultState();

		float fStaticAmount = params[STATICAMOUNT]->GetFloatValue();

		//x is static, y is inverse static
		float pc0[4] = { fStaticAmount, 1.0f - fStaticAmount, 0.0f, 0.0f };
		pShaderAPI->SetPixelShaderConstant( 0, pc0 );
		
		if( bStaticBlendTexture )
		{
			BindTexture( SHADER_SAMPLER0, STATICBLENDTEXTURE, STATICBLENDTEXTUREFRAME );
			if( bAlphaMaskTexture )
				BindTexture( SHADER_SAMPLER1, ALPHAMASKTEXTURE, ALPHAMASKTEXTUREFRAME );
		}
		else if( bAlphaMaskTexture )
		{
			BindTexture( SHADER_SAMPLER0, ALPHAMASKTEXTURE, ALPHAMASKTEXTUREFRAME );
		}

		pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

		float vEyePos_SpecExponent[4];
		pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
		vEyePos_SpecExponent[3] = 0.0f;
		pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

		DECLARE_DYNAMIC_VERTEX_SHADER( portalstaticoverlay_vs20 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
		SET_DYNAMIC_VERTEX_SHADER( portalstaticoverlay_vs20 );

		//avoid setting a pixel shader when only doing depth/stencil operations, as recommended by PIX
		if( bColorWrites || bAlphaMaskTexture )
		{
			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( portalstaticoverlay_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED, IsHDREnabled() );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( portalstaticoverlay_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( portalstaticoverlay_ps20 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED, IsHDREnabled() );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( portalstaticoverlay_ps20 );
			}
		}
	}

	Draw();
}

END_SHADER


