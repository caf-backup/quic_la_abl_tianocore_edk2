/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
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

#include "avb_slot_verify.h"
#include "avb_chain_partition_descriptor.h"
#include "avb_footer.h"
#include "avb_hash_descriptor.h"
#include "avb_kernel_cmdline_descriptor.h"
#include "avb_sha.h"
#include "avb_util.h"
#include "avb_vbmeta_image.h"
#include "avb_version.h"
#include "BootStats.h"

#define IMAGE_SPLIT_SIZE 2

static AvbSlotVerifyResult Load_partition_to_verify (
    AvbOps* ops,
    char* part_name,
    int64_t Offset,
    uint8_t* image_buf,
    uint64_t ImageSize) {
  AvbIOResult IoRet;
  AvbSlotVerifyResult Ret = AVB_SLOT_VERIFY_RESULT_OK;
  size_t PartNumRead = 0;

  IoRet = ops->read_from_partition (
      ops, part_name, Offset, ImageSize, (image_buf + Offset), &PartNumRead);
  if (IoRet == AVB_IO_RESULT_ERROR_OOM) {
    Ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
    goto out;
  } else if (IoRet != AVB_IO_RESULT_OK) {
    avb_errorv (part_name, ": Error loading data from partition.\n", NULL);
    Ret = AVB_SLOT_VERIFY_RESULT_ERROR_IO;
    goto out;
  }
  if (PartNumRead != ImageSize) {
    avb_errorv (part_name, ": Read fewer than requested bytes.\n", NULL);
    Ret = AVB_SLOT_VERIFY_RESULT_ERROR_IO;
    goto out;
  }
out:
  return Ret;
}

static AvbSlotVerifyResult VerifyPartitionSha256 (
    AvbSHA256Ctx* Sha256Ctx,
    char* part_name,
    const  UINT8* DescDigest,
    uint32_t DescDigestLen,
    uint8_t* image_buf,
    uint64_t ImageSize,
    bool IsFinal) {
  uint8_t *digest = NULL ;
  size_t  digest_len = 0;
  AvbSlotVerifyResult Ret = AVB_SLOT_VERIFY_RESULT_OK;

  avb_sha256_update (Sha256Ctx, image_buf, ImageSize);
  if (IsFinal == true) {
    digest = avb_sha256_final (Sha256Ctx);
    digest_len = AVB_SHA256_DIGEST_SIZE;
    if (digest_len != DescDigestLen) {
      avb_errorv (
          part_name, ": Digest in descriptor not of expected size.\n", NULL);
      Ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
      goto out;
    }
    if (avb_safe_memcmp (digest, DescDigest, digest_len) != 0) {
      avb_errorv (part_name,
                 ": Hash of data does not match digest in descriptor.\n",
                 NULL);
      Ret = AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION;
      goto out;
    } else {
      avb_debugv (part_name, ": success: Image verification completed\n", NULL);
      goto out;
    }
  } else {
      avb_debugv (part_name, ": success: Image verification in parts\n", NULL);
      Ret = AVB_SLOT_VERIFY_RESULT_OK;
      goto out;
  }
out:
  return Ret;
}

static AvbSlotVerifyResult VerifyPartitionSha512 (
    AvbSHA512Ctx* Sha512Ctx,
    char* part_name,
    const UINT8* DescDigest,
    uint32_t DescDigestLen,
    uint8_t* image_buf,
    uint64_t ImageSize,
    bool IsFinal) {
  uint8_t *digest = NULL ;
  size_t  digest_len = 0;
  AvbSlotVerifyResult Ret = AVB_SLOT_VERIFY_RESULT_OK ;

  avb_sha512_update (Sha512Ctx, image_buf, ImageSize);
  if (IsFinal == true) {
    digest = avb_sha512_final (Sha512Ctx);
    digest_len = AVB_SHA512_DIGEST_SIZE;
    if (digest_len != DescDigestLen) {
      avb_errorv (
          part_name, ": Digest in descriptor not of expected size.\n", NULL);
      Ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
      goto out;
    }
    if (avb_safe_memcmp (digest, DescDigest, digest_len) != 0) {
      avb_errorv (part_name,
                 ": Hash of data does not match digest in descriptor.\n",
                 NULL);
      Ret = AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION;
      goto out;
    } else {
      avb_debugv (part_name, ": success: Image verification completed\n", NULL);
      goto out;
    }
  } else {
      avb_debugv (part_name, ": success: Image verification in parts\n", NULL);
      Ret = AVB_SLOT_VERIFY_RESULT_OK;
      goto out;
  }
out:
  return Ret;
}

