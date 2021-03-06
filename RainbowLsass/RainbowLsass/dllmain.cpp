// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <TlHelp32.h>
#include <memory>
#include <string>
#include <Wtsapi32.h>
#include <D3DX10math.h>
#include <iostream>
#include <vector>
#include <Winternl.h>
#include <stack>
#include "stdafx.h"
#include <iostream>
#include <time.h>
#include <Psapi.h>
#include <iostream>
#include <fstream>
#include <vector>  
#include <iostream> 
#include <map>  
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>
#include <WtsApi32.h>

#include "Main.h"

VOID MessageBox_(LPCSTR Text, LPCSTR Title)
{
	DWORD response;

	WTSSendMessageA(WTS_CURRENT_SERVER_HANDLE,       // hServer
		WTSGetActiveConsoleSessionId(),  // ID for the console seesion (1)
		const_cast<LPSTR>(Title),        // MessageBox Caption
		strlen(Title),                   // 
		const_cast<LPSTR>(Text),         // MessageBox Text
		strlen(Text),                    // 
		MB_OK,                           // Buttons, etc
		10,                              // Timeout period in seconds
		&response,                       // What button was clicked (if bWait == TRUE)
		FALSE);                          // bWait - Blocks until user click
}

std::uint32_t find(const wchar_t* proc)
{
	auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	auto pe = PROCESSENTRY32W{ sizeof(PROCESSENTRY32W) };

	if (Process32First(snapshot, &pe)) {
		do {
			if (!_wcsicmp(proc, pe.szExeFile)) {
				CloseHandle(snapshot);
				return pe.th32ProcessID;
			}
		} while (Process32Next(snapshot, &pe));
	}
	CloseHandle(snapshot);
	return 0;
}

typedef NTSTATUS(NTAPI* NtQuerySystemInformationFn)(ULONG, PVOID, ULONG, PULONG);
static HANDLE GetProcessHandle(uint64_t targetProcessId)
{
	auto NtQuerySystemInformation = reinterpret_cast<NtQuerySystemInformationFn>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation"));
	NTSTATUS status;
	ULONG handleInfoSize = 0x10000;

	auto handleInfo = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION>(malloc(handleInfoSize));

	while ((status = NtQuerySystemInformation(SystemHandleInformation, handleInfo, handleInfoSize, nullptr)) == STATUS_INFO_LENGTH_MISMATCH)
		handleInfo = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION>(realloc(handleInfo, handleInfoSize *= 2));

	if (!NT_SUCCESS(status))
	{
		MessageBox_("Error: Handle Not Found", "Error!");
	}

	for (auto i = 0; i < handleInfo->HandleCount; i++)
	{
		auto handle = handleInfo->Handles[i];

		const auto process = reinterpret_cast<HANDLE>(handle.Handle);
		if (handle.ProcessId == GetCurrentProcessId() && GetProcessId(process) == targetProcessId)
			return process;
	}

	free(handleInfo);

	return nullptr;
}

class Vector2
{
public:
	Vector2() : x(0.f), y(0.f)
	{

	}

	Vector2(float _x, float _y) : x(_x), y(_y)
	{

	}
	~Vector2()
	{

	}

	float x;
	float y;
};
class Vector4
{
public:
	Vector4() : x(0.f), y(0.f), z(0.f), w(0.f)
	{

	}

	Vector4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w)
	{

	}
	~Vector4()
	{

	}

	float x;
	float y;
	float z;
	float w;
};
class Vector3
{
public:
	Vector3() : x(0.f), y(0.f), z(0.f)
	{

	}

	Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
	{

	}
	~Vector3()
	{

	}

	float x;
	float y;
	float z;

	inline float Dot(Vector3 v)
	{
		return x * v.x + y * v.y + z * v.z;
	}

	inline float Distance(Vector3 v)
	{
		return float(sqrtf(powf(v.x - x, 2.0) + powf(v.y - y, 2.0) + powf(v.z - z, 2.0)));
	}

	Vector3 operator+(Vector3 v)
	{
		return Vector3(x + v.x, y + v.y, z + v.z);
	}

	Vector3 operator-(Vector3 v)
	{
		return Vector3(x - v.x, y - v.y, z - v.z);
	}

	Vector3 operator*(float number) const
	{
		return Vector3(x * number, y * number, z * number);
	}
};

HANDLE GAME = GetProcessHandle(find(L"RainbowSix.exe"));
HANDLE GameHandle()
{
	return GAME;
}

template<typename T> T
RPM(SIZE_T address)
{
	//The buffer for data that is going to be read from memory
	T buffer;

	//The actual RPM
	ReadProcessMemory(GameHandle(), (LPCVOID)address, &buffer, sizeof(T), NULL);

	//Return our buffer
	return buffer;
}

