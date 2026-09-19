#include <windows.h>

#include "memory_hunt.h"


/*
 * Hacker Disassembler Engine 32
 * Copyright (c) 2006-2009, Vyacheslav Patkov.
 * All rights reserved.
 *
 * hde32.h: C/C++ header file
 *
 */

#ifndef _HDE32_H_
#define _HDE32_H_

/* stdint.h - C99 standard header
 * http://en.wikipedia.org/wiki/stdint.h
 *
 * if your compiler doesn't contain "stdint.h" header (for
 * example, Microsoft Visual C++), you can download file:
 *   http://www.azillionmonkeys.com/qed/pstdint.h
 * and change next line to:
 *   #include "pstdint.h"
 */
#include "stdint.h"

#define F_MODRM 0x00000001
#define F_SIB 0x00000002
#define F_IMM8 0x00000004
#define F_IMM16 0x00000008
#define F_IMM32 0x00000010
#define F_DISP8 0x00000020
#define F_DISP16 0x00000040
#define F_DISP32 0x00000080
#define F_RELATIVE 0x00000100
#define F_2IMM16 0x00000800
#define F_ERROR 0x00001000
#define F_ERROR_OPCODE 0x00002000
#define F_ERROR_LENGTH 0x00004000
#define F_ERROR_LOCK 0x00008000
#define F_ERROR_OPERAND 0x00010000
#define F_PREFIX_REPNZ 0x01000000
#define F_PREFIX_REPX 0x02000000
#define F_PREFIX_REP 0x03000000
#define F_PREFIX_66 0x04000000
#define F_PREFIX_67 0x08000000
#define F_PREFIX_LOCK 0x10000000
#define F_PREFIX_SEG 0x20000000
#define F_PREFIX_ANY 0x3f000000

#define PREFIX_SEGMENT_CS 0x2e
#define PREFIX_SEGMENT_SS 0x36
#define PREFIX_SEGMENT_DS 0x3e
#define PREFIX_SEGMENT_ES 0x26
#define PREFIX_SEGMENT_FS 0x64
#define PREFIX_SEGMENT_GS 0x65
#define PREFIX_LOCK 0xf0
#define PREFIX_REPNZ 0xf2
#define PREFIX_REPX 0xf3
#define PREFIX_OPERAND_SIZE 0x66
#define PREFIX_ADDRESS_SIZE 0x67

#pragma pack(push, 1)

typedef struct
{
	uint8_t len;
	uint8_t p_rep;
	uint8_t p_lock;
	uint8_t p_seg;
	uint8_t p_66;
	uint8_t p_67;
	uint8_t opcode;
	uint8_t opcode2;
	uint8_t modrm;
	uint8_t modrm_mod;
	uint8_t modrm_reg;
	uint8_t modrm_rm;
	uint8_t sib;
	uint8_t sib_scale;
	uint8_t sib_index;
	uint8_t sib_base;
	union
	{
		uint8_t imm8;
		uint16_t imm16;
		uint32_t imm32;
	} imm;
	union
	{
		uint8_t disp8;
		uint16_t disp16;
		uint32_t disp32;
	} disp;
	uint32_t flags;
} hde32s;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

/* __cdecl */
unsigned int hde32_disasm(const void* code, hde32s* hs);

#ifdef __cplusplus
}
#endif

#endif /* _HDE32_H_ */


size_t GetModuleSize(HMODULE hModule)
{
	if (!hModule)
		return 0;

	// Base pointer
	auto base = reinterpret_cast<BYTE*>(hModule);

	// DOS header
	auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return 0;

	// NT headers
	auto ntHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dosHeader->e_lfanew);
	if (ntHeader->Signature != IMAGE_NT_SIGNATURE)
		return 0;

	// Size of the image in memory
	return ntHeader->OptionalHeader.SizeOfImage;
}

