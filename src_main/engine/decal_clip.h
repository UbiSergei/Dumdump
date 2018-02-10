// Copyright � 1996-2017, Valve Corporation, All rights reserved.

#ifndef DECAL_CLIP_H
#define DECAL_CLIP_H

#include "decal_private.h"
#include "filesystem.h"
#include "gl_model_private.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialsystem.h"
#include "mathlib/compressed_vector.h"
#include "mathlib/vector.h"
#include "mathlib/vector2d.h"

#define MAX_DECALCLIPVERT 48

class CDecalVert {
 public:
  Vector m_vPos;

  // These are the texcoords for the decal itself
  Vector2D m_ctCoords;
  // Vector2d32 m_ctCoords;

  // Lightmap texcoords for the decal.
  Vector2D m_cLMCoords;
  // Vector2d32 m_cLMCoords;
};

// Clip pOutVerts/nStartVerts into the decal's texture space.
CDecalVert *R_DoDecalSHClip(CDecalVert *pInVerts, CDecalVert *pOutVerts,
                            decal_t *pDecal, int nStartVerts,
                            const Vector &vecNormal, int *pVertCount);

// Generate clipped vertex list for decal pdecal projected onto polygon psurf
CDecalVert *R_DecalVertsClip(CDecalVert *pOutVerts, decal_t *pDecal,
                             SurfaceHandle_t surfID, IMaterial *pMaterial,
                             int *pVertCount);

// Compute the unscaled basis for the decal.
void R_DecalComputeBasis(Vector const &surfaceNormal, Vector const *pSAxis,
                         Vector *textureSpaceBasis);

// Compute the basis for the decal and scale the axes so the whole decal fits
// into the (0,0) - (1,1) range.
void R_SetupDecalTextureSpaceBasis(decal_t *pDecal, Vector &vSurfNormal,
                                   IMaterial *pMaterial,
                                   Vector textureSpaceBasis[3],
                                   float decalWorldScale[2]);

#endif  // DECAL_CLIP_H
