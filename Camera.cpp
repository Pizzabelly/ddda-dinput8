#include "stdafx.h"
#include "Camera.h"
#include <winbase.h>
#include <xmmintrin.h>
#include <winDNS.h>

// Also taken from CheatEngine table by Cielos
// http://forum.cheatengine.org/viewtopic.php?p=5641841#5641841

LPBYTE hpWrite1Offset;
LPBYTE hpWrite1Original;
DWORD bUndead = 0x0;
DWORD pPartyBase[4] = { 0 };
DWORD pPartyHP[4] = { 0 };
void __declspec(naked) hpWrite1Hook()
{
	__asm
	{
		push ebx
		xor ebx,ebx
	invalidpointerschk:
		cmp [pPartyHP+ebx*0x4],0x0
		je forward1
		pushad
		push 0x00000004
		push [pPartyHP+ebx*0x4]
		call IsBadReadPtr
		test eax,eax
		popad
		jz forward1
		mov [pPartyHP+ebx*0x4],0x0
		mov [pPartyBase+ebx*0x4],0x0
	forward1:
		inc ebx
		cmp ebx,0x4
		jl invalidpointerschk

		pop ebx
		cmp [ecx+0x8],0xffffffff
		jne forward2
		pushad
		push 0x00000004
		push ecx
		call IsBadReadPtr
		test eax,eax
		popad
		jnz originalcode
		mov edx,[ecx]
		pushad
		push 0x00000004
		push edx
		call IsBadReadPtr
		test eax,eax
		popad
		jnz originalcode
		cmp [edx+0x74],0x0
		je originalcode

	forward2:
		test eax,eax
		jz originalcode

		mov edx,[ecx+0x8]
		cmp edx,0x4
		jae originalcode
		mov [pPartyHP+edx*0x4],eax
		mov [pPartyBase+edx*0x4],edi

	originalcode:
		movss [ecx+0x0000029C],xmm1

		jmp hpWrite1Original
	}
}

LPBYTE rhpReadOnHitOffset;
LPBYTE rhpReadOnHitOriginal;
void __declspec(naked) rhpReadOnHitHook()
{
	__asm
	{
		cmp dword ptr [bUndead],0x0
		je originalcode

		xor ecx,ecx
	backward1:
		cmp [pPartyHP+ecx*0x4],edi
		je forward1
		inc ecx
		cmp ecx,0x4
		jl backward1
		jmp originalcode

	forward1:
		cmp byte ptr [bUndead+ecx],0x0
		je originalcode
		movss xmm0,[edi+0x8]
		comiss xmm0,[esp+0xc]
		ja originalcode
		mov [edi+0x8],0x3f800000 // (float)1
		movss xmm0,[esp+0xc]
		addss xmm0,[edi+0x8]
		movss [edi+0x8],xmm0

	originalcode:
		movss xmm0,[edi+0x08]

		jmp rhpReadOnHitOriginal
	}
}

LPBYTE rhpFallDamageCalOffset;
LPBYTE rhpFallDamageCalOriginal;
void __declspec(naked) rhpFallDamageCalHook()
{
	__asm
	{
		cmp dword ptr [bUndead],0x0
		je originalcode

		xor edx,edx
	backward1:
		cmp [pPartyHP+edx*0x4],ebp
		je forward1
		inc edx
		cmp edx,0x4
		jl backward1
		jmp originalcode

	forward1:
		cmp byte ptr [bUndead+edx],0x0
		je originalcode
		xorps xmm1,xmm1
		comiss xmm0,xmm1
		jae originalcode

		addss xmm0,[ebp+0x8]
		mov edx,0x3f800000 // (float)1
		movd xmm1,edx
		comiss xmm0,xmm1
		jae forward2
		mov edx,0xbf800000 // (float)-1
		movd xmm1,edx
		movss xmm0,[ebp+0x8]
		addss xmm0,xmm1
		mulss xmm0,xmm1
		movss [esp+0x20],xmm0

	forward2:
		movss xmm0,[esp+0x20]

	originalcode:
		mov edx,[esp+0x24]
		push edx

		jmp rhpFallDamageCalOriginal
	}
}

