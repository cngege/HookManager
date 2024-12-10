// 定义应用程序的入口点。
//
//#define USE_LIGHTHOOK
#define USE_MINHOOK
//#define USE_DETOURS
#define EXTERNAL_INCLUDE_HOOKHEADER

#ifdef EXTERNAL_INCLUDE_HOOKHEADER
#include <MinHook.h>
//#include "LightHook/Source/LightHook.h"
//#include <Windows.h>
//#include "detours/detours.h"
#endif // EXTERNAL_INCLUDE_HOOKHEADER


#include "HookManager/HookManager.hpp"
#include <iostream>


HookInstance* h = nullptr;

// hook在Release模式下一定要考虑到内联和优化的问题
static __declspec(noinline) int add(int a, int b) {
	return a + b;
}

static __declspec(noinline) int hookadd(int a, int b) {
	//using fn = int(__fastcall*)(int, int);
	return h->oriFormSign(add)(a,b) * 10;
}

int main()
{
	HookManager::getInstance()->on([](HookManager::msgtype type, std::string msg) {
		if(type == HookManager::msgtype::info) {
			printf_s("[info] %s \n", msg.c_str());
		}
		else if(type == HookManager::msgtype::warn) {
			// 任何一个warn警告应该被重视
			printf_s("[warn] %s \n", msg.c_str());
		}
		else if(type == HookManager::msgtype::error) {
			// 任何一个error错误都应该被重视
			printf_s("[error] %s \n", msg.c_str());
		}
		else if(type == HookManager::msgtype::debug) {
			printf_s("[debug] %s \n", msg.c_str());
		}
	});


	std::cout << "add(5,6):" << add(5, 6) << std::endl;
	h = HookManager::getInstance()->addHook((uintptr_t) & add, &hookadd, "hookadd");
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

	// 试图重复添加Hook - 将会产生一个debug消息
	auto* _ = HookManager::getInstance()->addHook((uintptr_t)&add, &hookadd);

	return 0;
}
