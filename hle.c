/*$T hle.c GC 1.136 03/09/02 17:44:29 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Some experimental hle of some OS functions (very beginning primitive stuff). HLE OS is not necessary, but it will
    be interesting to see if it can help speed much.
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */


/*
 * 1964 Copyright (C) 1999-2002 Joel Middendorf, <schibo@emulation64.com> This
 * program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details. You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. To contact the
 * authors: email: schibo@emulation64.com, rice1964@yahoo.com
 */
#include "globals.h"
#include "hardware.h"
#include "dynarec/opcodedebugger.h"
#include "dynarec/x86.h"
#include "memory.h"
#include "dynarec/regcache.h"
#include "hle.h"
#include "hardware.h"

__declspec(naked)

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void osDisableInt(void)
{
	__asm
	{
		mov eax, [_status_] /* mfc0 t0,Status */
		mov ecx, eax
		and eax, 1			/* andi v0,t0,0x1 */
		and cl, 0xfffffffe	/* and t1,t0,at */
		mov[_v0_], eax		/* flush Return value */
		mov[_status_], ecx	/* mtc0 t1,Status */
		ret
	}
}

__declspec(naked)

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void osDispatchThread(void)
{
	_asm
	{

		ret 0
	}
}

__declspec(naked)

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void osPopThread(void)
{
	_asm
	{
		mov eax, [_a0_]
		mov eax, [eax + 0xA0000000]

		/*
		 * mov ecx, [eax+0x80000000] £
		 * mov ecx, [ecx+0xA0000000]
		 */
		mov[_v0_], eax	/* lw v0, [a0] */

		/* mov [_t9_], ecx ; lw t9, [v0] */
		ret 0
	}
}

__declspec(naked)

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void osEnqueueAndYield(void)
{
	__asm
	{
		mov edx, 0x203359b0			/* lui a1,0xffff8033 */
		mov edx, [edx]				/* lw a1,[a1+0x59B0] */
		mov eax, [_status_]			/* mfc0 t0,Status */
		mov ecx, [edx + 0xA0000018] /* lw k1,[a1+0x18] */
		mov ebx, eax
		or ebx, 2					/* ori k0,t0,0x2 */

		mov[edx + 0xA0000118], eax	/* sw t0,[a1+0x118] */
		mov edi, [_s0_]				/* sd s0,[a1+0x098] */
		mov esi, [_s0_ + 4]

		add edx, 0xA0000098

		mov[edx + 4], edi
		mov[edx], esi
		mov edi, [_s1_]				/* sd s1,[a1+0x0A0] */
		mov esi, [_s1_ + 4]
		mov[edx + 12], edi
		mov[edx + 8], esi
		mov edi, [_s2_]				/* sd s2,[a1+0x0A8] */
		mov esi, [_s2_ + 4]
		mov[edx + 20], edi
		mov[edx + 16], esi
		mov edi, [_s3_]				/* sd s3,[a1+0x0B0] */
		mov esi, [_s3_ + 4]
		mov[edx + 28], edi
		mov[edx + 24], esi

		mov edi, [_s4_]				/* sd s4,[a1+0x0B8] */
		mov esi, [_s4_ + 4]
		mov[edx + 36], edi
		mov[edx + 32], esi
		sub edx, 0xA0000098

		mov edi, [_s5_]				/* sd s5,[a1+0x0C0] */
		mov esi, [_s5_ + 4]
		mov[edx + 0xA00000C4], edi
		mov[edx + 0xA00000C0], esi
		mov edi, [_s6_]				/* sd s6,[a1+0x0C8] */
		mov esi, [_s6_ + 4]
		mov[edx + 0xA00000CC], edi
		mov[edx + 0xA00000C8], esi
		mov edi, [_s7_]				/* sd s7,[a1+0x0D0] */
		mov esi, [_s7_ + 4]
		mov[edx + 0xA00000D4], edi
		mov[edx + 0xA00000D0], esi
		mov edi, [_gp_]				/* sd gp,[a1+0x0E8] */
		mov esi, [_gp_ + 4]
		mov[edx + 0xA00000EC], edi
		mov[edx + 0xA00000E8], esi
		mov edi, [_sp_]				/* sd sp,[a1+0x0F0] */
		mov esi, [_sp_ + 4]
		mov[edx + 0xA00000F4], edi
		mov[edx + 0xA00000F0], esi
		mov edi, [_s8_]				/* sd s8,[a1+0x0F8] */
		mov esi, [_s8_ + 4]
		mov[edx + 0xA00000FC], edi
		mov[edx + 0xA00000F8], esi
		mov edi, [_ra_]				/* sd ra,[a1+0x100] */
		mov esi, [_ra_ + 4]
		mov[edx + 0xA0000104], edi
		mov[edx + 0xA0000100], esi
		mov[edx + 0xA000011C], edi	/* sw ra,[a1+0x11C] */
		test ecx, ecx				/* beq k1,r0,80327Cf0 */
		jz _true1
		mov ecx, [_COP1Con31_]		/* cfc1 k1,con31 */
		mov edi, [edx + 0xA0000180] /* ldc1 fp21,[a1+0x180] */
		mov esi, [edx + 0xA0000184]
		mov[_f21_hi_], edi
		mov[_f21_lo_], esi
		mov edi, [edx + 0xA0000188] /* ldc1 fp23,[a1+0x188] */
		mov esi, [edx + 0xA000018C]
		mov[_f23_hi_], edi
		mov[_f23_lo_], esi
		mov edi, [edx + 0xA0000190] /* ldc1 fp25,[a1+0x190] */
		mov esi, [edx + 0xA0000194]
		mov[_f25_hi_], edi
		mov[_f25_lo_], esi
		mov edi, [edx + 0xA0000198] /* ldc1 fp27,[a1+0x198] */
		mov esi, [edx + 0xA000019C]
		mov[_f27_hi_], edi
		mov[_f27_lo_], esi
		mov edi, [edx + 0xA00001A0] /* ldc1 fp29,[a1+0x1A0] */
		mov esi, [edx + 0xA00001A4]
		mov[_f29_hi_], edi
		mov[_f29_lo_], esi
		mov edi, [edx + 0xA00001A8] /* ldc1 fp31,[a1+0x1A8] */
		mov esi, [edx + 0xA00001AC]
		mov[_f31_hi_], edi
		mov[_f31_lo_], esi
		mov[edx + 0xA000012C], ecx	/* sw k1,[a1+0x12C] */

		/* flush 32bits */
		_true1 :
		mov eax, 0xA430000C			/* lui k1,0xffffA430 */
		mov edi, eax
		shr edi, SHIFTER2_READ
		call memory_read_functions[edi * 4]
		mov ecx, [eax]				/* lw k1,[k1+0xC] (mi_intr_mask_reg) */

		/*
		 * 30 £
		 * mov [edx+0xA0000128], ecx ;sw k1,[a1+0x128] £
		 * flush
		 */
		mov[_k1_], ecx
		mov[_k0_], ebx
		mov[_a1_], edx

		/*
		 * //flush. mov [_k1_], ecx mov [_k0_], ebx mov edi, [_a0_] push edi and edi,
		 * 0x0fffffff cmp edi, 0 jz _false2 ;beq a0,r0,_false2 ;jal 80327D10 ;nop ;j
		 * _false2 ;nop _true2: and edi, 0x0fffffff mov esi, [edi+0x20000000] ;lw t8,[a0]
		 * mov eax, [edx+0xA0000004] ;lw t7,[a1+0x4] mov ecx, edi ;or t9,a0,r0 and ecx,
		 * 0x0fffffff push esi and esi, 0x0fffffff mov ebx, [esi+0x20000004] ;lw
		 * t6,[t8+0x4] pop esi cmp eax, ebx ;slt at,t6,t7 jl _true3 ;bne at,r0,_true3 ;nop
		 * _loop: or ecx, esi ;or t9,t8,r0 push esi and esi, 0x0fffffff mov esi,
		 * [esi+0x20000000] ;lw t8,[t8] and esi, 0x0fffffff mov ebx, [esi+0x20000004] ;lw
		 * t6,[t8+4] pop esi cmp eax, ebx ;slt at,t6,t7 jge _loop ;beq at,r0,_loop ;nop
		 * _true3: push ecx and ecx, 0x0fffffff mov esi, [ecx+0x20000000] ;lw t8,[t9] mov
		 * [edx+0xA0000000], esi ;sw t8,[a1] mov [ecx+0x20000000], edx ;sw a1,[t9] pop ecx
		 * pop edi mov [edx+0xA0000008], edi ;sw a0,[a1+0x8] push edi jmp _true2 ;jr
		 * _true2 // ;sw a0,[a1+0x8] // ;lw v0,[a0] // ;lw t9,[v0] // ;jr ra..sw t9,[a0]
		 * //flush // mov [_t6_], ebx // mov [_t7_], eax // mov [_t8_], esi // mov [_t9_],
		 * ecx _false2: pop edi mov [_a1_], edx shr edx, 31 mov [_a1_+4], edx
		 */
		ret
	}
}