LPBYTE nvXReadOffset;
LPBYTE nvXReadOriginal;
__m128 vNV = { 0 };
DWORD pNVector = 0;
void __declspec(naked) nvXReadHook()
{
	__asm
	{
		movss xmm0,[ecx]

		movss xmm1,[ecx+0x8]
		shufps xmm1,xmm1,0xc6
		addps xmm0,xmm1

		movss xmm1,[ecx+0x18]
		shufps xmm1,xmm1,0xe1
		addps xmm0,xmm1

		movaps [vNV],xmm0

		lea eax,[ecx+0x20]
		mov [pNVector],eax

		mov eax,[esp+0x04]
		fstp dword ptr [eax]

		jmp nvXReadOriginal
	}
}

LPBYTE axisXYWriteOffset;
LPBYTE axisXYWriteOriginal;
DWORD pMovementInfo = 0;
void __declspec(naked) axisXYWriteHook()
{
	__asm
	{
		mov [pMovementInfo],edi
		movss dword ptr [edi+0x14],xmm7
		jmp axisXYWriteOriginal
	}
}

LPBYTE camZReadOffset;
LPBYTE camZReadOriginal;
BYTE bUseCusCamOffsets = 1;
BYTE bCamChangeMethod = 0;
BYTE bDynamicCusCamXOffset = 0;
DWORD pCamBase1 = 0;
DWORD fCusCamZOffset[6] = {
	0xc1f00000, // (float)-30
	0x40000000, // (float)2
	0x00000000, // (float)0
	0,			// 0
	0xc2080000, // (float)-34
	0xc1a00000	// (float)-20
};
DWORD fCusCamXOffset[6] = {
	0x42200000, // (float)40
	0x40000000, // (float)2
	0x00000000, // (float)0
	0,			// 0
	0x42200000, // (float)40
	0x00000000	// (float)0
};
DWORD fCusFOVOffset[6] = {
	0xc0c00000, // (float)-6
	0x3dcccccd, // (float)0.1
	0x00000000, // (float)0
	0,			// 0
	0xc1400000, // (float)-12
	0xc0c00000	// (float)-6
};
DWORD fLastUseXAxis = 0x3f800000; // (float)1
void __declspec(naked) camZReadHook()
{
	__asm
	{
		push edx
		lea eax,[eax-0x10]
		mov [pCamBase1],eax
		lea eax,[eax+0x10]

		movss xmm0,[fCusCamZOffset+0x10]
		movss xmm1,[fCusFOVOffset+0x10]
		mov edx,[pPartyBase]
		test edx,edx
		jz forward1
		test byte ptr [edx+0x2360],0x80
		je forward1
		// in battle
		movss xmm0,[fCusCamZOffset+0x14]
		movss xmm1,[fCusFOVOffset+0x14]

	forward1:
		movss [fCusCamZOffset],xmm0
		movss [fCusFOVOffset],xmm1

		mov edx,0x41f00000 // (float)30
		movd xmm0,edx
		cmp dword ptr [fCusCamZOffset],0x0
		je forward2
		movss xmm0,[fCusCamZOffset]

	forward2:
		divss xmm0,[fCusCamZOffset+0x4]

		push 0x40800000 // (float)4
		mulss xmm0,[esp]
		movss [esp],xmm0

		mov edx,0x40c00000 // (float)6
		movd xmm0,edx
		cmp dword ptr [fCusFOVOffset],0x0
		je forward3
		movss xmm0,[fCusFOVOffset]

	forward3:
		divss xmm0,[esp]

		mov [esp],0x0
		comiss xmm0,[esp]
		mov dword ptr [esp],0xbf800000 // (float)-1
		ja forward4
		mulss xmm0,[esp]
	forward4:
		movss [fCusFOVOffset+0x4],xmm0

		add esp,0x4

		pop edx
		fld dword ptr [eax+0x04]
		fstp dword ptr [ecx+0x04]

		jmp camZReadOriginal
	}
}

