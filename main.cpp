// 定义应用程序的入口点。
//
#define USE_LIGHTHOOK
//#define USE_MINHOOK
//#define USE_DETOURS
#define EXTERNAL_INCLUDE_HOOKHEADER

#ifdef EXTERNAL_INCLUDE_HOOKHEADER
//#include <MinHook.h>
#include "LightHook/Source/LightHook.h"
//#include <Windows.h>
//#include "detours/detours.h"
#endif // EXTERNAL_INCLUDE_HOOKHEADER


#include "HookManager/HookManager.hpp"
#include <iostream>


HookInstance* h = nullptr;
HookInstance* h2 = nullptr;

// hook在Release模式下一定要考虑到内联和优化的问题
static __declspec(noinline) int add(int a, int b) {
    return a + b;
}

static __declspec(noinline) int hookadd(int a, int b) {
    //using fn = int(__fastcall*)(int, int);
    return h->oriForSign(add)(a,b) * 10;
}
static __declspec(noinline) int hookadd2(int a, int b) {
    //using fn = int(__fastcall*)(int, int);
    return h2->oriForSign(add)(a, b) * 10;
}


static void test() {

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

    std::cout << "&add:";
    printf_s("%p \n", &add);

    std::cout << "add(5,6):" << add(5, 6) << std::endl;
    h = HookManager::getInstance()->addHook((uintptr_t)&add, &hookadd, "hookadd1");
    if(!h->hook()) {
        std::cout << "hook fail!!!" << std::endl;
    }


    std::cout << "hooked 1:" << std::endl;
    std::cout << "add(7,8):" << add(7, 8) << std::endl;

    h2 = HookManager::getInstance()->addHook((uintptr_t)&add, &hookadd2, "hookadd2");
    if(!h2->hook()) {
        std::cout << "hook2 fail!!!" << std::endl;
    }

    std::cout << "hooked 2:" << std::endl;
    std::cout << "add(9,10):" << add(9, 10) << std::endl;

    //std::cout << "removehook:" << std::endl;
    //if(!h->unhook() || !h2->unhook()) {
    //    std::cout << "unhook fail!!!" << std::endl;
    //}
    HookManager::getInstance()->disableAllHook();
    std::cout << "add(11,12):" << add(11, 12) << std::endl;
    // 试图重复添加Hook - 将会产生一个debug消息
    //auto* _ = HookManager::getInstance()->addHook((uintptr_t)&add, &hookadd);
    
    system("pause");
}

int main()
{
    test();
    std::cout << "add(13,14):" << add(13, 14) << std::endl;
    system("pause");
    return 0;
}
