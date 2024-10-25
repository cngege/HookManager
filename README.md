<div align=center>
  <img src="https://capsule-render.vercel.app/api?type=Waving&color=timeGradient&height=200&animation=fadeIn&section=header&text=HookManager&fontSize=60" />
</div>

---

 ## 本项目核心 HookManager.hpp
 > 使用方法: 将 include 中的 HookMananger文件夹放在你的项目中  
   然后引用 `#include "HookManager/HookManager.hpp"` 即可

 ## 依赖: 
 HookManager 只是一个Hook的管理器, 后续可能支持多家Hook，统一接口，但此项目本身并不会去Hook，  
 所以你需要额外加上 LightHook 等Hook代码， 后续可能支持MinHook等其他Hook（目前支持LightHook）

 ## 使用: 
 
 ### HookManager + LightHook
 将 `LightHook.h` 文件放到 LightHook 文件夹中， LightHook文件夹放到可直接引用的目录，如：include

 ## TODO
 - [ ] 支持多次hook, hook后函数存储到数组,触发调用时遍历执行.
 - [ ] 支持minhook, 并通过宏区分开不同hook.
 - [ ] 发生失败、错误、警告时的正反馈.
 - [ ] 使本项目支持CMAKE自动依赖功能.

 ## 示例代码：

 ```cpp

#include "HookManager/HookManager.hpp"
#include <iostream>

HookInstance* h = nullptr;

// hook在Release模式下一定要考虑到内联和优化的问题
__declspec(noinline) int add(int a, int b) {
	return a + b;
}

int hookadd(int a, int b) {
	using fn = int(__fastcall*)(int, int);
	return ((fn)h->origin)(a, b) * 10;
}

int main()
{

	std::cout << "add(5,6):" << add(5, 6) << std::endl;
	h = HookManager::getInstance()->addHook((uintptr_t) & add, &hookadd);
	h->hook();
	std::cout << "hooked:" << std::endl;
	std::cout << "add(5,6):" << add(5, 16) << std::endl;

	std::cout << "removehook:" << std::endl;
	h->unhook();

	std::cout << "add(5,6):" << add(5, 6) << std::endl;
	return 0;
}

```

## 输出：

```
add(5,6):11
hooked:
add(5,6):210
removehook:
add(5,6):11
```

