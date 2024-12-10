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
 - 将 `MinHook.h` 文件放到 minhook 文件夹中， minHook文件夹放到可直接引用的目录，如：include
 - 或者 在引用HookManager 头文件前定义 宏:`EXTERNAL_INCLUDE_HOOKHEADER` 然后手动引用minhook头文件

 ## TODO
 - [ ] 支持多次hook, hook后函数存储到数组,触发调用时遍历执行.
 - [x] 支持minhook, 并通过宏区分开不同hook.
 - [x] 支持微软 [Detours](https://github.com/microsoft/Detours). 使用宏:`USE_DETOURS` 
 - [x] 发生失败、错误、警告时的正反馈.
 - [ ] 使本项目支持~~CMAKE~~ xmake自动依赖功能.

 ## 常用Hook库(已支持)
 - [LightHook](https://github.com/SamuelTulach/LightHook).
 - [minhook](https://github.com/TsudaKageyu/minhook).
 - [Detours](https://github.com/microsoft/Detours).

 ## 示例代码：

 ```cpp

// 定义应用程序的入口点。
//
//#define USE_LIGHTHOOK
#define USE_MINHOOK
#define EXTERNAL_INCLUDE_HOOKHEADER

#ifdef EXTERNAL_INCLUDE_HOOKHEADER
#include <MinHook.h>
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
	//return ((fn)h->origin)(a, b) * 10;
	//or
	return h->oriFormSign(add)(a,b) * 10;
 //将会改为
 //return h->oriForSign(add)(a,b)*10;
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


```

## 输出：

```
add(5,6):11
hooked:
add(7,8):150
removehook:
add(9,10):19
[debug] 暂不支持重复Hook, addHook 新增Hook失败, hook指针:[00007FF605B3CF70],已存在的Hook目标: [hookadd]
```

