/*$T plugins.h GC 1.136 02/28/02 09:02:31 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

#ifndef _PLUGINS_H__1964_
#define _PLUGINS_H__1964_

#define PLUGIN_TYPE_RSP			1
#define PLUGIN_TYPE_GFX			2
#define PLUGIN_TYPE_AUDIO		3
#define PLUGIN_TYPE_CONTROLLER	4

typedef struct
{
	uint16	Version;
	uint16	Type;
	char	Name[100];

	int		NormalMemory;
	int		MemoryBswaped;
} PLUGIN_INFO;

typedef struct
{
	HWND	hWnd;
	HWND	hStatusBar;
	int		MemoryBswaped;
	_int8	*HEADER;
	_int8	*RDRAM;
	_int8	*DMEM;
	_int8	*IMEM;
	uint32	*MI_INTR_RG;
	uint32	*DPC_START_RG;
	uint32	*DPC_END_RG;
	uint32	*DPC_CURRENT_RG;
	uint32	*DPC_STATUS_RG;
	uint32	*DPC_CLOCK_RG;
	uint32	*DPC_BUFBUSY_RG;
	uint32	*DPC_PIPEBUSY_RG;
	uint32	*DPC_TMEM_RG;
	uint32	*VI_STATUS_RG;
	uint32	*VI_ORIGIN_RG;
	uint32	*VI_WIDTH_RG;
	uint32	*VI_INTR_RG;
	uint32	*VI_V_CURRENT_LINE_RG;
	uint32	*VI_TIMING_RG;
	uint32	*VI_V_SYNC_RG;
	uint32	*VI_H_SYNC_RG;
	uint32	*VI_LEAP_RG;
	uint32	*VI_H_START_RG;
	uint32	*VI_V_START_RG;
	uint32	*VI_V_BURST_RG;
	uint32	*VI_X_SCALE_RG;
	uint32	*VI_Y_SCALE_RG;
	void (*CheckInterrupts) (void);
}
GFX_INFO;

/* Note: BOOL, BYTE, WORD, DWORD, TRUE, FALSE are defined in windows.h */
#define PLUGIN_TYPE_AUDIO	3

#define EXPORT				__declspec(dllexport)
#define CALL				_cdecl

#define TV_SYSTEM_NTSC		1
#define TV_SYSTEM_PAL		0
#define TV_SYSTEM_MPAL		0

typedef struct
{
	HWND		hwnd;
	HINSTANCE	hinst;
	BOOL		MemoryBswaped;	/* If this is set to TRUE, then the memory has been pre bswapped on a dword (32 bits)
								 * boundry eg. the first 8 bytes are stored like this: 4 3 2 1 8 7 6 5 */
	BYTE		*HEADER;		/* This is the rom header (first 40h bytes of the rom */

	/* This will be in the same memory format as the rest of the memory. */
	BYTE		*__RDRAM;
	BYTE		*__DMEM;
	BYTE		*__IMEM;
	DWORD		*__MI_INTR_REG;
	DWORD		*__AI_DRAM_ADDR_REG;
	DWORD		*__AI_LEN_REG;
	DWORD		*__AI_CONTROL_REG;
	DWORD		*__AI_STATUS_REG;
	DWORD		*__AI_DACRATE_REG;
	DWORD		*__AI_BITRATE_REG;

	void (*CheckInterrupts) (void);
}
AUDIO_INFO;

/* Controller plugin's */
#define PLUGIN_NONE			1
#define PLUGIN_MEMPAK		2
#define PLUGIN_RUMBLE_PAK	3	/* not implemeted for non raw data */
#define PLUGIN_TANSFER_PAK	4	/* not implemeted for non raw data */

typedef struct
{
	BOOL	Present;
	BOOL	RawData;
	int		Plugin;
} CONTROL;

typedef union
{
	DWORD	Value;
	struct
	{
		unsigned	R_DPAD : 1;
		unsigned	L_DPAD : 1;
		unsigned	D_DPAD : 1;
		unsigned	U_DPAD : 1;
		unsigned	START_BUTTON : 1;
		unsigned	Z_TRIG : 1;
		unsigned	B_BUTTON : 1;
		unsigned	A_BUTTON : 1;
		unsigned	R_CBUTTON : 1;
		unsigned	L_CBUTTON : 1;
		unsigned	D_CBUTTON : 1;
		unsigned	U_CBUTTON : 1;
		unsigned	R_TRIG : 1;
		unsigned	L_TRIG : 1;
		unsigned	Reserved1 : 1;
		unsigned	Reserved2 : 1;
		signed		Y_AXIS : 8;
		signed		X_AXIS : 8;
	};
} BUTTONS;

extern GFX_INFO		Gfx_Info;
extern AUDIO_INFO	Audio_Info;
extern CONTROL		Controls[4];
#endif
