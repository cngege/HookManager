// 定义应用程序的入口点。
//
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
