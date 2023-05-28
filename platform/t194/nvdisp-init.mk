#
# Copyright (c) 2017-2022, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software and related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#

# Add any needed GLOBAL_DEFINES here
GLOBAL_DEFINES += \
	CONFIG_OS_IS_L4T=1 \
	CONFIG_ENABLE_DISPLAY=1 \
	CONFIG_ENABLE_DP=1 \
	CONFIG_ENABLE_NVDISP_INIT=1 \
	CONFIG_NVDISP_SIZE=0x60000 \
	CONFIG_NVDISP_UEFI_SIZE=0x300000