std::string RPMString(SIZE_T address)
{
	//Make a char array of 20 bytes
	char name[20];

	//The actual RPM
	ReadProcessMemory(GameHandle(), (LPCVOID)address, &name, sizeof(name), NULL);

	//Add each char to our string
	//While also checking for a null terminator to end the string
	std::string nameString;
	for (int i = 0; i < sizeof(name); i++) {
		if (name[i] == 0)
			break;
		else
			nameString += name[i];
	}

	return nameString;
}

template<typename T> void WPM(SIZE_T address, T buffer)
{
	//A couple checks to try and avoid crashing
	//These don't actually make sense, feel free to remove redundent ones
	if (address == 0 || (LPVOID)address == nullptr || address == NULL) {
		return;
	}

	WriteProcessMemory(GameHandle(), (LPVOID)address, &buffer, sizeof(buffer), NULL);

}

DWORD_PTR GetBase()
{
	HMODULE hMods[1024];
	DWORD cbNeeded;
	unsigned int i;

	if (EnumProcessModules(GameHandle(), hMods, sizeof(hMods), &cbNeeded))
	{
		for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
		{
			TCHAR szModName[MAX_PATH];
			if (GetModuleFileNameEx(GameHandle(), hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
			{
				std::wstring wstrModName = szModName;
				std::wstring wstrModContain = L"RainbowSix.exe";
				if (wstrModName.find(wstrModContain) != std::string::npos)
				{
					return (DWORD_PTR)hMods[i];
				}
			}
		}
	}
	return false;
}

namespace Offset 
{


	DWORD_PTR GameManager = 0x47F00D0;
	DWORD_PTR EntityList = 0xC0;

	DWORD_PTR Entity = 0x0008;
	DWORD_PTR EntityRef = 0x20;

	DWORD_PTR EntityInfo = 0x18;
	DWORD_PTR MainComponent = 0xB8;
	DWORD_PTR ChildComponent = 0x8;
	DWORD_PTR Health = 0x108;

	DWORD_PTR PlayerInfo = 0x2A0;
	DWORD_PTR PlayerInfoDeref = 0x0;
	DWORD_PTR PlayerTeamId = 0x140;
	DWORD_PTR PlayerName = 0x158;

	DWORD_PTR FeetPosition = 0x1C0;
	DWORD_PTR HeadPosition = 0x160;

	DWORD_PTR WeaponComp = 0x38;
	DWORD_PTR WeaponProcessor = 0xF0;
	DWORD_PTR Weapon = 0x0;
	DWORD_PTR WeaponInfo = 0x110;
	DWORD_PTR Spread = 0x2A0;
	DWORD_PTR Recoil = 0x2D8;
	DWORD_PTR Recoil2 = 0x354;
	DWORD_PTR Recoil3 = 0x304;
	DWORD_PTR AdsRecoil = 0x330;

	DWORD_PTR Renderer = 0x47A4930;
	DWORD_PTR GameRenderer = 0x0;
	DWORD_PTR EngineLink = 0xd8;
	DWORD_PTR Engine = 0x218;
	DWORD_PTR Camera = 0x38;

	DWORD_PTR ViewTranslastion = 0x1A0;
	DWORD_PTR ViewRight = 0x170;
	DWORD_PTR ViewUp = 0x180;
	DWORD_PTR ViewForward = 0x190;
	DWORD_PTR FOVX = 0x1B0;
	DWORD_PTR FOVY = 0x1C4;
}

//Game
DWORD_PTR pGameManager;
DWORD_PTR pEntityList;

//Camera
DWORD_PTR pRender;
DWORD_PTR pGameRender;
DWORD_PTR pEngineLink;
DWORD_PTR pEngine;
DWORD_PTR pCamera;

DWORD_PTR GetEntity(int i)
{
	DWORD_PTR entityBase = RPM<DWORD_PTR>(pEntityList + (i * Offset::Entity));
	return RPM<DWORD_PTR>(entityBase + Offset::EntityRef);
}
Vector3 GetEntityFeetPosition(DWORD_PTR entity)
{
	return RPM<Vector3>(entity + Offset::FeetPosition);
}
std::string GetEntityPlayerName(DWORD_PTR entity)
{
	DWORD_PTR playerInfo = RPM<DWORD_PTR>(entity + Offset::PlayerInfo);
	DWORD_PTR playerInfoD = RPM<DWORD_PTR>(playerInfo + Offset::PlayerInfoDeref);

	return RPMString(RPM<DWORD_PTR>(playerInfoD + Offset::PlayerName) + 0x0);
}
Vector3 GetEntityHeadPosition(DWORD_PTR entity)
{
	return RPM<Vector3>(entity + Offset::HeadPosition);
}
BYTE GetEntityTeamId(DWORD_PTR entity)
{
	DWORD_PTR playerInfo = RPM<DWORD_PTR>(entity + Offset::PlayerInfo);
	DWORD_PTR playerInfoD = RPM<DWORD_PTR>(playerInfo + Offset::PlayerInfoDeref);

	return RPM<BYTE>(playerInfoD + Offset::PlayerTeamId);
}
Vector3 GetViewTranslation()
{
	return RPM<Vector3>(pCamera + Offset::ViewTranslastion);
}

Vector3 GetViewRight()
{
	return RPM<Vector3>(pCamera + Offset::ViewRight);
}

Vector3 GetViewUp()
{
	return RPM<Vector3>(pCamera + Offset::ViewUp);
}

Vector3 GetViewForward()
{
	return RPM<Vector3>(pCamera + Offset::ViewForward);
}
float GetFOVX()
{
	return RPM<float>(pCamera + Offset::FOVX);
}
float GetFOVY()
{
	return RPM<float>(pCamera + Offset::FOVY);
}
Vector3 WorldToScreen(Vector3 position)
{
	int displayWidth = 1280;
	int displayHeight = 768;

	Vector3 temp = position - GetViewTranslation();

	float x = temp.Dot(GetViewRight());
	float y = temp.Dot(GetViewUp());
	float z = temp.Dot(GetViewForward() * -1);

	return Vector3((displayWidth / 2) * (1 + x / GetFOVX() / z), (displayHeight / 2) * (1 - y / GetFOVY() / z), z);
}
DWORD GetEntityHealth(DWORD_PTR entity)
{
	DWORD_PTR EntityInfo = RPM<DWORD_PTR>(entity + Offset::EntityInfo);
	DWORD_PTR MainComponent = RPM<DWORD_PTR>(EntityInfo + Offset::MainComponent);
	DWORD_PTR ChildComponent = RPM<DWORD_PTR>(MainComponent + Offset::ChildComponent);

	return RPM<DWORD>(ChildComponent + Offset::Health);
}

DWORD Main(void*)
{
	while (1)
	{
		pGameManager = RPM<DWORD_PTR>(GetBase() + Offset::GameManager);
		pEntityList = RPM<DWORD_PTR>(pGameManager + Offset::EntityList);
		pRender = RPM<DWORD_PTR>(GetBase() + Offset::Renderer);
		pGameRender = RPM<DWORD_PTR>(pRender + Offset::GameRenderer);
		pEngineLink = RPM<DWORD_PTR>(pGameRender + Offset::EngineLink);
		pEngine = RPM<DWORD_PTR>(pEngineLink + Offset::Engine);
		pCamera = RPM<DWORD_PTR>(pEngine + Offset::Camera);

		for (int i = 8; i < 12; i++)
		{
			//Get the current entity
			BYTE LocalPlayerID = NULL;
			BYTE EntityID = NULL;
			DWORD_PTR Entity;
			Entity = GetEntity(i);
			DWORD_PTR entityInfo;
			entityInfo = RPM<DWORD_PTR>(Entity + Offset::EntityInfo);
			DWORD_PTR mainComp;
			mainComp = RPM<DWORD_PTR>(entityInfo + Offset::MainComponent);
			DWORD_PTR weaponComp;
			weaponComp = RPM<DWORD_PTR>(mainComp + Offset::WeaponComp);
			DWORD_PTR weaponProc;
			weaponProc = RPM<DWORD_PTR>(weaponComp + Offset::WeaponProcessor);
			DWORD_PTR weapon;
			weapon = RPM<DWORD_PTR>(weaponProc + Offset::Weapon);
			DWORD_PTR weaponInfo;
			weaponInfo = RPM<DWORD_PTR>(weapon + Offset::WeaponInfo);
			WPM<float>(weaponInfo + Offset::Spread, 0);
			float recoil;
			recoil = RPM<float>(weaponInfo + Offset::Recoil);
			float recoil2;
			recoil2 = RPM<float>(weaponInfo + Offset::Recoil2);
			Vector4 recoil3;
			recoil3 = RPM<Vector4>(weaponInfo + Offset::Recoil3);
			Vector2 adsRecoil;
			adsRecoil = RPM<Vector2>(weaponInfo + Offset::AdsRecoil);
			if (recoil != 0 || recoil2 != 0 || !(recoil3.x == 0 && recoil3.y == 0 && recoil3.z == 0 && recoil3.w == 0) || !(adsRecoil.x == 0 && adsRecoil.y == 0))
			{
				if (recoil != 0) {
					WPM<float>(weaponInfo + Offset::Recoil, 0);
				}

				if (recoil2 != 0) {
					WPM<float>(weaponInfo + Offset::Recoil2, 0);
				}

				if (!(recoil3.x == 0 && recoil3.y == 0 && recoil3.z == 0 && recoil3.w == 0)) {
					WPM<Vector4>(weaponInfo + Offset::Recoil3, Vector4(0, 0, 0, 0));
				}

				if (!(adsRecoil.x == 0 && adsRecoil.y == 0)) {
					WPM<Vector2>(weaponInfo + Offset::AdsRecoil, Vector2(0, 0));
				}
			}
		}
	}
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		MessageBox_("Injected", "Injected");
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Main, 0, 0, 0);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

