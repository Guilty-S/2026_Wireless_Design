#此文件从模板自动生成! 请勿更改!
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.30)

# specify cross-compilers and tools
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER  arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# project settings
project(desk_design C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

#Uncomment for hardware floating point
#add_compile_definitions(ARM_MATH_CM4;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)
#add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
#add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)

#Uncomment for software floating point
#add_compile_options(-mfloat-abi=soft)

add_compile_options(-mcpu=cortex-m3 -mthumb -mthumb-interwork)
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)

# uncomment to mitigate c++17 absolute addresses warnings
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register")

# Enable assembler files preprocessing
add_compile_options($<$<COMPILE_LANGUAGE:ASM>:-x$<SEMICOLON>assembler-with-cpp>)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-Og -g)
endif ()

include_directories(
        Core/Inc
        Drivers/STM32F1xx_HAL_Driver/Inc
        Drivers/STM32F1xx_HAL_Driver/Inc/Legacy
        Drivers/CMSIS/Device/ST/STM32F1xx/Include
        Drivers/CMSIS/Include
        easylogger/inc
        easylogger/port
        user
        mem
        MDK-ARM/RTE/CMSIS
        Clion_Lib/RTX/Inc
        Clion_Lib/RTX/SRC
)

add_definitions(-DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB)

#file(GLOB_RECURSE SOURCES "Core/*.*" "Drivers/*.*")
file(GLOB_RECURSE SOURCES
        "Core/*.c" "Core/*.s"
        "Drivers/*.c"
        "easylogger/src/*.c"
        "easylogger/port/*.c"
        "user/*.c"
        "MDK-ARM/RTE/CMSIS/*.c"
        "mem/*.c"
)
# 假设你的芯片是 STM32F1 (Cortex-M3)
set(RTX_SOURCES
        Clion_Lib/RTX/SRC/rt_Event.c
        Clion_Lib/RTX/SRC/rt_List.c
        Clion_Lib/RTX/SRC/rt_Mailbox.c
        Clion_Lib/RTX/SRC/rt_MemBox.c
        Clion_Lib/RTX/SRC/rt_Memory.c
        Clion_Lib/RTX/SRC/rt_Mutex.c
        Clion_Lib/RTX/SRC/rt_Robin.c
        Clion_Lib/RTX/SRC/rt_Semaphore.c
        Clion_Lib/RTX/SRC/rt_System.c
        Clion_Lib/RTX/SRC/rt_Task.c
        Clion_Lib/RTX/SRC/rt_Time.c
        Clion_Lib/RTX/SRC/rt_Timer.c
        Clion_Lib/RTX/SRC/rt_CMSIS.c
        Clion_Lib/RTX/SRC/HAL_CM.c
        # --- 核心修复点：后缀是大写的 .S ---
        Clion_Lib/RTX/SRC/GCC/HAL_CM3.S    # 针对 Cortex-M3 (F103)
        Clion_Lib/RTX/SRC/GCC/SVC_Table.S  # 必须包含，否则会报 rt_ret_val 找不到
)


# 也就是只包含 src 和 port，不包含 plugins。

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32F103C8TX_FLASH.ld)

add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
# 启用针对单片机的精简版 C 库（将 printf 体积从 40KB 压缩到 2KB）
add_compile_options(--specs=nano.specs)
add_link_options(--specs=nano.specs)
#add_link_options(-u _printf_float)
add_link_options(-mcpu=cortex-m3 -mthumb -mthumb-interwork)
add_link_options(-T ${LINKER_SCRIPT})

set(ALL_SOURCES ${SOURCES} ${RTX_SOURCES})
# --- 第三步：添加内核宏定义 ---
# 解决 "#error Missing __CORTEX_Mx definition"
add_definitions(-D__CORTEX_M3 -D__CMSIS_RTOS -D__RTX)
set_source_files_properties(Clion_Lib/RTX/SRC/GCC/HAL_CM3.S PROPERTIES LANGUAGE ASM)
set_source_files_properties(Clion_Lib/RTX/SRC/GCC/SVC_Table.S PROPERTIES LANGUAGE ASM)
add_executable(${PROJECT_NAME}.elf ${ALL_SOURCES} ${LINKER_SCRIPT})

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")
