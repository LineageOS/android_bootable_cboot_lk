#
# Copyright (c) 2015-2016, NVIDIA Corporation.  All Rights Reserved.
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
	../../common/lib/external/asn1 \
	../../common/lib/external/mincrypt \
	../../common/lib/external/mbedtls \
	../common/drivers/se \
	../common/soc/t186/pkc_ops

GLOBAL_INCLUDES += \
	$(LOCAL_DIR) \
	app/android_boot/ \
	../../common/include/lib \
	include/lib \
	../common/include/soc/t186

MODULE_SRCS += \
	$(LOCAL_DIR)/signature_parser.c \
	$(LOCAL_DIR)/verified_boot.c \
	$(LOCAL_DIR)/verified_boot_ui.c \
	$(LOCAL_DIR)/menu_data.c \
	$(LOCAL_DIR)/monitor_interface.S


include make/module.mk
