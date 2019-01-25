/*$T dynaCPU_defines.h GC 1.136 03/09/02 16:22:38 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
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

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void LoadLowMipsCpuRegister(unsigned long mips_reg, unsigned char x86_reg)
{
	if(mips_reg == 0)
		XOR_Reg2ToReg1(1, x86_reg, x86_reg);
	else
	{
		FetchEBP_Params(mips_reg);
		MOV_MemoryToReg(1, x86_reg, x86params.ModRM, x86params.Address);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void LoadHighMipsCpuRegister(unsigned long mips_reg, unsigned char x86_reg)
{
	if(mips_reg == 0)
		XOR_Reg2ToReg1(1, x86_reg, x86_reg);
	else
	{
		FetchEBP_Params(mips_reg);
		MOV_MemoryToReg(1, x86_reg, x86params.ModRM, 4 + x86params.Address);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void StoreLowMipsCpuRegister(unsigned long iMipsReg, unsigned char iIntelReg)
{
	MOV_RegToMemory(1, iIntelReg, ModRM_disp32, (unsigned long) &gHWS_GPR[iMipsReg]);
}