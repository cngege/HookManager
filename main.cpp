// 定义应用程序的入口点。
//
//#define USE_LIGHTHOOK


#define USE_DETOURS
#define EXTERNAL_INCLUDE_HOOKHEADER

#ifdef EXTERNAL_INCLUDE_HOOKHEADER
//#include <MinHook.h>
//#include "LightHook/Source/LightHook.h"
#include <Windows.h>
#include "detours/detours.h"
#endif // EXTERNAL_INCLUDE_HOOKHEADER


#include "HookManager/HookManager.hpp"
#include <iostream>


HookInstance* h = nullptr;

// hook在Release模式下一定要考虑到内联和优化的问题
__declspec(noinline) int add(int a, int b) {
	return a + b;
}

__declspec(noinline) int hookadd(int a, int b) {
	using fn = int(__fastcall*)(int, int);
	return ((fn)h->origin)(a, b) * 10;
}

int main()
{

	std::cout << "add(5,6):" << add(5, 6) << std::endl;
	h = HookManager::getInstance()->addHook((uintptr_t) & add, &hookadd);
	if(!h->hook()) {
		std::cout << "hook fail!!!" << std::endl;
	}
	std::cout << "hooked:" << std::endl;
	std::cout << "add(7,8):" << add(7, 8) << std::endl;

	std::cout << "removehook:" << std::endl;
	if(!h->unhook()) {
		std::cout << "unhook fail!!!" << std::endl;
	}

	std::cout << "add(9,10):" << add(9, 10) << std::endl;
	return 0;
}