bool is_readable_memory(const MEMORY_BASIC_INFORMATION& mbi)
{
	if (mbi.State != MEM_COMMIT)
		return false;
	DWORD prot = mbi.Protect & ~PAGE_GUARD & ~PAGE_NOCACHE;
	if (prot == 0)
		return false;

	return (prot & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0;
}

std::uintptr_t findPatternInModule(uint8_t* hModule, size_t iSize, byte* pattern, int patternsize)
{
	if (!hModule || !pattern)
		return 0;

	BYTE* base = hModule;
	SIZE_T size = iSize;

	SIZE_T offset = 0;
	while (offset < size)
	{
		MEMORY_BASIC_INFORMATION mbi;
		if (!VirtualQuery(base + offset, &mbi, sizeof(mbi)))
			break;
		if (is_readable_memory(mbi))
		{
			SIZE_T regionSize = std::min<SIZE_T>(mbi.RegionSize, size - offset);
			BYTE* regionBase = reinterpret_cast<BYTE*>(mbi.BaseAddress);
			for (SIZE_T i = 0; i + patternsize <= regionSize; ++i)
			{
				if (memcmp(regionBase + i, pattern, patternsize) == 0)
				{
					return reinterpret_cast<std::uintptr_t>(regionBase + i);
				}
			}
		}
		offset += mbi.RegionSize;
	}
	return 0;
}

void* findCallFunction(const uint8_t* start, uintptr_t* address, uint32_t limit)
{
	//////////////////////////////////////////////////////////////////////////
	//	 Opcode     |   Mnemonic    |           Description					//
	//----------------------------------------------------------------------//
	//	 E8 cw		|	CALL rel16	|	Call near, relative, displacement   //
	//				|				|	  relative to next instruction		//
	//----------------------------------------------------------------------//
	//	 E8 cd		|  CALL rel32	|	Call near, relative, displacement   //
	//				|				|	  relative to next instruction		//
	//----------------------------------------------------------------------//

	// FF call opcodes are kinda hard to solve
	// opcode 9A is almost never used

	const byte callrelative = 0xE8;


	for (uint32_t i = 0; i < limit;)
	{
		hde32s hdeinfo;
		hde32_disasm(start + i, &hdeinfo);

		if (hdeinfo.flags & F_ERROR)
			break;

		if (hdeinfo.opcode != callrelative)
		{
			i += hdeinfo.len;
			continue;
		}

		uint32_t rel = *(int32_t*)((start + i) + 1);
		const byte* next = (start + i) + hdeinfo.len;
		if (address)
			*address = (uintptr_t)(next);

		return (void*)(next + rel);
	}
	return nullptr;
}

void* findCallnJmpFunction(const uint8_t* start, uintptr_t* address, uint32_t limit)
{
	const byte callrelative = 0xE8;
	const byte jmprelative = 0xE9;

	for (uint32_t i = 0; i < limit;)
	{
		hde32s hdeinfo;
		hde32_disasm(start + i, &hdeinfo);

		if (hdeinfo.flags & F_ERROR)
			break;

		if ((hdeinfo.opcode != callrelative) && (hdeinfo.opcode != jmprelative))
		{
			i += hdeinfo.len;
			continue;
		}

		uint32_t rel = *(int32_t*)((start + i) + 1);
		const byte* next = (start + i) + hdeinfo.len;
		if (address)
			*address = (uintptr_t)(next);

		return (void*)(next + rel);
	}
	return nullptr;
}

void* findJmpFunction(const uint8_t* start, uintptr_t* address, uint32_t limit)
{
	//////////////////////////////////////////////////////////////////////////
	//	 Opcode     |   Mnemonic    |           Description					//
	//----------------------------------------------------------------------//
	//	 E9 cd		|  JMP rel32	|	Jump near, relative, displacement   //
	//				|				|	  relative to next instruction		//
	//----------------------------------------------------------------------//

	const byte jmprelative = 0xE9;

	for (uint32_t i = 0; i < limit;)
	{
		hde32s hdeinfo;
		hde32_disasm(start + i, &hdeinfo);

		if (hdeinfo.flags & F_ERROR)
			break;

		if (hdeinfo.opcode != jmprelative)
		{
			i += hdeinfo.len;
			continue;
		}

		uint32_t rel = *(int32_t*)((start + i) + 1);
		const byte* next = (start + i) + hdeinfo.len;
		if (address)
			*address = (uintptr_t)(next);

		return (void*)(next + rel);
	}
	return nullptr;
}

void* findMovVarAddress(const uint8_t* start, uintptr_t* address, uint32_t limit)
{
	//////////////////////////////////////////////////////////////////////////
	//	 Opcode     |   Mnemonic		|           Description				//
	//----------------------------------------------------------------------//
	//	 89 /r		| MOV r/m16, r16	|		Move r16 into r/m16			//
	//				|					|									//
	//----------------------------------------------------------------------//
	//	 89 /r		| MOV r/m32, r32	|		Move r32 into r/m32			//
	//				|					|									//
	//----------------------------------------------------------------------//
	//	 8B /r		| MOV m16, r/r16	|		Move r/r16 into m16			//
	//				|					|									//
	//----------------------------------------------------------------------//
	//	 8B /r		| MOV m32, r/r32	|		Move r/r32 into m32			//
	//				|					|									//
	//----------------------------------------------------------------------//
	//	 A1 /r		| MOV AX, moffs16	|		Move word at (seg:offset)	//
	//				|					|		to AL						//
	//----------------------------------------------------------------------//
	//	 A1 /r		| MOV EAX, moffs32	|		Move doubleword at			//
	//				|					|		(seg:offset) to EAX			//
	//----------------------------------------------------------------------//

	// The moffs8, moffs16, and moffs32 operands specify a simple offset
	//	relative to the segment base, where 8, 16, and 32 refer to the size of the data.
	//	The address-size attribute of the instruction determines the size of the offset,
	//	either 16 or 32 bits.
	//
	// In 32-bit mode, the assembler may insert the 16-bit operand-size prefix with this instruction.

	const byte movopcode_r = 0x8B; // moves registry into memory (only if mod = 0 & rm = 101)
	const byte movopcode_m = 0xA1; // moves memory into registry

	for (uint32_t i = 0; i < limit;)
	{
		hde32s hdeinfo;
		hde32_disasm(start + i, &hdeinfo);

		if (hdeinfo.flags & F_ERROR)
			break;

		if (hdeinfo.opcode == movopcode_r)
		{
			uint32_t disp = *(uint32_t*)((start + i) + (hdeinfo.len - 4));

			//	If the "mod" field is 11, the "r/m" field encodes a register in the same manner
			//	as the "reg" field. However, if the "mod" field is anything else (00, 01, or 10),
			//  the "r/m" field specifies an addressing mode.
			byte mod = hdeinfo.modrm_mod;
			byte rm = hdeinfo.modrm_rm;

			if (address)
				*address = (uintptr_t)(disp);

			if (mod == 0 && rm == 5) // if mod = 0 and rm is 101([disp32])
			{
				return (void*)disp;
			}
		}
		else if (hdeinfo.opcode == movopcode_m)
		{
			byte* info = (byte*)((start + i) + (hdeinfo.len - 4)); // skip opcode
			uintptr_t varaddress = *(uintptr_t*)(info);
			return (void*)varaddress;
		}

		i += hdeinfo.len;
	}

	return nullptr;
}

void* findCmpVarAddress(const uint8_t* start, uintptr_t* address, uint32_t limit)
{
	const byte cmp1 = 0x83; // cmp memory, number (ex: cmp dword_2D59AE0, 5)
	const byte cmp2 = 0x39; // cmp memory, registry (ex: cmp dword_2D59AE0, eax)
	const byte cmp3 = 0x3B; // cmp reg, memory (ex: cmp eax, dword_2D59AE0)

	for (uint32_t i = 0; i < limit;)
	{
		hde32s hdeinfo;
		hde32_disasm(start + i, &hdeinfo);

		i += hdeinfo.len;

		if (hdeinfo.flags & F_ERROR)
			break;

		if (hdeinfo.modrm_mod != 0 || hdeinfo.modrm_rm != 5 || !(hdeinfo.flags & F_MODRM))
			continue;


		if ((hdeinfo.opcode == cmp1 && hdeinfo.modrm_reg == 7) || hdeinfo.opcode == cmp2 || hdeinfo.opcode == cmp3)
		{
			uint32_t disp = hdeinfo.disp.disp32;

			if (address)
				*address = (uintptr_t)(disp);

			return (void*)disp;
		}
	}

	return nullptr;
}