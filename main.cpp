// 定义应用程序的入口点。
//
//#define USE_LIGHTHOOK
#define USE_MINHOOK
//#define USE_DETOURS
//#define EXTERNAL_INCLUDE_HOOKHEADER

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
	return h->oriForSign(add)(a,b) * 10;
}



static void test() {

#ifdef _HM_IS_X64
	byte JMPCODE[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00 };
#else
	byte JMPCODE[] = { 0xE9 , 0x00, 0x00, 0x00, 0x00 };
#endif // _HM_IS_X64
	LPVOID allocPtr = VirtualAlloc(NULL, sizeof(JMPCODE), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if(allocPtr == NULL) {
		std::cerr << "内存申请失败" << std::endl;
		return;
	}
	memset(allocPtr, 0, sizeof(JMPCODE));

#ifdef _HM_IS_X64
	*reinterpret_cast<uintptr_t*>(JMPCODE + 6) = (uintptr_t)&hookadd;
#else
	*reinterpret_cast<uintptr_t*>(JMPCODE + 1) = (uintptr_t) (&hookadd) - ((uintptr_t)allocPtr + 5 );
#endif

	memcpy_s(allocPtr, sizeof(JMPCODE), JMPCODE, sizeof(JMPCODE));

	printf_s("%p \n", allocPtr);

	h = HookManager::getInstance()->addHook((uintptr_t)&add, (void*)allocPtr, "hookadd");
	h->hook();
	system("pause");
	std::cout << "add(5,6):" << add(5, 6) << std::endl;
	system("pause");
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

	//test();
	//return 0;



	std::cout << "&add:";
	printf_s("%p \n", &add);

	std::cout << "add(5,6):" << add(5, 6) << std::endl;
	h = HookManager::getInstance()->addHook((uintptr_t) & add, &hookadd, "hookadd");
	if(!h->hook()) {
		std::cout << "hook fail!!!" << std::endl;
	}
	std::cout << "hooked:" << std::endl;
	std::cout << "add(7,8):" << add(7, 8) << std::endl;
	system("pause");
	std::cout << "removehook:" << std::endl;
	if(!h->unhook()) {
		std::cout << "unhook fail!!!" << std::endl;
	}
	std::cout << "add(9,10):" << add(9, 10) << std::endl;

	// 试图重复添加Hook - 将会产生一个debug消息
	auto* _ = HookManager::getInstance()->addHook((uintptr_t)&add, &hookadd);
	
	return 0;
}
