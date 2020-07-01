#
# Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software and related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

MODULE_DEPS += \
	$(LOCAL_DIR)/../../../../common/drivers/usbh/xusbh

GLOBAL_DEFINES += \
	CONFIG_OS_IS_ANDROID=1 \
	CONFIG_ENABLE_NVDEC=1 \
	CONFIG_ENABLE_NCT=1 \
	CONFIG_ENABLE_VERIFIED_BOOT=1 \
	CONFIG_ENABLE_DISPLAY=1 \
	CONFIG_ENABLE_DP=1 \
	CONFIG_INITIALIZE_DISPLAY=2 \
	CONFIG_ENABLE_XUSBH_UNCACHED_STRUCT=1 \
	CONFIG_ENABLE_XUSBH=1 \
	CONFIG_PROFILER_RECORD_LEVEL=PROFILER_RECORD_MINIMAL
# CONFIG_INITIALIZE_DISPLAY: 0-DSI, 1-HDMI, 2-DP, 3-EDP
