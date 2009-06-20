/**********************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.

  (c) Copyright 1996 - 2002  Gary Henderson (gary.henderson@ntlworld.com),
                             Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2002 - 2004  Matthew Kendora

  (c) Copyright 2002 - 2005  Peter Bortas (peter@bortas.org)

  (c) Copyright 2004 - 2005  Joel Yliluoma (http://iki.fi/bisqwit/)

  (c) Copyright 2001 - 2006  John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2006  funkyass (funkyass@spam.shaw.ca),
                             Kris Bleakley (codeviolation@hotmail.com)

  (c) Copyright 2002 - 2007  Brad Jorsch (anomie@users.sourceforge.net),
                             Nach (n-a-c-h@users.sourceforge.net),
                             zones (kasumitokoduck@yahoo.com)

  (c) Copyright 2006 - 2007  nitsuja


  BS-X C emulator code
  (c) Copyright 2005 - 2006  Dreamer Nom,
                             zones

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003  _Demo_ (_demo_@zsnes.com),
                             Nach,
                             zsKnight (zsknight@zsnes.com)

  C4 C++ code
  (c) Copyright 2003 - 2006  Brad Jorsch,
                             Nach

  DSP-1 emulator code
  (c) Copyright 1998 - 2006  _Demo_,
                             Andreas Naive (andreasnaive@gmail.com)
                             Gary Henderson,
                             Ivar (ivar@snes9x.com),
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora,
                             Nach,
                             neviksti (neviksti@hotmail.com)

  DSP-2 emulator code
  (c) Copyright 2003         John Weidman,
                             Kris Bleakley,
                             Lord Nightmare (lord_nightmare@users.sourceforge.net),
                             Matthew Kendora,
                             neviksti


  DSP-3 emulator code
  (c) Copyright 2003 - 2006  John Weidman,
                             Kris Bleakley,
                             Lancer,
                             z80 gaiden

  DSP-4 emulator code
  (c) Copyright 2004 - 2006  Dreamer Nom,
                             John Weidman,
                             Kris Bleakley,
                             Nach,
                             z80 gaiden

  OBC1 emulator code
  (c) Copyright 2001 - 2004  zsKnight,
                             pagefault (pagefault@zsnes.com),
                             Kris Bleakley,
                             Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002         Matthew Kendora with research by
                             zsKnight,
                             John Weidman,
                             Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003         Brad Jorsch with research by
                             Andreas Naive,
                             John Weidman

  S-RTC C emulator code
  (c) Copyright 2001-2006    byuu,
                             John Weidman

  ST010 C++ emulator code
  (c) Copyright 2003         Feather,
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora

  Super FX x86 assembler emulator code
  (c) Copyright 1998 - 2003  _Demo_,
                             pagefault,
                             zsKnight,

  Super FX C emulator code
  (c) Copyright 1997 - 1999  Ivar,
                             Gary Henderson,
                             John Weidman

  Sound DSP emulator code is derived from SNEeSe and OpenSPC:
  (c) Copyright 1998 - 2003  Brad Martin
  (c) Copyright 1998 - 2006  Charles Bilyue'

  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004  Marcus Comstedt (marcus@mc.pp.se)

  2xSaI filter
  (c) Copyright 1999 - 2001  Derek Liauw Kie Fa

  HQ2x, HQ3x, HQ4x filters
  (c) Copyright 2003         Maxim Stepin (maxim@hiend3d.com)

  Win32 GUI code
  (c) Copyright 2003 - 2006  blip,
                             funkyass,
                             Matthew Kendora,
                             Nach,
                             nitsuja

  Mac OS GUI code
  (c) Copyright 1998 - 2001  John Stiles
  (c) Copyright 2001 - 2007  zones


  Specific ports contains the works of other authors. See headers in
  individual files.


  Snes9x homepage: http://www.snes9x.com

  Permission to use, copy, modify and/or distribute Snes9x in both binary
  and source form, for non-commercial purposes, is hereby granted without
  fee, providing that this license information and copyright notice appear
  with all copies and any derived work.

  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software or it's derivatives.

  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes,
  but is not limited to, charging money for Snes9x or software derived from
  Snes9x, including Snes9x or derivatives in commercial game bundles, and/or
  using Snes9x as a promotion for your commercial product.

  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.

  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
**********************************************************************************/


#include <assert.h>
#include "snes9x.h"
#include "memmap.h"
#include "dma.h"
#include "apu/apu.h"
//#include "soundux.h"
#include "fxinst.h"
#include "fxemu.h"
#include "sdd1.h"
#include "srtc.h"
#include "snapshot.h"
#include "controls.h"
#include "movie.h"
#include "display.h"
#include "language.h"

#ifndef min
#define min(a,b)	(((a) < (b)) ? (a) : (b))
#endif

typedef struct
{
	int			offset;
	int			offset2;
	int			size;
	int			type;
	uint16		debuted_in;
	uint16		deleted_in;
	const char	*name;
}	FreezeData;

enum
{
	INT_V,
	uint8_ARRAY_V,
	uint16_ARRAY_V,
	uint32_ARRAY_V,
	uint8_INDIR_ARRAY_V,
	uint16_INDIR_ARRAY_V,
	uint32_INDIR_ARRAY_V,
	POINTER_V
};

#define COUNT(ARRAY)				(sizeof(ARRAY) / sizeof(ARRAY[0]))
#define Offset(field, structure)	((int) (((char *) (&(((structure) NULL)->field))) - ((char *) NULL)))
#define OFFSET(f)					Offset(f, STRUCT *)
#define DUMMY(f)					Offset(f, struct Obsolete *)
#define DELETED(f)					(-1)

#define INT_ENTRY(save_version_introduced, field) \
{ \
	OFFSET(field), \
	0, \
	sizeof(((STRUCT *) NULL)->field), \
	INT_V, \
	save_version_introduced, \
	9999, \
	#field \
}

#define ARRAY_ENTRY(save_version_introduced, field, count, elemType) \
{ \
	OFFSET(field), \
	0, \
	count, \
	elemType, \
	save_version_introduced, \
	9999, \
	#field \
}

#define POINTER_ENTRY(save_version_introduced, field, relativeToField) \
{ \
	OFFSET(field), \
	OFFSET(relativeToField), \
	4, \
	POINTER_V, \
	save_version_introduced, \
	9999, \
	#field \
}

#define OBSOLETE_INT_ENTRY(save_version_introduced, save_version_removed, field) \
{ \
	DUMMY(field), \
	0, \
	sizeof(((struct Obsolete *) NULL)->field), \
	INT_V, \
	save_version_introduced, \
	save_version_removed, \
	#field \
}

#define OBSOLETE_ARRAY_ENTRY(save_version_introduced, save_version_removed, field, count, elemType) \
{ \
	DUMMY(field), \
	0, \
	count, \
	elemType, \
	save_version_introduced, \
	save_version_removed, \
	#field \
}

#define OBSOLETE_POINTER_ENTRY(save_version_introduced, save_version_removed, field, relativeToField) \
{ \
	DUMMY(field), \
	DUMMY(relativeToField), \
	4, \
	POINTER_V, \
	save_version_introduced, \
	save_version_removed, \
	#field \
}

#define DELETED_INT_ENTRY(save_version_introduced, save_version_removed, field, size) \
{ \
	DELETED(field), \
	0, \
	size, \
	INT_V, \
	save_version_introduced, \
	save_version_removed, \
	#field \
}

#define DELETED_ARRAY_ENTRY(save_version_introduced, save_version_removed, field, count, elemType) \
{ \
	DELETED(field), \
	0, \
	count, \
	elemType, \
	save_version_introduced, \
	save_version_removed, \
	#field \
}

#define DELETED_POINTER_ENTRY(save_version_introduced, save_version_removed, field, relativeToField) \
{ \
	DELETED(field), \
	DELETED(relativeToField), \
	4, \
	POINTER_V, \
	save_version_introduced, \
	save_version_removed, \
	#field \
}

struct SnapshotMovieInfo
{
	uint32	MovieInputDataSize;
};

struct SnapshotScreenshotInfo
{
	uint16	Width;
	uint16	Height;
	uint8	Data[MAX_SNES_WIDTH * MAX_SNES_HEIGHT * 3];
	uint8	Interlaced;
};

struct SDMASnapshot
{
	struct SDMA	dma[8];
};

#define APU_OLDCYCLES_OBSOLETE_VALUE	-99999999

static struct Obsolete
{
	uint8	SPPU_Joypad1ButtonReadPos;
	uint8	SPPU_Joypad2ButtonReadPos;
	uint8	SPPU_Joypad3ButtonReadPos;
	uint8	SPPU_MouseSpeed[2];
	uint8	SAPU_Flags;
	int32	SAPU_OldCycles;
}	Obsolete;

#undef STRUCT
#define STRUCT struct SCPUState

static FreezeData	SnapCPU[] =
{
	INT_ENTRY(1, Flags),
	INT_ENTRY(1, BranchSkip),
	DELETED_INT_ENTRY(1, 4, NMIActive, 1),
	INT_ENTRY(1, IRQActive),
	INT_ENTRY(1, WaitingForInterrupt),
	INT_ENTRY(1, WhichEvent),
	INT_ENTRY(1, Cycles),
	INT_ENTRY(1, NextEvent),
	INT_ENTRY(1, V_Counter),
	INT_ENTRY(1, MemSpeed),
	INT_ENTRY(1, MemSpeedx2),
	INT_ENTRY(1, FastROMSpeed),
	INT_ENTRY(3, InDMAorHDMA),
	INT_ENTRY(3, InWRAMDMAorHDMA),
	INT_ENTRY(3, PBPCAtOpcodeStart),
	INT_ENTRY(3, WaitAddress),
	INT_ENTRY(3, WaitCounter),
	DELETED_INT_ENTRY(3, 4, AutoSaveTimer, 4),
	DELETED_INT_ENTRY(3, 4, SRAMModified, 1),
	DELETED_INT_ENTRY(3, 4, BRKTriggered, 1),
	DELETED_INT_ENTRY(3, 6, TriedInterleavedMode2, 1),
	INT_ENTRY(4, IRQPending),
	INT_ENTRY(4, InDMA),
	INT_ENTRY(4, InHDMA),
	INT_ENTRY(4, HDMARanInDMA),
	INT_ENTRY(4, PrevCycles)
};

#undef STRUCT
#define STRUCT struct SRegisters

static FreezeData	SnapRegisters[] =
{
	INT_ENTRY(1, PB),
	INT_ENTRY(1, DB),
	INT_ENTRY(1, P.W),
	INT_ENTRY(1, A.W),
	INT_ENTRY(1, D.W),
	INT_ENTRY(1, S.W),
	INT_ENTRY(1, X.W),
	INT_ENTRY(1, Y.W),
	INT_ENTRY(1, PCw)
};

