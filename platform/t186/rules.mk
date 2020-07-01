#
# Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software and related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm64
ARM64_CPU := cortex-a57

MODULE_DEPS += \
	platform/tegra_shared \
	lib/menu \
	lib/exit \
	$(LOCAL_DIR)/../../../../common/arch/arm64 \
	$(LOCAL_DIR)/../../../../common/drivers/i2c \
	$(LOCAL_DIR)/../../../../common/drivers/i2c_dev \
	$(LOCAL_DIR)/../../../../common/drivers/eeprom \
	$(LOCAL_DIR)/../../../../common/lib/tegrabl_nct \
	$(LOCAL_DIR)/../../../../common/lib/tegrabl_eeprom_manager \
	$(LOCAL_DIR)/../../../common/lib/ipc \
	$(LOCAL_DIR)/../../../common/drivers/fuse \
	$(LOCAL_DIR)/../../../common/drivers/gpcdma \
	$(LOCAL_DIR)/../../../common/soc/$(TARGET)/misc \
	$(LOCAL_DIR)/../../../common/drivers/soc/$(TARGET)/power \
	$(LOCAL_DIR)/../../../common/lib/mce \
	$(LOCAL_DIR)/../../../common/lib/rollback_prevention \
	$(LOCAL_DIR)/../../../common/lib/tegrabl_auth \
	$(LOCAL_DIR)/../../../common/lib/tegrabl_se_keystore \
	$(LOCAL_DIR)/../../../../common/drivers/display \
	$(LOCAL_DIR)/../../../../common/drivers/gpio \
	$(LOCAL_DIR)/../../../../common/drivers/qspi \
	$(LOCAL_DIR)/../../../../common/drivers/qspi_flash \
	$(LOCAL_DIR)/../../../../common/drivers/ufs \
	$(LOCAL_DIR)/../../../../common/drivers/keyboard \
	$(LOCAL_DIR)/../../../../common/lib/tegrabl_nvblob \
	$(LOCAL_DIR)/../../../../common/lib/tegrabl_nvblob_bmp \
	$(LOCAL_DIR)/../../../../common/lib/profiler \
	$(LOCAL_DIR)/../../../../common/lib/psci \
	$(LOCAL_DIR)/../../../../common/lib/tegrabl_exit \
	$(LOCAL_DIR)/../../../../common/drivers/spi \
	$(LOCAL_DIR)/../../../../common/drivers/sata \
	$(LOCAL_DIR)/../../../../common/lib/tegrabl_bootloader_update \
	$(LOCAL_DIR)/../../../../common/drivers/pmic \
	$(LOCAL_DIR)/../../../../common/drivers/pmic/max77620 \
	$(LOCAL_DIR)/../../../../common/drivers/regulator \
	$(LOCAL_DIR)/../../../../common/lib/tegrabl_storage

GLOBAL_INCLUDES += \
	$(LOCAL_DIR) \
	$(LOCAL_DIR)/../../../common/include/soc/t186 \
	$(LOCAL_DIR)/../../../../../../hwinc-t18x \
	$(LOCAL_DIR)/../../../../../../mb1/partner-t18x/include

MODULE_SRCS += \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/ratchet_update.c \
	$(LOCAL_DIR)/storage_loader.c \
	$(LOCAL_DIR)/dram_ecc.c

MEMBASE := 0x96000000
FIXED_PLLP_FREQ := 408000000
GLOBAL_DEFINES += \
	MEMBASE=$(MEMBASE) \
	IS_A64_MODE=$(IS_A64_MODE) \
	FIXED_PLLP_FREQ=$(FIXED_PLLP_FREQ) \
	MAX_CPU_CLUSTERS=2 \
	MAX_CPUS_PER_CLUSTER=4 \
	CONFIG_ENABLE_GPT=1 \
	CONFIG_ENABLE_UART=1 \
	CONFIG_ENABLE_GPIO=1 \
	CONFIG_ENABLE_PARTITION_MANAGER=1 \
	CONFIG_ENABLE_EMMC=1 \
	CONFIG_ENABLE_SDMMC_64_BIT_SUPPORT=1 \
	CONFIG_ENABLE_QSPI=1 \
	CONFIG_ENABLE_UFS=1 \
	CONFIG_ENABLE_UFS_HS_MODE=1 \
	CONFIG_ENABLE_I2C=1 \
	CONFIG_ENABLE_EEPROM=1 \
	CONFIG_DT_SUPPORT=1 \
	CONFIG_MULTICORE_SUPPORT=1 \
	CONFIG_DEBUG_TIMESTAMP=1 \
	CONFIG_ENABLE_WDT=1 \
	CONFIG_POWER_I2C_BPMPFW=1 \
	CONFIG_ENABLE_DPAUX=1 \
	CONFIG_ENABLE_XUSBF_UNCACHED_STRUCT=1 \
	CONFIG_ENABLE_XUSBF_REENUMERATION=1 \
	CONFIG_ENABLE_NVBLOB=1 \
	CONFIG_ENABLE_SE=1 \
	CONFIG_ENABLE_USBF_SNO=1 \
	CONFIG_ENABLE_A_B_SLOT=1 \
	CONFIG_ENABLE_PMIC_MAX77620=1 \
	CONFIG_PAGE_SIZE_LOG2=16 \
	CONFIG_ENABLE_GPCDMA=1 \
	CONFIG_ENABLE_QSPI_FLASH_READ_ID=1 \
	CONFIG_ENABLE_QSPI_QDDR_READ=1 \
	CONFIG_ENABLE_XUSBF_SS=1 \
	CONFIG_BOOT_PROFILER=1 \
	CONFIG_ENABLE_DRAM_ECC=1

# Move optional CONFIG items into sub-make files
ifeq ($(NV_BUILD_SYSTEM_TYPE),l4t)
include $(LOCAL_DIR)/l4t.mk
else
include $(LOCAL_DIR)/android.mk
endif

GLOBAL_DEFINES += WITH_CPU_EARLY_INIT=1

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

include make/module.mk
