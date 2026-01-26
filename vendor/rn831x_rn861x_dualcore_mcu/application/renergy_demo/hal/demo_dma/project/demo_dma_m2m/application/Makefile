TARGET = demo_sea

NUCLEI_SDK_ROOT = ../../../..

SOC := rn831x_rn861x_dualcore_mcu

CORE := rn831x_rn861x_core0

DOWNLOAD := flash

SRCDIRS = code unity

INCDIRS = code unity

COMMON_FLAGS := -Oz -mcmodel=medlow -mno-save-restore -std=gnu11

CFLAGS += -DCORE0 -DUSE_FULL_LL_DRIVER

LDFLAGS +=

OPENOCD_PORT_ARGS :=

include $(NUCLEI_SDK_ROOT)/Build/Makefile.base

OPENOCD_ARGS += -c "program demo_sea.elf reset exit"
