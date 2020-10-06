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

#include "NandMultiSlotBoot.h"
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BootLinux.h>
#include <Protocol/EFINandPartiGuid.h>

#if NAND_AB_ATTR_SUPPORT

STATIC struct NandABAttr *NandAttr;

STATIC EFI_STATUS
GetNandABAttrPartiGuid (EFI_GUID *Ptype)
{
  EFI_STATUS Status = EFI_INVALID_PARAMETER;
  EFI_NAND_PARTI_GUID_PROTOCOL *NandPartiGuid;

  Status = gBS->LocateProtocol (&gEfiNandPartiGuidProtocolGuid, NULL,
                                (VOID **)&NandPartiGuid);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Unable to locate NandPartiGuid protocol: %r\n",
                                Status));
    return Status;
  }

  Status = NandPartiGuid->GenGuid (NandPartiGuid,
                               (CONST CHAR16 *)L"nand_ab_attr",
                               StrLen ((CONST CHAR16 *)L"nand_ab_attr"),
                               Ptype);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "NandPartiGuid GenGuid failed with: %r\n", Status));
    return Status;
  }

  return Status;
}

STATIC BOOLEAN NandSlotAttrValid (VOID)
{
  if ((NandAttr->Cookie == NAND_COOKIE_UPDATER_MAGIC ||
       NandAttr->Cookie == NAND_COOKIE_BL_MAGIC ) &&
       (NandAttr->SlotName == NAND_SLOT_A_MAGIC ||
       NandAttr->SlotName == NAND_SLOT_B_MAGIC)) {
    return TRUE;
  }
  return FALSE;
}

STATIC EFI_STATUS NandInitSlot (Slot *ActiveSlot, CHAR16 *Suffix)
{
  EFI_STATUS Status;
  GUARD (StrnCpyS (ActiveSlot->Suffix, ARRAY_SIZE (ActiveSlot->Suffix),
                       Suffix, StrLen (Suffix)));
  return Status;
}

/*
 * If the previous boot attempt had failed, the Bootloader
 * will set the cookie to NAND_COOKIE_BL_MAGIC.
 */
STATIC BOOLEAN IsAlternateSlotUnbootable (VOID)
{
  if (NandAttr->Cookie == NAND_COOKIE_BL_MAGIC) {
    return TRUE;
  }
  return FALSE;
}