__declspec(naked)

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void osRestoreInt(void)
{
	__asm
	{
		mov eax, [_a0_]		/* mfc0 t0,Status */
		or[_status_], eax	/* or t0,t0,a0..mtc0 t0,Status, nop, nop */
		ret
	}
}

__declspec(naked)

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void osSendMessage(void)
{
	__asm
	{
		mov esi, dword ptr[_sp_]	/* addiu sp,sp,0xffffffc8 */
		lea esi, [esi + 0xA0000000 + 0xffffffc8]
		mov ebx, [_s2_]				/* sw s2,[sp+0x20] */
		mov eax, [_ra_]				/* sw ra,[sp+0x24] */
		mov ecx, [_a0_]				/* sw a0,[sp+0x38] */
		mov edi, [_a1_]				/* sw a1,[sp+0x3C] */
		mov[esi + 20], ebx
		mov[esi + 0x24], eax
		mov[esi + 0x38], ecx
		mov[esi + 0x3C], edi
		mov eax, [_a2_]				/* sw a2,[sp+0x40] */
		mov ecx, [_s1_]				/* sw s1,[sp+0x1C] */
		mov edx, [_s0_]				/* sw s0,[sp+0x18] */
		mov[esi + 0x40], eax
		mov[esi + 0x1C], eax
		mov[esi + 0x18], eax
		call osDisableInt			/* jal osDisableInt */
		mov ecx, [esi + 0x38]		/* lw t6,[sp+0x38] */
		mov ebx, [_v0_]				/* or s0,v0,r0 */
		mov edx, [ecx + 0xA0000008] /* lw t7,[t6+0x8] */
		mov eax, [ecx + 0xA0000010] /* lw t8,[t6+0x10] */
		xor edi, edi				/* slt at, t7,t8 */
		cmp edx, eax
		jl _true

		/* flush 32bits */
		mov esi, [_sp_]
		add esi, 0xffffffc8			/* restore stack pointer */
		mov[_at_], edi
		mov[_sp_], esi
		mov[_s0_], ebx
		ret
		_true : inc edi

		/* flush 32bits */
		mov esi, dword ptr[_sp_]
		add esi, 0xffffffc8			/* restore stack pointer */
		mov[_at_], edi
		mov[_sp_], esi
		mov[_s0_], ebx
		ret
	}
}
