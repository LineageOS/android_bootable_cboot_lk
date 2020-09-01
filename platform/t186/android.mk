#
# Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software and related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

GLOBAL_DEFINES += \
	CONFIG_OS_IS_ANDROID=1 \
	CONFIG_ENABLE_NVDEC=1 \
	CONFIG_ENABLE_NCT=1 \
	CONFIG_ENABLE_VERIFIED_BOOT=1 \
	CONFIG_ENABLE_DISPLAY=1 \
	CONFIG_ENABLE_DISPLAY_MONITOR_INFO=1 \
	CONFIG_ENABLE_DP=1 \
	CONFIG_INITIALIZE_DISPLAY=2 \
	CONFIG_PROFILER_RECORD_LEVEL=PROFILER_RECORD_MINIMAL \
	CONFIG_ENABLE_DTB_OVERLAY=1 \
	CONFIG_ENABLE_FASTBOOT=1

ifeq ($(PLATFORM_IS_AFTER_N),1)
GLOBAL_DEFINES += \
       CONFIG_ENABLE_SYSTEM_AS_ROOT=1
endif

MODULE_DEPS += \
	$(LOCAL_DIR)/../../../../common/lib/nct \
	$(LOCAL_DIR)/../../../../common/lib/tos

# CONFIG_INITIALIZE_DISPLAY: 0-DSI, 1-HDMI, 2-DP, 3-EDP
