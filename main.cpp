// 定义应用程序的入口点。
//
#include <iostream>
#include "HookManager/HookManager.hpp"

int add(int a, int b) {
	return a + b;
}

int hookadd(int a, int b) {
	return a * b;
}

int main()
{
	std::cout << "add(5,6):" << add(5, 6) << std::endl;
	auto h = HookManager::getInstance()->addHook((uintptr_t) & add, &hookadd);
	h->hook();
	std::cout << "hooked:" << std::endl;
	std::cout << "add(5,6):" << add(5, 6) << std::endl;

	std::cout << "removehook:" << std::endl;
	h->unhook();

	std::cout << "add(5,6):" << add(5, 6) << std::endl;
	return 0;
}