#undef STRUCT
#define STRUCT struct SPPU

static FreezeData	SnapPPU[] =
{
	INT_ENTRY(1, BGMode),
	INT_ENTRY(1, BG3Priority),
	INT_ENTRY(1, Brightness),
	INT_ENTRY(1, VMA.High),
	INT_ENTRY(1, VMA.Increment),
	INT_ENTRY(1, VMA.Address),
	INT_ENTRY(1, VMA.Mask1),
	INT_ENTRY(1, VMA.FullGraphicCount),
	INT_ENTRY(1, VMA.Shift),
#define O(N) \
	INT_ENTRY(1, BG[N].SCBase), \
	INT_ENTRY(1, BG[N].VOffset), \
	INT_ENTRY(1, BG[N].HOffset), \
	INT_ENTRY(1, BG[N].BGSize), \
	INT_ENTRY(1, BG[N].NameBase), \
	INT_ENTRY(1, BG[N].SCSize)
	O(0), O(1), O(2), O(3),
#undef O
	INT_ENTRY(1, CGFLIP),
	ARRAY_ENTRY(1, CGDATA, 256, uint16_ARRAY_V),
	INT_ENTRY(1, FirstSprite),
	INT_ENTRY(3, LastSprite),
#define O(N) \
	INT_ENTRY(1, OBJ[N].HPos), \
	INT_ENTRY(1, OBJ[N].VPos), \
	INT_ENTRY(1, OBJ[N].Name), \
	INT_ENTRY(1, OBJ[N].VFlip), \
	INT_ENTRY(1, OBJ[N].HFlip), \
	INT_ENTRY(1, OBJ[N].Priority), \
	INT_ENTRY(1, OBJ[N].Palette), \
	INT_ENTRY(1, OBJ[N].Size)
	O(  0), O(  1), O(  2), O(  3), O(  4), O(  5), O(  6), O(  7),
	O(  8), O(  9), O( 10), O( 11), O( 12), O( 13), O( 14), O( 15),
	O( 16), O( 17), O( 18), O( 19), O( 20), O( 21), O( 22), O( 23),
	O( 24), O( 25), O( 26), O( 27), O( 28), O( 29), O( 30), O( 31),
	O( 32), O( 33), O( 34), O( 35), O( 36), O( 37), O( 38), O( 39),
	O( 40), O( 41), O( 42), O( 43), O( 44), O( 45), O( 46), O( 47),
	O( 48), O( 49), O( 50), O( 51), O( 52), O( 53), O( 54), O( 55),
	O( 56), O( 57), O( 58), O( 59), O( 60), O( 61), O( 62), O( 63),
	O( 64), O( 65), O( 66), O( 67), O( 68), O( 69), O( 70), O( 71),
	O( 72), O( 73), O( 74), O( 75), O( 76), O( 77), O( 78), O( 79),
	O( 80), O( 81), O( 82), O( 83), O( 84), O( 85), O( 86), O( 87),
	O( 88), O( 89), O( 90), O( 91), O( 92), O( 93), O( 94), O( 95),
	O( 96), O( 97), O( 98), O( 99), O(100), O(101), O(102), O(103),
	O(104), O(105), O(106), O(107), O(108), O(109), O(110), O(111),
	O(112), O(113), O(114), O(115), O(116), O(117), O(118), O(119),
	O(120), O(121), O(122), O(123), O(124), O(125), O(126), O(127),
#undef O
	INT_ENTRY(1, OAMPriorityRotation),
	INT_ENTRY(1, OAMAddr),
	INT_ENTRY(1, OAMFlip),
	INT_ENTRY(1, OAMTileAddress),
	INT_ENTRY(1, IRQVBeamPos),
	INT_ENTRY(1, IRQHBeamPos),
	INT_ENTRY(1, VBeamPosLatched),
	INT_ENTRY(1, HBeamPosLatched),
	INT_ENTRY(1, HBeamFlip),
	INT_ENTRY(1, VBeamFlip),
	INT_ENTRY(1, HVBeamCounterLatched),
	INT_ENTRY(1, MatrixA),
	INT_ENTRY(1, MatrixB),
	INT_ENTRY(1, MatrixC),
	INT_ENTRY(1, MatrixD),
	INT_ENTRY(1, CentreX),
	INT_ENTRY(1, CentreY),
	INT_ENTRY(2, M7HOFS),
	INT_ENTRY(2, M7VOFS),
	OBSOLETE_INT_ENTRY(1, 2, SPPU_Joypad1ButtonReadPos),
	OBSOLETE_INT_ENTRY(1, 2, SPPU_Joypad2ButtonReadPos),
	OBSOLETE_INT_ENTRY(1, 2, SPPU_Joypad3ButtonReadPos),
	INT_ENTRY(1, CGADD),
	INT_ENTRY(1, FixedColourRed),
	INT_ENTRY(1, FixedColourGreen),
	INT_ENTRY(1, FixedColourBlue),
	INT_ENTRY(1, SavedOAMAddr),
	INT_ENTRY(1, ScreenHeight),
	INT_ENTRY(1, WRAM),
	DELETED_INT_ENTRY(3, 3, BG_Forced, 1),
	INT_ENTRY(1, ForcedBlanking),
	INT_ENTRY(3, OBJThroughMain),
	INT_ENTRY(3, OBJThroughSub),
	INT_ENTRY(1, OBJNameSelect),
	INT_ENTRY(1, OBJSizeSelect),
	INT_ENTRY(1, OBJNameBase),
	INT_ENTRY(3, OBJAddition),
	INT_ENTRY(1, OAMReadFlip),
	INT_ENTRY(1, VTimerEnabled),
	INT_ENTRY(1, HTimerEnabled),
	INT_ENTRY(1, HTimerPosition),
	INT_ENTRY(1, Mosaic),
	INT_ENTRY(3, MosaicStart),
	INT_ENTRY(1, Mode7HFlip),
	INT_ENTRY(1, Mode7VFlip),
	INT_ENTRY(1, Mode7Repeat),
	INT_ENTRY(1, Window1Left),
	INT_ENTRY(1, Window1Right),
	INT_ENTRY(1, Window2Left),
	INT_ENTRY(1, Window2Right),
#define O(N) \
	INT_ENTRY(3, ClipCounts[N]), \
	INT_ENTRY(1, ClipWindowOverlapLogic[N]), \
	INT_ENTRY(1, ClipWindow1Enable[N]), \
	INT_ENTRY(1, ClipWindow2Enable[N]), \
	INT_ENTRY(1, ClipWindow1Inside[N]), \
	INT_ENTRY(1, ClipWindow2Inside[N])
	O(0), O(1), O(2), O(3), O(4), O(5),
#undef O
	INT_ENTRY(3, RecomputeClipWindows),
	INT_ENTRY(1, CGFLIPRead),
	INT_ENTRY(1, Need16x8Mulitply),
	ARRAY_ENTRY(1, BGMosaic, 4, uint8_ARRAY_V),
	ARRAY_ENTRY(1, OAMData, 512 + 32, uint8_ARRAY_V),
	INT_ENTRY(1, Need16x8Mulitply),
	OBSOLETE_ARRAY_ENTRY(1, 2, SPPU_MouseSpeed, 2, uint8_ARRAY_V),
	INT_ENTRY(2, OAMWriteRegister),
	INT_ENTRY(2, BGnxOFSbyte),
	INT_ENTRY(2, M7byte),
	INT_ENTRY(2, OpenBus1),
	INT_ENTRY(2, OpenBus2),
	INT_ENTRY(3, GunVLatch),
	INT_ENTRY(3, GunHLatch),
	INT_ENTRY(2, VTimerPosition),
	INT_ENTRY(5, HDMA),
	INT_ENTRY(5, HDMAEnded)
};

#undef STRUCT
#define STRUCT struct SDMASnapshot

