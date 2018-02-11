// Copyright � 1996-2017, Valve Corporation, All rights reserved.
//
// Purpose: Variant Pearson Hash general purpose hashing algorithm described
// by Cargill in C++ Report 1994. Generates a 16-bit result.

#include "tier1/generichash.h"

#include <ctype.h>
#include <cstdlib>

#include "base/include/base_types.h"

// Table of randomly shuffled values from 0-255.

static unsigned g_nRandomValues[256] = {
    238, 164, 191, 168, 115, 16,  142, 11,  213, 214, 57,  151, 248, 252, 26,
    198, 13,  105, 102, 25,  43,  42,  227, 107, 210, 251, 86,  66,  83,  193,
    126, 108, 131, 3,   64,  186, 192, 81,  37,  158, 39,  244, 14,  254, 75,
    30,  2,   88,  172, 176, 255, 69,  0,   45,  116, 139, 23,  65,  183, 148,
    33,  46,  203, 20,  143, 205, 60,  197, 118, 9,   171, 51,  233, 135, 220,
    49,  71,  184, 82,  109, 36,  161, 169, 150, 63,  96,  173, 125, 113, 67,
    224, 78,  232, 215, 35,  219, 79,  181, 41,  229, 149, 153, 111, 217, 21,
    72,  120, 163, 133, 40,  122, 140, 208, 231, 211, 200, 160, 182, 104, 110,
    178, 237, 15,  101, 27,  50,  24,  189, 177, 130, 187, 92,  253, 136, 100,
    212, 19,  174, 70,  22,  170, 206, 162, 74,  247, 5,   47,  32,  179, 117,
    132, 195, 124, 123, 245, 128, 236, 223, 12,  84,  54,  218, 146, 228, 157,
    94,  106, 31,  17,  29,  194, 34,  56,  134, 239, 246, 241, 216, 127, 98,
    7,   204, 154, 152, 209, 188, 48,  61,  87,  97,  225, 85,  90,  167, 155,
    112, 145, 114, 141, 93,  250, 4,   201, 156, 38,  89,  226, 196, 1,   235,
    44,  180, 159, 121, 119, 166, 190, 144, 10,  91,  76,  230, 221, 80,  207,
    55,  58,  53,  175, 8,   6,   52,  68,  242, 18,  222, 103, 249, 147, 129,
    138, 243, 28,  185, 62,  59,  240, 202, 234, 99,  77,  73,  199, 137, 95,
    165,
};

//-----------------------------------------------------------------------------
// String
//-----------------------------------------------------------------------------
unsigned FASTCALL HashString(const char *pszKey) {
  const u8 *k = (const u8 *)pszKey;
  unsigned even = 0, odd = 0, n;

  while ((n = *k++) != 0) {
    even = g_nRandomValues[odd ^ n];
    if ((n = *k++) != 0)
      odd = g_nRandomValues[even ^ n];
    else
      break;
  }

  return (even << 8) | odd;
}

//-----------------------------------------------------------------------------
// Case-insensitive string
//-----------------------------------------------------------------------------
unsigned FASTCALL HashStringCaseless(const char *pszKey) {
  const u8 *k = (const u8 *)pszKey;
  unsigned even = 0, odd = 0, n;

  while ((n = toupper(*k++)) != 0) {
    even = g_nRandomValues[odd ^ n];
    if ((n = toupper(*k++)) != 0)
      odd = g_nRandomValues[even ^ n];
    else
      break;
  }

  return (even << 8) | odd;
}

//-----------------------------------------------------------------------------
// 32 bit conventional case-insensitive string
//-----------------------------------------------------------------------------
unsigned FASTCALL HashStringCaselessConventional(const char *pszKey) {
  unsigned hash = 0xAAAAAAAA;  // Alternating 1's and 0's to maximize the effect
                               // of the later multiply and add

  for (; *pszKey; pszKey++) {
    hash = ((hash << 5) + hash) + (u8)tolower(*pszKey);
  }

  return hash;
}

//-----------------------------------------------------------------------------
// int hash
//-----------------------------------------------------------------------------
unsigned FASTCALL HashInt(const int n) {
  unsigned even = g_nRandomValues[n & 0xff],
           odd = g_nRandomValues[((n >> 8) & 0xff)];

  even = g_nRandomValues[odd ^ (n >> 24)];
  odd = g_nRandomValues[even ^ (n >> 16) & 0xff];
  even = g_nRandomValues[odd ^ ((n >> 8) & 0xff)];
  odd = g_nRandomValues[even ^ (n & 0xff)];

  return (even << 8) | odd;
}

//-----------------------------------------------------------------------------
// 4-byte hash
//-----------------------------------------------------------------------------
unsigned FASTCALL Hash4(const void *pKey) {
  const uint32_t *p = (const uint32_t *)pKey;
  unsigned even, odd, n;
  n = *p;
  even = g_nRandomValues[n & 0xff];
  odd = g_nRandomValues[((n >> 8) & 0xff)];

  even = g_nRandomValues[odd ^ (n >> 24)];
  odd = g_nRandomValues[even ^ (n >> 16) & 0xff];
  even = g_nRandomValues[odd ^ ((n >> 8) & 0xff)];
  odd = g_nRandomValues[even ^ (n & 0xff)];

  return (even << 8) | odd;
}