STATIC EFI_STATUS
ReadFromNandPartition (EFI_GUID *Ptype, VOID *Msg, UINT32 Size, UINT32 PageOffset)
{
  EFI_STATUS Status;
  EFI_BLOCK_IO_PROTOCOL *BlkIo = NULL;
  PartiSelectFilter HandleFilter;
  HandleInfo HandleInfoList[1];
  UINT32 MaxHandles;
  UINT32 BlkIOAttrib = 0;
  UINT64 MsgSize;
  UINT64 PartitionSize;
  EFI_LBA Lba;
  UINT64 PagesPerBlock;

  BlkIOAttrib = BLK_IO_SEL_PARTITIONED_GPT;
  BlkIOAttrib |= BLK_IO_SEL_MEDIA_TYPE_NON_REMOVABLE;
  BlkIOAttrib |= BLK_IO_SEL_MATCH_PARTITION_TYPE_GUID;

  HandleFilter.RootDeviceType = NULL;
  HandleFilter.PartitionType = Ptype;
  HandleFilter.VolumeName = NULL;

  MaxHandles = ARRAY_SIZE (HandleInfoList);

  Status =
      GetBlkIOHandles (BlkIOAttrib, &HandleFilter, HandleInfoList, &MaxHandles);

  if (Status == EFI_SUCCESS) {
    if (MaxHandles == 0)
      return EFI_NO_MEDIA;

    if (MaxHandles != 1) {
      // Unable to deterministically load from single partition
      DEBUG ((EFI_D_INFO, "%s: multiple partitions found.\r\n", __func__));
      return EFI_LOAD_ERROR;
    }
  } else {
    DEBUG ((EFI_D_ERROR,
           "%s: GetBlkIOHandles failed: %r\n", __func__, Status));
    return Status;
  }

  BlkIo = HandleInfoList[0].BlkIo;
  MsgSize = ROUND_TO_PAGE (Size, BlkIo->Media->BlockSize - 1);
  PartitionSize = (BlkIo->Media->LastBlock + 1) * BlkIo->Media->BlockSize;
  if (MsgSize > PartitionSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  PagesPerBlock = BlkIo->Media->BlockSize / EFI_PAGE_SIZE;
  Lba = PageOffset * PagesPerBlock;

  Status = BlkIo->ReadBlocks (BlkIo, BlkIo->Media->MediaId, Lba, MsgSize, Msg);

  return Status;
}

STATIC EFI_STATUS
WriteBlockToNandPartition (EFI_BLOCK_IO_PROTOCOL *BlockIo,
                   IN EFI_HANDLE *Handle,
                   IN UINT64 Offset,
                   IN UINT64 Size,
                   IN VOID *Image)
{
  EFI_STATUS Status = EFI_SUCCESS;
  CHAR8 *ImageBuffer = NULL;
  UINT32 WriteBlockSize;
  UINT64 WriteUnitSize = EFI_PAGE_SIZE;

  if ((BlockIo == NULL) ||
    (Image == NULL)) {
    DEBUG ((EFI_D_ERROR, "NUll BlockIo or Image\n"));
    return EFI_INVALID_PARAMETER;
  }

  WriteBlockSize = BlockIo->Media->BlockSize;
  WriteUnitSize = ROUND_TO_PAGE (WriteUnitSize, WriteBlockSize - 1);
  ImageBuffer = AllocateZeroPool (WriteBlockSize);
  if (ImageBuffer == NULL) {
    DEBUG ((EFI_D_ERROR, "Failed to allocate zero pool for ImageBuffer\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  /* Read firstly to ensure the final write is not to change data
   * in this Block other than "Size - DivMsgBufSize"
   */
  Status = BlockIo->ReadBlocks (BlockIo,
                                 BlockIo->Media->MediaId,
                                 Offset,
                                 WriteBlockSize,
                                 ImageBuffer);

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Reading block failed :%r\n", Status));
    FreePool (ImageBuffer);
    ImageBuffer = NULL;
    return Status;
  }

  gBS->CopyMem (ImageBuffer, Image, Size);
  Status = BlockIo->WriteBlocks (BlockIo,
                                 BlockIo->Media->MediaId,
                                 Offset,
                                 WriteBlockSize,
                                 ImageBuffer);

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Writing single block failed :%r\n", Status));
  }

  FreePool (ImageBuffer);
  ImageBuffer = NULL;

  return Status;
}

STATIC EFI_STATUS
WriteToNandPartition (EFI_GUID *Ptype, VOID *Msg, UINT32 MsgSize, UINT32 PageOffset)
{
  EFI_STATUS Status;
  EFI_BLOCK_IO_PROTOCOL *BlkIo = NULL;
  PartiSelectFilter HandleFilter;
  HandleInfo HandleInfoList[1];
  UINT32 MaxHandles;
  UINT32 BlkIOAttrib = 0;
  EFI_HANDLE *Handle = NULL;
  EFI_LBA Lba;
  UINT64 PagesPerBlock;

  if (Msg == NULL)
    return EFI_INVALID_PARAMETER;

  BlkIOAttrib = BLK_IO_SEL_PARTITIONED_GPT;
  BlkIOAttrib |= BLK_IO_SEL_MEDIA_TYPE_NON_REMOVABLE;
  BlkIOAttrib |= BLK_IO_SEL_MATCH_PARTITION_TYPE_GUID;

  HandleFilter.RootDeviceType = NULL;
  HandleFilter.PartitionType = Ptype;
  HandleFilter.VolumeName = NULL;

  MaxHandles = ARRAY_SIZE (HandleInfoList);
  Status =
      GetBlkIOHandles (BlkIOAttrib, &HandleFilter, HandleInfoList, &MaxHandles);

  if (Status == EFI_SUCCESS) {
    if (MaxHandles == 0)
      return EFI_NO_MEDIA;

    if (MaxHandles != 1) {
      // Unable to deterministically load from single partition
      DEBUG ((EFI_D_INFO, "%s: multiple partitions found.\r\n", __func__));
      return EFI_LOAD_ERROR;
    }
  } else {
    DEBUG ((EFI_D_ERROR,
            "%s: GetBlkIOHandles failed: %r\n", __func__, Status));
    return Status;
  }

  BlkIo = HandleInfoList[0].BlkIo;
  Handle = HandleInfoList[0].Handle;
  PagesPerBlock = BlkIo->Media->BlockSize / EFI_PAGE_SIZE;
  Lba = PageOffset * PagesPerBlock;
  Status = WriteBlockToNandPartition (BlkIo, Handle, Lba, MsgSize, Msg);

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR,
          "Write the Msg failed :%r\n", Status));
  }

  return Status;
}

BOOLEAN IsNandABAttrSupport (VOID)
{
  INT32 Index = INVALID_PTN;

  if (CheckRootDeviceType () == NAND) {
    Index = GetPartitionIndex ((CHAR16 *)L"nand_ab_attr");
    if (Index == INVALID_PTN) {
      return FALSE;
    } else {
      return TRUE;
    }
  }
  return FALSE;
}