static FreezeData	SnapDMA[] =
{
#define O(N) \
	INT_ENTRY(1, dma[N].ReverseTransfer), \
	INT_ENTRY(1, dma[N].AAddressFixed), \
	INT_ENTRY(1, dma[N].AAddressDecrement), \
	INT_ENTRY(1, dma[N].TransferMode), \
	INT_ENTRY(1, dma[N].ABank), \
	INT_ENTRY(1, dma[N].AAddress), \
	INT_ENTRY(1, dma[N].Address), \
	INT_ENTRY(1, dma[N].BAddress), \
	DELETED_INT_ENTRY(1, 2, dma[N].TransferBytes, 2), \
	INT_ENTRY(1, dma[N].HDMAIndirectAddressing), \
	INT_ENTRY(1, dma[N].DMACount_Or_HDMAIndirectAddress), \
	INT_ENTRY(1, dma[N].IndirectBank), \
	INT_ENTRY(1, dma[N].Repeat), \
	INT_ENTRY(1, dma[N].LineCount), \
	INT_ENTRY(1, dma[N].DoTransfer), \
	INT_ENTRY(2, dma[N].UnknownByte), \
	INT_ENTRY(2, dma[N].UnusedBit43x0)
	O(0), O(1), O(2), O(3), O(4), O(5), O(6), O(7)
#undef O
};
/*
#undef STRUCT
#define STRUCT struct SAPU

static FreezeData	SnapAPU[] =
{
	OBSOLETE_INT_ENTRY(1, 6, SAPU_OldCycles),
	INT_ENTRY(1, ShowROM),
	OBSOLETE_INT_ENTRY(1, 2, SAPU_Flags),
	INT_ENTRY(2, Flags),
	INT_ENTRY(1, Internal_KON),
	ARRAY_ENTRY(1, OutPorts, 4, uint8_ARRAY_V),
	ARRAY_ENTRY(1, DSP, 0x80, uint8_ARRAY_V),
	ARRAY_ENTRY(1, ExtraRAM, 64, uint8_ARRAY_V),
	ARRAY_ENTRY(1, Timer, 3, uint16_ARRAY_V),
	ARRAY_ENTRY(1, TimerTarget, 3, uint16_ARRAY_V),
	ARRAY_ENTRY(1, TimerEnabled, 3, uint8_ARRAY_V),
	DELETED_ARRAY_ENTRY(1, 6, TimerValueWritten, 3, uint8_ARRAY_V),
	INT_ENTRY(4, Cycles),
	INT_ENTRY(5, NextAPUTimerPos),
	INT_ENTRY(5, APUTimerCounter),
	INT_ENTRY(6, OUTX_ENVX_pos),
	INT_ENTRY(6, OUTX_ENVX_samples),
	INT_ENTRY(6, OUTX_ENVX_counter),
	INT_ENTRY(6, OUTX_ENVX_counter_max),
#define O(N) \
	ARRAY_ENTRY(6, OUTX_buffer[N], MAX_OUTX_ENVX_BUFFER, uint8_ARRAY_V), \
	ARRAY_ENTRY(6, ENVX_buffer[N], MAX_OUTX_ENVX_BUFFER, uint8_ARRAY_V)
	O(0), O(1), O(2), O(3), O(4), O(5), O(6), O(7)
#undef O
};

#undef STRUCT
#define STRUCT struct SAPURegisters

static FreezeData	SnapAPURegisters[] =
{
	INT_ENTRY(1, P),
	INT_ENTRY(1, YA.W),
	INT_ENTRY(1, X),
	INT_ENTRY(1, S),
	INT_ENTRY(1, PC)
};

#undef STRUCT
#define STRUCT SSoundData

static FreezeData	SnapSoundData[] =
{
#define O(N) \
	INT_ENTRY(6, channels[N].state), \
	INT_ENTRY(6, channels[N].type), \
	INT_ENTRY(6, channels[N].volume_left), \
	INT_ENTRY(6, channels[N].volume_right), \
	INT_ENTRY(6, channels[N].pitch), \
	INT_ENTRY(6, channels[N].frequency), \
	INT_ENTRY(6, channels[N].needs_decode), \
	INT_ENTRY(6, channels[N].last_block), \
	INT_ENTRY(6, channels[N].loop), \
	ARRAY_ENTRY(6, channels[N].decoded, 16, uint16_ARRAY_V), \
	ARRAY_ENTRY(6, channels[N].previous, 2, uint32_ARRAY_V), \
	INT_ENTRY(6, channels[N].block_pointer), \
	INT_ENTRY(6, channels[N].sample_pointer), \
	INT_ENTRY(6, channels[N].sample), \
	ARRAY_ENTRY(6, channels[N].nb_sample, 4, uint16_ARRAY_V), \
	INT_ENTRY(6, channels[N].nb_index), \
	INT_ENTRY(6, channels[N].sample_number), \
	INT_ENTRY(6, channels[N].xenvx), \
	INT_ENTRY(6, channels[N].xenvx_target), \
	INT_ENTRY(6, channels[N].xenv_count), \
	INT_ENTRY(6, channels[N].xenv_rate), \
	INT_ENTRY(6, channels[N].xsmp_count), \
	INT_ENTRY(6, channels[N].xattack_rate), \
	INT_ENTRY(6, channels[N].xdecay_rate), \
	INT_ENTRY(6, channels[N].xsustain_rate), \
	INT_ENTRY(6, channels[N].xsustain_level)
	O(0), O(1), O(2), O(3), O(4), O(5), O(6), O(7),
#undef O
	ARRAY_ENTRY(6, master_volume, 2, uint32_ARRAY_V),
	ARRAY_ENTRY(6, echo_volume, 2, uint32_ARRAY_V),
	INT_ENTRY(6, echo_feedback),
	INT_ENTRY(6, echo_ring_pointer),
	INT_ENTRY(6, echo_ring_size),
	ARRAY_ENTRY(6, alt_echo_ring, 24000, uint16_ARRAY_V),
	INT_ENTRY(6, echo_write_enabled),
	INT_ENTRY(6, fir_index),
	ARRAY_ENTRY(6, fir_tap, 8, uint32_ARRAY_V),
	ARRAY_ENTRY(6, fir_buffer, 16, uint32_ARRAY_V),
	INT_ENTRY(6, no_filter),
	INT_ENTRY(6, noise_count),
	INT_ENTRY(6, noise_rate),
	INT_ENTRY(6, noise_seed),
	INT_ENTRY(6, mute),
	INT_ENTRY(6, stereo),
	INT_ENTRY(6, sixteen_bit),
	INT_ENTRY(6, disable_echo),
	INT_ENTRY(6, playback_rate)
}; */

#undef STRUCT
#define STRUCT struct STimings

static FreezeData	SnapTimings[] =
{
	INT_ENTRY(2, H_Max_Master),
	INT_ENTRY(2, H_Max),
	INT_ENTRY(2, V_Max_Master),
	INT_ENTRY(2, V_Max),
	INT_ENTRY(2, HBlankStart),
	INT_ENTRY(2, HBlankEnd),
	INT_ENTRY(2, HDMAInit),
	INT_ENTRY(2, HDMAStart),
	INT_ENTRY(2, NMITriggerPos),
	INT_ENTRY(2, WRAMRefreshPos),
	INT_ENTRY(2, RenderPos),
	INT_ENTRY(2, InterlaceField),
	INT_ENTRY(4, DMACPUSync),
	INT_ENTRY(6, NMIDMADelay),
	INT_ENTRY(6, IRQPendCount)
};

#undef STRUCT
#define STRUCT struct SControlSnapshot

static FreezeData	SnapControls[] =
{
	INT_ENTRY(2, ver),
	ARRAY_ENTRY(2, port1_read_idx, 2, uint8_ARRAY_V),
	ARRAY_ENTRY(2, dummy1, 4, uint8_ARRAY_V),
	ARRAY_ENTRY(2, port2_read_idx, 2, uint8_ARRAY_V),
	ARRAY_ENTRY(2, dummy2, 4, uint8_ARRAY_V),
	ARRAY_ENTRY(2, mouse_speed, 2, uint8_ARRAY_V),
	INT_ENTRY(2, justifier_select),
	ARRAY_ENTRY(2, dummy3, 8, uint8_ARRAY_V),
	INT_ENTRY(4, pad_read),
	INT_ENTRY(4, pad_read_last),
	ARRAY_ENTRY(3, internal, 60, uint8_ARRAY_V)
};

#ifndef ZSNES_FX

#undef STRUCT
#define STRUCT struct FxRegs_s

static FreezeData	SnapFX[] =
{
	INT_ENTRY(4, vColorReg),
	INT_ENTRY(4, vPlotOptionReg),
	INT_ENTRY(4, vStatusReg),
	INT_ENTRY(4, vPrgBankReg),
	INT_ENTRY(4, vRomBankReg),
	INT_ENTRY(4, vRamBankReg),
	INT_ENTRY(4, vCacheBaseReg),
	INT_ENTRY(4, vCacheFlags),
	INT_ENTRY(4, vLastRamAdr),
	INT_ENTRY(4, vPipeAdr),
	INT_ENTRY(4, vSign),
	INT_ENTRY(4, vZero),
	INT_ENTRY(4, vCarry),
	INT_ENTRY(4, vOverflow),
	INT_ENTRY(4, vErrorCode),
	INT_ENTRY(4, vIllegalAddress),
	INT_ENTRY(4, vBreakPoint),
	INT_ENTRY(4, vStepPoint),
	INT_ENTRY(4, nRamBanks),
	INT_ENTRY(4, nRomBanks),
	INT_ENTRY(4, vMode),
	INT_ENTRY(4, vPrevMode),
	INT_ENTRY(4, vScreenHeight),
	INT_ENTRY(4, vScreenRealHeight),
	INT_ENTRY(4, vPrevScreenHeight),
	INT_ENTRY(4, vScreenSize),
	INT_ENTRY(4, vCounter),
	INT_ENTRY(4, vInstCount),
	INT_ENTRY(4, vSCBRDirty),
	INT_ENTRY(4, vRomBuffer),
	INT_ENTRY(4, vPipe),
	INT_ENTRY(4, bCacheActive),
	INT_ENTRY(4, bBreakPoint),
	ARRAY_ENTRY(4, avCacheBackup, 512, uint8_ARRAY_V),
	ARRAY_ENTRY(4, avReg, 16, uint32_ARRAY_V),
	ARRAY_ENTRY(4, x, 32, uint32_ARRAY_V),
	POINTER_ENTRY(4, pvScreenBase, pvRam),
	POINTER_ENTRY(4, pvPrgBank, apvRomBank),
	POINTER_ENTRY(4, pvDreg, avReg),
	POINTER_ENTRY(4, pvSreg, avReg),
#define O(N) \
	POINTER_ENTRY(4, apvScreen[N], pvRam)
	O(  0), O(  1), O(  2), O(  3), O(  4), O(  5), O(  6), O(  7),
	O(  8), O(  9), O( 10), O( 11), O( 12), O( 13), O( 14), O( 15),
	O( 16), O( 17), O( 18), O( 19), O( 20), O( 21), O( 22), O( 23),
	O( 24), O( 25), O( 26), O( 27), O( 28), O( 29), O( 30), O( 31),
#undef O
	POINTER_ENTRY(4, pvRamBank, apvRamBank),
	POINTER_ENTRY(4, pvRomBank, apvRomBank),
	POINTER_ENTRY(4, pvCache, pvRegisters),
#define O(N) \
	POINTER_ENTRY(4, apvRamBank[N], pvRam)
	O(0), O(1), O(2), O(3)
#undef O
};
#endif

#undef STRUCT
#define STRUCT struct SDSP1

static FreezeData	SnapDSP1[] =
{
//	INT_ENTRY(3, version),
	INT_ENTRY(3, waiting4command),
	INT_ENTRY(3, first_parameter),
	INT_ENTRY(3, command),
//	INT_ENTRY(3, in_count),
//	INT_ENTRY(3, in_index),
//	INT_ENTRY(3, out_count),
//	INT_ENTRY(3, out_index),
//	ARRAY_ENTRY(3, parameters, 512, uint8_ARRAY_V),
//	ARRAY_ENTRY(3, output, 512, uint8_ARRAY_V),
//	ARRAY_ENTRY(4, temp_save_data, 406, uint8_ARRAY_V)
};

#undef STRUCT
#define STRUCT struct SSA1

static FreezeData	SnapSA1[] =
{
	INT_ENTRY(1, Flags),
	DELETED_INT_ENTRY(1, 6, NMIActive, 1),
	INT_ENTRY(1, IRQActive),
	INT_ENTRY(1, WaitingForInterrupt),
	INT_ENTRY(1, op1),
	INT_ENTRY(1, op2),
	INT_ENTRY(1, arithmetic_op),
	INT_ENTRY(1, sum),
	INT_ENTRY(1, overflow),
	INT_ENTRY(3, CPUExecuting),
	INT_ENTRY(3, ShiftedPB),
	INT_ENTRY(3, ShiftedDB),
	INT_ENTRY(3, Executing),
	INT_ENTRY(3, Waiting),
	INT_ENTRY(3, PBPCAtOpcodeStart),
	INT_ENTRY(3, WaitAddress),
	INT_ENTRY(3, WaitCounter),
	INT_ENTRY(3, VirtualBitmapFormat),
	INT_ENTRY(3, in_char_dma),
	INT_ENTRY(3, variable_bit_pos)
};

#undef STRUCT
#define STRUCT struct SSA1Registers

static FreezeData	SnapSA1Registers[] =
{
	INT_ENTRY(1, PB),
	INT_ENTRY(1, DB),
	INT_ENTRY(1, P.W),
	INT_ENTRY(1, A.W),
	INT_ENTRY(1, D.W),
	INT_ENTRY(1, S.W),
	INT_ENTRY(1, X.W),
	INT_ENTRY(1, Y.W),
	INT_ENTRY(1, PCw)
};

