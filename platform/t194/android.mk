#
# Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software and related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

GLOBAL_DEFINES += \
	CONFIG_OS_IS_ANDROID=1 \
	CONFIG_ENABLE_FASTBOOT=1 \
	CONFIG_ENABLE_VERIFIED_BOOT=1 \
	CONFIG_ENABLE_NCT=1 \
	CONFIG_ENABLE_DISPLAY=1 \
	CONFIG_ENABLE_DP=1 \
	CONFIG_ENABLE_EDP=1 \
	CONFIG_ENABLE_USBF_SNO=1 \
	CONFIG_ENABLE_DISPLAY_MONITOR_INFO=1 \
	CONFIG_ENABLE_ANDROID_BOOTREASON=1 \
	CONFIG_ENABLE_SDCARD=1 \
	CONFIG_ENABLE_USB_MS=1

MODULE_DEPS += \
	$(LOCAL_DIR)/../../../common/lib/nct \
	$(LOCAL_DIR)/../../../common/lib/tos
