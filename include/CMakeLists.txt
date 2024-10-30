add_subdirectory(minhook)

target_include_directories(HookManager PRIVATE "${CMAKE_CURRENT_LIST_DIR}/minhook/include")