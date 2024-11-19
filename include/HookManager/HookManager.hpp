/*
* FROM https://github.com/cngege/HookManager IN CNGEGE
*/

#pragma once
#ifndef HOOKMANAGER_HPP
#define HOOKMANAGER_HPP

// 定义 USE_LIGHTHOOK 宏或定义 USE_MINHOOK, 来表示使用何种Hook实现底层逻辑
// 如果定义 EXTERNAL_INCLUDE_HOOKHEADER 宏，则表示引用底层Hook的头文件由外部实现

#ifdef USE_LIGHTHOOK
#ifndef EXTERNAL_INCLUDE_HOOKHEADER
#include <Windows.h>
#include "LightHook/LightHook.h"
#endif // EXTERNAL_INCLUDE_HOOKHEADER

#elif defined USE_DETOURS
#ifndef EXTERNAL_INCLUDE_HOOKHEADER
#include "detours/detours.h"
#endif // EXTERNAL_INCLUDE_HOOKHEADER

#elif defined USE_MINHOOK
#ifndef EXTERNAL_INCLUDE_HOOKHEADER
#include "minhook/MinHook.h"
#endif // EXTERNAL_INCLUDE_HOOKHEADER

#else 
#error You Need Define One Hooklib, USE_LIGHTHOOK OR USE_MINHOOK OR USE_DETOURS
#endif // USE_LIGHTHOOK

#if !(defined USE_LIGHTHOOK) && !(defined USE_MINHOOK) && !(defined USE_DETOURS)
#error You Need Define One Hooklib, USE_LIGHTHOOK OR USE_MINHOOK OR USE_DETOURS
#endif


#include <unordered_map>
#include <mutex>
#include <shared_mutex>

class HookInstance {
private:
    uintptr_t m_ptr = 0;
    uintptr_t m_mapindex = 0;
public:
    void* origin = nullptr;
public:
    HookInstance(uintptr_t ptr) : m_ptr(ptr), m_mapindex(ptr){};
    uintptr_t& ptr();
    uintptr_t ptr() const;
    uintptr_t mapindex() const { return m_mapindex; };
    bool hook();
    bool unhook();
};

class HookManager
{
private:
    std::shared_mutex map_lock_mutex;
#ifdef USE_LIGHTHOOK
    std::unordered_map<uintptr_t, std::pair<HookInformation, HookInstance>> hookInfoHash{};
#endif

#ifdef USE_MINHOOK
    std::unordered_map<uintptr_t, std::pair<uintptr_t, HookInstance>> hookInfoHash{};
#endif // USE_MINHOOK

#ifdef USE_DETOURS
    std::unordered_map<uintptr_t, std::pair<uintptr_t, HookInstance>> hookInfoHash{};
#endif // USE_MINHOOK

public:
    static HookManager* getInstance();

public:

    auto addHook(uintptr_t ptr, void* fun) -> HookInstance*;

    auto enableHook(HookInstance&) -> bool;
    auto disableHook(HookInstance&) -> bool;
    auto enableAllHook() -> void;
    auto disableAllHook() -> void;
    auto findHookInstance(uintptr_t) -> HookInstance*;
    
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
#ifdef USE_MINHOOK
    MH_Initialize();
#endif // USE_MINHOOK
}
auto HookManager::uninit() -> void {
#ifdef USE_MINHOOK
    MH_RemoveHook(MH_ALL_HOOKS);
    MH_Uninitialize();
#endif // USE_MINHOOK
}


auto HookManager::addHook(uintptr_t ptr, void* fun) -> HookInstance* {
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
    auto it = hookInfoHash.find(ptr);
    if(it != hookInfoHash.end()) {
        // TODO:
        //spdlog::warn("addHook 新增Hook失败,当前函数已被hook({}) - fromFunction {} - in {}", (const void*)ptr, __FUNCTION__, __LINE__);
        return nullptr;
    }
    else {
#ifdef USE_LIGHTHOOK
        hookInfoHash[ptr] = { CreateHook((void*)ptr, fun), HookInstance(ptr) };
#endif // USE_LIGHTHOOK

#ifdef USE_MINHOOK
        hookInfoHash[ptr] = { ptr, HookInstance(ptr) };
        MH_CreateHook((LPVOID)ptr, (LPVOID)fun, reinterpret_cast<LPVOID*>(&hookInfoHash[ptr].second.origin));
#endif // USE_LIGHTHOOK

#ifdef USE_DETOURS
        hookInfoHash[ptr] = { (uintptr_t)fun, HookInstance(ptr) };
#endif // USE_LIGHTHOOK
        return &hookInfoHash[ptr].second;
    }
}