#undef STRUCT
#define STRUCT struct SSPC7110Snapshot

static FreezeData	SnapSPC7110Snap[] =
{
	INT_ENTRY(6, r4801),
	INT_ENTRY(6, r4802),
	INT_ENTRY(6, r4803),
	INT_ENTRY(6, r4804),
	INT_ENTRY(6, r4805),
	INT_ENTRY(6, r4806),
	INT_ENTRY(6, r4807),
	INT_ENTRY(6, r4808),
	INT_ENTRY(6, r4809),
	INT_ENTRY(6, r480a),
	INT_ENTRY(6, r480b),
	INT_ENTRY(6, r480c),
	INT_ENTRY(6, r4811),
	INT_ENTRY(6, r4812),
	INT_ENTRY(6, r4813),
	INT_ENTRY(6, r4814),
	INT_ENTRY(6, r4815),
	INT_ENTRY(6, r4816),
	INT_ENTRY(6, r4817),
	INT_ENTRY(6, r4818),
	INT_ENTRY(6, r481x),
	INT_ENTRY(6, r4814_latch),
	INT_ENTRY(6, r4815_latch),
	INT_ENTRY(6, r4820),
	INT_ENTRY(6, r4821),
	INT_ENTRY(6, r4822),
	INT_ENTRY(6, r4823),
	INT_ENTRY(6, r4824),
	INT_ENTRY(6, r4825),
	INT_ENTRY(6, r4826),
	INT_ENTRY(6, r4827),
	INT_ENTRY(6, r4828),
	INT_ENTRY(6, r4829),
	INT_ENTRY(6, r482a),
	INT_ENTRY(6, r482b),
	INT_ENTRY(6, r482c),
	INT_ENTRY(6, r482d),
	INT_ENTRY(6, r482e),
	INT_ENTRY(6, r482f),
	INT_ENTRY(6, r4830),
	INT_ENTRY(6, r4831),
	INT_ENTRY(6, r4832),
	INT_ENTRY(6, r4833),
	INT_ENTRY(6, r4834),
	INT_ENTRY(6, dx_offset),
	INT_ENTRY(6, ex_offset),
	INT_ENTRY(6, fx_offset),
	INT_ENTRY(6, r4840),
	INT_ENTRY(6, r4841),
	INT_ENTRY(6, r4842),
	INT_ENTRY(6, rtc_state),
	INT_ENTRY(6, rtc_mode),
	INT_ENTRY(6, rtc_index),
	INT_ENTRY(6, decomp_mode),
	INT_ENTRY(6, decomp_offset),
	ARRAY_ENTRY(6, decomp_buffer, SPC7110_DECOMP_BUFFER_SIZE, uint8_ARRAY_V),
	INT_ENTRY(6, decomp_buffer_rdoffset),
	INT_ENTRY(6, decomp_buffer_wroffset),
	INT_ENTRY(6, decomp_buffer_length),
#define O(N) \
	INT_ENTRY(6, context[N].index), \
	INT_ENTRY(6, context[N].invert)
	O(  0), O(  1), O(  2), O(  3), O(  4), O(  5), O(  6), O(  7),
	O(  8), O(  9), O( 10), O( 11), O( 12), O( 13), O( 14), O( 15),
	O( 16), O( 17), O( 18), O( 19), O( 20), O( 21), O( 22), O( 23),
	O( 24), O( 25), O( 26), O( 27), O( 28), O( 29), O( 30), O( 31)
#undef O
};

#undef STRUCT
#define STRUCT struct SSRTCSnapshot

static FreezeData	SnapSRTCSnap[] =
{
	INT_ENTRY(6, rtc_mode),
	INT_ENTRY(6, rtc_index)
};

#undef STRUCT
#define STRUCT struct SRTCData

static FreezeData	SnapRTCData[] =
{
	ARRAY_ENTRY(6, reg, 20, uint8_ARRAY_V),
};

#undef STRUCT
#define STRUCT struct SBSX

static FreezeData	SnapBSX[] =
{
	INT_ENTRY(2, dirty),
	INT_ENTRY(2, dirty2),
	INT_ENTRY(2, bootup),
	INT_ENTRY(2, flash_enable),
	INT_ENTRY(2, write_enable),
	INT_ENTRY(2, read_enable),
	INT_ENTRY(2, flash_command),
	INT_ENTRY(2, old_write),
	INT_ENTRY(2, new_write),
	INT_ENTRY(2, out_index),
	ARRAY_ENTRY(2, output, 32, uint8_ARRAY_V),
	ARRAY_ENTRY(2, PPU, 32, uint8_ARRAY_V),
	ARRAY_ENTRY(2, MMC, 16, uint8_ARRAY_V),
	ARRAY_ENTRY(2, prevMMC, 16, uint8_ARRAY_V),
	ARRAY_ENTRY(2, test2192, 32, uint8_ARRAY_V)
};

#undef STRUCT
#define STRUCT struct SnapshotMovieInfo

static FreezeData	SnapMovie[] =
{
	INT_ENTRY(1, MovieInputDataSize)
};

#undef STRUCT
#define STRUCT struct SnapshotScreenshotInfo

static FreezeData	SnapScreenshot[] =
{
	INT_ENTRY(4, Width),
	INT_ENTRY(4, Height),
	ARRAY_ENTRY(4, Data, MAX_SNES_WIDTH * MAX_SNES_HEIGHT * 3, uint8_ARRAY_V),
	INT_ENTRY(4, Interlaced)
};

// for backward compatibility
/*
#undef STRUCT
#define STRUCT SOldSoundData

static FreezeData	SnapOldSoundData[] =
{
	INT_ENTRY(1, master_volume_left),
	INT_ENTRY(1, master_volume_right),
	INT_ENTRY(1, echo_volume_left),
	INT_ENTRY(1, echo_volume_right),
	INT_ENTRY(1, echo_enable),
	INT_ENTRY(1, echo_feedback),
	INT_ENTRY(1, echo_ptr),
	INT_ENTRY(1, echo_buffer_size),
	INT_ENTRY(1, echo_write_enabled),
	INT_ENTRY(1, echo_channel_enable),
	INT_ENTRY(1, pitch_mod),
	ARRAY_ENTRY(1, dummy, 3, uint32_ARRAY_V),
#define O(N) \
	INT_ENTRY(1, channels[N].state), \
	INT_ENTRY(1, channels[N].type), \
	INT_ENTRY(1, channels[N].volume_left), \
	INT_ENTRY(1, channels[N].volume_right), \
	INT_ENTRY(1, channels[N].hertz), \
	INT_ENTRY(1, channels[N].count), \
	INT_ENTRY(1, channels[N].loop), \
	INT_ENTRY(1, channels[N].envx), \
	INT_ENTRY(1, channels[N].left_vol_level), \
	INT_ENTRY(1, channels[N].right_vol_level), \
	INT_ENTRY(1, channels[N].envx_target), \
	INT_ENTRY(1, channels[N].env_error), \
	INT_ENTRY(1, channels[N].erate), \
	INT_ENTRY(1, channels[N].direction), \
	INT_ENTRY(1, channels[N].attack_rate), \
	INT_ENTRY(1, channels[N].decay_rate), \
	INT_ENTRY(1, channels[N].sustain_rate), \
	INT_ENTRY(1, channels[N].release_rate), \
	INT_ENTRY(1, channels[N].sustain_level), \
	INT_ENTRY(1, channels[N].sample), \
	ARRAY_ENTRY(1, channels[N].decoded, 16, uint16_ARRAY_V), \
	ARRAY_ENTRY(1, channels[N].previous16, 2, uint16_ARRAY_V), \
	INT_ENTRY(1, channels[N].sample_number), \
	INT_ENTRY(1, channels[N].last_block), \
	INT_ENTRY(1, channels[N].needs_decode), \
	INT_ENTRY(1, channels[N].block_pointer), \
	INT_ENTRY(1, channels[N].sample_pointer), \
	INT_ENTRY(1, channels[N].mode)
	O(0), O(1), O(2), O(3), O(4), O(5), O(6), O(7),
#undef O
	INT_ENTRY(2, noise_rate),
#define O(N) \
	INT_ENTRY(2, channels[N].out_sample), \
	INT_ENTRY(2, channels[N].xenvx), \
	INT_ENTRY(2, channels[N].xenvx_target), \
	INT_ENTRY(2, channels[N].xenv_count), \
	INT_ENTRY(2, channels[N].xenv_rate), \
	INT_ENTRY(2, channels[N].xattack_rate), \
	INT_ENTRY(2, channels[N].xdecay_rate), \
	INT_ENTRY(2, channels[N].xsustain_rate), \
	INT_ENTRY(2, channels[N].xsustain_level)
	O(0), O(1), O(2), O(3), O(4), O(5), O(6), O(7),
#undef O
	INT_ENTRY(4, noise_count),
	INT_ENTRY(4, no_filter),
	INT_ENTRY(4, echo_volume[0]),
	INT_ENTRY(4, echo_volume[1]),
	INT_ENTRY(4, master_volume[0]),
	INT_ENTRY(4, master_volume[1]),
};
*/
// deleted blocks
static FreezeData	SnapIPPU[] =
{
	DELETED_ARRAY_ENTRY(3, 4, Junk, 2, uint32_ARRAY_V)
};

static FreezeData	SnapGFX[] =
{
	DELETED_ARRAY_ENTRY(3, 4, Junk, 22 + 256 + MAX_SNES_WIDTH * MAX_SNES_HEIGHT * 2, uint8_ARRAY_V)
};

static int UnfreezeBlock (STREAM, const char *, uint8 *, int);
static int UnfreezeBlockCopy (STREAM, const char *, uint8 **, int);
static int UnfreezeStruct (STREAM, const char *, void *, FreezeData *, int, int);
static int UnfreezeStructCopy (STREAM, const char *, uint8 **, FreezeData *, int, int);
static void UnfreezeStructFromCopy (void *, FreezeData *, int, uint8 *, int);
static void FreezeBlock (STREAM, const char *, uint8 *, int);
static void FreezeStruct (STREAM, const char *, void *, FreezeData *, int);


void S9xResetSaveTimer (bool8 dontsave)
{
	static time_t	t = -1;

	if (!Settings.DontSaveOopsSnapshot && !dontsave && t != -1 && time(NULL) - t > 300)
	{
		char	filename[PATH_MAX + 1];
		char	drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], def[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

		_splitpath(Memory.ROMFilename, drive, dir, def, ext);
		sprintf(filename, "%s%s%s.%.*s", S9xGetDirectory(SNAPSHOT_DIR), SLASH_STR, def, _MAX_EXT - 1, "oops");
		S9xMessage(S9X_INFO, S9X_FREEZE_FILE_INFO, SAVE_INFO_OOPS);
		S9xFreezeGame(filename);
	}

	t = time(NULL);
}