LPBYTE camZ2WriteOffset;
LPBYTE camZ2WriteOriginal;
void __declspec(naked) camZ2WriteHook()
{
	__asm
	{
		cmp dword ptr [pCamBase1],0x0
		je originalcode

		fld dword ptr [eax+0x64]

		cmp byte ptr [bUseCusCamOffsets],0x1
		jne notusecustom2

		pushad
		push 0x00000004
		push [pPartyBase]
		call IsBadReadPtr
		test eax,eax
		popad
		jnz addcustomzoffset2
		push eax
		mov eax,[pPartyBase]
		test byte ptr [eax+0x273e],0x1
		pop eax
		jne notusecustom2  // in conversation

		push eax
		mov eax,[pPartyBase]
		cmp byte ptr [eax+0x355b],0x9 // is magick bow
		pop eax
		je forward1
		push eax
		mov eax,[pPartyBase]
		cmp byte ptr [eax+0x355b],0xa // is bow
		pop eax
		je forward1
		push eax
		mov eax,[pPartyBase]
		cmp byte ptr [eax+0x355b],0xb // is longbow
		pop eax
		je forward1
		jmp usecustom2

	forward1:
		push eax
		mov eax,[pPartyBase]
		test byte ptr [eax+0x273d],0x8
		pop eax
		jne notusecustom2 // using bow

	usecustom2:
		// not using bow / not in conversation
		cmp byte ptr [bCamChangeMethod],0x1
		je forward2
		push [fCusCamZOffset]
		pop [fCusCamZOffset+0x8]
		jmp addcustomzoffset2

	forward2:
		movss xmm0,[fCusCamZOffset+0x8]
		comiss xmm0,[fCusCamZOffset]
		ja forward3
		addss xmm0,[fCusCamZOffset+0x4]
		movss [fCusCamZOffset+0x8],xmm0
		comiss xmm0,[fCusCamZOffset]
		jbe docustomzoffsetend2
		jmp hardsetcustomzoffset2

	forward3:
		subss xmm0,[fCusCamZOffset+0x4]
		movss [fCusCamZOffset+0x8],xmm0
		comiss xmm0,[fCusCamZOffset]
		jae docustomzoffsetend2

	hardsetcustomzoffset2:
		movss xmm0,[fCusCamZOffset]
		movss [fCusCamZOffset+0x8],xmm0

	docustomzoffsetend2:
		xorps xmm0,xmm0
		jmp addcustomzoffset2

	notusecustom2:
		// using bow / in conversation
		cmp byte ptr [bCamChangeMethod],0x1
		je forward4
		fldz
		fstp dword ptr [fCusCamZOffset+0x8]
		jmp addcustomzoffset2

	forward4:
		xorps xmm0,xmm0
		comiss xmm0,[fCusCamZOffset+0x8]
		ja forward5
		movss xmm0,[fCusCamZOffset+0x8]
		subss xmm0,[fCusCamZOffset+0x4]
		movss [fCusCamZOffset+0x8],xmm0
		xorps xmm0,xmm0
		comiss xmm0,[fCusCamZOffset+0x8]
		jbe addcustomzoffset2
		jmp resetcustomzoffsetend2

	forward5:
		movss xmm0,[fCusCamZOffset+0x8]
		addss xmm0,[fCusCamZOffset+0x4]
		movss [fCusCamZOffset+0x8],xmm0
		xorps xmm0,xmm0
		comiss xmm0,[fCusCamZOffset+0x8]
		jae addcustomzoffset2

	resetcustomzoffsetend2:
		movss [fCusCamZOffset+0x8],xmm0
		jmp addcustomzoffset2

	addcustomzoffset2:
		fadd dword ptr [fCusCamZOffset+0x8]

	originalcode:
		fstp dword ptr [edi+0x00000094]

		jmp camZ2WriteOriginal
	}
}

