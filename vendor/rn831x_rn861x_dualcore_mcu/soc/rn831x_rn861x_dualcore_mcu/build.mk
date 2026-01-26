##
#### SoC Build Settings Notice
# The following variables need be defined according to your SoC and Board
# BOARD: MUST, set this variable to one of the valid folder basename under $(NUCLEI_SDK_SOC)/Board/
# CORE: MUST, set this variable to one of SUPPORTED_CORES defined in $(NUCLEI_SDK_BUILD)/Makefile.core
# VARIANT: OPTIONAL, set this variable to consecutive characters better less than 8 chars, such as lite, v1, v2.
#          It can be used to define different variant of similar board
# NUCLEI_SDK_SOC_BOARD: MUST, set this variable to $(NUCLEI_SDK_SOC)/Board/$(BOARD), this means the
#                       base folder for selected BOARD
# NUCLEI_SDK_SOC_COMMON: MUST, set this variable to $(NUCLEI_SDK_SOC)/Common, this means the base
#                       folder for selected SOC
# OPENOCD_CFG: MUST, set this variable to openocd configuration file used for the Board or SoC
# LINKER_SCRIPT: MUST, set this variable to linker script file for selected board and download mode
# RISCV_ARCH: MUST, set this variable to the riscv arch you are using, see $(NUCLEI_SDK_BUILD)/Makefile.core
# RISCV_ABI: MUST, set this variable to the riscv abi you are using, see $(NUCLEI_SDK_BUILD)/Makefile.core
# RISCV_TUNE: Optional, set this variable to the mtune you are using, see $(NUCLEI_SDK_BUILD)/Makefile.core
# If your SoC used a fixed BOARD, CORE or DOWNLOAD mode, please use override to define a fixed variable value, eg.
# override CORE := n205
# override DOWNLOAD := flashxip
# override BOARD := nuclei_fpga_eval
#### Source Code Management Notice
# you need to declare the following items:
# 1. C/ASM/CPP source code folders which need to be compiled
# 2. Some C/ASM/CPP source code files whihc need to be compiled
# You can use these makefile variables defined in $(NUCLEI_SDK_BUILD)/Makefile.files to achieve this, for details,
# please check https://doc.nucleisys.com/nuclei_sdk/develop/buildsystem.html#build-related-makefile-variables-used-only-in-application-makefile
##

##### Put your SoC build configurations below #####

BOARD ?= rn8318

CORE ?= rn831x_rn861x_core0

# SMP must be a number, and above 1, means smp cpu count in a cluster
# it will define c macro SMP_CPU_CNT to be SMP value
# and define a ld symbol __SMP_CPU_CNT to be used by linker script
SMP ?=

# BOOT_HARTID must be a number, and above 0, the hartid is taken the mhartid bit 0-7
# it will define a c macro BOOT_HARTID to be boot hart id,
# other harts other than boot hartid will do wfi when in AMP mode,
# or run application in smp mode
BOOT_HARTID ?= 0

# HARTID_OFS is used to set hartid offset to the cpu hartid to get current hart index
# for example, cpu hartid maybe 1, but hart index is 0, so the offset is 1
# This is not neccessary for most cases, only useful in cases such as a multiple cpu
# system, cpu0 has 1 hart, hartid is 0, cpu1 has 1 hart, hartid is 1, but cpu1
# hart index is 0, so in this case set this value to 1
HARTID_OFS ?=

# JTAGSN must be a jtag serial number
# If not specified, it will not bind serial number
JTAGSN ?=

NUCLEI_SDK_SOC_BOARD := $(NUCLEI_SDK_SOC)/Board/$(BOARD)
NUCLEI_SDK_SOC_COMMON := $(NUCLEI_SDK_SOC)/Common

OPENOCD_OPT ?= -s $(NUCLEI_SDK_SOC_BOARD)

ifeq ($(CORE), rn831x_rn861x_core1)
ifeq ($(DOWNLOAD), sram)
OPENOCD_CFG ?= $(NUCLEI_SDK_SOC_BOARD)/$(BOARD)_core1_sram.cfg # not supported
else
OPENOCD_CFG ?= $(NUCLEI_SDK_SOC_BOARD)/$(BOARD)_core1_flash.cfg
endif
else
ifeq ($(DOWNLOAD), sram)
OPENOCD_CFG ?= $(NUCLEI_SDK_SOC_BOARD)/$(BOARD)_core0_sram.cfg
else
OPENOCD_CFG ?= $(NUCLEI_SDK_SOC_BOARD)/$(BOARD)_core0_flash.cfg
endif
endif