bool8 S9xFreezeGame (const char *filename)
{
	STREAM	stream = NULL;

	if (S9xOpenSnapshotFile(filename, FALSE, &stream))
	{
		S9xFreezeToStream(stream);
		S9xCloseSnapshotFile(stream);

		S9xResetSaveTimer(TRUE);

		const char *base = S9xBasename(filename);
		if (S9xMovieActive())
			sprintf(String, MOVIE_INFO_SNAPSHOT " %s", base);
		else
			sprintf(String, SAVE_INFO_SNAPSHOT " %s", base);

		S9xMessage(S9X_INFO, S9X_FREEZE_FILE_INFO, String);

		return (TRUE);
	}

	return (FALSE);
}

bool8 S9xUnfreezeGame (const char *filename)
{
	STREAM	stream = NULL;
	char	drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], def[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

	const char	*base = S9xBasename(filename);

	_splitpath(filename, drive, dir, def, ext);
	S9xResetSaveTimer(!strcmp(ext, "oops") || !strcmp(ext, "oop") || !strcmp(ext, ".oops") || !strcmp(ext, ".oop"));

	if (S9xOpenSnapshotFile(filename, TRUE, &stream))
	{
		int	result;

		ZeroMemory(&Obsolete, sizeof(Obsolete));
		Obsolete.SAPU_OldCycles = APU_OLDCYCLES_OBSOLETE_VALUE;

		result = S9xUnfreezeFromStream(stream);
		S9xCloseSnapshotFile(stream);

		if (result != SUCCESS)
		{
			switch (result)
			{
				case WRONG_FORMAT:
					S9xMessage(S9X_ERROR, S9X_WRONG_FORMAT, SAVE_ERR_WRONG_FORMAT);
					break;

				case WRONG_VERSION:
					S9xMessage(S9X_ERROR, S9X_WRONG_VERSION, SAVE_ERR_WRONG_VERSION);
					break;

				case WRONG_MOVIE_SNAPSHOT:
					S9xMessage(S9X_ERROR, S9X_WRONG_MOVIE_SNAPSHOT, MOVIE_ERR_SNAPSHOT_WRONG_MOVIE);
					break;

				case NOT_A_MOVIE_SNAPSHOT:
					S9xMessage(S9X_ERROR, S9X_NOT_A_MOVIE_SNAPSHOT, MOVIE_ERR_SNAPSHOT_NOT_MOVIE);
					break;

				case SNAPSHOT_INCONSISTENT:
					S9xMessage(S9X_ERROR, S9X_SNAPSHOT_INCONSISTENT, MOVIE_ERR_SNAPSHOT_INCONSISTENT);
					break;

				case FILE_NOT_FOUND:
				default:
					sprintf(String, SAVE_ERR_ROM_NOT_FOUND, base);
					S9xMessage(S9X_ERROR, S9X_ROM_NOT_FOUND, String);
					break;
			}

			return (FALSE);
		}

		if (S9xMovieActive())
		{
			if (S9xMovieReadOnly())
				sprintf(String, MOVIE_INFO_REWIND " %s", base);
			else
				sprintf(String, MOVIE_INFO_RERECORD " %s", base);
		}
		else
			sprintf(String, SAVE_INFO_LOAD " %s", base);

		S9xMessage(S9X_INFO, S9X_FREEZE_FILE_INFO, String);

		return (TRUE);
	}

	sprintf(String, SAVE_ERR_SAVE_NOT_FOUND, base);
	S9xMessage(S9X_INFO, S9X_FREEZE_FILE_INFO, String);

	return (FALSE);
}

void S9xFreezeToStream (STREAM stream)
{
	char	buffer[1024];

	S9xSetSoundMute(TRUE);

#ifdef ZSNES_FX
	if (Settings.SuperFX)
		S9xSuperFXPreSaveState();
#endif

	sprintf(buffer, "%s:%04d\n", SNAPSHOT_MAGIC, SNAPSHOT_VERSION);
	WRITE_STREAM(buffer, strlen(buffer), stream);

	sprintf(buffer, "NAM:%06d:%s%c", (int) strlen(Memory.ROMFilename) + 1, Memory.ROMFilename, 0);
	WRITE_STREAM(buffer, strlen(buffer) + 1, stream);

	FreezeStruct(stream, "CPU", &CPU, SnapCPU, COUNT(SnapCPU));

	FreezeStruct(stream, "REG", &Registers, SnapRegisters, COUNT(SnapRegisters));

	FreezeStruct(stream, "PPU", &PPU, SnapPPU, COUNT(SnapPPU));

	struct SDMASnapshot dma_snap;
	for (int d = 0; d < 8; d++)
		dma_snap.dma[d] = DMA[d];
	FreezeStruct(stream, "DMA", &dma_snap, SnapDMA, COUNT(SnapDMA));

	FreezeBlock(stream, "VRA", Memory.VRAM, 0x10000);

	FreezeBlock(stream, "RAM", Memory.RAM, 0x20000);

	FreezeBlock(stream, "SRA", Memory.SRAM, 0x20000);

	FreezeBlock(stream, "FIL", Memory.FillRAM, 0x8000);

	/* FreezeStruct(stream, "APU", &APU, SnapAPU, COUNT(SnapAPU));

	FreezeStruct(stream, "ARE", &APURegisters, SnapAPURegisters, COUNT(SnapAPURegisters));

	FreezeBlock(stream, "ARA", IAPU.RAM, 0x10000); */

	struct SControlSnapshot ctl_snap;
	S9xControlPreSaveState(&ctl_snap);
	FreezeStruct(stream, "CTL", &ctl_snap, SnapControls, COUNT(SnapControls));

	FreezeStruct(stream, "TIM", &Timings, SnapTimings, COUNT(SnapTimings));

	if (Settings.SA1)
	{
		S9xSA1PackStatus();
		FreezeStruct(stream, "SA1", &SA1, SnapSA1, COUNT(SnapSA1));
		FreezeStruct(stream, "SAR", &SA1Registers, SnapSA1Registers, COUNT(SnapSA1Registers));
	}

	if (Settings.BS)
		FreezeStruct(stream, "BSX", &BSX, SnapBSX, COUNT(SnapBSX));

	if (S9xMovieActive())
	{
		uint8	*movie_freeze_buf;
		uint32	movie_freeze_size;

		S9xMovieFreeze(&movie_freeze_buf, &movie_freeze_size);
		if (movie_freeze_buf)
		{
			struct SnapshotMovieInfo mi;

			mi.MovieInputDataSize = movie_freeze_size;
			FreezeStruct(stream, "MOV", &mi, SnapMovie, COUNT(SnapMovie));
			FreezeBlock(stream, "MID", movie_freeze_buf, movie_freeze_size);

			delete [] movie_freeze_buf;
		}
	}

	if (Settings.DSP)
	{
		//S9xDSP1PreSaveState();
		FreezeStruct(stream, "DSP", &DSP1, SnapDSP1, COUNT(SnapDSP1));
	}

	if (Settings.C4)
#ifndef ZSNES_C4
		FreezeBlock(stream, "CX4", Memory.C4RAM, 8192);
#else
	{
		if (C4Ram)
			FreezeBlock(stream, "CX4", C4Ram, 8192);
	}
#endif

#ifndef ZSNES_FX
	if (Settings.SuperFX)
		FreezeStruct(stream, "SFX", &GSU, SnapFX, COUNT(SnapFX));
#endif

	if (Settings.SnapshotScreenshots)
	{
		SnapshotScreenshotInfo *ssi = new SnapshotScreenshotInfo;

		ssi->Width  = min(IPPU.RenderedScreenWidth,  MAX_SNES_WIDTH);
		ssi->Height = min(IPPU.RenderedScreenHeight, MAX_SNES_HEIGHT);
		ssi->Interlaced = GFX.DoInterlace;

		uint8	*rowpix = ssi->Data;
		uint16	*screen = GFX.Screen;

		for (int y = 0; y < ssi->Height; y++, screen += GFX.RealPPL)
		{
			for (int x = 0; x < ssi->Width; x++)
			{
				uint32	r, g, b;

				DECOMPOSE_PIXEL(screen[x], r, g, b);
				*(rowpix++) = r;
				*(rowpix++) = g;
				*(rowpix++) = b;
			}
		}

		memset(rowpix, 0, sizeof(ssi->Data) + ssi->Data - rowpix);

		FreezeStruct(stream, "SHO", ssi, SnapScreenshot, COUNT(SnapScreenshot));

		delete ssi;
	}
/*
	S9xSoundPreSaveState();
	FreezeStruct(stream, "SND", &SoundData, SnapSoundData, COUNT(SnapSoundData));
*/
	if (Settings.SPC7110)
	{
		S9xSPC7110PreSaveState();
		FreezeStruct(stream, "S71", &s7snap, SnapSPC7110Snap, COUNT(SnapSPC7110Snap));
	}

	if (Settings.SRTC)
	{
		S9xSRTCPreSaveState();
		FreezeStruct(stream, "SRT", &srtcsnap, SnapSRTCSnap, COUNT(SnapSRTCSnap));
	}

	if (Settings.SRTC || Settings.SPC7110RTC)
		FreezeStruct(stream, "CLK", &RTCData, SnapRTCData, COUNT(SnapRTCData));

#ifdef ZSNES_FX
	if (Settings.SuperFX)
		S9xSuperFXPostSaveState();
#endif

	S9xSetSoundMute(FALSE);
}