LPBYTE FOVReadOffset;
LPBYTE FOVReadOriginal;
void __declspec(naked) FOVReadHook()
{
	__asm
	{
		cmp dword ptr [pCamBase1],0x0
		je originalcode

		xorps xmm1,xmm1

		cmp byte ptr [bUseCusCamOffsets],0x1
		jne notusecustomfov3

		mov edi,[pPartyBase]
		test edi,edi
		jz addcustomfovoffset3
		test byte ptr [edi+0x273e],0x1
		jne notusecustomfov3  // in conversation

		cmp byte ptr [edi+0x355b],0x9 // is magick bow
		je forward1
		cmp byte ptr [edi+0x355b],0xa // is bow
		je forward1
		cmp byte ptr [edi+0x355b],0xb // is longbow
		je forward1
		jmp usecustomfov3

	forward1:
		test byte ptr [edi+0x273d],0x8
		jne notusecustomfov3 // using bow

	usecustomfov3:
		// not using bow / not in conversation
		cmp byte ptr [bCamChangeMethod],0x1
		je forward2
		push [fCusFOVOffset]
		pop [fCusFOVOffset+0x8]
		jmp addcustomfovoffset3

	forward2:
		movss xmm1,[fCusFOVOffset+0x8]
		comiss xmm1,[fCusFOVOffset]
		ja forward3
		addss xmm1,[fCusFOVOffset+0x4]
		movss [fCusFOVOffset+0x8],xmm1
		comiss xmm1,[fCusFOVOffset]
		jbe docustomfovend3
		jmp hardsetcustomfov3

	forward3:
		subss xmm1,[fCusFOVOffset+0x4]
		movss [fCusFOVOffset+0x8],xmm1
		comiss xmm1,[fCusFOVOffset]
		jae docustomfovend3

	hardsetcustomfov3:
		movss xmm1,[fCusFOVOffset]
		movss [fCusFOVOffset+0x8],xmm1

	docustomfovend3:
		xorps xmm1,xmm1
		jmp addcustomfovoffset3

	notusecustomfov3:
		// using bow / in conversation
		cmp byte ptr [bCamChangeMethod],0x1
		je forward4
		fldz
		fstp dword ptr [fCusFOVOffset+0x8]
		jmp addcustomfovoffset3

	forward4:
		xorps xmm1,xmm1
		comiss xmm1,[fCusFOVOffset+0x8]
		ja forward5
		movss xmm1,[fCusFOVOffset+0x8]
		subss xmm1,[fCusFOVOffset+0x4]
		movss [fCusFOVOffset+0x8],xmm1
		xorps xmm1,xmm1
		comiss xmm1,[fCusFOVOffset+0x8]
		jbe addcustomfovoffset3
		jmp resetcustomfovend3

	forward5:
		movss xmm1,[fCusFOVOffset+0x8]
		addss xmm1,[fCusFOVOffset+0x4]
		movss [fCusFOVOffset+0x8],xmm1
		xorps xmm1,xmm1
		comiss xmm1,[fCusFOVOffset+0x8]
		jae addcustomfovoffset3

	resetcustomfovend3:
		movss [fCusFOVOffset+0x8],xmm1
		jmp addcustomfovoffset3

	addcustomfovoffset3:
		addss xmm0,[fCusFOVOffset+0x8]

	originalcode:
		addss xmm0,[esi+0x3C]

		jmp FOVReadOriginal
	}
}

void __stdcall UpdateCamera()
{
}