auto HookManager::enableHook(HookInstance& instance) -> bool {
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
#ifdef USE_LIGHTHOOK
    int ret = EnableHook(&hookInfoHash[instance.mapindex()].first);
    instance.origin = hookInfoHash[instance.mapindex()].first.Trampoline;
    return ret;
#endif // USE_LIGHTHOOK
#ifdef USE_MINHOOK
    MH_STATUS ret = MH_EnableHook((LPVOID)instance.ptr());
    return ret == MH_OK;
#endif // USE_MINHOOK

#ifdef USE_DETOURS
    if(DetourTransactionBegin() == NO_ERROR) {
        if(DetourUpdateThread(GetCurrentThread()) == NO_ERROR) {
            if(DetourAttachEx((PVOID*)&instance.ptr(), (PVOID)hookInfoHash[instance.mapindex()].first, (PDETOUR_TRAMPOLINE*) & instance.origin, 0, 0) == NO_ERROR) {
                if(DetourTransactionCommit() == NO_ERROR) {
                    return true;
                }
            }
        }
        DetourTransactionAbort();
    }
    return false;
#endif // USE_MINHOOK
    
}

auto HookManager::disableHook(HookInstance& instance) -> bool {
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);

#ifdef USE_LIGHTHOOK
    return DisableHook(&hookInfoHash[instance.mapindex()].first);
#endif // USE_LIGHTHOOK
#ifdef USE_MINHOOK
    MH_STATUS ret = MH_DisableHook((LPVOID)instance.ptr());
    return ret == MH_OK;
#endif // USE_MINHOOK

#ifdef USE_DETOURS
    if(DetourTransactionBegin() == NO_ERROR) {
        if(DetourUpdateThread(GetCurrentThread()) == NO_ERROR) {
            if(DetourDetach((PVOID*)&instance.ptr(), (PVOID)hookInfoHash[instance.mapindex()].first) == NO_ERROR) {
                if(DetourTransactionCommit() == NO_ERROR) {
                    return true;
                }
            }
        }
        DetourTransactionAbort();
    }
    return false;
#endif // USE_MINHOOK
}

auto HookManager::enableAllHook() -> void {
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);

#ifdef USE_LIGHTHOOK
    for(auto& item : hookInfoHash) {
        if(item.second.first.Enabled) {
            if(!DisableHook(&item.second.first)) {
                // TODO:
                //spdlog::warn("DisableHook 关闭Hook失败({}) - fromFunction {} - in {}", (const void*)item.first, __FUNCTION__, __LINE__);
            }
        }
    }
#endif // USE_LIGHTHOOK

#ifdef USE_MINHOOK
    for(auto& item : hookInfoHash) {
        if(MH_EnableHook((LPVOID)item.second.first) != MH_OK) {
            // TODO:
            //spdlog::warn("MH_EnableHook 开启Hook失败({}) - fromFunction {} - in {}", (const void*)item.first, __FUNCTION__, __LINE__);
        }
    }
#endif // USE_MINHOOK

#ifdef USE_DETOURS
    if(DetourTransactionBegin() == NO_ERROR) {
        if(DetourUpdateThread(GetCurrentThread()) == NO_ERROR) {
            for(auto& item : hookInfoHash) {
                if(DetourAttachEx((PVOID*)&item.second.second.ptr(), (PVOID)item.second.first, (PDETOUR_TRAMPOLINE*)&item.second.second.origin, 0, 0) == NO_ERROR) {
                    if(DetourTransactionCommit() == NO_ERROR) {
                        //success
                    }
                }
            }
            if(DetourTransactionCommit() == NO_ERROR) {
                // success
            }
        }
    }
#endif // USE_DETOURS
}

auto HookManager::disableAllHook() -> void {
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);

#ifdef USE_LIGHTHOOK
    for(auto& item : hookInfoHash) {
        if(item.second.first.Enabled) {
            if(!DisableHook(&item.second.first)) {
                // TODO:
                //spdlog::warn("DisableHook 关闭Hook失败({}) - fromFunction {} - in {}", (const void*)item.first, __FUNCTION__, __LINE__);
            }
        }
    }
#endif // USE_LIGHTHOOK
    
#ifdef USE_MINHOOK
    for(auto& item : hookInfoHash) {
        if(MH_DisableHook((LPVOID)item.second.first) != MH_OK) {
            // TODO:
            //spdlog::warn("MH_DisableHook 关闭Hook失败({}) - fromFunction {} - in {}", (const void*)item.first, __FUNCTION__, __LINE__);
        }
    }
#endif // USE_MINHOOK

#ifdef USE_DETOURS
    if(DetourTransactionBegin() == NO_ERROR) {
        if(DetourUpdateThread(GetCurrentThread()) == NO_ERROR) {
            for(auto& item : hookInfoHash) {
                if(DetourDetach((PVOID*)&item.second.second.ptr(), (PVOID)item.second.first) == NO_ERROR) {
                    // success
                }
            }
            if(DetourTransactionCommit() == NO_ERROR) {
                // success
            }
        }
    }
#endif // USE_DETOURS
}

auto HookManager::findHookInstance(uintptr_t indexptr) -> HookInstance* {
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
    auto it = hookInfoHash.find(indexptr);
    if(it != hookInfoHash.end()) {
        return &((*it).second.second);
    }
    return nullptr;
}


uintptr_t& HookInstance::ptr() {
    return m_ptr;
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