EFI_STATUS NandGetActiveSlot (Slot *ActiveSlot)
{
  EFI_GUID Ptype;
  EFI_STATUS Status;
  UINT32 PageSize;
  Slot Slots[] = {{L"_a"}, {L"_b"}};

  GetPageSize (&PageSize);
  Status = GetNandABAttrPartiGuid (&Ptype);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (!NandAttr) {
    GetPageSize (&PageSize);
    NandAttr = AllocateZeroPool (PageSize);
    if (!NandAttr) {
      DEBUG ((EFI_D_ERROR, "Error allocation attribute struct.\n"));
      return EFI_OUT_OF_RESOURCES;
    }

    Status = ReadFromNandPartition (&Ptype, (VOID *)NandAttr, PageSize, 0);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "Error Reading attributes from nand_ab_attr "
                       "partition: %r\n", Status));
      return Status;
    }
  }

  DEBUG ((EFI_D_INFO, "NAND cookie 0x%llx, slot 0x%llx\n",
               NandAttr->Cookie, NandAttr->SlotName));

  if (!NandSlotAttrValid ()) {
    NandInitSlot (ActiveSlot, Slots[0].Suffix);
    return Status;
  }

  switch (NandAttr->Cookie) {

    case NAND_COOKIE_UPDATER_MAGIC:
       if (NandAttr->SlotName == NAND_SLOT_B_MAGIC) {
          NandInitSlot (ActiveSlot, Slots[1].Suffix);
       } else {
          NandInitSlot (ActiveSlot, Slots[0].Suffix);
       }
       break;

    case NAND_COOKIE_BL_MAGIC:
       if (NandAttr->SlotName == NAND_SLOT_B_MAGIC) {
          NandInitSlot (ActiveSlot, Slots[1].Suffix);
       } else {
          NandInitSlot (ActiveSlot, Slots[0].Suffix);
       }
       break;
  }

  return Status;
}

EFI_STATUS
NandSetActiveSlot (Slot *NewSlot)
{
  EFI_GUID Ptype;
  EFI_STATUS Status;
  Slot Slots[] = {{L"_a"}, {L"_b"}};
  UINT32 PageSize;

  GetPageSize (&PageSize);
  Status = GetNandABAttrPartiGuid (&Ptype);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  NandAttr->Cookie = NAND_COOKIE_BL_MAGIC;

  if (!StrnCmp (NewSlot->Suffix, Slots[0].Suffix, StrLen (Slots[0].Suffix))) {
    NandAttr->SlotName = NAND_SLOT_A_MAGIC;
  } else {
    NandAttr->SlotName = NAND_SLOT_B_MAGIC;
  }

  NandAttr->BootSuccess = 1;

  Status = WriteToNandPartition (&Ptype, (VOID *)NandAttr,
                                       sizeof (struct NandABAttr), 0);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Error Writing attributes to misc partition: %r\n",
               Status));
    return Status;
  }

  return Status;
}

EFI_STATUS NandSwitchSlot (VOID)
{
  EFI_STATUS Status;
  Slot Slots[] = {{L"_a"}, {L"_b"}};

  if (IsAlternateSlotUnbootable ()) {
    DEBUG ((EFI_D_ERROR, "NAND slot switch disabled\n"));
    return EFI_NOT_FOUND;
  }

  if (NandAttr->SlotName == NAND_SLOT_B_MAGIC) {
    Status = NandSetActiveSlot (&Slots[0]);
  } else {
    Status = NandSetActiveSlot (&Slots[1]);
  }

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "NandSetActiveSlot Failed\n"));
  }

  return Status;
}

BOOLEAN IsNandBootAfterOTA (VOID)
{
  if (NandAttr->Cookie == NAND_COOKIE_UPDATER_MAGIC) {
    return TRUE;
  }
  return FALSE;
}

/*
 * BootSuccess = 0 specifies the boot was success.
 */
UINT64 GetNandBootSuccess (VOID)
{
  return (NandAttr->BootSuccess) ? 0 : 1;
}

/*
 * RetryCount is valid only after an OTA.
 * Return 1 in other cases, allow boot with the current slot.
 */
UINT64 GetNandRetryCount (VOID)
{
  if (IsNandBootAfterOTA ()) {
    return NandAttr->RetryCount;
  }
  return 1;
}

EFI_STATUS NandUpdateRetryCount (VOID)
{
  EFI_GUID Ptype;
  EFI_STATUS Status;

  Status = GetNandABAttrPartiGuid (&Ptype);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  NandAttr->RetryCount -= 1;
  NandAttr->BootSuccess = 1;

  Status = WriteToNandPartition (&Ptype, (VOID *)NandAttr,
                               sizeof (struct NandABAttr), 0);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Error Writing nand_ab_attr partition: %r\n", Status));
    return Status;
  }

  return Status;
}

#endif
