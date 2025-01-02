<div align=center>
  <img src="https://capsule-render.vercel.app/api?type=Waving&color=timeGradient&height=200&animation=fadeIn&section=header&text=HookManager&fontSize=60" />
</div>

---

 ## 本项目核心 HookManager.hpp
 > 使用方法: 将 include 中的 HookMananger文件夹放在你的项目中  
   然后引用 `#include "HookManager/HookManager.hpp"` 即可

 ## 依赖: 
 HookManager 只是一个Hook的管理器, 后续可能支持多家Hook，统一接口，但此项目不负责Hook的底层实现，  
 所以你需要额外加上 LightHook 等Hook库，

 ## 使用: 
 
 ### HookManager + LightHook
 - 在引用 `HookManager.hpp` 前定义宏:`USE_LIGHTHOOK`
 - 将 `LightHook.h` 文件放到 LightHook 文件夹中， LightHook文件夹放到可直接引用的目录，如：include
 - 或者 在引用HookManager 头文件前定义 宏:`EXTERNAL_INCLUDE_HOOKHEADER` 然后手动引用lighthook头文件

 ### HookManager + MinHook
 - 在引用 `HookManager.hpp` 前定义宏:`USE_MinHOOK`
 - 安装 `MinHook.h` 或将 `MinHook.h` 文件放到可直接引用的目录，如：include
 - 或者 在引用HookManager 头文件前定义 宏:`EXTERNAL_INCLUDE_HOOKHEADER` 然后手动引用minhook头文件

 ### HookManager + Detours
 - 在引用 `HookManager.hpp` 前定义宏:`USE_DETOURS`
 - 将 `Detours.h` 文件放到可直接引用的目录，如：include
 - 或者 在引用HookManager 头文件前定义 宏:`EXTERNAL_INCLUDE_HOOKHEADER` 然后手动引用Detours头文件

 ## TODO
 - [x] 支持多次hook, hook后函数存储到数组,触发调用时传递执行.
 - [x] 支持minhook, 并通过宏区分开不同hook.
 - [x] 支持微软 [Detours](https://github.com/microsoft/Detours). 使用宏:`USE_DETOURS` 
 - [x] 发生失败、错误、警告时的正反馈.
 - [x] 使本项目支持~~CMAKE~~ xmake自动依赖功能.

 ## 常用Hook库(已支持)
 - [LightHook](https://github.com/SamuelTulach/LightHook).
 - [minhook](https://github.com/TsudaKageyu/minhook).
 - [Detours](https://github.com/microsoft/Detours).

 ## 工作逻辑
 - HookManager 为此库统一管理接口, 设计为单例模式  
 - 所有相同目标hook指针的hook，都为一组, 一组会有一个最终hook
	- 所有addHook添加进来的子函数(库中以HookInstance的指针形式存在)都将保存到一个数组中, hook被触发调用后, 会依次传递执行数组中的所有call
	- HookInstance 的函数被触发后, 其Origin函数指向数组中的下一项开启的单例
	- 如果此单例为最后一项, 或最后开启的一项，则Origin会指向真实函数(被hook的原函数)
	- 当添加hook时（addHook）如果当前是第一个hook会自动创建hook，不会开启
	- 当HookInstance::hook 后,如果队列中自身是唯一开启的hook，则自动启用真实hook
	- 当HookInstance::unhook后，如果队列中没有开启的hook，则会关闭真实hook
	- removeHook, 关闭一组hook单例，并关闭移除此组hook
	- enablehook / disablehook 同单例的hook / unhook
	- enableAllHook / disableAllHook 仅关闭或开启（仅单例中有存在开启的hook）所有底层hook
	- HookManager 自动析构释放内存


 ## 示例代码：

 ```cpp

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
    return h->oriForSign(add)(a,b) * 10;
}
static __declspec(noinline) int hookadd2(int a, int b) {
    return h2->oriForSign(add)(a, b) * 11;
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

    //std::cout << "&add:";
    //printf_s("%p \n", &add);

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
    
}

int main()
{
    test();
    std::cout << "add(13,14):" << add(13, 14) << std::endl;
    return 0;
}




```

## 输出：

```
add(5,6):11
hooked 1:
add(7,8):150
hooked 2:
add(9,10):2090
add(11,12):23
add(13,14):27
[info] ~HookManager() 已进行释放
```

