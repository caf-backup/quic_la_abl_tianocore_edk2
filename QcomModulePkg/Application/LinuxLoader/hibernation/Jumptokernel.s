/* Copyright (c) 2021, The Linux Foundation. All rights reserved.
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

#if HIBERNATION_SUPPORT

.globl JumpToKernel;

/*
 * x18 = bounce_pfn_entry_table
 * x19 = bounce_count
 * x20 = bounce_start
 * x21 = cpu_resume
 * x22 = PreparePlatformHardware
 */
JumpToKernel:
	ldp	x4, x5, [x18], #16		// x4 = dst_pfn, x5 = pages, post increment x18
	bl	copy				// copy pages
	sub	x19, x19, #1			// decrement bounce_count
	cbnz	x19, JumpToKernel		// loop until bounce_count equals 0
	blr	x22				// call PreparePlatformHardware
	br	x21
/*
 * copy pages
 * x2 - bounce buffer
 * x4 - dst_pfn
 * x5 - no of pages
 */
copy:
	lsl 	x4, x4, #12			// convert pfn to address
	lsl 	x5, x5, #12			// convert no of pages to bytes
1:	ldp	x6, x7, [x20], #16		// x6 = [x20], x7 = [x2 + 8 bytes], post increment x20
	stp 	x6, x7, [x4], #16		// x6 -> [x4], x7 -> [x4 + 8 bytes], post increment x4
	sub 	x5, x5, #16			// reduce copied bytes from size
	cbnz	x5, 1b
	ret

#endif