//-----------------------------------------------------------------------------
// 8-byte hash
//-----------------------------------------------------------------------------
unsigned FASTCALL Hash8(const void *pKey) {
  const uint32_t *p = (const uint32_t *)pKey;
  unsigned even, odd, n;
  n = *p;
  even = g_nRandomValues[n & 0xff];
  odd = g_nRandomValues[((n >> 8) & 0xff)];

  even = g_nRandomValues[odd ^ (n >> 24)];
  odd = g_nRandomValues[even ^ (n >> 16) & 0xff];
  even = g_nRandomValues[odd ^ ((n >> 8) & 0xff)];
  odd = g_nRandomValues[even ^ (n & 0xff)];

  n = *(p + 1);
  even = g_nRandomValues[odd ^ (n >> 24)];
  odd = g_nRandomValues[even ^ ((n >> 16) & 0xff)];
  even = g_nRandomValues[odd ^ ((n >> 8) & 0xff)];
  odd = g_nRandomValues[even ^ (n & 0xff)];

  return (even << 8) | odd;
}

//-----------------------------------------------------------------------------
// 12-byte hash
//-----------------------------------------------------------------------------
unsigned FASTCALL Hash12(const void *pKey) {
  const uint32_t *p = (const uint32_t *)pKey;
  unsigned even, odd, n;
  n = *p;
  even = g_nRandomValues[n & 0xff];
  odd = g_nRandomValues[((n >> 8) & 0xff)];

  even = g_nRandomValues[odd ^ (n >> 24)];
  odd = g_nRandomValues[even ^ (n >> 16) & 0xff];
  even = g_nRandomValues[odd ^ ((n >> 8) & 0xff)];
  odd = g_nRandomValues[even ^ (n & 0xff)];

  n = *(p + 1);
  even = g_nRandomValues[odd ^ (n >> 24)];
  odd = g_nRandomValues[even ^ ((n >> 16) & 0xff)];
  even = g_nRandomValues[odd ^ ((n >> 8) & 0xff)];
  odd = g_nRandomValues[even ^ (n & 0xff)];

  n = *(p + 2);
  even = g_nRandomValues[odd ^ (n >> 24)];
  odd = g_nRandomValues[even ^ ((n >> 16) & 0xff)];
  even = g_nRandomValues[odd ^ ((n >> 8) & 0xff)];
  odd = g_nRandomValues[even ^ (n & 0xff)];

  return (even << 8) | odd;
}

//-----------------------------------------------------------------------------
// 16-byte hash
//-----------------------------------------------------------------------------
unsigned FASTCALL Hash16(const void *pKey) {
  const uint32_t *p = (const uint32_t *)pKey;
  unsigned even, odd, n;
  n = *p;
  even = g_nRandomValues[n & 0xff];
  odd = g_nRandomValues[((n >> 8) & 0xff)];

  even = g_nRandomValues[odd ^ (n >> 24)];
  odd = g_nRandomValues[even ^ (n >> 16) & 0xff];
  even = g_nRandomValues[odd ^ ((n >> 8) & 0xff)];
  odd = g_nRandomValues[even ^ (n & 0xff)];

  n = *(p + 1);
  even = g_nRandomValues[odd ^ (n >> 24)];
  odd = g_nRandomValues[even ^ ((n >> 16) & 0xff)];
  even = g_nRandomValues[odd ^ ((n >> 8) & 0xff)];
  odd = g_nRandomValues[even ^ (n & 0xff)];

  n = *(p + 2);
  even = g_nRandomValues[odd ^ (n >> 24)];
  odd = g_nRandomValues[even ^ ((n >> 16) & 0xff)];
  even = g_nRandomValues[odd ^ ((n >> 8) & 0xff)];
  odd = g_nRandomValues[even ^ (n & 0xff)];

  n = *(p + 3);
  even = g_nRandomValues[odd ^ (n >> 24)];
  odd = g_nRandomValues[even ^ ((n >> 16) & 0xff)];
  even = g_nRandomValues[odd ^ ((n >> 8) & 0xff)];
  odd = g_nRandomValues[even ^ (n & 0xff)];

  return (even << 8) | odd;
}

//-----------------------------------------------------------------------------
// Arbitrary fixed length hash
//-----------------------------------------------------------------------------
unsigned FASTCALL HashBlock(const void *pKey, unsigned size) {
  const u8 *k = (const u8 *)pKey;
  unsigned even = 0, odd = 0, n;

  while (size) {
    --size;
    n = *k++;
    even = g_nRandomValues[odd ^ n];
    if (size) {
      --size;
      n = *k++;
      odd = g_nRandomValues[even ^ n];
    } else
      break;
  }

  return (even << 8) | odd;
}
