// Copyright � 1996-2018, Valve Corporation, All rights reserved.

#include "dedicated.h"

#include <dlfcn.h>
#include <malloc.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "conproc.h"
#include "datacache/idatacache.h"
#include "datacache/imdlcache.h"
#include "engine_hlds_api.h"
#include "icvar.h"
#include "idedicatedexports.h"
#include "istudiorender.h"
#include "isys.h"
#include "materialsystem/imaterialsystem.h"
#include "mathlib/mathlib.h"
#include "tier0/include/dbg.h"
#include "tier0/include/icommandline.h"
#include "tier0/include/vcrmode.h"
#include "tier1/checksum_md5.h"
#include "tier1/interface.h"
#include "tier1/strtools.h"
#include "vphysics_interface.h"

bool InitInstance();
void ProcessConsoleInput(void);

#define stringize(a) #a
#define engine_binary(a, b, c) a stringize(b) c

static const char *g_pszengine = engine_binary("bin/engine_", i486, ".so");
static const char *g_pszsoundemitter = "bin/soundemitter_i486.so";

char g_szEXEName[256];

#include "console/TextConsoleUnix.h"
extern CTextConsoleUnix console;

//-----------------------------------------------------------------------------
// Implementation of IVCRHelpers.
//-----------------------------------------------------------------------------

class CVCRHelpers : public IVCRHelpers {
 public:
  virtual void ErrorMessage(const char *pMsg) {
    fprintf(stderr, "ERROR: %s\n", pMsg);
  }

  virtual void *GetMainWindow() { return 0; }
};
static CVCRHelpers g_VCRHelpers;

//-----------------------------------------------------------------------------
// Purpose: Implements OS Specific layer ( loosely )
//-----------------------------------------------------------------------------
class CSys : public ISys {
 public:
  virtual ~CSys();

  virtual bool LoadModules(CDedicatedAppSystemGroup *pAppSystemGroup);

  void Sleep(int msec);
  bool GetExecutableName(char *out);
  void ErrorMessage(int level, const char *msg);

  void WriteStatusText(char *szText);
  void UpdateStatus(int force);

  long LoadLibrary(char *lib);
  void FreeLibrary(long library);
  void *GetProcAddress(long library, const char *name);

  bool CreateConsoleWindow(void);
  void DestroyConsoleWindow(void);

  void ConsoleOutput(char *string);
  char *ConsoleInput();
  void Printf(char *fmt, ...);
};

static CSys g_Sys;
ISys *sys = &g_Sys;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CSys::~CSys() { sys = NULL; }

//-----------------------------------------------------------------------------
// Purpose:
// Input  : msec
// Output :
//-----------------------------------------------------------------------------
void CSys::Sleep(int msec) { usleep(msec * 1000); }

//-----------------------------------------------------------------------------
// Purpose:
// Input  : handle, function name-
// Output : void *
//-----------------------------------------------------------------------------
void *CSys::GetProcAddress(long library, const char *name) {
  return dlsym(library, name);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *lib -
// Output : long
//-----------------------------------------------------------------------------
long CSys::LoadLibrary(char *lib) {
  void *hDll = NULL;

  char cwd[1024];
  char absolute_lib[1024];

  if (!getcwd(cwd, sizeof(cwd)))
    ErrorMessage(1, "Sys_LoadLibrary: Couldn't determine current directory.");

  if (cwd[strlen(cwd) - 1] == '/') cwd[strlen(cwd) - 1] = 0;

  Q_snprintf(absolute_lib, sizeof(absolute_lib), "%s/%s", cwd, lib);

  hDll = dlopen(absolute_lib, RTLD_NOW);
  if (!hDll) {
    ErrorMessage(1, dlerror());
  }
  return (long)hDll;
}

void CSys::FreeLibrary(long library) {
  if (!library) return;

  dlclose((void *)library);
}

bool CSys::GetExecutableName(char *out) {
  char *name = strrchr(g_szEXEName, '/');
  if (name) {
    strcpy(out, name + 1);
    return true;
  } else {
    return false;
  }
}

/*
==============
ErrorMessage

Engine is erroring out, display error in message box
==============
*/
void CSys::ErrorMessage(int level, const char *msg) {
  Error("%s\n", msg);
  exit(-1);
}

void CSys::UpdateStatus(int force) {}

/*
================
ConsoleOutput

Print text to the dedicated console
================
*/
void CSys::ConsoleOutput(char *string) { console.Print(string); }

/*
==============
Printf

Engine is printing to console
==============
*/
void CSys::Printf(char *fmt, ...) {
  // Dump text to debugging console.
  va_list argptr;
  char szText[1024];

  va_start(argptr, fmt);
  Q_vsnprintf(szText, sizeof(szText), fmt, argptr);
  va_end(argptr);

  // Get Current text and append it.
  ConsoleOutput(szText);
}

/*
================
ConsoleInput

================
*/
char *CSys::ConsoleInput(void) { return console.GetLine(); }

/*
==============
WriteStatusText

==============
*/
void CSys::WriteStatusText(char *szText) {}

/*
==============
CreateConsoleWindow

Create console window ( overridable? )
==============
*/
bool CSys::CreateConsoleWindow(void) { return true; }

/*
==============
DestroyConsoleWindow

==============
*/
void CSys::DestroyConsoleWindow(void) {}

/*
================
GameInit
================
*/
bool CSys::LoadModules(CDedicatedAppSystemGroup *pAppSystemGroup) {
  AppSystemInfo_t appSystems[] = {
      {"bin/soundemittersystem_i486.so",
       SOUNDEMITTERSYSTEM_INTERFACE_VERSION},  // loaded for backwards
                                               // compatability, prevents crash
                                               // on exit for old game dlls
      {"bin/materialsystem_i486.so", MATERIAL_SYSTEM_INTERFACE_VERSION},
      {"bin/studiorender_i486.so", STUDIO_RENDER_INTERFACE_VERSION},
      {"bin/vphysics_i486.so", VPHYSICS_INTERFACE_VERSION},
      {"bin/datacache_i486.so", DATACACHE_INTERFACE_VERSION},
      {"bin/datacache_i486.so", MDLCACHE_INTERFACE_VERSION},
      {"bin/datacache_i486.so", STUDIO_DATA_CACHE_INTERFACE_VERSION},
      {g_pszengine, VENGINE_HLDS_API_VERSION},
      {"", ""}  // Required to terminate the list
  };

  if (!pAppSystemGroup->AddSystems(appSystems)) return false;

  engine = (IDedicatedServerAPI *)pAppSystemGroup->FindSystem(
      VENGINE_HLDS_API_VERSION);
  // obsolete i think SetCVarIF( (ICvar*)pAppSystemGroup->FindSystem(
  // VENGINE_CVAR_INTERFACE_VERSION ) );

  IMaterialSystem *pMaterialSystem =
      (IMaterialSystem *)pAppSystemGroup->FindSystem(
          MATERIAL_SYSTEM_INTERFACE_VERSION);
  pMaterialSystem->SetShaderAPI("bin/shaderapiempty_i486.so");
  return true;
}

bool NET_Init() { return true; }

void NET_Shutdown() {}

extern int main(int argc, char *argv[]);
extern "C" int DedicatedMain(int argc, char *argv[]) {
  return main(argc, argv);
}