AvbSlotVerifyResult LoadAndVerifyBootHashPartition (
    AvbOps* ops,
    AvbHashDescriptor HashDesc,
    char* part_name,
    const UINT8* DescDigest,
    const UINT8* DescSalt,
    uint8_t* image_buf,
    uint64_t ImageSize) {
  AvbSHA256Ctx Sha256Ctx;
  AvbSHA512Ctx Sha512Ctx;
  AvbSlotVerifyResult Ret;
  uint64_t ImagePartLoop = 0;
  uint64_t ImageOffset = 0;
  uint64_t SplitImageSize = 0;
  uint64_t RemainImageSize = 0;
  bool Sha256Hash = false;
  bool IsFinal = false;

  if (image_buf == NULL) {
    Ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
    goto out;
  }

  if (Avb_StrnCmp ( (CONST CHAR8*)HashDesc.hash_algorithm, "sha256",
                 avb_strlen ("sha256")) == 0) {
    Sha256Hash  = true;
    avb_sha256_init (&Sha256Ctx);
    avb_sha256_update (&Sha256Ctx, DescSalt, HashDesc.salt_len);
  } else if (Avb_StrnCmp ( (CONST CHAR8*)HashDesc.hash_algorithm, "sha512",
                  avb_strlen ("sha512")) == 0) {
    avb_sha512_init (&Sha512Ctx);
    avb_sha512_update (&Sha512Ctx, DescSalt, HashDesc.salt_len);
  } else {
    avb_errorv (part_name, ": Unsupported hash algorithm.\n", NULL);
    Ret = AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA;
    goto out;
  }
  /*Dividing boot image to two chuncks*/
  SplitImageSize = ImageSize / IMAGE_SPLIT_SIZE;
  RemainImageSize = ImageSize % IMAGE_SPLIT_SIZE;
  ImageOffset = 0;
  for (ImagePartLoop = 1; ImagePartLoop <= IMAGE_SPLIT_SIZE; ImagePartLoop++) {
    if (IMAGE_SPLIT_SIZE == ImagePartLoop) { /*True: if final split of image*/
      IsFinal = true;
      SplitImageSize += RemainImageSize;
    }
    Ret = Load_partition_to_verify (ops,
          part_name,
          ImageOffset,
          image_buf,
          SplitImageSize);
    if (Ret != AVB_SLOT_VERIFY_RESULT_OK) {
      goto out;
    }

    if (Sha256Hash == true) {
      if (IMAGE_SPLIT_SIZE == ImagePartLoop) {
        /*Final split of Image*/
        Ret = VerifyPartitionSha256 (&Sha256Ctx,
                                      part_name,
                                      DescDigest,
                                      HashDesc.digest_len,
                                      (image_buf + ImageOffset),
                                      SplitImageSize,
                                      IsFinal);
        if (Ret != AVB_SLOT_VERIFY_RESULT_OK) {
          goto out;
        }
      } else {
        Ret = VerifyPartitionSha256 (&Sha256Ctx,
                                      part_name,
                                      DescDigest,
                                      HashDesc.digest_len,
                                      (image_buf + ImageOffset),
                                      SplitImageSize,
                                      IsFinal);
        if (Ret != AVB_SLOT_VERIFY_RESULT_OK) {
          goto out;
        }
        ImageOffset += SplitImageSize;
        continue;
      }
    } else {
      if (IMAGE_SPLIT_SIZE == ImagePartLoop) {
        Ret = VerifyPartitionSha512 (&Sha512Ctx,
                                      part_name,
                                      DescDigest,
                                      HashDesc.digest_len,
                                      (image_buf + ImageOffset),
                                      SplitImageSize,
                                      IsFinal);
        if (Ret != AVB_SLOT_VERIFY_RESULT_OK) {
          goto out;
        }
      } else {
        Ret = VerifyPartitionSha512 (&Sha512Ctx,
                                      part_name,
                                      DescDigest,
                                      HashDesc.digest_len,
                                      (image_buf + ImageOffset),
                                      SplitImageSize,
                                      IsFinal);
        if (Ret != AVB_SLOT_VERIFY_RESULT_OK) {
          goto out;
        }
        ImageOffset += SplitImageSize;
        continue;
      }
    }
  }
  Ret = AVB_SLOT_VERIFY_RESULT_OK;
out:
  return Ret;
}