int S9xUnfreezeFromStream (STREAM stream)
{
	int		result = SUCCESS;
	int		version, len;
	char	buffer[PATH_MAX + 1];

	len = strlen(SNAPSHOT_MAGIC) + 1 + 4 + 1;
	if (READ_STREAM(buffer, len, stream) != len)
		return (WRONG_FORMAT);

	if (strncmp(buffer, SNAPSHOT_MAGIC, strlen(SNAPSHOT_MAGIC)) != 0)
		return (WRONG_FORMAT);

	version = atoi(&buffer[strlen(SNAPSHOT_MAGIC) + 1]);
	if (version > SNAPSHOT_VERSION)
		return (WRONG_VERSION);

	result = UnfreezeBlock(stream, "NAM", (uint8 *) buffer, PATH_MAX);
	if (result != SUCCESS)
		return (result);

	uint8	*local_cpu           = NULL;
	uint8	*local_registers     = NULL;
	uint8	*local_ppu           = NULL;
	uint8	*local_dma           = NULL;
	uint8	*local_vram          = NULL;
	uint8	*local_ram           = NULL;
	uint8	*local_sram          = NULL;
	uint8	*local_fillram       = NULL;
	uint8	*local_apu           = NULL;
	uint8	*local_apu_registers = NULL;
	uint8	*local_apu_ram       = NULL;
	uint8	*local_apu_sound     = NULL;
	uint8	*local_apu_oldsound  = NULL;
	uint8	*local_control_data  = NULL;
	uint8	*local_timing_data   = NULL;
	uint8	*local_sa1           = NULL;
	uint8	*local_sa1_registers = NULL;
	uint8	*local_spc7110       = NULL;
	uint8	*local_srtc          = NULL;
	uint8	*local_rtc_data      = NULL;
	uint8	*local_bsx_data      = NULL;
	uint8	*local_dsp1          = NULL;
	uint8	*local_cx4_data      = NULL;
	uint8	*local_superfx       = NULL;
	uint8	*local_movie_data    = NULL;
	uint8	*local_screenshot    = NULL;
	uint8	*local_dummy1        = NULL;
	uint8	*local_dummy2        = NULL;

	do
	{
		result = UnfreezeStructCopy(stream, "CPU", &local_cpu, SnapCPU, COUNT(SnapCPU), version);
		if (result != SUCCESS)
			break;

		result = UnfreezeStructCopy(stream, "REG", &local_registers, SnapRegisters, COUNT(SnapRegisters), version);
		if (result != SUCCESS)
			break;

		result = UnfreezeStructCopy(stream, "PPU", &local_ppu, SnapPPU, COUNT(SnapPPU), version);
		if (result != SUCCESS)
			break;

		result = UnfreezeStructCopy(stream, "DMA", &local_dma, SnapDMA, COUNT(SnapDMA), version);
		if (result != SUCCESS)
			break;

		result = UnfreezeBlockCopy(stream, "VRA", &local_vram, 0x10000);
		if (result != SUCCESS)
			break;

		result = UnfreezeBlockCopy(stream, "RAM", &local_ram, 0x20000);
		if (result != SUCCESS)
			break;

		result = UnfreezeBlockCopy(stream, "SRA", &local_sram, 0x20000);
		if (result != SUCCESS)
			break;

		result = UnfreezeBlockCopy(stream, "FIL", &local_fillram, 0x8000);
		if (result != SUCCESS)
			break;
/*
		result = UnfreezeStructCopy(stream, "APU", &local_apu, SnapAPU, COUNT(SnapAPU), version);
		if (result != SUCCESS)
			break;

		result = UnfreezeStructCopy(stream, "ARE", &local_apu_registers, SnapAPURegisters, COUNT(SnapAPURegisters), version);
		if (result != SUCCESS)
			break;

		result = UnfreezeBlockCopy(stream, "ARA", &local_apu_ram, 0x10000);
		if (result != SUCCESS)
			break;

		// sound DSP snapshot older than 1.52
		result = UnfreezeStructCopy(stream, "SOU", &local_apu_oldsound, SnapOldSoundData, COUNT(SnapOldSoundData), version);
		if (result != SUCCESS && version <= 4)
			break; */

		result = UnfreezeStructCopy(stream, "CTL", &local_control_data, SnapControls, COUNT(SnapControls), version);
		if (result != SUCCESS && version >= 2)
			break;

		result = UnfreezeStructCopy(stream, "TIM", &local_timing_data, SnapTimings, COUNT(SnapTimings), version);
		if (result != SUCCESS && version >= 2)
			break;

		result = UnfreezeStructCopy(stream, "SA1", &local_sa1, SnapSA1, COUNT(SnapSA1), version);
		if (result != SUCCESS && Settings.SA1)
			break;

		result = UnfreezeStructCopy(stream, "SAR", &local_sa1_registers, SnapSA1Registers, COUNT(SnapSA1Registers), version);
		if (result != SUCCESS && Settings.SA1)
			break;

		result = UnfreezeStructCopy(stream, "BSX", &local_bsx_data, SnapBSX, COUNT(SnapBSX), version);
		if (result != SUCCESS && version >= 2 && Settings.BS)
			break;

		SnapshotMovieInfo	mi;

		result = UnfreezeStruct(stream, "MOV", &mi, SnapMovie, COUNT(SnapMovie), version);
		if (result != SUCCESS)
		{
			if (S9xMovieActive())
			{
				result = NOT_A_MOVIE_SNAPSHOT;
				break;
			}
		}
		else
		{
			result = UnfreezeBlockCopy(stream, "MID", &local_movie_data, mi.MovieInputDataSize);
			if (result != SUCCESS)
			{
				if (S9xMovieActive())
				{
					result = NOT_A_MOVIE_SNAPSHOT;
					break;
				}
			}

			if (S9xMovieActive())
			{
				result = S9xMovieUnfreeze(local_movie_data, mi.MovieInputDataSize);
				if (result != SUCCESS)
					break;
			}
		}

		result = UnfreezeStructCopy(stream, "DSP", &local_dsp1, SnapDSP1, COUNT(SnapDSP1), version);
		if (result != SUCCESS && version >= 3 && Settings.DSP)
			break;

		result = UnfreezeBlockCopy(stream, "CX4", &local_cx4_data, 8192);
		if (result != SUCCESS && version >= 4 && Settings.C4)
			break;

	#ifndef ZSNES_FX
		result = UnfreezeStructCopy(stream, "SFX", &local_superfx, SnapFX, COUNT(SnapFX), version);
		if (result != SUCCESS && version >= 4 && Settings.SuperFX)
			break;
	#endif

		// obsolete
		result = UnfreezeStructCopy(stream, "IPU", &local_dummy1, SnapIPPU, COUNT(SnapIPPU), version);
		result = UnfreezeStructCopy(stream, "GFX", &local_dummy2, SnapGFX, COUNT(SnapGFX), version);

		result = UnfreezeStructCopy(stream, "SHO", &local_screenshot, SnapScreenshot, COUNT(SnapScreenshot), version);

		// sound DSP snapshot from 1.52
		/*
		result = UnfreezeStructCopy(stream, "SND", &local_apu_sound, SnapSoundData, COUNT(SnapSoundData), version);
		if (result != SUCCESS && version >= 6)
			break; */

		// SPC7110 and SRTC snapshot from 1.52, incompatible with 1.51.
		result = UnfreezeStructCopy(stream, "S71", &local_spc7110, SnapSPC7110Snap, COUNT(SnapSPC7110Snap), version);
		if (result != SUCCESS && Settings.SPC7110)
			break;

		result = UnfreezeStructCopy(stream, "SRT", &local_srtc, SnapSRTCSnap, COUNT(SnapSRTCSnap), version);
		if (result != SUCCESS && Settings.SRTC)
			break;

		result = UnfreezeStructCopy(stream, "CLK", &local_rtc_data, SnapRTCData, COUNT(SnapRTCData), version);
		if (result != SUCCESS && (Settings.SRTC || Settings.SPC7110RTC))
			break;

		result = SUCCESS;
	} while (false);

	if (result == SUCCESS)
	{
		S9xSetSoundMute(TRUE);

		uint32 old_flags     = CPU.Flags;
		uint32 sa1_old_flags = SA1.Flags;

		S9xReset();

		UnfreezeStructFromCopy(&CPU, SnapCPU, COUNT(SnapCPU), local_cpu, version);

		UnfreezeStructFromCopy(&Registers, SnapRegisters, COUNT(SnapRegisters), local_registers, version);

		UnfreezeStructFromCopy(&PPU, SnapPPU, COUNT(SnapPPU), local_ppu, version);

		struct SDMASnapshot dma_snap;
		UnfreezeStructFromCopy(&dma_snap, SnapDMA, COUNT(SnapDMA), local_dma, version);

		memcpy(Memory.VRAM, local_vram, 0x10000);

		memcpy(Memory.RAM, local_ram, 0x20000);

		memcpy(Memory.SRAM, local_sram, 0x20000);

		memcpy(Memory.FillRAM, local_fillram, 0x8000);

		/*
		UnfreezeStructFromCopy(&APU, SnapAPU, COUNT(SnapAPU), local_apu, version);

		UnfreezeStructFromCopy(&APURegisters, SnapAPURegisters, COUNT(SnapAPURegisters), local_apu_registers, version);

		memcpy(IAPU.RAM, local_apu_ram, 0x10000);

		if (local_apu_sound)
			UnfreezeStructFromCopy(&SoundData, SnapSoundData, COUNT(SnapSoundData), local_apu_sound, version);

		if (local_apu_oldsound)
			UnfreezeStructFromCopy(&OldSoundData, SnapOldSoundData, COUNT(SnapOldSoundData), local_apu_oldsound, version);
        */
		struct SControlSnapshot ctl_snap;
		if (local_control_data)
			UnfreezeStructFromCopy(&ctl_snap, SnapControls, COUNT(SnapControls), local_control_data, version);

		if (local_timing_data)
			UnfreezeStructFromCopy(&Timings, SnapTimings, COUNT(SnapTimings), local_timing_data, version);

		if (local_sa1)
			UnfreezeStructFromCopy(&SA1, SnapSA1, COUNT(SnapSA1), local_sa1, version);

		if (local_sa1_registers)
			UnfreezeStructFromCopy(&SA1Registers, SnapSA1Registers, COUNT(SnapSA1Registers), local_sa1_registers, version);

		if (local_bsx_data)
			UnfreezeStructFromCopy(&BSX, SnapBSX, COUNT(SnapBSX), local_bsx_data, version);

		if (local_dsp1)
			UnfreezeStructFromCopy(&DSP1, SnapDSP1, COUNT(SnapDSP1), local_dsp1, version);

		if (local_cx4_data)
	#ifndef ZSNES_C4
			memcpy(Memory.C4RAM, local_cx4_data, 8192);
	#else
		{
			if (C4Ram)
				memcpy(C4Ram, local_cx4_data, 8192);
		}
	#endif

		if (local_spc7110)
			UnfreezeStructFromCopy(&s7snap, SnapSPC7110Snap, COUNT(SnapSPC7110Snap), local_spc7110, version);

		if (local_srtc)
			UnfreezeStructFromCopy(&srtcsnap, SnapSRTCSnap, COUNT(SnapSRTCSnap), local_srtc, version);

		if (local_rtc_data)
			UnfreezeStructFromCopy(&RTCData, SnapRTCData, COUNT(SnapRTCData), local_rtc_data, version);

	#ifndef ZSNES_FX
		if (local_superfx)
			UnfreezeStructFromCopy(&GSU, SnapFX, COUNT(SnapFX), local_superfx, version);
	#endif

		CPU.Flags |= old_flags & (DEBUG_MODE_FLAG | TRACE_FLAG | SINGLE_STEP_FLAG | FRAME_ADVANCE_FLAG);
		ICPU.ShiftedPB = Registers.PB << 16;
		ICPU.ShiftedDB = Registers.DB << 16;
		S9xSetPCBase(Registers.PBPC);
		S9xUnpackStatus();
		S9xFixCycles();
		Memory.FixROMSpeed();

		for (int d = 0; d < 8; d++)
			DMA[d] = dma_snap.dma[d];
		CPU.InDMA = CPU.InHDMA = FALSE;
		CPU.InDMAorHDMA = CPU.InWRAMDMAorHDMA = FALSE;
		CPU.HDMARanInDMA = 0;

		S9xFixColourBrightness();
		IPPU.ColorsChanged = TRUE;
		IPPU.OBJChanged = TRUE;
		IPPU.RenderThisFrame = TRUE;

		uint8 hdma_byte = Memory.FillRAM[0x420c];
		S9xSetCPU(hdma_byte, 0x420c);

		if (version < 2)
		{
			for (int d = 0; d < 8; d++)
			{
				DMA[d].UnknownByte = Memory.FillRAM[0x430b + (d << 4)];
				DMA[d].UnusedBit43x0 = (Memory.FillRAM[0x4300 + (d << 4)] & 0x20) ? 1 : 0;
			}

			PPU.M7HOFS = PPU.BG[0].HOffset;
			PPU.M7VOFS = PPU.BG[0].VOffset;

			if (Memory.FillRAM[0x4213] == 0)
			{
				// most likely an old savestate
				Memory.FillRAM[0x4213] = Memory.FillRAM[0x4201];
				if (Memory.FillRAM[0x4213] == 0)
					Memory.FillRAM[0x4213] = Memory.FillRAM[0x4201] = 0xFF;
			}

			// Assuming the old savesate was made outside S9xMainLoop().
			// V=0 and HDMA was already initialized.
			CPU.WhichEvent = HC_HDMA_INIT_EVENT;
			CPU.NextEvent = Timings.HDMAInit;
			S9xReschedule();
			S9xUpdateHVTimerPosition();

			/* APU.Flags = Obsolete.SAPU_Flags; */

			ZeroMemory(&ctl_snap, sizeof(ctl_snap));
			ctl_snap.ver = 0;
			ctl_snap.port1_read_idx[0] = Obsolete.SPPU_Joypad1ButtonReadPos;
			ctl_snap.port2_read_idx[0] = Obsolete.SPPU_Joypad2ButtonReadPos;
			ctl_snap.port2_read_idx[1] = Obsolete.SPPU_Joypad3ButtonReadPos;
			// Old snes9x used MouseSpeed[0] for both mice. Weird.
			ctl_snap.mouse_speed[0] = ctl_snap.mouse_speed[1] = Obsolete.SPPU_MouseSpeed[0];
			ctl_snap.justifier_select = 0;
		}

		S9xControlPostLoadState(&ctl_snap);

/*		if (Obsolete.SAPU_OldCycles != APU_OLDCYCLES_OBSOLETE_VALUE) // < 1.5
			APU.Cycles = (Obsolete.SAPU_OldCycles << SNES_APU_ACCURACY);
		IAPU.PC = IAPU.RAM + APURegisters.PC;
		S9xAPUUnpackStatus();
		if (APUCheckDirectPage())
			IAPU.DirectPage = IAPU.RAM + 0x100;
		else
			IAPU.DirectPage = IAPU.RAM;
		
		if (version < 5)
		{
			APU.NextAPUTimerPos = (CPU.Cycles << SNES_APU_ACCURACY);
			APU.APUTimerCounter = 0;
		}

		IAPU.APUExecuting = TRUE;
		Settings.APUEnabled = TRUE;
		S9xSoundPostLoadState(version); */

		if (local_sa1 && local_sa1_registers)
		{
			SA1.Flags |= sa1_old_flags & TRACE_FLAG;
			S9xSA1PostLoadState();
		}

		if (local_spc7110)
			S9xSPC7110PostLoadState(version);

		if (local_srtc)
			S9xSRTCPostLoadState(version);

		if (local_bsx_data)
			S9xBSXPostLoadState();

		if (local_dsp1)
			//S9xDSP1PostLoadState();

	#ifndef ZSNES_FX
		if (local_superfx)
		{
			GSU.pfPlot = fx_PlotTable[GSU.vMode];
			GSU.pfRpix = fx_PlotTable[GSU.vMode + 5];
		}
	#else
		if (Settings.SuperFX)
			S9xSuperFXPostLoadState();
	#endif

		if (Settings.SDD1)
			S9xSDD1PostLoadState();

		if (local_movie_data)
		{
			// restore last displayed pad_read status
			extern bool8	pad_read, pad_read_last;
			bool8			pad_read_temp = pad_read;

			pad_read = pad_read_last;
			S9xUpdateFrameCounter(-1);
			pad_read = pad_read_temp;
		}

		if (local_screenshot)
		{
			SnapshotScreenshotInfo	*ssi = new SnapshotScreenshotInfo;

			UnfreezeStructFromCopy(ssi, SnapScreenshot, COUNT(SnapScreenshot), local_screenshot, version);

			IPPU.RenderedScreenWidth  = min(ssi->Width,  IMAGE_WIDTH);
			IPPU.RenderedScreenHeight = min(ssi->Height, IMAGE_HEIGHT);
			const bool8 scaleDownX = IPPU.RenderedScreenWidth  < ssi->Width;
			const bool8 scaleDownY = IPPU.RenderedScreenHeight < ssi->Height && ssi->Height > SNES_HEIGHT_EXTENDED;
			GFX.DoInterlace = Settings.SupportHiRes ? ssi->Interlaced : 0;

			uint8	*rowpix = ssi->Data;
			uint16	*screen = GFX.Screen;

			for (int y = 0; y < IPPU.RenderedScreenHeight; y++, screen += GFX.RealPPL)
			{
				for (int x = 0; x < IPPU.RenderedScreenWidth; x++)
				{
					uint32	r, g, b;

					r = *(rowpix++);
					g = *(rowpix++);
					b = *(rowpix++);

					if (scaleDownX)
					{
						r = (r + *(rowpix++)) >> 1;
						g = (g + *(rowpix++)) >> 1;
						b = (b + *(rowpix++)) >> 1;

						if (x + x + 1 >= ssi->Width)
							break;
					}

					screen[x] = BUILD_PIXEL(r, g, b);
				}

				if (scaleDownY)
				{
					rowpix += 3 * ssi->Width;
					if (y + y + 1 >= ssi->Height)
						break;
				}
			}

			// black out what we might have missed
			for (uint32 y = IPPU.RenderedScreenHeight; y < (uint32) (IMAGE_HEIGHT); y++)
				memset(GFX.Screen + y * GFX.RealPPL, 0, GFX.RealPPL * 2);

			delete ssi;
		}
		else
		{
			// couldn't load graphics, so black out the screen instead
			for (uint32 y = 0; y < (uint32) (IMAGE_HEIGHT); y++)
				memset(GFX.Screen + y * GFX.RealPPL, 0, GFX.RealPPL * 2);
		}

		S9xSetSoundMute(FALSE);
	}

	if (local_cpu)				delete [] local_cpu;
	if (local_registers)		delete [] local_registers;
	if (local_ppu)				delete [] local_ppu;
	if (local_dma)				delete [] local_dma;
	if (local_vram)				delete [] local_vram;
	if (local_ram)				delete [] local_ram;
	if (local_sram)				delete [] local_sram;
	if (local_fillram)			delete [] local_fillram;
	if (local_apu)				delete [] local_apu;
	if (local_apu_registers)	delete [] local_apu_registers;
	if (local_apu_ram)			delete [] local_apu_ram;
	if (local_apu_sound)		delete [] local_apu_sound;
	if (local_apu_oldsound)		delete [] local_apu_oldsound;
	if (local_control_data)		delete [] local_control_data;
	if (local_timing_data)		delete [] local_timing_data;
	if (local_sa1)				delete [] local_sa1;
	if (local_sa1_registers)	delete [] local_sa1_registers;
	if (local_spc7110)			delete [] local_spc7110;
	if (local_srtc)				delete [] local_srtc;
	if (local_rtc_data)			delete [] local_rtc_data;
	if (local_bsx_data)			delete [] local_bsx_data;
	if (local_dsp1)				delete [] local_dsp1;
	if (local_cx4_data)			delete [] local_cx4_data;
	if (local_superfx)			delete [] local_superfx;
	if (local_movie_data)		delete [] local_movie_data;
	if (local_screenshot)		delete [] local_screenshot;
	if (local_dummy1)			delete [] local_dummy1;
	if (local_dummy2)			delete [] local_dummy2;

	return (result);
}

