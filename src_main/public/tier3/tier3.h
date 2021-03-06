// Copyright � 1996-2018, Valve Corporation, All rights reserved.
//
// Purpose: A higher level link library for general use in the game and tools.

#ifndef SOURCE_TIER3_TIER3_H_
#define SOURCE_TIER3_TIER3_H_

#ifdef _WIN32
#pragma once
#endif

#include "base/include/base_types.h"
#include "tier2/tier2.h"

class IStudioRender;
class IMatSystemSurface;
class IDataCache;
class IMDLCache;
class IAvi;
class IBik;
class IDmeMakefileUtils;
class IPhysicsCollision;
class ISoundEmitterSystemBase;

namespace vgui {
class ISurface;
class IVGui;
class IInput;
class IPanel;
class ILocalize;
class ISchemeManager;
class ISystem;
}  // namespace vgui

// These tier3 libraries must be set by any users of this library.
// They can be set by calling ConnectTier3Libraries.
// It is hoped that setting this, and using this library will be the common
// mechanism for allowing link libraries to access tier3 library interfaces

extern IStudioRender *g_pStudioRender;
extern IStudioRender *studiorender;
extern IMatSystemSurface *g_pMatSystemSurface;
extern vgui::ISurface *g_pVGuiSurface;
extern vgui::IInput *g_pVGuiInput;
extern vgui::IVGui *g_pVGui;
extern vgui::IPanel *g_pVGuiPanel;
extern vgui::ILocalize *g_pVGuiLocalize;
extern vgui::ISchemeManager *g_pVGuiSchemeManager;
extern vgui::ISystem *g_pVGuiSystem;
// TODO(d.rattman): Should IDataCache be in tier2?
extern IDataCache *g_pDataCache;
extern IMDLCache *g_pMDLCache;
extern IMDLCache *mdlcache;
extern IAvi *g_pAVI;
extern IBik *g_pBIK;
extern IDmeMakefileUtils *g_pDmeMakefileUtils;
extern IPhysicsCollision *g_pPhysicsCollision;
extern ISoundEmitterSystemBase *g_pSoundEmitterSystem;

// Call this to connect to/disconnect from all tier 3 libraries.
// It's up to the caller to check the globals it cares about to see if ones are
// missing

void ConnectTier3Libraries(CreateInterfaceFn *factory_list,
                           usize nFactoryCount);
void DisconnectTier3Libraries();

// Helper empty implementation of an IAppSystem for tier2 libraries

template <typename IInterface, int ConVarFlag = 0>
class CTier3AppSystem : public CTier2AppSystem<IInterface, ConVarFlag> {
  using BaseClass = CTier2AppSystem<IInterface, ConVarFlag>;

 public:
  CTier3AppSystem(bool bIsPrimaryAppSystem = true)
      : BaseClass{bIsPrimaryAppSystem} {}

  bool Connect(CreateInterfaceFn factory) override {
    if (BaseClass::Connect(factory)) {
      if (this->IsPrimaryAppSystem()) {
        ConnectTier3Libraries(&factory, 1);
      }
      return true;
    }

    return false;
  }

  void Disconnect() override {
    if (this->IsPrimaryAppSystem()) {
      DisconnectTier3Libraries();
    }

    BaseClass::Disconnect();
  }
};

#endif  // SOURCE_TIER3_TIER3_H_
