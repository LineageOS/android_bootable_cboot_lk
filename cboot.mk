# Copyright (c) 2014-2015, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.

# Define module cboot.
# cboot: Compile the cboot kernel and generate cboot.bin

LOCAL_PATH := $(call my-dir)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE        := cboot
LOCAL_MODULE_SUFFIX := .bin
LOCAL_MODULE_PATH   := $(PRODUCT_OUT)
LOCAL_MODULE_CLASS  := EXECUTABLES

_cboot_intermediates := $(call intermediates-dir-for,$(LOCAL_MODULE_CLASS),$(LOCAL_MODULE))
_cboot_project := $(TARGET_TEGRA_VERSION)

LOCAL_BUILT_MODULE_STEM := build-$(_cboot_project)/lk.bin
_cboot_lk_bin := $(_cboot_intermediates)/$(LOCAL_BUILT_MODULE_STEM)

# Generate lk.bin with PRIVATE_CUSTOM_TOOL
# Call make in lk directory
ifeq ($(TARGET_ARCH),arm64)
$(_cboot_lk_bin): PRIVATE_CUSTOM_TOOL_ARGS := \
	TOOLCHAIN_PREFIX=$(abspath $(TARGET_TOOLS_PREFIX))
else
$(_cboot_lk_bin): PRIVATE_CUSTOM_TOOL_ARGS := \
	TOOLCHAIN_PREFIX=$(ARM_EABI_TOOLCHAIN)/../../../aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
endif
$(_cboot_lk_bin): PRIVATE_CUSTOM_TOOL_ARGS += \
		BUILDROOT=$(PWD)/$(_cboot_intermediates) \
		PROJECT=$(_cboot_project) \
		NOECHO=$(hide) \
		DEBUG=2 \
		-C $(LOCAL_PATH)
$(_cboot_lk_bin): PRIVATE_MODULE := $(LOCAL_MODULE)

.PHONY: $(_cboot_lk_bin)
$(_cboot_lk_bin):
	@mkdir -p $(dir $@)
	$(hide) +$(MAKE) $(PRIVATE_CUSTOM_TOOL_ARGS)

include $(NVIDIA_BASE)
include $(BUILD_SYSTEM)/base_rules.mk
include $(NVIDIA_POST)

# Clean variables
_cboot_intermediates :=
_cboot_project :=
_cboot_lk_bin :=
