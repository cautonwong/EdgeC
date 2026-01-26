# Copyright (c) 2025 Renergy Technology
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
  ../../NMSIS/Core/Include
  ../../SoC/rn831x_rn861x_dualcore_mcu/Common/Include
  ../../SoC/rn831x_rn861x_dualcore_mcu/Common/Include/hal
  ../../SoC/rn831x_rn861x_dualcore_mcu/Common/Include/ll
)

zephyr_library()

# 根据 build.mk 补全指令集优化参数
if(CONFIG_SOC_RENERGY_CORE1)
    # Core 1: rv32imafc + 众多加速扩展
    zephyr_library_compile_options(-march=rv32imafc_zba_zbb_zbc_zbs_zca_zcb_zcmp_zcmt_zicond)
    zephyr_library_compile_definitions(CORE1)
elseif(CONFIG_SOC_RENERGY_CORE0)
    # Core 0: rv32imc + 基础扩展
    zephyr_library_compile_options(-march=rv32imc_zba_zbb_zbc_zbs_zca_zcb_zcmp_zcmt)
    zephyr_library_compile_definitions(CORE0)
endif()

zephyr_library_compile_definitions(USE_FULL_LL_DRIVER)

# 包含核心启动与驱动
zephyr_library_sources(
  ../../SoC/rn831x_rn861x_dualcore_mcu/Common/Source/Drivers/hal/rn_rv_hal.c
  ../../SoC/rn831x_rn861x_dualcore_mcu/Common/Source/Drivers/hal/rn_rv_hal_system.c
  ../../SoC/rn831x_rn861x_dualcore_mcu/Common/Source/Drivers/hal/rn_rv_hal_uart.c
  ../../SoC/rn831x_rn861x_dualcore_mcu/Common/Source/Drivers/hal/rn_rv_hal_gpio.c
)

# 包含底层的中断与启动汇编 (Non-intrusive approach)
zephyr_library_sources(
  ../../SoC/rn831x_rn861x_dualcore_mcu/Common/Source/GCC/intexc_rn831x_rn861x.S
)