LPBYTE camCoords2PostWriteOffset;
LPBYTE camCoords2PostWriteOriginal;
void __declspec(naked) camCoords2PostWriteHook()
{
	__asm
	{
		sub esp,0x16
		movdqu xmmword ptr [esp],xmm1
		cmp dword ptr [pCamBase1],0x0
		je originalcode

		cmp byte ptr [bUseCusCamOffsets],0x1
		jne notusecustom4

		mov eax,[pPartyBase]
		test eax,eax
		jz addcustomxoffset4
		movss xmm0,[fCusCamXOffset+0x10]
		test byte ptr [eax+0x2360],0x80
		je forward1
		// in battle
		movss xmm0,[fCusCamXOffset+0x14]
	forward1:
		cmp byte ptr [bDynamicCusCamXOffset],0x0
		je forward2
		mov eax,[pMovementInfo]
		test eax,eax
		jz forward2
		cmp [eax+0x14],0x0
		je forward2
		push [eax+0x14]
		pop [fLastUseXAxis]

		cmp byte ptr [bDynamicCusCamXOffset],0x1
		jne forward2
		mov eax,0xbf800000 // (float)-1
		movd xmm1,eax
		mulss xmm1,[fLastUseXAxis]
		movss [fLastUseXAxis],xmm1

	forward2:
		mulss xmm0,[fLastUseXAxis]
		mov eax,[pPartyBase]
		movss [fCusCamXOffset],xmm0
		test byte ptr [eax+0x273e],0x1
		jne notusecustom4  // in conversation

		cmp byte ptr [eax+0x355b],0x9 // is magick bow
		je forward3
		cmp byte ptr [eax+0x355b],0xa // is bow
		je forward3
		cmp byte ptr [eax+0x355b],0xb // is longbow
		je forward3
		jmp usecustom4

	forward3:
		test byte ptr [eax+0x273d],0x8
		jne notusecustom4 // using bow

	usecustom4:
		// not using bow / not in conversation
		cmp byte ptr [bCamChangeMethod],0x1
		je forward4
		push [fCusCamXOffset]
		pop [fCusCamXOffset+0x8]
		jmp addcustomxoffset4

	forward4:
		movss xmm0,[fCusCamXOffset+0x8]
		comiss xmm0,[fCusCamXOffset]
		ja forward5
		addss xmm0,[fCusCamXOffset+0x4]
		movss [fCusCamXOffset+0x8],xmm0
		comiss xmm0,[fCusCamXOffset]
		jbe docustomxoffsetend4
		jmp hardsetcustomxoffset4

	forward5:
		subss xmm0,[fCusCamXOffset+0x4]
		movss [fCusCamXOffset+0x8],xmm0
		comiss xmm0,[fCusCamXOffset]
		jae docustomxoffsetend4

	hardsetcustomxoffset4:
		movss xmm0,[fCusCamXOffset]
		movss [fCusCamXOffset+0x8],xmm0

	docustomxoffsetend4:
		jmp addcustomxoffset4

	notusecustom4:
		// using bow / in conversation
		cmp byte ptr [bCamChangeMethod],0x1
		je forward6
		fldz
		fstp dword ptr [fCusCamXOffset+0x8]
		jmp addcustomxoffset4

	forward6:
		xorps xmm0,xmm0
		comiss xmm0,[fCusCamXOffset+0x8]
		ja forward7
		movss xmm0,[fCusCamXOffset+0x8]
		subss xmm0,[fCusCamXOffset+0x4]
		movss [fCusCamXOffset+0x8],xmm0
		xorps xmm0,xmm0
		comiss xmm0,[fCusCamXOffset+0x8]
		jbe addcustomxoffset4
		jmp resetcustomxoffsetend4

	forward7:
		movss xmm0,[fCusCamXOffset+0x8]
		addss xmm0,[fCusCamXOffset+0x4]
		movss [fCusCamXOffset+0x8],xmm0
		xorps xmm0,xmm0
		comiss xmm0,[fCusCamXOffset+0x8]
		jae addcustomxoffset4

	resetcustomxoffsetend4:
		movss [fCusCamXOffset+0x8],xmm0
		jmp addcustomxoffset4

	addcustomxoffset4:
		// do x
		movss xmm0,[fCusCamXOffset+0x8]
		shufps xmm0,xmm0,0xc4

		movaps xmm1,[vNV]
		mulps xmm0,xmm1

		movups xmm1,[edi+0x90]
		addps xmm1,xmm0
		movss [edi+0x90],xmm1
		shufps xmm1,xmm1,0x39
		movss [edi+0x94],xmm1
		shufps xmm1,xmm1,0x39
		movss [edi+0x98],xmm1

		call UpdateCamera

	originalcode:
		xorps xmm0,xmm0
		movdqu xmm1,xmmword ptr [esp]
		add esp,0x16
		mov eax,[edi+0x00000180]

		jmp camCoords2PostWriteOriginal
	}
}

