#include "stdafx.h"
#include "Hotkeys.h"

INPUT keyInput = { INPUT_KEYBOARD, {} };
DWORD menuPause;
void(*keys[0x100])() = { nullptr };
void SendKeyPress(DWORD vKey)
{
	keyInput.ki.wVk = (WORD)vKey;
	keyInput.ki.dwFlags = 0;
	SendInput(1, &keyInput, sizeof(INPUT));
	keyInput.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &keyInput, sizeof(INPUT));
}

DWORD WINAPI hotkeyProc(LPVOID vKey)
{
#ifndef DISABLE_UNWANTED_HOOKS
	Sleep(menuPause);
#endif
	if ((DWORD)vKey)
		SendKeyPress((DWORD)vKey);
	SendKeyPress(VK_RETURN);
	return 0;
}

void hotkeyStart(DWORD vKey)
{
	SendKeyPress(VK_ESCAPE);
	QueueUserWorkItem(hotkeyProc, (LPVOID)vKey, WT_EXECUTEDEFAULT);
}

bool borderlessFullscreen = false, ignoreFocusLoss = false;
void setBorderlessFullscreen(HWND hwnd)
{
	LONG lStyle = GetWindowLong(hwnd, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZE | WS_MINIMIZE);
	SetWindowLong(hwnd, GWL_STYLE, lStyle);
	LONG lExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_COMPOSITED | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_LAYERED | WS_EX_STATICEDGE | WS_EX_TOOLWINDOW | WS_EX_APPWINDOW);
	SetWindowLong(hwnd, GWL_EXSTYLE, lExStyle);

	HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info;
	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &info);
	int width = info.rcMonitor.right - info.rcMonitor.left;
	int height = info.rcMonitor.bottom - info.rcMonitor.top;
	SetWindowPos(hwnd, nullptr, info.rcMonitor.left, info.rcMonitor.top, width, height, SWP_NOSENDCHANGING | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

BYTE **pSave = nullptr;
WNDPROC oWndProc, wndProcHandler = nullptr;
LRESULT CALLBACK HWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_SIZE && borderlessFullscreen)
		setBorderlessFullscreen(hwnd);

	if (ignoreFocusLoss)
	{
		// https://github.com/Lyall/DDDAFix/blob/e7689f81c7ba0ce06b4bb6d6747b398e93bd4fcb/src/dllmain.cpp#L55
		if (msg == WM_ACTIVATEAPP && wParam == FALSE)
		{
			return 0;
		}
		else if (msg == WM_KILLFOCUS)
		{
			return 0;
		}
	}

	if (wndProcHandler(hwnd, msg, wParam, lParam))
		return 0;
	if ((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) &&
		(HIWORD(lParam) & KF_REPEAT) == 0 &&
		wParam < 0xFF && keys[wParam] != nullptr)
	{
		keys[wParam]();
		return 0;
	}
	return oWndProc(hwnd, msg, wParam, lParam);
}

void Hooks::HotkeysAdd(LPCSTR name, WORD defKey, void(*func)())
{
	unsigned int key = config.getUInt("hotkeys", name, defKey);
	if (key < 0xFF)
		keys[key] = func;
}