static int FreezeSize (int size, int type)
{
	switch (type)
	{
		case uint32_ARRAY_V:
		case uint32_INDIR_ARRAY_V:
			return (size * 4);

		case uint16_ARRAY_V:
		case uint16_INDIR_ARRAY_V:
			return (size * 2);

		default:
			return (size);
	}
}

static void FreezeStruct (STREAM stream, const char *name, void *base, FreezeData *fields, int num_fields)
{
	int	len = 0;
	int	i, j;

	for (i = 0; i < num_fields; i++)
	{
		if (SNAPSHOT_VERSION < fields[i].debuted_in)
		{
			fprintf(stderr, "%s[%p]: field has bad debuted_in value %d, > %d.", name, (void *) fields, fields[i].debuted_in, SNAPSHOT_VERSION);
			continue;
		}

		if (SNAPSHOT_VERSION < fields[i].deleted_in)
			len += FreezeSize(fields[i].size, fields[i].type);
	}

	uint8	*block = new uint8[len];
	uint8	*ptr = block;
	uint8	*addr;
	uint16	word;
	uint32	dword;
	int64	qword;
	int		relativeAddr;

	for (i = 0; i < num_fields; i++)
	{
		if (SNAPSHOT_VERSION >= fields[i].deleted_in || SNAPSHOT_VERSION < fields[i].debuted_in)
			continue;

		addr = (uint8 *) base + fields[i].offset;

		// determine real address of indirect-type fields
		// (where the structure contains a pointer to an array rather than the array itself)
		if (fields[i].type == uint8_INDIR_ARRAY_V || fields[i].type == uint16_INDIR_ARRAY_V || fields[i].type == uint32_INDIR_ARRAY_V)
			addr = (uint8 *) (*((pint *) addr));

		// convert pointer-type saves from absolute to relative pointers
		if (fields[i].type == POINTER_V)
		{
			uint8	*pointer    = (uint8 *) *((pint *) ((uint8 *) base + fields[i].offset));
			uint8	*relativeTo = (uint8 *) *((pint *) ((uint8 *) base + fields[i].offset2));
			relativeAddr = pointer - relativeTo;
			addr = (uint8 *) &relativeAddr;
		}

		switch (fields[i].type)
		{
			case INT_V:
			case POINTER_V:
				switch (fields[i].size)
				{
					case 1:
						*ptr++ = *(addr);
						break;

					case 2:
						word = *((uint16 *) (addr));
						*ptr++ = (uint8) (word >> 8);
						*ptr++ = (uint8) word;
						break;

					case 4:
						dword = *((uint32 *) (addr));
						*ptr++ = (uint8) (dword >> 24);
						*ptr++ = (uint8) (dword >> 16);
						*ptr++ = (uint8) (dword >> 8);
						*ptr++ = (uint8) dword;
						break;

					case 8:
						qword = *((int64 *) (addr));
						*ptr++ = (uint8) (qword >> 56);
						*ptr++ = (uint8) (qword >> 48);
						*ptr++ = (uint8) (qword >> 40);
						*ptr++ = (uint8) (qword >> 32);
						*ptr++ = (uint8) (qword >> 24);
						*ptr++ = (uint8) (qword >> 16);
						*ptr++ = (uint8) (qword >> 8);
						*ptr++ = (uint8) qword;
						break;
				}

				break;

			case uint8_ARRAY_V:
			case uint8_INDIR_ARRAY_V:
				memmove(ptr, addr, fields[i].size);
				ptr += fields[i].size;

				break;

			case uint16_ARRAY_V:
			case uint16_INDIR_ARRAY_V:
				for (j = 0; j < fields[i].size; j++)
				{
					word = *((uint16 *) (addr + j * 2));
					*ptr++ = (uint8) (word >> 8);
					*ptr++ = (uint8) word;
				}

				break;

			case uint32_ARRAY_V:
			case uint32_INDIR_ARRAY_V:
				for (j = 0; j < fields[i].size; j++)
				{
					dword = *((uint32 *) (addr + j * 4));
					*ptr++ = (uint8) (dword >> 24);
					*ptr++ = (uint8) (dword >> 16);
					*ptr++ = (uint8) (dword >> 8);
					*ptr++ = (uint8) dword;
				}

				break;
		}
	}

	FreezeBlock(stream, name, block, len);
	delete [] block;
}