float  Lx, Ly, Rx, Ry;
LPVOID pControllerState = nullptr;
// This is a problem with 2 or more controllers because
// it only addresses the first controller.
void __stdcall UpdateController()
{
	if (pControllerState)
	{
		Lx = *(float *)(((BYTE *)pControllerState) + 0x17b0);
		Ly = *(float *)(((BYTE *)pControllerState) + 0x17b4);
		Rx = *(float *)(((BYTE *)pControllerState) + 0x17b8);
		Ry = *(float *)(((BYTE *)pControllerState) + 0x17bc);
	}
}

LPBYTE pGetControllerHook1;
LPVOID oGetControllerHook;
void __declspec(naked) HGetController()
{
	__asm	mov		pControllerState, ebp
	__asm	call	UpdateController
	__asm	jmp		oGetControllerHook
}

void renderCameraUI()
{
	if (ImGui::CollapsingHeader("Camera"))
	{
		ImGui::Text("Controller:");
		ImGui::Text(" Pointer: %x", pControllerState);
		ImGui::Text(" Lx: %f", Lx);
		ImGui::Text(" Ly: %f", Ly);
		ImGui::Text(" Rx: %f", Rx);
		ImGui::Text(" Ry: %f", Ry);

		ImGui::Text("CamBase1: %lx", pCamBase1);

		float *zOffset = (float *)fCusCamZOffset;
		float *xOffset = (float *)fCusCamXOffset;
		float *fovOffset = (float *)fCusFOVOffset;
		ImGui::DragFloat("Camera Z Offset:", &zOffset[4], 1.f);
		ImGui::DragFloat("Camera X Offset:", &xOffset[4], 1.f);
		ImGui::DragFloat("Camera FOV:", &fovOffset[4], 1.f);
		ImGui::DragFloat("Battle Camera Z Offset:", &zOffset[5], 1.f);
		ImGui::DragFloat("Battle Camera X Offset:", &xOffset[5], 1.f);
		ImGui::DragFloat("Battle Camera FOV", &fovOffset[5], 1.f);
	}
}

