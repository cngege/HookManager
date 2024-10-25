#pragma once
#ifndef HOOKMANAGER_HPP
#define HOOKMANAGER_HPP

#include "LightHook/LightHook.h"
#include <unordered_map>
#include <mutex>
#include <shared_mutex>


class HookInstance {
private:
	uintptr_t m_ptr = 0;
public:
	void* origin = nullptr;
public:
	HookInstance(){};
	HookInstance(uintptr_t ptr) : m_ptr(ptr) {};

	uintptr_t ptr() const;
	bool hook();
	bool unhook();
};

class HookManager
{
private:
	std::shared_mutex map_lock_mutex;
	std::unordered_map<uintptr_t, std::pair<HookInformation, HookInstance>> hookInfoHash{};

public:
	static HookManager* getInstance();

public:

	auto addHook(uintptr_t ptr, void* fun) -> HookInstance*;

	auto enableHook(HookInstance&) -> int;
	auto disableHook(HookInstance&) -> int;
	auto enableAllHook() -> void;
	auto disableAllHook() -> void;
	
private:
	HookManager();
	~HookManager();
	auto init() -> void;
	auto uninit() -> void;
};

////////////////////////////////////////////////


HookManager* HookManager::getInstance() {
	static HookManager hookManager{};
	return &hookManager;
}

HookManager::HookManager() {
	init();
}

HookManager::~HookManager() {
	uninit();
}

auto HookManager::init()->void {
	//TODO: ...
}
auto HookManager::uninit() -> void {}


auto HookManager::addHook(uintptr_t ptr, void* fun) -> HookInstance* {
	std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
	auto it = hookInfoHash.find(ptr);
	if(it != hookInfoHash.end()) {
		// TODO:
		//spdlog::warn("addHook 新增Hook失败,当前函数已被hook({}) - fromFunction {} - in {}", (const void*)ptr, __FUNCTION__, __LINE__);
		return nullptr;
	}
	else {
		hookInfoHash[ptr] = {
			CreateHook((void*)ptr, fun),
			HookInstance(ptr)
		};
		return &hookInfoHash[ptr].second;
	}
}

auto HookManager::enableHook(HookInstance& instance) -> int {
	std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
	int ret = EnableHook(&hookInfoHash[instance.ptr()].first);
	instance.origin = hookInfoHash[instance.ptr()].first.Trampoline;
	return ret;
}

auto HookManager::disableHook(HookInstance& instance) -> int {
	std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
	return DisableHook(&hookInfoHash[instance.ptr()].first);
}

auto HookManager::enableAllHook() -> void {
	std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
	for(auto& item : hookInfoHash) {
		if(!item.second.first.Enabled) {
			if(!EnableHook(&item.second.first)) {
				// TODO:
				//spdlog::warn("EnableHook 开启Hook失败({}) - fromFunction {} - in {}", (const void*)item.first, __FUNCTION__, __LINE__);
			}
		}
	}
}

auto HookManager::disableAllHook() -> void {
	std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
	for(auto& item : hookInfoHash) {
		if(item.second.first.Enabled) {
			if(!DisableHook(&item.second.first)) {
				// TODO:
				//spdlog::warn("DisableHook 关闭Hook失败({}) - fromFunction {} - in {}", (const void*)item.first, __FUNCTION__, __LINE__);
			}
		}
	}
}



uintptr_t HookInstance::ptr() const {
	return m_ptr;
}

bool HookInstance::hook() {
	return HookManager::getInstance()->enableHook(*this);
}

bool HookInstance::unhook() {
	return HookManager::getInstance()->disableHook(*this);
}


#endif   //HOOKMANAGER_HPP