static void FreezeBlock (STREAM stream, const char *name, uint8 *block, int size)
{
	char	buffer[20];

	// check if it fits in 6 digits. (letting it go over and using strlen isn't safe)
	if (size <= 999999)
		sprintf(buffer, "%s:%06d:", name, size);
	else
	{
		// to make it fit, pack it in the bytes instead of as digits
		sprintf(buffer, "%s:------:", name);
		buffer[6] = (unsigned char) ((unsigned) size >> 24);
		buffer[7] = (unsigned char) ((unsigned) size >> 16);
		buffer[8] = (unsigned char) ((unsigned) size >> 8);
		buffer[9] = (unsigned char) ((unsigned) size >> 0);
	}

	buffer[11] = 0;

	WRITE_STREAM(buffer, 11, stream);
	WRITE_STREAM(block, size, stream);
}

static int UnfreezeBlock (STREAM stream, const char *name, uint8 *block, int size)
{
	char	buffer[20];
	int		len = 0, rem = 0;
	long	rewind = FIND_STREAM(stream);

	size_t	l = READ_STREAM(buffer, 11, stream);
	buffer[l] = 0;

	if (l != 11 || strncmp(buffer, name, 3) != 0 || buffer[3] != ':')
	{
	err:
		fprintf(stdout, "absent: %s(%d); next: '%.11s'\n", name, size, buffer);
		REVERT_STREAM(stream, FIND_STREAM(stream) - l, 0);
		return (WRONG_FORMAT);
	}

	if (buffer[4] == '-')
	{
		len = (((unsigned char) buffer[6]) << 24)
			| (((unsigned char) buffer[7]) << 16)
			| (((unsigned char) buffer[8]) << 8)
			| (((unsigned char) buffer[9]) << 0);
	}
	else
		len = atoi(buffer + 4);

	if (len <= 0)
		goto err;

	if (len > size)
	{
		rem = len - size;
		len = size;
	}

	ZeroMemory(block, size);

	if (READ_STREAM(block, len, stream) != len)
	{
		REVERT_STREAM(stream, rewind, 0);
		return (WRONG_FORMAT);
	}

	if (rem)
	{
		char	*junk = new char[rem];
		len = READ_STREAM(junk, rem, stream);
		delete [] junk;
		if (len != rem)
		{
			REVERT_STREAM(stream, rewind, 0);
			return (WRONG_FORMAT);
		}
	}

	return (SUCCESS);
}

static int UnfreezeBlockCopy (STREAM stream, const char *name, uint8 **block, int size)
{
	int	result;

	*block = new uint8[size];

	result = UnfreezeBlock(stream, name, *block, size);
	if (result != SUCCESS)
	{
		delete [] (*block);
		*block = NULL;
		return (result);
	}

	return (SUCCESS);
}

static int UnfreezeStruct (STREAM stream, const char *name, void *base, FreezeData *fields, int num_fields, int version)
{
	int		result;
	uint8	*block = NULL;

	result = UnfreezeStructCopy(stream, name, &block, fields, num_fields, version);
	if (result != SUCCESS)
	{
		if (block != NULL)
			delete [] block;
		return (result);
	}

	UnfreezeStructFromCopy(base, fields, num_fields, block, version);
	delete [] block;

	return (SUCCESS);
}

static int UnfreezeStructCopy (STREAM stream, const char *name, uint8 **block, FreezeData *fields, int num_fields, int version)
{
	int	len = 0;

	for (int i = 0; i < num_fields; i++)
	{
		if (version >= fields[i].debuted_in && version < fields[i].deleted_in)
			len += FreezeSize(fields[i].size, fields[i].type);
	}

	return (UnfreezeBlockCopy(stream, name, block, len));
}

static void UnfreezeStructFromCopy (void *sbase, FreezeData *fields, int num_fields, uint8 *block, int version)
{
	uint8	*ptr = block;
	uint16	word;
	uint32	dword;
	int64	qword;
	uint8	*addr;
	void	*base;
	int		relativeAddr;
	int		i, j;

	for (i = 0; i < num_fields; i++)
	{
		if (version < fields[i].debuted_in || version >= fields[i].deleted_in)
			continue;

		base = (SNAPSHOT_VERSION >= fields[i].deleted_in) ? ((void *) &Obsolete) : sbase;
		addr = (uint8 *) base + fields[i].offset;

		if (fields[i].type == uint8_INDIR_ARRAY_V || fields[i].type == uint16_INDIR_ARRAY_V || fields[i].type == uint32_INDIR_ARRAY_V)
			addr = (uint8 *) (*((pint *) addr));

		switch (fields[i].type)
		{
			case INT_V:
			case POINTER_V:
				switch (fields[i].size)
				{
					case 1:
						if (fields[i].offset < 0)
						{
							ptr++; 
							break;
						}

						*(addr) = *ptr++;
						break;

					case 2:
						if (fields[i].offset < 0)
						{
							ptr += 2;
							break;
						}

						word  = *ptr++ << 8;
						word |= *ptr++;
						*((uint16 *) (addr)) = word;
						break;

					case 4:
						if (fields[i].offset < 0)
						{
							ptr += 4;
							break;
						}

						dword  = *ptr++ << 24;
						dword |= *ptr++ << 16;
						dword |= *ptr++ << 8;
						dword |= *ptr++;
						*((uint32 *) (addr)) = dword;
						break;

					case 8:
						if (fields[i].offset < 0)
						{
							ptr += 8;
							break;
						}

						qword  = (int64) *ptr++ << 56;
						qword |= (int64) *ptr++ << 48;
						qword |= (int64) *ptr++ << 40;
						qword |= (int64) *ptr++ << 32;
						qword |= (int64) *ptr++ << 24;
						qword |= (int64) *ptr++ << 16;
						qword |= (int64) *ptr++ << 8;
						qword |= (int64) *ptr++;
						*((int64 *) (addr)) = qword;
						break;

					default:
						assert(0);
						break;
				}

				break;

			case uint8_ARRAY_V:
			case uint8_INDIR_ARRAY_V:
				if (fields[i].offset >= 0)
					memmove(addr, ptr, fields[i].size);
				ptr += fields[i].size;

				break;

			case uint16_ARRAY_V:
			case uint16_INDIR_ARRAY_V:
				if (fields[i].offset < 0)
				{
					ptr += fields[i].size * 2;
					break;
				}

				for (j = 0; j < fields[i].size; j++)
				{
					word  = *ptr++ << 8;
					word |= *ptr++;
					*((uint16 *) (addr + j * 2)) = word;
				}

				break;

			case uint32_ARRAY_V:
			case uint32_INDIR_ARRAY_V:
				if (fields[i].offset < 0)
				{
					ptr += fields[i].size * 4;
					break;
				}

				for (j = 0; j < fields[i].size; j++)
				{
					dword  = *ptr++ << 24;
					dword |= *ptr++ << 16;
					dword |= *ptr++ << 8;
					dword |= *ptr++;
					*((uint32 *) (addr + j * 4)) = dword;
				}

				break;
		}

		if (fields[i].type == POINTER_V)
		{
			relativeAddr = (int) *((pint *) ((uint8 *) base + fields[i].offset));
			uint8	*relativeTo = (uint8 *) *((pint *) ((uint8 *) base + fields[i].offset2));
			*((pint *) (addr)) = (pint) (relativeTo + relativeAddr);
		}
	}
}

bool8 S9xSPCDump (const char *filename)
{/*
	FILE		*fs;
	struct tm	*lt;
	time_t		t;
	uint8		buf[256];

	fs = fopen(filename, "wb");
	if (!fs)
		return (FALSE);

	S9xSetSoundMute(TRUE);

	t = time(NULL);
	lt = localtime(&t);

	ZeroMemory(buf, sizeof(buf));

	strcpy((char *) buf + 0x00, "SNES-SPC700 Sound File Data v0.30");
	buf[0x21] = 26;
	buf[0x22] = 26;
	buf[0x23] = 26;
	buf[0x24] = 30;
	buf[0x25] =  APURegisters.PC       & 0xff;
	buf[0x26] = (APURegisters.PC >> 8) & 0xff;
	buf[0x27] = APURegisters.YA.B.A;
	buf[0x28] = APURegisters.X;
	buf[0x29] = APURegisters.YA.B.Y;
	buf[0x2a] = APURegisters.P;
	buf[0x2b] = APURegisters.S;
	strcpy((char *) buf + 0x4e, Memory.ROMName);
	buf[0x9e] = lt->tm_mday;
	buf[0x9f] = lt->tm_mon + 1;
	buf[0xa0] =  (lt->tm_year + 1900)       & 0xff;
	buf[0xa1] = ((lt->tm_year + 1900) >> 8) & 0xff;
	buf[0xd0] = 0;
	buf[0xd1] = 2;

	fwrite(buf, 1, 256, fs);

	ZeroMemory(buf, sizeof(buf));

	fwrite(IAPU.RAM,     1, 0x10000, fs);
	fwrite(APU.DSP,      1,     128, fs);
	fwrite(buf,          1,      64, fs);
	fwrite(APU.ExtraRAM, 1,      64, fs);

	fclose(fs);

	S9xSetSoundMute(FALSE);
*/
	return (TRUE);
}