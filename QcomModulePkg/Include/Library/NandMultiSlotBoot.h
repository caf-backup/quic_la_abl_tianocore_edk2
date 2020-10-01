/* Copyright (c) 2020 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 *  with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __NANDMULTISLOTBOOT_H__
#define __NANDMULTISLOTBOOT_H__

#include <Uefi.h>
#include "PartitionTableUpdate.h"

#if NAND_AB_ATTR_SUPPORT

#define NAND_COOKIE_BL_MAGIC 0xBABC
#define NAND_COOKIE_UPDATER_MAGIC 0xDABC
#define NAND_SLOT_A_MAGIC 0x5F61
#define NAND_SLOT_B_MAGIC 0x5F62
#define MAX_NAND_ATTR_PAGES 64

/*
 * Attributes used for NAND partitions
 */
typedef struct NandABAttr {

  /*
   * 0xBABC : SBL cookie : set when SBL switched partition on
   * finding a corrupt partition
   * 0xDABC: Downloader cookie: set by AB updater during OTA
   */
  UINT16 Cookie;

  /*
   * Slot name is 0x5F61 for _a or 0x5F62 for _b
   */
  UINT16 SlotName;

  UINT8 RetryCount;

  /*
   * BootSuccess = 0 in case of success, 1 in case of failure.
   */
  UINT8 BootSuccess;
} NandABAttr;

BOOLEAN IsNandABAttrSupport (VOID);
EFI_STATUS NandGetActiveSlot (Slot *ActiveSlot);
EFI_STATUS NandSetActiveSlot (Slot *NewSlot);
EFI_STATUS NandSwitchSlot (VOID);
UINT64 GetNandBootSuccess (VOID);
UINT64 GetNandRetryCount (VOID);
EFI_STATUS NandUpdateRetryCount (VOID);
BOOLEAN IsNandBootAfterOTA (VOID);

#else

STATIC inline EFI_STATUS
GetNandABAttrPartiGuid (EFI_GUID *Ptype)
{
  return EFI_NOT_FOUND;
}

STATIC inline BOOLEAN IsNandABAttrSupport (VOID)
{
  return FALSE;
}

STATIC inline EFI_STATUS
NandGetActiveSlot (Slot *ActiveSlot)
{
  return EFI_NOT_FOUND;
}

STATIC inline EFI_STATUS
NandSetActiveSlot (Slot *NewSlot)
{
  return EFI_NOT_FOUND;
}

STATIC inline BOOLEAN
IsAlternateSlotUnbootable (VOID)
{
  return FALSE;
}

STATIC inline EFI_STATUS
NandSwitchSlot (VOID)
{
  return EFI_NOT_FOUND;
}

STATIC inline UINT64
GetNandBootSuccess (VOID)
{
  return 0;
}

STATIC inline UINT64
GetNandRetryCount (VOID)
{
  return 0;
}

STATIC inline BOOLEAN
IsNandDefaultSlotA (VOID)
{
  return FALSE;
}

STATIC inline EFI_STATUS
NandUpdateRetryCount (VOID)
{
  return EFI_NOT_FOUND;
}

STATIC inline BOOLEAN
IsNandBootAfterOTA (VOID)
{
  return FALSE;
}

#endif
#endif
