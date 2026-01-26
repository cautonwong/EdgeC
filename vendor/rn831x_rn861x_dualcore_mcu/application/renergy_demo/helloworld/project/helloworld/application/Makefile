TARGET = helloworld

NUCLEI_SDK_ROOT = ../../../../../..

SOC := rn831x_rn861x_dualcore_mcu

CORE := rn831x_rn861x_core0

DOWNLOAD := flash

SRCDIRS = . unity

INCDIRS = . unity

C_SRCS =

COMMON_FLAGS := -Oz -mcmodel=medlow -mno-save-restore -std=gnu11

CFLAGS +=

LDFLAGS +=

OPENOCD_PORT_ARGS :=

include $(NUCLEI_SDK_ROOT)/Build/Makefile.base

# run 'make run_openocd' command with this args: download and run.
# if you only need to download, just comment this args.
OPENOCD_ARGS += -c "program helloworld.elf reset exit"
