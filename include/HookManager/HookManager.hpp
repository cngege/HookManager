/*
* FROM https://github.com/cngege/HookManager IN CNGEGE
*/

#pragma once
#ifndef HOOKMANAGER_HPP
#define HOOKMANAGER_HPP

// 定义 USE_LIGHTHOOK 宏或定义 USE_MINHOOK, 来表示使用何种Hook实现底层逻辑
// 如果定义 EXTERNAL_INCLUDE_HOOKHEADER 宏，则表示引用底层Hook的头文件由外部实现

#ifdef USE_LIGHTHOOK
#include <Windows.h>
#ifndef EXTERNAL_INCLUDE_HOOKHEADER
#include <lighthook/LightHook.h>
#endif // EXTERNAL_INCLUDE_HOOKHEADER

#elif defined USE_DETOURS
#include <Windows.h>
#ifndef EXTERNAL_INCLUDE_HOOKHEADER
#include <detours.h>
//#include "detours/detours.h"
#endif // EXTERNAL_INCLUDE_HOOKHEADER

#elif defined USE_MINHOOK
#ifndef EXTERNAL_INCLUDE_HOOKHEADER
#include <MinHook.h>
#endif // EXTERNAL_INCLUDE_HOOKHEADER

#else 
#error You Need Define One Hooklib, USE_LIGHTHOOK OR USE_MINHOOK OR USE_DETOURS
#endif // USE_LIGHTHOOK

#if !(defined USE_LIGHTHOOK) && !(defined USE_MINHOOK) && !(defined USE_DETOURS)
#error You Need Define One Hooklib, USE_LIGHTHOOK OR USE_MINHOOK OR USE_DETOURS
#endif

#if (defined __amd64__) || (defined __amd64) || (defined __x86_64__) || (defined __x86_64) || (defined _M_AMD64)
#define _HM_IS_X64
#else
#define _HM_IS_X86
#endif // _X64_

#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>
#include <functional>

class HookInstance {
private:
    uintptr_t m_ptr = 0;
    void* m_fun = nullptr;

    /**
     * @brief hash表的key
     */
    uintptr_t m_mapindex = 0;
    /**
     * @brief 单例的描述
     */
    std::string m_describe{};

public:
    void* origin = nullptr;
    size_t arrayIndex = -1;
    bool enable = false;
public:
    HookInstance(){};
    HookInstance(uintptr_t ptr, void* fun) : m_ptr(ptr), m_fun(fun), m_mapindex(ptr) {};
    HookInstance(uintptr_t ptr, void* fun, std::string describe) : m_ptr(ptr), m_fun(fun), m_mapindex(ptr), m_describe(describe){};
    uintptr_t& ptr();
    uintptr_t ptr() const;
    void* fun();
    uintptr_t mapindex() const { return m_mapindex; };
    std::string describe() const;
    bool isEnable() const { return enable; };
    bool hook();
    bool unhook();

    template<typename T>
    T oriForSign(T sign) {
        return static_cast<T>(origin);
    }
};

struct HookTarget {
    std::vector<HookInstance*> Instances;
    /**
     * @brief 被hook函数的原始地址
     */
    uintptr_t HookPtr;
    /**
     * @brief Hook后的转道地址, 也是uninit时要释放的地址
     */
    uintptr_t FreePtr;
    void* Origin = nullptr;
    /**
     * @brief 此hook是否开启
     */
    bool hookEnable = false;
#ifdef USE_LIGHTHOOK
    HookInformation HookInfo;
#endif // USE_MINHOOK
    void setJmpFun(void* fun) {
        uintptr_t* jmp_addr = reinterpret_cast<uintptr_t*>(getJmpAddress());
#ifdef _HM_IS_X64
        *jmp_addr = (uintptr_t)fun;
#else
        *jmp_addr = (uintptr_t)(fun)-(getJmpAddress() + 4);
#endif
    }
    uintptr_t getJmpAddress() const {
#ifdef _HM_IS_X64
        return FreePtr + 6;
#else
        return FreePtr + 1;
#endif
    }
}_;

class HookManager
{
private:
    std::shared_mutex map_lock_mutex;
    std::unordered_map<uintptr_t, HookTarget> hookInfoHash{};
public:
    enum msgtype
    {
        info = 0x00,
        warn = 0x01,
        error = 0x02,
        debug = 0x03
    };

public:
    /**
     * @brief 拿到HookManager的单例, 通过此单例操控Hook事件和接口
     */
    static HookManager* getInstance();

public:
    /**
     * @brief 添加一个Hook
     * @param ptr hook的目标地址
     * @param fun hook拦截后要执行的本地函数
     * @param hook_describe 关于这个hook的描述 (可留空)
     * @return 返回针对这个hook的控制器(单例，单个hook管理器)
     */
    auto addHook(uintptr_t ptr, void* fun) -> HookInstance*;
    auto addHook(uintptr_t ptr, void* fun, std::string hook_describe) -> HookInstance*;

