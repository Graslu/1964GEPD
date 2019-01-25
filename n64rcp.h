/*$T n64rcp.h GC 1.136 02/28/02 08:11:31 */


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
#ifndef _N64RCP_H__1964_
#define _N64RCP_H__1964_

#include "globals.h"
#include "memory.h"

#define NUMBEROFRDRAMREG		10
#define RDRAM_CONFIG_REG		gMS_ramRegs0[0]
#define RDRAM_DEVICE_ID_REG		gMS_ramRegs0[1]
#define RDRAM_DELAY_REG			gMS_ramRegs0[2]
#define RDRAM_MODE_REG			gMS_ramRegs0[3]
#define RDRAM_REF_INTERVAL_REG	gMS_ramRegs0[4]
#define RDRAM_REF_ROW_REG		gMS_ramRegs0[5]
#define RDRAM_RAS_INTERVAL_REG	gMS_ramRegs0[6]
#define RDRAM_MIN_INTERVAL_REG	gMS_ramRegs0[7]
#define RDRAM_ADDR_SELECT_REG	gMS_ramRegs0[8]
#define RDRAM_DEVICE_MANUF_REG	gMS_ramRegs0[9]

#define SP_DMEM_START			0x04000000	/* read/write */
#define SP_DMEM_END				0x04000FFF
#define SP_IMEM_START			0x04001000	/* read/write */
#define SP_IMEM_END				0x04001FFF

#define SP_DMEM					gMS_SP_MEM[0]
#define SP_IMEM					gMS_SP_MEM[0x400]
#define SP_MEM_ADDR_REG			gMS_SP_REG_1[0x0]
#define SP_DRAM_ADDR_REG		gMS_SP_REG_1[0x1]
#define SP_RD_LEN_REG			gMS_SP_REG_1[0x2]
#define SP_WR_LEN_REG			gMS_SP_REG_1[0x3]
#define SP_STATUS_REG			gMS_SP_REG_1[0x4]
#define SP_DMA_FULL_REG			gMS_SP_REG_1[0x5]
#define SP_DMA_BUSY_REG			gMS_SP_REG_1[0x6]
#define SP_SEMAPHORE_REG		gMS_SP_REG_1[0x7]
#define SP_PC_REG				gMS_SP_REG_2[0x0]
#define SP_IBIST_REG			gMS_SP_REG_2[0x1]
#define HLE_DMEM_TASK			gMS_SP_MEM[0x03F0]
#define NUMBEROFSPREG			8

#define DPC_START_REG			gMS_DPC[0]
#define DPC_END_REG				gMS_DPC[1]
#define DPC_CURRENT_REG			gMS_DPC[2]
#define DPC_STATUS_REG			gMS_DPC[3]
#define DPC_CLOCK_REG			gMS_DPC[4]
#define DPC_BUFBUSY_REG			gMS_DPC[5]
#define DPC_PIPEBUSY_REG		gMS_DPC[6]
#define DPC_TMEM_REG			gMS_DPC[7]
#define NUMBEROFDPREG			8

#define DPS_TBIST_REG			gMS_DPS[0]
#define DPS_TEST_MODE_REG		gMS_DPS[1]
#define DPS_BUFTEST_ADDR_REG	gMS_DPS[2]
#define DPS_BUFTEST_DATA_REG	gMS_DPS[3]
#define NUMBEROFDPSREG			4

#define MI_INIT_MODE_REG_R		gMS_MI[0]
#define MI_VERSION_REG_R		gMS_MI[1]
#define MI_INTR_REG_R			gMS_MI[2]
#define MI_INTR_MASK_REG_R		gMS_MI[3]
#define NUMBEROFMIREG			4

#define VI_STATUS_REG			gMS_VI[0]
#define VI_ORIGIN_REG			gMS_VI[1]
#define VI_WIDTH_REG			gMS_VI[2]
#define VI_INTR_REG				gMS_VI[3]
#define VI_CURRENT_REG			gMS_VI[4]
#define VI_BURST_REG			gMS_VI[5]
#define VI_V_SYNC_REG			gMS_VI[6]
#define VI_H_SYNC_REG			gMS_VI[7]
#define VI_LEAP_REG				gMS_VI[8]
#define VI_H_START_REG			gMS_VI[9]
#define VI_V_START_REG			gMS_VI[10]
#define VI_V_BURST_REG			gMS_VI[11]
#define VI_X_SCALE_REG			gMS_VI[12]
#define VI_Y_SCALE_REG			gMS_VI[13]
#define NUMBEROFVIREG			14

#define AI_DRAM_ADDR_REG		gMS_AI[0]
#define AI_LEN_REG				gMS_AI[1]
#define AI_CONTROL_REG			gMS_AI[2]
#define AI_STATUS_REG			gMS_AI[3]
#define AI_DACRATE_REG			gMS_AI[4]
#define AI_BITRATE_REG			gMS_AI[5]
#define NUMBEROFAIREG			6

#define AI_STATUS_FIFO_FULL		0x80000000	/* Bit 31: full */
#define AI_STATUS_DMA_BUSY		0x40000000	/* Bit 30: busy */

#define PI_DRAM_ADDR_REG		gMS_PI[0]
#define PI_CART_ADDR_REG		gMS_PI[1]
#define PI_RD_LEN_REG			gMS_PI[2]
#define PI_WR_LEN_REG			gMS_PI[3]
#define PI_STATUS_REG			gMS_PI[4]
#define PI_BSD_DOM1_LAT_REG		gMS_PI[5]
#define PI_BSD_DOM1_PWD_REG		gMS_PI[6]
#define PI_BSD_DOM1_PGS_REG		gMS_PI[7]
#define PI_BSD_DOM1_RLS_REG		gMS_PI[8]
#define PI_BSD_DOM2_LAT_REG		gMS_PI[9]
#define PI_BSD_DOM2_PWD_REG		gMS_PI[10]
#define PI_BSD_DOM2_PGS_REG		gMS_PI[11]
#define PI_BSD_DOM2_RLS_REG		gMS_PI[12]
#define NUMBEROFPIREG			13

#define PI_STATUS_RESET			0x01
#define PI_STATUS_CLR_INTR		0x02
#define PI_STATUS_ERROR			0x04
#define PI_STATUS_IO_BUSY		0x02
#define PI_STATUS_DMA_BUSY		0x01
#define PI_STATUS_DMA_IO_BUSY	0x03

#define RI_MODE_REG				gMS_RI[0]
#define RI_CONFIG_REG			gMS_RI[1]
#define RI_CURRENT_LOAD_REG		gMS_RI[2]
#define RI_SELECT_REG			gMS_RI[3]
#define RI_REFRESH_REG			gMS_RI[4]
#define RI_LATENCY_REG			gMS_RI[5]
#define RI_RERROR_REG			gMS_RI[6]
#define RI_WERROR_REG			gMS_RI[7]
#define NUMBEROFRIREG			8

#define SI_DRAM_ADDR_REG		gMS_SI[0]
#define SI_PIF_ADDR_RD64B_REG	gMS_SI[1]
#define SI_PIF_ADDR_WR64B_REG	gMS_SI[4]
#define SI_STATUS_REG			gMS_SI[6]
#define NUMBEROFSIREG			7

#define SI_STATUS_DMA_BUSY		0x0001
#define SI_STATUS_RD_BUSY		0x0002
#define SI_STATUS_DMA_ERROR		0x0008
#define SI_STATUS_INTERRUPT		0x1000

void	RCP_Reset(void);
#endif
