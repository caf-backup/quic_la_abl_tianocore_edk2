/***************************************************************************
Copyright (c) 2021 The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************/

#include "aes_public.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <Library/DebugLib.h>


#define E_SUCCESS 0
#define E_FAILURE 1

uint8 plain_text[8] ;

unsigned char key[32] = {0x49, 0x43, 0x80, 0x58, 0x43, 0x22, 0x20, 0x95,
               0x83, 0x40, 0x53, 0x40, 0x85, 0x03, 0x94, 0x85,
               0x09, 0x34, 0x85, 0x09, 0x83, 0x40, 0x95, 0x80,
               0x95, 0x43, 0x20, 0x95, 0x34, 0x58, 0x45, 0x85} ;

unsigned char iv[16] = {0x45, 0x80, 0x43, 0x98, 0x50, 0x23, 0x49, 0x52,
               0x48, 0x50, 0x43, 0x98, 0x59, 0x43, 0x05, 0x84} ;

unsigned char aad[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
uint8 expected_cipher_text[8] = {0x4D, 0x29, 0x53, 0x7F, 0x84, 0x35, 0x0E, 0x4B};
unsigned char expected_tag[16] = {0x48, 0xB6, 0x3D, 0x2B, 0x52, 0x25, 0x56, 0xA9, 0x12, 0xCE, 0x59, 0x0E, 0xB0, 0xAD, 0xD7, 0xA2};

/* enable below for -ve testing */
/* unsigned char expected_tag[16] = {0xCB, 0xB6, 0xD2, 0x8A, 0xF1, 0xC0, 0x62, 0xCE, 0x0F, 0xE5, 0x64, 0x15, 0x69, 0x8F, 0x36, 0x99}; */

/**
   @brief
   Sample code for how to call QSEE AES GCM software crypto
   API's
*/
int tz_app_ufcrypto_aes_gcm_func(uintnt *pt,
                                 uint32 pt_len,
                                 uint8 *key,
                                 uint32 key_len,
                                 uint8 *iv,
                                 uint32 iv_len,
                                 uintnt *ptr_tmp,
                                 uintnt *header_data,
                                 uintnt *auth_struct)
{
   int status = E_SUCCESS;
   IovecListType   ioVecIn;
   IovecListType   ioVecOut;
   IovecType       IovecIn;
   IovecType       IovecOut;
   uint8          tag_auth[16] = {0};

   uint32 ct_len  = pt_len;
   SW_CipherEncryptDir dir = SW_CIPHER_ENCRYPT;
   SW_CipherModeType mode = SW_CIPHER_MODE_GCM;

   /*------------------------------------------------------------------
     Init ctx
     ------------------------------------------------------------------*/

   //Determine algorithm
   if(key_len == 16)
   {
      if(0 != SW_Cipher_Init( SW_CIPHER_ALG_AES128))
      {
         status = -E_FAILURE;
      }
   }
   else
   {
      if(0 != SW_Cipher_Init( SW_CIPHER_ALG_AES256))
      {
         status = -E_FAILURE;
      }
   }

   /*--------------------------------------------------------------------
     Set direction to encryption
     ----------------------------------------------------------------------*/

   if (E_SUCCESS == status &&
       SW_Cipher_SetParam( SW_CIPHER_PARAM_DIRECTION, &dir, sizeof(SW_CipherEncryptDir)) != 0)
   {
      status = -E_FAILURE;
   }
   /*--------------------------------------------------------------------
     Set AES mode
     ----------------------------------------------------------------------*/
   if (E_SUCCESS == status &&
       SW_Cipher_SetParam( SW_CIPHER_PARAM_MODE, &mode, sizeof(mode)) != 0)
   {
      status = -E_FAILURE;
   }

   /*--------------------------------------------------------------------
     Set key for encryption
     ----------------------------------------------------------------------*/
   if (E_SUCCESS == status &&
       SW_Cipher_SetParam( SW_CIPHER_PARAM_KEY, key, key_len) != 0)
   {
      status = -E_FAILURE;
   }

   /*--------------------------------------------------------------------
     Set IV
     ----------------------------------------------------------------------*/
   if((mode == SW_CIPHER_ALG_AES256) || (mode == SW_CIPHER_ALG_AES128))
   {
      if(E_SUCCESS == status &&
         SW_Cipher_SetParam( SW_CIPHER_PARAM_IV, iv, iv_len) != 0)
      {
         status = -E_FAILURE;
      }
   }

   /*--------------------------------------------------------------------
     Set AAD if present
     ----------------------------------------------------------------------*/
   if(E_SUCCESS == status &&
      SW_Cipher_SetParam( SW_CIPHER_PARAM_AAD, (void*)((uintnt)header_data[0]), header_data[1]) != 0)
   {
      status = -E_FAILURE;
   }

   /* Input IOVEC */
   for (int dec_cnt= 0; dec_cnt < 1; dec_cnt++) {
       ioVecIn.size = 1;
       ioVecIn.iov = &IovecIn;
       ioVecIn.iov[0].dwLen = pt_len/1;
       ioVecIn.iov[0].pvBase = (void*)((uintnt)pt[0]+ (dec_cnt*pt_len/1));
       /* Output IOVEC */
       ioVecOut.size = 1;
       ioVecOut.iov = &IovecOut;
       ioVecOut.iov[0].dwLen = ct_len/1;
       ioVecOut.iov[0].pvBase = (void*)((uintnt)ptr_tmp[1]+ (dec_cnt*ct_len/1));

       /*-----------------------------------------------------------------------
         Now encrypt the data
         -------------------------------------------------------------------------*/
       if (E_SUCCESS == status &&
           SW_CipherData( ioVecIn, &ioVecOut) != 0)
       {
          status = -E_FAILURE;
       }
   }
   /*-----------------------------------------------------------------------
     Get the auth tag
     -------------------------------------------------------------------------*/
   if(E_SUCCESS == status &&
      0 != SW_Cipher_GetParam( SW_CIPHER_PARAM_TAG, &tag_auth[0], 16))
   {
      status = -E_FAILURE;
   }

   if(0 != SW_Cipher_DeInit())
   {
      status = -E_FAILURE;
   }

   /*--------------------------------------------------------------------------
     If NULL key pointer then we are using HW key so don't compare encrypted result
     -----------------------------------------------------------------------------*/
   if ((E_SUCCESS == status))
   {
      if(pt[2] == 0 || pt[3] == 1)
      {
         if(pt[3] == 0)
         {
            //Now compare encrypted results
            if (CRYPTO_memcmp((void*)((uintnt)pt[1]), (void*)((uintnt)ptr_tmp[1]), ct_len) != 0)
            {
               status = -E_FAILURE;
            }
            else
              DEBUG ((EFI_D_ERROR,("**** matched with expected cypher data *****\n")));
            if(CRYPTO_memcmp(tag_auth, (void*)((uintnt)auth_struct[0]), auth_struct[1]) != 0)
            {
               status = -E_FAILURE;
            }
            else
              DEBUG ((EFI_D_ERROR,("**** matched with expected TAG after encryption *****\n")));
         }
         //Determine algorithm
         if(key_len == 16)
         {
            if(0 != SW_Cipher_Init( SW_CIPHER_ALG_AES128))
            {
               status = -E_FAILURE;
            }
         }
         else
         {
            if(0 != SW_Cipher_Init( SW_CIPHER_ALG_AES256))
            {
               status = -E_FAILURE;
            }
         }

         /*--------------------------------------------------------------------
           Set direction to decryption
           ----------------------------------------------------------------------*/

         dir = SW_CIPHER_DECRYPT;
         if (E_SUCCESS == status &&
             SW_Cipher_SetParam( SW_CIPHER_PARAM_DIRECTION, &dir, sizeof(SW_CipherEncryptDir)) != 0)
         {
            status = -E_FAILURE;
         }

         /*--------------------------------------------------------------------
           Set AES mode
           ----------------------------------------------------------------------*/
         if (E_SUCCESS == status &&
             SW_Cipher_SetParam( SW_CIPHER_PARAM_MODE, &mode, sizeof(mode)) != 0)
         {
            status = -E_FAILURE;
         }

         /*--------------------------------------------------------------------
           Set key for encryption
           ----------------------------------------------------------------------*/
         if (E_SUCCESS == status &&
             SW_Cipher_SetParam( SW_CIPHER_PARAM_KEY, key, key_len) != 0)
         {
            status = -E_FAILURE;
         }

         /*--------------------------------------------------------------------
           Set IV
           ----------------------------------------------------------------------*/
          if(E_SUCCESS == status &&
             SW_Cipher_SetParam( SW_CIPHER_PARAM_IV, iv, iv_len) != 0)
          {
             status = -E_FAILURE;
          }

         /*--------------------------------------------------------------------
           Set AAD
           ----------------------------------------------------------------------*/
         if(E_SUCCESS == status &&
            0 != SW_Cipher_SetParam( SW_CIPHER_PARAM_AAD, (void*)((uintnt)header_data[0]), header_data[1]))
         {
            status = -E_FAILURE;
         }

         /*--------------------------------------------------------------------
           Set Tag
           ----------------------------------------------------------------------*/
#if 0
         if(E_SUCCESS == status &&
            0 != SW_Cipher_SetParam( SW_CIPHER_PARAM_TAG, tag_auth, 16))
         {
            status = -E_FAILURE;
         }
#endif
         for (int dec_cnt= 0; dec_cnt < 1; dec_cnt++) {
              /* Input IOVEC */
              ioVecIn.size = 1;
              ioVecIn.iov = &IovecIn;
              ioVecIn.iov[0].dwLen = ct_len/1;
              ioVecIn.iov[0].pvBase = (void*)((uintnt)(ptr_tmp[1]+(dec_cnt*(ct_len/1))));

              /* Output IOVEC */
              ioVecOut.size = 1;
              ioVecOut.iov = &IovecOut;
              ioVecOut.iov[0].dwLen = pt_len/1;
              ioVecOut.iov[0].pvBase = (void*)((uintnt)(ptr_tmp[0]+(dec_cnt*(pt_len/1))));

              //DEBUG ((EFI_D_ERROR,("**** < decrypting '%d' bytes > *****\n", (int)ioVecIn.iov[0].dwLen)));

              /*----------------------------------------------------------------------------------
                Now decrypt the data
                ------------------------------------------------------------------------------------*/
              if (E_SUCCESS == status &&
                  SW_CipherData( ioVecIn, &ioVecOut) != 0)
              {
                 status = -E_FAILURE;
              }
              /*-------------------------------------------------------------------------------------
                Now compare decrypted results
                ---------------------------------------------------------------------------------------*/
              if ((E_SUCCESS == status))
              {
                 //Now compare decrypted results
                 if (CRYPTO_memcmp((void*)((uintnt)pt[0]), (void*)((uintnt)ptr_tmp[0]), pt_len/1) != 0)
                 {
                    status = -E_FAILURE;
                 }
                 else
                 {
                    DEBUG ((EFI_D_ERROR,("**** matched with expected plain text after decryption *****\n")));
                    status = E_SUCCESS;
                 }
              }
         }
          /*-----------------------------------------------------------------------
          Get the auth tag
          -------------------------------------------------------------------------*/
          if(E_SUCCESS == status &&
          0 != SW_Cipher_GetParam( SW_CIPHER_PARAM_TAG, &tag_auth[0], 16))
          {
             status = -E_FAILURE;
          }

          if(CRYPTO_memcmp(tag_auth, (void*)((uintnt)auth_struct[0]), auth_struct[1]) != 0)
          {
             status = -E_FAILURE;
          }

          else
           DEBUG ((EFI_D_ERROR,("**** matched with expected TAG after decryption *****\n")));

         /*-------------------------------------------------------------------------------
           Free ctx
           --------------------------------------------------------------------------------*/
           if(0 != SW_Cipher_DeInit())
           {
              status = -E_FAILURE;
           }
      }
   }

   return status;
}


/**
   @brief
   Sample code for SW AES functional tests
*/
int tz_app_sw_aes_func(void)
{
   int status = E_SUCCESS;
   uintnt ptr_struct[4] = {0,0,0,0};
   uintnt header_struct[2];
   uintnt tag_struct[2];
   uintnt ptr_tmp_struct[2];
   ptr_tmp_struct[0] = (uintnt)plain_text;
   ptr_tmp_struct[1] = (uintnt)expected_cipher_text;

   ptr_struct[0] = (uintnt)plain_text;
   memset(plain_text, 0, sizeof(plain_text));
   ptr_struct[1] = (uintnt)expected_cipher_text;
   header_struct[0] = (uintnt)aad;
   header_struct[1] = (uintnt)sizeof(aad);
   tag_struct[0] = (uintnt)expected_tag;
   tag_struct[1] = 16;
   status = tz_app_ufcrypto_aes_gcm_func(ptr_struct,
                                                     sizeof(plain_text),
                                                     key,
                                                     sizeof(key),
                                                     iv,
                                                     sizeof(iv),
                                                     ptr_tmp_struct,
                                                     header_struct,
                                                     tag_struct);
   if (status == E_SUCCESS)
   {
   }
   else
   {
     return status;
   }
   return status;
}

/**
   @brief
   Sample code to call crypto performance test
*/
int aes_start_test(void)
{
   int status = E_SUCCESS;

   if (E_SUCCESS == status && tz_app_sw_aes_func() < 0)
   {
      status = -E_FAILURE;
   }
   return status;
}
