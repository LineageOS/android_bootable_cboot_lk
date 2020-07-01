#
# Copyright (c) 2016-2017, NVIDIA Corporation.  All Rights Reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property and
# proprietary rights in and to this software and related documentation.  Any
# use, reproduction, disclosure or distribution of this software and related
# documentation without an express license agreement from NVIDIA Corporation
# is strictly prohibited.
#

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	$(LOCAL_DIR)/../../../common/drivers/timer \
	$(LOCAL_DIR)/../../../common/drivers/soc/$(TARGET)/clocks \
	$(LOCAL_DIR)/../../../common/lib/tegrabl_brbct \
	$(LOCAL_DIR)/../../../common/lib/tegrabl_brbit \
	$(LOCAL_DIR)/../../../common/lib/mb1bct \
	$(LOCAL_DIR)/../../../../common/drivers/uart \
	$(LOCAL_DIR)/../../../../common/lib/console \
	$(LOCAL_DIR)/../../../../common/lib/debug \
	$(LOCAL_DIR)/../../../../common/lib/tegrabl_malloc \
	$(LOCAL_DIR)/../../../../common/drivers/blockdev \
	$(LOCAL_DIR)/../../../../common/drivers/sdmmc \
	$(LOCAL_DIR)/../../../../common/lib/tegrabl_gpt \
	$(LOCAL_DIR)/../../../../common/lib/tegrabl_utils \
	$(LOCAL_DIR)/../../../../common/lib/tegrabl_partition_manager \
	$(LOCAL_DIR)/../../../../common/lib/tegrabl_fastboot \
	$(LOCAL_DIR)/../../../../common/lib/tegrabl_frp \
	$(LOCAL_DIR)/../../../common/drivers/soc/t186/wdt \
	$(LOCAL_DIR)/verified_boot

GLOBAL_INCLUDES += \
	$(LOCAL_DIR) \
	../../common/include \
	$(LOCAL_DIR)/verified_boot

MODULE_SRCS += \
	$(LOCAL_DIR)/android_boot.c \
	$(LOCAL_DIR)/fastboot.c \
	$(LOCAL_DIR)/fastboot_a_b.c \
	$(LOCAL_DIR)/android_boot_menu.c \
	$(LOCAL_DIR)/fastboot_menu.c

include make/module.mk
