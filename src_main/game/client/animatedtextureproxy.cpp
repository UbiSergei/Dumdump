// Copyright � 1996-2017, Valve Corporation, All rights reserved.

#include "cbase.h"

#include "BaseAnimatedTextureProxy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CAnimatedTextureProxy : public CBaseAnimatedTextureProxy {
 public:
  CAnimatedTextureProxy() {}
  virtual ~CAnimatedTextureProxy() {}
  virtual float GetAnimationStartTime(void* pBaseEntity);
};

EXPOSE_INTERFACE(CAnimatedTextureProxy, IMaterialProxy,
                 "AnimatedTexture" IMATERIAL_PROXY_INTERFACE_VERSION);

float CAnimatedTextureProxy::GetAnimationStartTime(void* pBaseEntity) {
  return 0;
}