void Hooks::HotkeysHandler(WNDPROC proc) { wndProcHandler = proc; }
void Hooks::Hotkeys()
{
	if (config.getBool("hotkeys", "enabled", false))
	{
		borderlessFullscreen = config.getBool("main", "borderlessFullscreen", false);
		ignoreFocusLoss = config.getBool("main", "disablePauseOnFocusLoss", false);
#ifndef DISABLE_UNWANTED_HOOKS
		menuPause = config.getUInt("hotkeys", "menuPause", 500);
		HotkeysAdd("keySave", VK_F5, []() { if (pSave && *pSave) (*pSave)[0x21AFD6] = 1; });
		HotkeysAdd("keyCheckpoint", VK_F9, []() { if (pSave && *pSave) (*pSave)[0x21AFD5] = 1; });
		HotkeysAdd("keyMap", 'M', []() { hotkeyStart(0); });
		HotkeysAdd("keyJournal", 'J', []() { hotkeyStart(VK_LEFT); });
		HotkeysAdd("keyEquipment", 'U', []() { hotkeyStart(VK_RIGHT); });
		HotkeysAdd("keyStatus", 'K', []() { hotkeyStart(VK_DOWN); });
#endif

		BYTE sig[] = { 0x83, 0xEC, 0x50,			//sub	esp, 50h
						0x53,						//push	ebx
						0x8B, 0x5C, 0x24, 0x58,		//mov	ebx, [esp+54h+hWnd]
						0x56,						//push	esi
						0x8B, 0x74, 0x24, 0x60 };	//mov	esi, [esp+58h+Msg]

		BYTE sig2[] = { 0x83, 0xEC, 0x50,			//sub	esp, 50h
						0x56,						//push	esi
						0x8B, 0x74, 0x24, 0x5C,		//mov	esi, [esp+54h+Msg]
						0x57,						//push	edi
						0x8B, 0x7C, 0x24, 0x5C };	//mov	edi, [esp+58h+hRecipient]

		BYTE *pOffset;
		if (FindSignature("Hotkeys1", sig, &pOffset) || FindSignature("Hotkeys2", sig2, &pOffset))
			CreateHook("Hotkeys", pOffset, &HWndProc, &oWndProc);

		BYTE sig3[] = { 0x8B, 0x08,	//mov ecx,[eax]
			0x8D, 0x54, 0x24, 0x28,	//lea edx, [esp + 28]
			0x52,					//push edx
			0x6A, 0x14,				//push 14
			0x50,					//push eax
			0x8B, 0x41, 0x24,		//mov eax, [ecx + 24]
			0xFF, 0xD0,				//call eax
			0x85, 0xC0 };			//test eax, eax
									//DDDA.exe+A04C23 - 0F88 29010000		  - js DDDA.exe+A04D52

		/*
		DDDA.exe+A04C2F - 0F5B C0				- cvtdq2ps xmm0,xmm0
		DDDA.exe+A04C32 - F3 0F59 87 0C010000	- mulss xmm0,[edi+0000010C]
		DDDA.exe+A04C3A - F3 0F2C C8			- cvttss2si ecx,xmm0

		DDDA.exe+A04C44 - 0F5B C0				- cvtdq2ps xmm0,xmm0
		DDDA.exe+A04C47 - 89 4E 0C				- mov [esi+0C],ecx
		DDDA.exe+A04C4A - F3 0F59 87 0C010000	- mulss xmm0,[edi+0000010C]
		DDDA.exe+A04C52 - F3 0F2C D0			- cvttss2si edx,xmm0

		DDDA.exe+A04C29 - C5F9EFC0				- vpxor xmm0,xmm0,xmm0
		*/

		if (FindSignature("ReadMouse", sig3, &pOffset))
		{
			Set<BYTE>(pOffset + 0x17, { 0xC5, 0xF9, 0xEF, 0xC0, 0x90, 0x90 });
			Set<BYTE>(pOffset + 0x2c, { 0xC5, 0xF9, 0xEF, 0xC0, 0x90, 0x90 });
			Set<BYTE>(pOffset + 0x44, { 0xC5, 0xF9, 0xEF, 0xC0, 0x90, 0x90 });
		}

#ifndef DISABLE_UNWANTED_HOOKS
		BYTE sigSave[] = { 0x8B, 0x15, 0xCC, 0xCC, 0xCC, 0xCC,	//mov	edx, savePointer
							0x0F, 0x95, 0xC0,					//setnz	al
							0x83, 0xC9, 0xFF };					//or	ecx, 0FFFFFFFFh

		if (FindSignature("HotkeysSave", sigSave, &pOffset))
			pSave = (BYTE**)*(LPDWORD)(pOffset + 2);
#endif
	}
	else
		logFile << "Hotkeys: disabled" << std::endl;
}