void Hooks::Camera()
{
	BYTE hpWrite1AOB[] = { 0xF3, 0x0F, 0x11, 0x89, 0x9C, 0x02, 0x00, 0x00, 0x89, 0xB7, 0xF4, 0x3D, 0x00, 0x00 };
	BYTE rhpReadOnHitAOB[] = { 0xF3, 0x0F, 0x10, 0x47, 0x08, 0x8B, 0x8F, 0xB4, 0x01, 0x00, 0x00 };
	BYTE rhpFallDamageCalAOB[] = { 0x8b, 0x54, 0x24, 0x24, 0x52, 0x8B, 0xC7 };
	BYTE nvXReadAOB[] = { 0xD9, 0x84, 0xC1, 0xC0, 0xCD, 0x04, 0x00, 0x8D, 0x8C, 0xC1, 0xC0, 0xCD, 0x04, 0x00, 0x8B, 0x44, 0x24, 0x04, 0xD9, 0x18 };
	BYTE axisXYWriteAOB[] = { 0xF3, 0x0F, 0x11, 0x7F, 0x14, 0xF3, 0x0F, 0x11, 0x77, 0x18 };

	if (FindSignature("hpWrite1AOB", hpWrite1AOB, &hpWrite1Offset) &&
		FindSignature("rhpReadOnHitAOB", rhpReadOnHitAOB, &rhpReadOnHitOffset) &&
		FindSignature("rhpFallDamageCalAOB", rhpFallDamageCalAOB, &rhpFallDamageCalOffset) &&
		FindSignature("nvXReadAOB", nvXReadAOB, &nvXReadOffset) &&
		FindSignature("axisXYWriteAOB", axisXYWriteAOB, &axisXYWriteOffset))
	{
		CreateHook("hpWrite1Hook", hpWrite1Offset, &hpWrite1Hook, &hpWrite1Original, true);
		hpWrite1Original += 8;

		CreateHook("rhpReadOnHitHook", rhpReadOnHitOffset, &rhpReadOnHitHook, &rhpReadOnHitOriginal, true);
		rhpReadOnHitOriginal += 5;

		CreateHook("rhpFallDamageCalHook", rhpFallDamageCalOffset, &rhpFallDamageCalHook, &rhpFallDamageCalOriginal, true);
		rhpFallDamageCalOriginal += 5;

		nvXReadOffset += 0xe;
		CreateHook("nvXReadHook", nvXReadOffset, &nvXReadHook, &nvXReadOriginal, true);
		nvXReadOriginal += 6;

		CreateHook("axisXYWriteHook", axisXYWriteOffset, &axisXYWriteHook, &axisXYWriteOriginal, true);
		axisXYWriteOriginal += 5;
	}

	BYTE camZReadAOB[] = { 0xD9, 0x40, 0x04, 0xD9, 0x59, 0x04, 0xD9, 0x40, 0x08, 0xD9, 0x59, 0x08, 0x0F, 0x57, 0xC0, 0xF3, 0x0F, 0x11, 0x41, 0x0C, 0xEB, 0x4B };
	BYTE camZ2WriteAOB[] = { 0xD9, 0x40, 0x64, 0xD9, 0x9F, 0x94, 0x00, 0x00, 0x00 };
	BYTE FOVReadAOB[] = { 0xF3, 0x0F, 0x58, 0x46, 0x3C, 0xEB, 0x05 };
	BYTE camCoords2PostWriteAOB[] = { 0x8B, 0x87, 0x80, 0x01, 0x00, 0x00, 0xD9, 0x80, 0xB8, 0x00, 0x00, 0x00 };

	if (FindSignature("camZReadAOB", camZReadAOB, &camZReadOffset) &&
		FindSignature("camZ2WriteAOB", camZ2WriteAOB, &camZ2WriteOffset) &&
		FindSignature("FOVReadAOB", FOVReadAOB, &FOVReadOffset) &&
		FindSignature("camCoords2PostWriteAOB", camCoords2PostWriteAOB, &camCoords2PostWriteOffset))
	{
		CreateHook("camZReadHook", camZReadOffset, &camZReadHook, &camZReadOriginal, true);
		camZReadOriginal += 6;

		CreateHook("camZ2WriteHook", camZ2WriteOffset, &camZ2WriteHook, &camZ2WriteOriginal, true);
		camZ2WriteOriginal += 9;

		CreateHook("FOVReadHook", FOVReadOffset, &FOVReadHook, &FOVReadOriginal, true);
		FOVReadOriginal += 5;

		CreateHook("camCoords2PostWriteHook", camCoords2PostWriteOffset, &camCoords2PostWriteHook, &camCoords2PostWriteOriginal, true);
		camCoords2PostWriteOriginal += 6;
	}

	/*
	BYTE sigCamFollow[] = { 0xF3, 0x0F, 0x10, 0x41, 0x20, 0xF3, 0x0F, 0x58, 0x41, 0x10, 0xF3, 0x0F, 0x11, 0x41, 0x10, 0xF3, 0x0F, 0x10, 0x41, 0x24, 0xF3, 0x0F, 0x58, 0x41, 0x14, 0xF3, 0x0F, 0x11, 0x41, 0x14, 0xF3, 0x0F, 0x10, 0x41, 0x28, 0xF3, 0x0F, 0x58, 0x41, 0x18, 0xF3, 0x0F, 0x11, 0x41, 0x18, 0xC2, 0x04, 0x00 };
	BYTE camFollowReplace[] = { 0xF3, 0x0F, 0x10, 0x41, 0x20, 0xF3, 0x0F, 0x58, 0x41, 0x10, 0x90, 0x90, 0x90, 0x90, 0x90, 0xF3, 0x0F, 0x10, 0x41, 0x24, 0xF3, 0x0F, 0x58, 0x41, 0x14, 0x90, 0x90, 0x90, 0x90, 0x90, 0xF3, 0x0F, 0x10, 0x41, 0x28, 0xF3, 0x0F, 0x58, 0x41, 0x18, 0x90, 0x90, 0x90, 0x90, 0x90, 0xC2, 0x04, 0x00 };

	BYTE sigCamCollision1[] = { 0xF3, 0x0F, 0x10, 0x84, 0x24, 0xE0, 0x00, 0x00, 0x00, 0xF3, 0x0F, 0x11, 0x06, 0xF3, 0x0F, 0x10, 0x84, 0x24, 0xE4, 0x00, 0x00, 0x00, 0xF3, 0x0F, 0x11, 0x46, 0x04, 0xF3, 0x0F, 0x10, 0x84, 0x24, 0xE8, 0x00, 0x00, 0x00, 0xF3, 0x0F, 0x11, 0x46, 0x08, 0x0F, 0x57, 0xC0, 0xF3, 0x0F, 0x11, 0x46, 0x0C, 0xC6, 0x44, 0x24, 0x07, 0x01 };
	BYTE sigCamCollision2[] = { 0xF3, 0x0F, 0x11, 0x06, 0xF3, 0x0F, 0x11, 0x4E, 0x04, 0xF3, 0x0F, 0x11, 0x56, 0x08, 0x0F, 0x57, 0xC0, 0xF3, 0x0F, 0x11, 0x46, 0x0C, 0xC6, 0x44, 0x24, 0x07, 0x01, 0x8D, 0x8C, 0x24, 0x10, 0x01, 0x00, 0x00 };

	BYTE camCollision1Replace[] = { 0xF3, 0x0F, 0x10, 0x84, 0x24, 0xE0, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0xF3, 0x0F, 0x10, 0x84, 0x24, 0xE4, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0xF3, 0x0F, 0x10, 0x84, 0x24, 0xE8, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x0F, 0x57, 0xC0, 0xF3, 0x0F, 0x11, 0x46, 0x0C, 0xC6, 0x44, 0x24, 0x07, 0x01 };
	BYTE camCollision2Replace[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x0F, 0x57, 0xC0, 0xF3, 0x0F, 0x11, 0x46, 0x0C, 0xC6, 0x44, 0x24, 0x07, 0x01, 0x8D, 0x8C, 0x24, 0x10, 0x01, 0x00, 0x00 };

	BYTE *pCamCollision1, *pCamCollision2;
	if (FindSignature("CamCollision1", sigCamCollision1, &pCamCollision1) &&
		FindSignature("CamCollision2", sigCamCollision2, &pCamCollision2))
	{
		Set<BYTE>(pCamCollision1, camCollision1Replace, ARRAYSIZE(camCollision1Replace));
		Set<BYTE>(pCamCollision2, camCollision2Replace, ARRAYSIZE(camCollision2Replace));
	}
	*/

	BYTE sigController[] = { 0x66, 0x0F, 0x6E, 0x95, 0xB8, 0x01, 0x00, 0x00, 0xF3, 0x0F, 0x10, 0x0D, 0xF0, 0xC7, 0x61, 0x01, 0x0F, 0x5B, 0xD2, 0xF3, 0x0F, 0x59, 0xD1, 0xF3, 0x0F, 0x11, 0x56, 0x3C };
	if (FindSignature("GetControllerAOB", sigController, &pGetControllerHook1))
	{
		CreateHook("GetController", pGetControllerHook1, &HGetController, &oGetControllerHook, true);
	}

	InGameUIAdd(renderCameraUI);
}