    /**
     * @brief 关闭此hook,并强行关闭且释放此hook下的所有单例
     * @param ptr 要移除的 被hook的函数地址
     * @return 
     */
    auto removeHook(uintptr_t ptr) -> void;
    auto removeHook(std::unordered_map<uintptr_t, HookTarget>::iterator& it) -> void;
    /**
     * @brief 开启一个hook
     * @return 是否成功
     */
    auto enableHook(HookInstance&) -> bool;
    /**
     * @brief 关闭一个hook
     * @return 是否成功
     */
    auto disableHook(HookInstance&) -> bool;
    /**
     * @brief 开启所有添加过的hook
     * @return 
     */
    auto enableAllHook() -> void;
    /**
     * @brief 关闭所有添加过，且是开启的hook
     * @return 
     */
    auto disableAllHook() -> void;
    /**
     * @brief 通过hook的目标函数地址找到对应hook的所有单例(这个函数后续可能有冲突,暂且保留)
     * @param  hook的目标函数地址
     * @return 返回找到的hook单例
     */
    auto findHookInstance(uintptr_t) -> std::vector<HookInstance*>;

    // Evenet
    using MessageEvent = std::function<void(msgtype type, std::string msg)>;

    /**
     * @brief 注册一个匿名函数监听消息
     * @param ev type: 0：info, 1: warn ,2: error, 3 debug (开发HookManager的debug信息, 请丢弃)
     * @return 
     */
    auto on(MessageEvent ev) -> void;

private:
    auto on(msgtype type, const char* fmt, ...)->void;
    
private:
    HookManager();
    ~HookManager();
    auto init() -> void;
    auto uninit() -> void;
    auto updateLastOrigin(HookTarget* target) -> void;

private:
    MessageEvent event = NULL;
};

////////////////////////////////////////////////

//static std::string str_fmt(const char* fmt, ...);

////////////////////////////////////////////////


inline HookManager* HookManager::getInstance() {
    static HookManager hookManager{};
    return &hookManager;
}

inline HookManager::HookManager() {
    init();
}

inline HookManager::~HookManager() {
    uninit();
}

inline auto HookManager::init()->void {
#ifdef USE_MINHOOK
    MH_STATUS status = MH_Initialize();
    if(status != MH_OK) {
        on(msgtype::error, "MH_Initialize 初始化失败:[%s]，文件:[%s] 函数: [%s] 行:[%d]", MH_StatusToString(status), __FILE__, __FUNCTION__, __LINE__);
    }
#endif // USE_MINHOOK
}
inline auto HookManager::uninit() -> void {
    // 关闭所有hook
    this->disableAllHook();
    // 释放申请的单例空间
    std::unordered_map<uintptr_t, HookTarget>::iterator it = hookInfoHash.begin();
    while(it != hookInfoHash.end()) {
        removeHook(it);
    }

#ifdef USE_MINHOOK
    MH_STATUS remove_status = MH_RemoveHook(MH_ALL_HOOKS);
    if(remove_status != MH_OK) {
        if(remove_status != MH_ERROR_NOT_CREATED) {
            on(msgtype::warn, "MH_RemoveHook 移除所有Hook时出现异常:[%s]，文件:[%s] 函数: [%s] 行:[%d]", MH_StatusToString(remove_status), __FILE__, __FUNCTION__, __LINE__);
        }
    }
    MH_STATUS uninit_status = MH_Uninitialize();
    if(uninit_status != MH_OK) {
        on(msgtype::error, "MH_Uninitialize 反初始化失败:[%s]，文件:[%s] 函数: [%s] 行:[%d],", MH_StatusToString(uninit_status), __FILE__, __FUNCTION__, __LINE__);
    }
#endif // USE_MINHOOK
}

inline auto HookManager::updateLastOrigin(HookTarget* target) -> void {
    for(long i = target->Instances.size() - 1; i >= 0; i--) {
        if(target->Instances[i]->isEnable()) {
            target->Instances[i]->origin = target->Origin;
            break;
        }
    }
}


inline auto HookManager::addHook(uintptr_t ptr, void* fun) -> HookInstance* {
    return addHook(ptr, fun, std::string());
}


