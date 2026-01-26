TARGET = demo_system

NUCLEI_SDK_ROOT = ../../../..

NUCLEI_TOOL_ROOT ?= D:\NucleiStudio\toolchain
SOC := rn831x_rn861x_dualcore_mcu

CFLAGS += -DCORE0
CORE := rn831x_rn861x_core0

SRCDIRS = code/system_demo_drv unity

INCDIRS = code/system_demo_drv unity

COMMON_FLAGS := -Oz -mcmodel=medlow -mno-save-restore -std=gnu11

LDFLAGS +=

include $(NUCLEI_SDK_ROOT)/Build/Makefile.base

OPENOCD_ARGS += -c "program system_demo.elf reset exit"
