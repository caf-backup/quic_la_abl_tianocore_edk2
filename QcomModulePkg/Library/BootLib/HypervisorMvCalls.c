/* Copyright (c) 2018, The Linux Foundation. All rights reserved.
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

#include <Library/HypervisorMvCalls.h>

/* From Linux Kernel asm/system.h */
#define __asmeq(x, y) ".ifnc " x "," y " ; .err ; .endif\n\t"

/**
 *
 * Control a pipe, including reset, ready and halt functionality.
 *
 * @param pipe_id
 *    The capability identifier of the pipe.
 * @param control
 *    The state control argument.
 *
 * @retval error
 *    The returned error code.
 *
 */
UINT32 HvcSysPipeControl(UINT32 PipeId, UINT32 Control)
{
    register UINT32 x0 __asm__("x0") = (UINT32)PipeId;
    register UINT32 x1 __asm__("x1") = (UINT32)Control;
    __asm__ volatile(
        __asmeq("%0", "x0")
        __asmeq("%1", "x1")
        "hvc #5146 \n\t"
        : "+r"(x0), "+r"(x1)
        :
        : "cc", "memory", "x2", "x3", "x4", "x5"
        );

    return (UINT32)x0;
}

/**
 *
 * Send a message to a microvisor pipe.
 *
 * @param pipe_id
 *    The capability identifier of the pipe.
 * @param size
 *    Size of the message to send.
 * @param data
 *    Pointer to the message payload to send.
 *
 * @retval error
 *    The returned error code.
 *
 */
UINT32 HvcSysPipeSend(UINT32 PipeId, UINT32 Size, const UINT8 *Data)
{
    register UINT32 x0 __asm__("x0") = (UINT32)PipeId;
    register UINT32 x1 __asm__("x1") = (UINT32)Size;
    register UINT32 x2 __asm__("x2") = (UINT32)(UINTN)Data;
    __asm__ volatile(
        __asmeq("%0", "x0")
        __asmeq("%1", "x1")
        __asmeq("%2", "x2")
        "hvc #5148 \n\t"
        : "+r"(x0), "+r"(x1), "+r"(x2)
        :
        : "cc", "memory", "x3", "x4", "x5"
        );


    return (UINT32)x0;
}