inline auto HookManager::addHook(uintptr_t ptr, void* fun, std::string hook_describe) -> HookInstance* {
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
    auto it = hookInfoHash.find(ptr);
    if(it != hookInfoHash.end()) {
        HookInstance* instance = new HookInstance(ptr, fun, hook_describe);
        instance->arrayIndex = it->second.Instances.size();
        // 将自身加入进去
        it->second.Instances.push_back(instance);
        return it->second.Instances[it->second.Instances.size() - 1];
    }
    else {
       // 首先要申请一块内存地址 20字节(占14字节 FF 25 00 00 00 00 XX XX XX XX XX XX XX XX)
#ifdef _HM_IS_X64
        UCHAR JMPCODE[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00 };
#else
        UCHAR JMPCODE[] = { 0xE9 , 0x00, 0x00, 0x00, 0x00 };
#endif // _HM_IS_X64
        LPVOID allocPtr = VirtualAlloc(NULL, sizeof(JMPCODE), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if(allocPtr == NULL) {
            on(msgtype::error, "addHook 新增Hook VirtualAlloc申请内存失败,来自addHook的描述[%s],文件:[%s] 函数: [%s] 行:[%d]", hook_describe.empty() ? "无" : hook_describe.c_str(), __FILE__, __FUNCTION__, __LINE__);
            return nullptr;
        }
        memset(allocPtr, 0, sizeof(JMPCODE));
        memcpy_s(allocPtr, sizeof(JMPCODE), JMPCODE, sizeof(JMPCODE));
        hookInfoHash[ptr] = {};
        hookInfoHash[ptr].FreePtr = (uintptr_t)allocPtr;
        hookInfoHash[ptr].HookPtr = ptr;
        hookInfoHash[ptr].Instances = std::vector<HookInstance*>();
        HookInstance* instance = new HookInstance(ptr, fun, hook_describe);

#ifdef USE_LIGHTHOOK
        hookInfoHash[ptr].HookInfo = CreateHook((void*)ptr, (void*)allocPtr);
        instance->arrayIndex = hookInfoHash[ptr].Instances.size();
        // 将自身加入进去
        hookInfoHash[ptr].Instances.push_back(instance);

#endif // USE_LIGHTHOOK

#ifdef USE_MINHOOK
        
        
        MH_STATUS status = MH_CreateHook((LPVOID)ptr, (LPVOID)allocPtr, reinterpret_cast<LPVOID*>(&hookInfoHash[ptr].Origin));
        //hookInfoHash[ptr].Instances = std::vector<HookInstance*>();
        //hookInfoHash[ptr].HookPtr = ptr;
        if(status != MH_OK) {
            hookInfoHash.erase(ptr);
            on(msgtype::error, "MH_CreateHook失败:[MH_STATUS:%s]，目标Hook描述信息:[%s],文件:[%s] 函数: [%s] 行:[%d]", MH_StatusToString(status), hook_describe.empty() ? "无" : hook_describe.c_str(), __FILE__, __FUNCTION__, __LINE__);
            BOOL result = VirtualFree(allocPtr, 0, MEM_RELEASE);
            if(result == 0) on(msgtype::error, "MH_CreateHook失败后,VirtualFree释放内存失败,来自addHook的描述[%s],文件:[%s] 函数: [%s] 行:[%d]", hook_describe.empty() ? "无" : hook_describe.c_str(), __FILE__, __FUNCTION__, __LINE__);
            return nullptr;
        }
        instance->arrayIndex = hookInfoHash[ptr].Instances.size();
        // 将自身加入进去
        hookInfoHash[ptr].Instances.push_back(instance);

#endif // USE_MINHOOK

#ifdef USE_DETOURS
        //hookInfoHash[ptr] = { (uintptr_t)fun, hook_describe.empty() ? HookInstance(ptr) : HookInstance(ptr, hook_describe) };
        instance->arrayIndex = hookInfoHash[ptr].Instances.size();
        // 将自身加入进去
        hookInfoHash[ptr].Instances.push_back(instance);
#endif // USE_DETOURS
        return hookInfoHash[ptr].Instances[hookInfoHash[ptr].Instances.size() - 1];
    }
}

inline auto HookManager::removeHook(uintptr_t ptr) -> void {
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
    auto it = hookInfoHash.find(ptr);
    removeHook(it);
}

inline auto HookManager::removeHook(std::unordered_map<uintptr_t, HookTarget>::iterator& it) -> void {
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
    if(it != hookInfoHash.end()) {
        // 首先关闭hook, 全部unhook后会自动关闭
        for(auto* p : it->second.Instances) {
            p->unhook();
        }
        for(auto* p : it->second.Instances) {
            delete p;
        }
        // 清除了单例数组
        it->second.Instances.clear();

        // 释放申请的跳转空间
        BOOL result = VirtualFree((LPVOID)it->second.FreePtr, 0, MEM_RELEASE);
        if(result == 0) on(msgtype::error, "VirtualFree 释放申请的跳转空间失败 ,文件:[%s] 函数: [%s] 行:[%d]", __FILE__, __FUNCTION__, __LINE__);

        // 将hookInfoHash中的此项移除
        it = hookInfoHash.erase(it);
    }
    else {
        on(msgtype::warn, "你要移除的hook并不存在, 需要检查逻辑");
        return;
    }
}

inline auto HookManager::enableHook(HookInstance& instance) -> bool {
    if(instance.isEnable()) return true;

    // 如果当前hook目标只有一个 则开启hook
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);

    // 首先判断有没有启用的hook, 没有则hook, 
    // 自身除外是否存在已经启用的hook
    bool has_enableHook = false;
    // 自己是否是最后一个
    //bool current_is_end = false;
    for(auto& ins : hookInfoHash[instance.mapindex()].Instances) {
        if(ins->isEnable()) {
            has_enableHook = true;
            break;
        }
    }
    // 往后找
    for(size_t i = instance.arrayIndex + 1; i <= hookInfoHash[instance.mapindex()].Instances.size(); i++) {
        if(i == hookInfoHash[instance.mapindex()].Instances.size()) {
            // 自己是最后一个 自己要调用的应该是真实的函数
            instance.origin = hookInfoHash[instance.mapindex()].Origin;
            //current_is_end = true;
            break;
        }else if(hookInfoHash[instance.mapindex()].Instances[i]->isEnable()) {
            instance.origin = hookInfoHash[instance.mapindex()].Instances[i]->fun();
            break;
        }
    }

    // 往前找
    for(long i = instance.arrayIndex - 1; i >= -1; i--) {
        if(i == -1) {
            // 表示没找到 则将hook所要调用的目标改为自己
            hookInfoHash[instance.mapindex()].setJmpFun(instance.fun());
            break;
        }
        else if(hookInfoHash[instance.mapindex()].Instances[i]->isEnable()) {
            hookInfoHash[instance.mapindex()].Instances[i]->origin = instance.fun();
            break;
        }
    }
    instance.enable = true;
    // Hook已经是开启状态
    if(hookInfoHash[instance.mapindex()].hookEnable) {
        return true;
    }
    if(has_enableHook) return true;
#ifdef USE_LIGHTHOOK
    int ret = EnableHook(&hookInfoHash[instance.mapindex()].HookInfo);
    if(ret == 0) {
        on(msgtype::error, "LightHook EnableHook 失败，目标Hook描述信息:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(), __FILE__, __FUNCTION__, __LINE__);
        return false;
    }
    (&hookInfoHash[instance.mapindex()])->hookEnable = true;
    (&hookInfoHash[instance.mapindex()])->Origin = hookInfoHash[instance.mapindex()].HookInfo.Trampoline;
    updateLastOrigin(&hookInfoHash[instance.mapindex()]);
    
#endif // USE_LIGHTHOOK
#ifdef USE_MINHOOK
    MH_STATUS status = MH_EnableHook((LPVOID)instance.ptr());
    if(status != MH_OK) {
        on(msgtype::error, "MH_EnableHook 返回标识失败:[%s]，目标Hook描述信息:[%s],文件:[%s] 函数: [%s] 行:[%d]", MH_StatusToString(status), instance.describe().empty() ? "无" : instance.describe().c_str(), __FILE__, __FUNCTION__, __LINE__);
        return false;
    }
    (&hookInfoHash[instance.mapindex()])->hookEnable = true;

#endif // USE_MINHOOK

#ifdef USE_DETOURS
    LONG d_t_b_msg = DetourTransactionBegin();
    if(d_t_b_msg != NO_ERROR) {
        if(d_t_b_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error, "Detour一个挂起的事务已经存在，事务还未提交就是重复执行了，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",instance.describe().empty()? "无" : instance.describe().c_str(), "DetourTransactionBegin", __FILE__, __FUNCTION__, __LINE__);
        }
        DetourTransactionAbort();
        return false;
    }

    LONG d_u_t_msg = DetourUpdateThread(GetCurrentThread());
    if(d_u_t_msg != NO_ERROR) {
        if(d_u_t_msg == ERROR_NOT_ENOUGH_MEMORY) {
            on(msgtype::error, "Detour没有足够的内存来记录线程的标识，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourUpdateThread", __FILE__, __FUNCTION__, __LINE__);
        }
        DetourTransactionAbort();
        return false;
    }

    LONG d_a_ex = DetourAttachEx((PVOID*)&hookInfoHash[instance.mapindex()].HookPtr, (PVOID)hookInfoHash[instance.mapindex()].FreePtr, (PDETOUR_TRAMPOLINE*)&((&hookInfoHash[instance.mapindex()])->Origin), 0, 0);
    if(d_a_ex != NO_ERROR) {
        if(d_a_ex == ERROR_INVALID_BLOCK) {
            on(msgtype::error, "Detour被引用的函数太小，不能Hook，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(),"DetourAttachEx", __FILE__, __FUNCTION__, __LINE__);
        }
        else if(d_a_ex == ERROR_INVALID_HANDLE) {
            on(msgtype::error, "Detour 此Hook的目标地址为NULL或指向NULL指针，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(),"DetourAttachEx", __FILE__, __FUNCTION__, __LINE__);
        }
        else if(d_a_ex == ERROR_INVALID_OPERATION) {
            on(msgtype::error, "Detour不存在挂起的事务，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", "DetourAttachEx", instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourAttachEx", __FILE__, __FUNCTION__, __LINE__);
        }
        else if(d_a_ex == ERROR_NOT_ENOUGH_MEMORY) {
            on(msgtype::error, "Detour没有足够的内存来记录线程的标识，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourAttachEx", __FILE__, __FUNCTION__, __LINE__);
        }
        DetourTransactionAbort();
        return false;
    }

    LONG d_t_c_msg = DetourTransactionCommit();
    if(d_t_c_msg != NO_ERROR) {
        if(d_t_c_msg == ERROR_INVALID_DATA) {
            on(msgtype::error, "Detour目标函数在事务的各个步骤之间被第三方更改，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(),"DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__);
        }
        else if(d_t_c_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error, "Detour不存在挂起的事务，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]",instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__);
        }
        else {
            on(msgtype::error, "Detour DetourTransactionCommit返回未知错误:[%ld]，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", d_t_c_msg, instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__);
        }
        DetourTransactionAbort();
        return false;
    }
    updateLastOrigin(&hookInfoHash[instance.mapindex()]);
    (&hookInfoHash[instance.mapindex()])->hookEnable = true;
#endif // USE_MINHOOK
    return true;
}

inline auto HookManager::disableHook(HookInstance& instance) -> bool {
    if(!instance.isEnable()) return true;
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);

    bool has_enableHook = false;
    HookInstance* lastcallInstance = nullptr;
    // 往前找
    for(long i = instance.arrayIndex - 1; i >= -1; i--) {
        if(i == -1) {
            break;
        }
        else if(hookInfoHash[instance.mapindex()].Instances[i]->isEnable()) {
            lastcallInstance = (hookInfoHash[instance.mapindex()]).Instances[i];
            has_enableHook = true;
            break;
        }
    }

    // 往后找
    for(size_t i = instance.arrayIndex + 1; i <= hookInfoHash[instance.mapindex()].Instances.size(); i++) {
        if(i == hookInfoHash[instance.mapindex()].Instances.size()) {
            // 如果没找到开启的
            instance.origin = nullptr;
            if(lastcallInstance) {
                lastcallInstance->origin = hookInfoHash[instance.mapindex()].Origin;
            }
            else {
                hookInfoHash[instance.mapindex()].setJmpFun(hookInfoHash[instance.mapindex()].Origin);
            }
            break;
        }
        else if(hookInfoHash[instance.mapindex()].Instances[i]->isEnable()) {
            instance.origin = nullptr;
            has_enableHook = true;
            if(lastcallInstance) {
                lastcallInstance->origin = hookInfoHash[instance.mapindex()].Instances[i]->fun();
            }
            else {
                hookInfoHash[instance.mapindex()].setJmpFun(hookInfoHash[instance.mapindex()].Instances[i]->fun());
            }
            break;
        }
    }

    instance.enable = false;

    if(!hookInfoHash[instance.mapindex()].hookEnable) return true;
    // 这里的关闭hook是自动管理
    if(has_enableHook) return true;

#ifdef USE_LIGHTHOOK
    int ret = DisableHook(&hookInfoHash[instance.mapindex()].HookInfo);
    if(ret == 0) {
        on(msgtype::error, "LightHook DisableHook 失败，目标Hook描述信息:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(), __FILE__, __FUNCTION__, __LINE__);
        return false;
    }

#endif // USE_LIGHTHOOK
#ifdef USE_MINHOOK
    MH_STATUS status = MH_DisableHook((LPVOID)instance.ptr());
    if(status != MH_OK) {
        on(msgtype::error, "MH_DisableHook 返回标识失败:[%s]，目标Hook描述信息:[%s],文件:[%s] 函数: [%s] 行:[%d]", MH_StatusToString(status), instance.describe().empty() ? "无" : instance.describe().c_str(), __FILE__, __FUNCTION__, __LINE__);
        return false;
    }
#endif // USE_MINHOOK

#ifdef USE_DETOURS
    LONG d_t_b_msg = DetourTransactionBegin();
    if(d_t_b_msg != NO_ERROR) {
        if(d_t_b_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error, "Detour一个挂起的事务已经存在，事务还未提交就是重复执行了，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(),"DetourTransactionBegin", __FILE__, __FUNCTION__, __LINE__);
        }
        DetourTransactionAbort();
        return false;
    }

    LONG d_u_t_msg = DetourUpdateThread(GetCurrentThread());
    if(d_u_t_msg != NO_ERROR) {
        if(d_u_t_msg == ERROR_NOT_ENOUGH_MEMORY) {
            on(msgtype::error, "Detour没有足够的内存来记录线程的标识，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourUpdateThread", __FILE__, __FUNCTION__, __LINE__);
        }
        DetourTransactionAbort();
        return false;
    }

    LONG d_d_msg = DetourDetach((PVOID*)&instance.ptr(), (PVOID)hookInfoHash[instance.mapindex()].HookPtr);
    if(d_d_msg != NO_ERROR) {
        if(d_d_msg == ERROR_INVALID_BLOCK) {
            on(msgtype::error, "Detour被引用的函数太小，不能Hook，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(),"DetourDetach", __FILE__, __FUNCTION__, __LINE__);
        }
        else if(d_d_msg == ERROR_INVALID_HANDLE) {
            on(msgtype::error, "Detour 此Hook的目标地址为NULL或指向NULL指针，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourDetach", __FILE__, __FUNCTION__, __LINE__);
        }
        else if(d_d_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error, "Detour不存在挂起的事务，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourDetach", __FILE__, __FUNCTION__, __LINE__);
        }
        else if(d_d_msg == ERROR_NOT_ENOUGH_MEMORY) {
            on(msgtype::error, "Detour没有足够的内存来记录线程的标识，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourDetach", __FILE__, __FUNCTION__, __LINE__);
        }
        DetourTransactionAbort();
        return false;
    }

    LONG d_t_c_msg = DetourTransactionCommit();
    if(d_t_c_msg != NO_ERROR) {
        if(d_t_c_msg == ERROR_INVALID_DATA) {
            on(msgtype::error, "Detour目标函数在事务的各个步骤之间被第三方更改，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(),"DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__);
        }
        else if(d_t_c_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error, "Detour不存在挂起的事务，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", instance.describe().empty() ? "无" : instance.describe().c_str(),"DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__);
        }
        else {
            on(msgtype::error, "Detour DetourTransactionCommit返回未知错误:[%ld]，目标Hook描述信息:[%s]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", d_t_c_msg, instance.describe().empty() ? "无" : instance.describe().c_str(), "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__);
        }
        DetourTransactionAbort();
        return false;
    }
    
    
#endif // USE_MINHOOK
    (&hookInfoHash[instance.mapindex()])->hookEnable = false;
    return true;
}

inline auto HookManager::enableAllHook() -> void {
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);

#ifdef USE_LIGHTHOOK
    for(auto& item : hookInfoHash) {
        // 如果此项的hook已经是开启状态 则跳过
        if(item.second.hookEnable) continue;
        // 轮询所有单例
        for(auto& ins : item.second.Instances) {
            // 如果有开启的单例
            if(ins->isEnable()) {
                if(!item.second.HookInfo.Enabled) {
                    if(!EnableHook(&item.second.HookInfo)) {
                        on(msgtype::error, "LightHook EnableHook 开启Hook失败,文件:[%s] 函数: [%s] 行:[%d]", __FILE__, __FUNCTION__, __LINE__);
                        break;
                    }
                }
                (&hookInfoHash[ins->mapindex()])->hookEnable = true;
                (&hookInfoHash[ins->mapindex()])->Origin = item.second.HookInfo.Trampoline;
                updateLastOrigin(&item.second);
                break;
            }
        }
    }
#endif // USE_LIGHTHOOK

#ifdef USE_MINHOOK
    for(auto& item : hookInfoHash) {
        // 如果此项的hook已经是开启状态 则跳过
        if(item.second.hookEnable) continue;
        for(auto& ins : item.second.Instances) {
            if(ins->isEnable()) {
                // 如果此项里面有开启的
                MH_STATUS status = MH_EnableHook((LPVOID)item.second.HookPtr);
                if(status != MH_OK) {
                    on(msgtype::error, "MH_EnableHook 返回标识失败:[%s],文件:[%s] 函数: [%s] 行:[%d]", MH_StatusToString(status), __FILE__, __FUNCTION__, __LINE__);
                }
                (&hookInfoHash[ins->mapindex()])->hookEnable = true;
                break;
            }
        }
    }
#endif // USE_MINHOOK

#ifdef USE_DETOURS
    LONG d_t_b_msg = DetourTransactionBegin();
    if(d_t_b_msg != NO_ERROR) {
        if(d_t_b_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error, "Detour一个挂起的事务已经存在，事务还未提交就是重复执行了，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", "DetourTransactionBegin", __FILE__, __FUNCTION__, __LINE__);
        }
        DetourTransactionAbort();
        return;
    }

    LONG d_u_t_msg = DetourUpdateThread(GetCurrentThread());
    if(d_u_t_msg != NO_ERROR) {
        if(d_u_t_msg == ERROR_NOT_ENOUGH_MEMORY) {
            on(msgtype::error, "Detour没有足够的内存来记录线程的标识，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", "DetourUpdateThread", __FILE__, __FUNCTION__, __LINE__);
        }
        DetourTransactionAbort();
        return;
    }


    for(auto& item : hookInfoHash) {
        // 如果此项的hook已经是开启状态 则跳过
        if(item.second.hookEnable) continue;
        for(auto& ins : item.second.Instances) {
            if(ins->isEnable()) {
                // 如果此项里面有开启的
                LONG d_a_ex = DetourAttachEx((PVOID*)&item.second.HookPtr, (PVOID)item.second.FreePtr, (PDETOUR_TRAMPOLINE*)&item.second.Origin, 0, 0);
                if(d_a_ex != NO_ERROR) {
                    if(d_a_ex == ERROR_INVALID_BLOCK) {
                        on(msgtype::error, "Detour被引用的函数太小，不能Hook，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", "DetourAttachEx", __FILE__, __FUNCTION__, __LINE__);
                    }
                    else if(d_a_ex == ERROR_INVALID_HANDLE) {
                        on(msgtype::error, "Detour 此Hook的目标地址为NULL或指向NULL指针，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", "DetourAttachEx", __FILE__, __FUNCTION__, __LINE__);
                    }
                    else if(d_a_ex == ERROR_INVALID_OPERATION) {
                        on(msgtype::error, "Detour不存在挂起的事务，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", "DetourAttachEx", __FILE__, __FUNCTION__, __LINE__);
                    }
                    else if(d_a_ex == ERROR_NOT_ENOUGH_MEMORY) {
                        on(msgtype::error, "Detour没有足够的内存来记录线程的标识，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", "DetourAttachEx", __FILE__, __FUNCTION__, __LINE__);
                    }
                    else {
                        on(msgtype::error, "Detour DetourAttachEx返回未知错误:[%ld]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", d_a_ex, "DetourAttachEx", __FILE__, __FUNCTION__, __LINE__);
                    }
                }
                (&item.second)->hookEnable = true;
                updateLastOrigin(&item.second);
                break;
            }
        }
    }
    LONG d_t_c_msg = DetourTransactionCommit();
    if(d_t_c_msg != NO_ERROR) {
        if(d_t_c_msg == ERROR_INVALID_DATA) {
            on(msgtype::error, "Detour目标函数在事务的各个步骤之间被第三方更改，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__);
        }
        else if(d_t_c_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error, "Detour不存在挂起的事务，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__);
        }
        else {
            on(msgtype::error, "Detour DetourTransactionCommit返回未知错误:[%ld]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", d_t_c_msg, "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__);
        }
        DetourTransactionAbort();
        return;
    }
#endif // USE_DETOURS
}

inline auto HookManager::disableAllHook() -> void {
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);

#ifdef USE_LIGHTHOOK
    for(auto& item : hookInfoHash) {
        // 如果此项本来就是已经关闭状态，则跳过
        if(!item.second.hookEnable) continue;
        // 轮询所有单例
        for(auto& ins : item.second.Instances) {
            if(ins->isEnable()) {
                if(item.second.HookInfo.Enabled) {
                    if(!DisableHook(&item.second.HookInfo)) {
                        on(msgtype::error, "LightHook DisableHook 关闭Hook失败,文件:[%s] 函数: [%s] 行:[%d]", __FILE__, __FUNCTION__, __LINE__);
                        break;
                    }
                }
                (&hookInfoHash[ins->mapindex()])->hookEnable = false;
                break;
            }
        }
    }
#endif // USE_LIGHTHOOK
    
#ifdef USE_MINHOOK
    for(auto& item : hookInfoHash) {
        // 如果此项本来就是已经关闭状态，则跳过
        if(!item.second.hookEnable) continue;
        for(auto& ins : item.second.Instances) {
            if(ins->isEnable()) {
                // 如果此项里面有开启的
                MH_STATUS status = MH_DisableHook((LPVOID)item.second.HookPtr);
                if(status != MH_OK) {
                    on(msgtype::error, "MH_DisableHook 返回标识失败:[%s],文件:[%s] 函数: [%s] 行:[%d]", MH_StatusToString(status), __FILE__, __FUNCTION__, __LINE__);
                }
                (&hookInfoHash[ins->mapindex()])->hookEnable = false;
                break;
            }
        }

    }
#endif // USE_MINHOOK

#ifdef USE_DETOURS
    LONG d_t_b_msg = DetourTransactionBegin();
    if(d_t_b_msg != NO_ERROR) {
        if(d_t_b_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error, "Detour一个挂起的事务已经存在，事务还未提交就是重复执行了，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]","DetourTransactionBegin", __FILE__, __FUNCTION__, __LINE__);
        }
        DetourTransactionAbort();
        return;
    }

    LONG d_u_t_msg = DetourUpdateThread(GetCurrentThread());
    if(d_u_t_msg != NO_ERROR) {
        if(d_u_t_msg == ERROR_NOT_ENOUGH_MEMORY) {
            on(msgtype::error, "Detour没有足够的内存来记录线程的标识，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]","DetourUpdateThread", __FILE__, __FUNCTION__, __LINE__);
        }
        DetourTransactionAbort();
        return;
    }

    for(auto& item : hookInfoHash) {
        // 如果此项本来就是已经关闭状态，则跳过
        if(!item.second.hookEnable) continue;
        for(auto& ins : item.second.Instances) {
            if(ins->isEnable()) {
                LONG d_d_msg = DetourDetach((PVOID*)&item.second.HookPtr, (PVOID)item.second.FreePtr);
                if(d_d_msg != NO_ERROR) {
                    if(d_d_msg == ERROR_INVALID_BLOCK) {
                        on(msgtype::error, "Detour被引用的函数太小，不能Hook，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", "DetourDetach", __FILE__, __FUNCTION__, __LINE__);
                    }
                    else if(d_d_msg == ERROR_INVALID_HANDLE) {
                        on(msgtype::error, "Detour 此Hook的目标地址为NULL或指向NULL指针，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", "DetourDetach", __FILE__, __FUNCTION__, __LINE__);
                    }
                    else if(d_d_msg == ERROR_INVALID_OPERATION) {
                        on(msgtype::error, "Detour不存在挂起的事务，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", "DetourDetach", __FILE__, __FUNCTION__, __LINE__);
                    }
                    else if(d_d_msg == ERROR_NOT_ENOUGH_MEMORY) {
                        on(msgtype::error, "Detour没有足够的内存来记录线程的标识，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", "DetourDetach", __FILE__, __FUNCTION__, __LINE__);
                    }
                    else {
                        on(msgtype::error, "Detour DetourDetach返回未知错误:[%ld]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", d_d_msg, "DetourDetach", __FILE__, __FUNCTION__, __LINE__);
                    }
                    continue;
                }
                (&item.second)->hookEnable = false;
                break;
            }
        }
    }
    LONG d_t_c_msg = DetourTransactionCommit();
    if(d_t_c_msg != NO_ERROR) {
        if(d_t_c_msg == ERROR_INVALID_DATA) {
            on(msgtype::error, "Detour目标函数在事务的各个步骤之间被第三方更改，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]","DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__);
        }
        else if(d_t_c_msg == ERROR_INVALID_OPERATION) {
            on(msgtype::error, "Detour不存在挂起的事务，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]","DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__);
        }
        else {
            on(msgtype::error, "Detour DetourTransactionCommit返回未知错误:[%ld]，流程:[%s],文件:[%s] 函数: [%s] 行:[%d]", d_t_c_msg, "DetourTransactionCommit", __FILE__, __FUNCTION__, __LINE__);
        }
        DetourTransactionAbort();
        return;
    }
#endif // USE_DETOURS
}

inline auto HookManager::findHookInstance(uintptr_t indexptr) -> std::vector<HookInstance*> {
    std::shared_lock<std::shared_mutex> guard(map_lock_mutex);
    auto it = hookInfoHash.find(indexptr);
    if(it != hookInfoHash.end()) {
        return ((*it).second.Instances);
    }
    return {};
}

inline auto HookManager::on(MessageEvent ev) -> void {
    event = ev;
}

inline auto HookManager::on(msgtype type, const char* fmt, ...) -> void {
    if(event) {
        va_list arg;
        va_start(arg, fmt);
        char str[500];
        vsprintf_s(str, sizeof(str) - 1, fmt, arg);
        va_end(arg);
        event(type, str);
    }
}

inline uintptr_t& HookInstance::ptr() {
    return m_ptr;
}

inline uintptr_t HookInstance::ptr() const {
    return m_ptr;
}

inline void* HookInstance::fun() {
    return m_fun;
}

inline std::string HookInstance::describe() const {
    return m_describe;
}

inline bool HookInstance::hook() {
    return HookManager::getInstance()->enableHook(*this);
}

inline bool HookInstance::unhook() {
    return HookManager::getInstance()->disableHook(*this);
}

///////////////////////////

//inline static std::string str_fmt(const char* fmt, ...) {
//    va_list arg;
//    va_start(arg, fmt);
//    char str[500];
//    vsprintf_s(str, sizeof(str) - 1, fmt, arg);
//    va_end(arg);
//    return str;
//}



#endif   //HOOKMANAGER_HPP