# smp use a different openocd configuration file
# and will set SMP value in the openocd configuration file
ifneq ($(SMP),)
OPENOCD_CMD_ARGS += set SMP $(SMP);
endif

ifeq ($(CORE), rn831x_rn861x_core1)
ifeq ($(DOWNLOAD), sram)
LINKER_SCRIPT ?= $(NUCLEI_SDK_SOC_BOARD)/Source/GCC/$(BOARD)_gcc_sram_core1.ld
else
LINKER_SCRIPT ?= $(NUCLEI_SDK_SOC_BOARD)/Source/GCC/$(BOARD)_gcc_flash_core1.ld
endif
else
ifeq ($(DOWNLOAD), sram)
LINKER_SCRIPT ?= $(NUCLEI_SDK_SOC_BOARD)/Source/GCC/$(BOARD)_gcc_sram_core0.ld
else
LINKER_SCRIPT ?= $(NUCLEI_SDK_SOC_BOARD)/Source/GCC/$(BOARD)_gcc_flash_core0.ld
endif
endif

# GDB extra flags
# GDB_UPLOAD_CMDS += -ex 'set mem inaccessible-by-default off'
# GDB_UPLOAD_CMDS += -ex 'set arch riscv:rv32' --nx

# File existence check for OPENOCD_CFG and LINKER_SCRIPT
ifeq ($(wildcard $(OPENOCD_CFG)),)
$(error The openocd configuration file $(OPENOCD_CFG) for $(SOC) doesn't exist, please check!)
endif
# Allow non-existance of LINKER_SCRIPT, it might be generated
ifeq ($(wildcard $(LINKER_SCRIPT)),)
$(warning The link script file $(LINKER_SCRIPT) for $(SOC) doesn't exist, please check!)
endif

-include $(NUCLEI_SDK_SOC)/runmode.mk

# Add extra cflags for SoC related
ifeq ($(DOWNLOAD), flash)
COMMON_FLAGS += -DVECTOR_TABLE_REMAPPED
endif

ifneq ($(SMP),)
$(call assert,$(call gt,$(SMP),1),SMP must be a integer number >= 2)
ifneq ($(BOOT_HARTID),)
$(call assert,$(call gt,$(SMP),$(BOOT_HARTID)),BOOT_HARTID must be small than SMP)
endif
CPU_CNT := $(SMP)
COMMON_FLAGS += -DSMP_CPU_CNT=$(CPU_CNT)
LDFLAGS += -Wl,--defsym=__SMP_CPU_CNT=$(CPU_CNT)
endif
ifneq ($(BOOT_HARTID),)
$(call assert,$(call gte,$(BOOT_HARTID),0),BOOT_HARTID must be a integer number >= 0)
ifeq ($(CPU_CNT),)
CPU_CNT := $(call inc,$(BOOT_HARTID))
endif
# if BOOT_HARTID is set, will set the BOOT_HARTID in openocd configuration file
OPENOCD_CMD_ARGS += set BOOT_HARTID $(BOOT_HARTID);
COMMON_FLAGS += -DBOOT_HARTID=$(BOOT_HARTID)
endif
# if JTAGSN is not empty, pass it via openocd command
ifneq ($(JTAGSN),)
OPENOCD_CMD_ARGS += set JTAGSN $(JTAGSN);
endif

# If HARTID_OFS is not empty
ifneq ($(HARTID_OFS),)
COMMON_FLAGS += -D__HARTID_OFFSET=$(HARTID_OFS)
endif

ifneq ($(CPU_CNT),)
QEMU_OPT += -smp $(CPU_CNT)
endif
## xlspike is only valid for nuclei demosoc/evalsoc
ifneq ($(CPU_CNT),)
XLSPIKE_OPT += -p$(CPU_CNT)
endif

# Set RISCV_ARCH and RISCV_ABI
CORE_UPPER := $(call uc, $(CORE))
include $(NUCLEI_SDK_BUILD)/Makefile.core
# you can override SUPPORTED_CORES defined in Makefile.core to limit the COREs used in this SoC
# eg. override SUPPORTED_CORES := n305 n307

CORE_ARCH_ABI := $($(CORE_UPPER)_CORE_ARCH_ABI)
# Check whether CORE is in SUPPORTED_CORES
ifeq ($(filter $(CORE), $(SUPPORTED_CORES)),)
$(error Here we only support these cores: $(SUPPORTED_CORES))
endif
# Check whether CORE_ARCH_ABI is presented for CORE
ifneq ($(words $(wordlist 1, 2, $(CORE_ARCH_ABI))), 2)
$(error No correct CORE_ARCH_ABI setting for CORE=$(CORE) found in $(realpath $(NUCLEI_SDK_BUILD)/Makefile.core))
endif

# Handle Nuclei RISC-V ARCH/ABI/CMODEL/TUNE
## ARCH_EXT could be combination of in order of bkpv, legal combination is list as below:
## bp: Bitmanip and Packed SIMD Extension present
## bpv: Bitmanip, Packed SIMD and Vector Extension present
## bkpv: Bitmanip, Packed SIMD, Scalar Cryptography and Vector Extension present
### If zc extension passed in ARCH_EXT, remove c in ARCH
ifeq ($(CORE), rn831x_rn861x_core1)
# Enable Bitmanip Extensions: _zba_zbb_zbc_zbs
# Enable Zc Extensions: _zca_zcb_zcmp_zcmt
# Enable Zicond Extensions
# Enable Nuclei custom extensions: Xxlcz Extensions
ARCH_EXT ?= _zba_zbb_zbc_zbs_zca_zcb_zcmp_zcmt_zicond_xxlcz
else
# Enable Bitmanip Extensions: _zba_zbb_zbc_zbs
# Enable Zc Extensions: _zca_zcb_zcmp_zcmt
ARCH_EXT ?= _zba_zbb_zbc_zbs_zca_zcb_zcmp_zcmt
endif
TEMP_RISCV_ARCH := $(word 1, $(CORE_ARCH_ABI))
ifneq ($(findstring zc,$(ARCH_EXT)),)
RISCV_ARCH ?= $(TEMP_RISCV_ARCH:c=)$(ARCH_EXT)
else
RISCV_ARCH ?= $(TEMP_RISCV_ARCH)$(ARCH_EXT)
endif
RISCV_ABI ?= $(word 2, $(CORE_ARCH_ABI))
RISCV_TUNE ?= $(word 3, $(CORE_ARCH_ABI))

# Handle QEMU Emulation
QEMU_MACHINE ?= nuclei_evalsoc,download=$(DOWNLOAD)
QEMU_ARCHEXT ?= ${ARCH_EXT}

ifeq ($(CORE), rn831x_rn861x_core1)
QEMU_CPU ?= nuclei-n300,ext=$(QEMU_ARCHEXT)
else ifeq ($(CORE), rn831x_rn861x_core0)
QEMU_CPU ?= nuclei-n200,ext=$(QEMU_ARCHEXT)
else
QEMU_CPU ?= nuclei-$(CORE),ext=$(QEMU_ARCHEXT)
endif

ifneq ($(SEMIHOST),)
QEMU_OPT += -semihosting
endif

##### Put your Source code Management configurations below #####

INCDIRS += $(NUCLEI_SDK_SOC_COMMON)/Include
INCDIRS += $(NUCLEI_SDK_SOC_COMMON)/Include/ll
INCDIRS += $(NUCLEI_SDK_SOC_COMMON)/Include/hal
# INCDIRS += $(NUCLEI_SDK_SOC_COMMON)/Include/rn8xxx_ll

C_SRCDIRS += $(NUCLEI_SDK_SOC_COMMON)/Source $(NUCLEI_SDK_SOC_COMMON)/Source/Drivers
C_SRCDIRS += $(NUCLEI_SDK_SOC_COMMON)/Source/Drivers/ll
C_SRCDIRS += $(NUCLEI_SDK_SOC_COMMON)/Source/Drivers/hal
# C_SRCDIRS += $(NUCLEI_SDK_SOC_COMMON)/Source/Drivers/rn8xxx_ll

# If semihosting is enabled, no stub function is needed
ifeq ($(SEMIHOST),)
ifneq ($(findstring newlib,$(STDCLIB)),)
C_SRCDIRS += $(NUCLEI_SDK_SOC_COMMON)/Source/Stubs/newlib
else
# no stubs will be used
endif
endif

ifneq ($(findstring libncrt,$(STDCLIB)),)
# semihosting currently not ported to support libncrt
C_SRCDIRS += $(NUCLEI_SDK_SOC_COMMON)/Source/Stubs/libncrt
endif

ASM_SRCS += $(NUCLEI_SDK_SOC_COMMON)/Source/GCC/intexc_rn831x_rn861x.S
ASM_SRCS += $(NUCLEI_SDK_SOC_COMMON)/Source/GCC/startup_rn831x_rn861x.S

# Add extra board related source files and header files
VALID_NUCLEI_SDK_SOC_BOARD := $(wildcard $(NUCLEI_SDK_SOC_BOARD))
ifneq ($(VALID_NUCLEI_SDK_SOC_BOARD),)
INCDIRS += $(VALID_NUCLEI_SDK_SOC_BOARD)/Include
C_SRCDIRS += $(VALID_NUCLEI_SDK_SOC_BOARD)/Source
endif
