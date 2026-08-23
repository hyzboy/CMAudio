cmake_minimum_required(VERSION 3.28)

project(AudioBankTool)

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 静态 CRT：与 CM 库（ULRE 主树 USE_STATIC_CRT=ON，MTd/MT）一致
# Qt 官方 DLL 为动态 CRT，但其接口类自含分配器，可安全混用
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Qt6
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 基础库根路径（ULRE 仓库：AudioBankTool → CMAudio → ULRE 根）
set(ULRE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# CMAudio/CMCore 等静态库输出目录（ULRE 主构建树 Debug）
set(ULRE_LIB_DIR "${ULRE_ROOT}/build/out/Windows_64_Debug")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# ---- 基础库编译宏（与 ULRE 主树 CMCMakeModule/system_bit.cmake 一致）----
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_compile_definitions(HGL_POINTER_UINT=uint64)
else()
    add_compile_definitions(HGL_POINTER_UINT=uint32)
endif()

# ---- 头文件路径（只依赖 CM 基础库，不碰 ULRE 渲染栈）----
include_directories(
    ${ULRE_ROOT}/CMCoreType/inc
    ${ULRE_ROOT}/CMMath/inc
    ${ULRE_ROOT}/CMCore/inc
    ${ULRE_ROOT}/CMAudio/inc
)

add_executable(AudioBankTool
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
)

target_link_libraries(AudioBankTool PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets
    ${ULRE_LIB_DIR}/CMAudio.lib
    ${ULRE_LIB_DIR}/CMCore.lib
    ${ULRE_LIB_DIR}/CMPlatform.lib
    ${ULRE_LIB_DIR}/CMCoreType.lib
    ${ULRE_LIB_DIR}/CMMath.lib
)

# 运行目录放 Qt DLL 旁边需要的资源（后续 wav 导入等）
set_target_properties(AudioBankTool PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

# ---- 端到端验证（P3：真实 WAV → 打包 → 读回 → 播放）----
add_executable(bank_e2e_verify verify/bank_e2e_verify.cpp)

target_link_libraries(bank_e2e_verify PRIVATE
    ${ULRE_LIB_DIR}/CMAudio.lib
    ${ULRE_LIB_DIR}/CMCore.lib
    ${ULRE_LIB_DIR}/CMPlatform.lib
    ${ULRE_LIB_DIR}/CMCoreType.lib
    ${ULRE_LIB_DIR}/CMMath.lib
)

set_target_properties(bank_e2e_verify PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

# 复制 Qt 运行库到输出目录（免环境变量）
if(WIN32)
    set(QT_BIN_DIR "${QT6_INSTALL_PREFIX}/bin")
    if(EXISTS "${QT_BIN_DIR}/Qt6Core.dll")
        add_custom_command(TARGET AudioBankTool POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT_BIN_DIR}/Qt6Core.dll"
                "${QT_BIN_DIR}/Qt6Gui.dll"
                "${QT_BIN_DIR}/Qt6Widgets.dll"
                $<TARGET_FILE_DIR:AudioBankTool>
        )
    endif()
endif()
