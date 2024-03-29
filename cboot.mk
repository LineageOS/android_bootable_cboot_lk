# Copyright (c) 2014-2018, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.

# Define module cboot.
# cboot: Compile the cboot kernel and generate cboot.bin

LOCAL_PATH := $(call my-dir)
_cboot_path := $(LOCAL_PATH)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE        := cboot
LOCAL_MODULE_SUFFIX := .bin
LOCAL_MODULE_PATH   := $(PRODUCT_OUT)
LOCAL_MODULE_CLASS  := EXECUTABLES

_cboot_intermediates := $(call intermediates-dir-for,$(LOCAL_MODULE_CLASS),$(LOCAL_MODULE))

_out_bin := $(_cboot_intermediates)/build-$(_cboot_project)/lk.bin
_cboot_lk_bin := $(_cboot_intermediates)/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)

# Generate lk.bin with PRIVATE_CUSTOM_TOOL
# Call make in lk directory
$(_cboot_lk_bin): PRIVATE_CUSTOM_TOOL_ARGS += \
		TOP=$(BUILD_TOP) \
		TOOLCHAIN_PREFIX=$(KERNEL_TOOLCHAIN)/$(KERNEL_TOOLCHAIN_PREFIX) \
		BUILDROOT=$(abspath $(_cboot_intermediates)) \
		PROJECT=$(_cboot_project) \
		NOECHO=$(hide) \
		DEBUG=2 \
		-C $(_cboot_path)

# Pass the PLATFORM_IS_AFTER_N value to cboot build
$(_cboot_lk_bin): PRIVATE_CUSTOM_TOOL_ARGS += \
		PLATFORM_IS_AFTER_N=$(PLATFORM_IS_AFTER_N)

$(_cboot_lk_bin): PRIVATE_MODULE := $(LOCAL_MODULE)

$(_cboot_lk_bin):
	@mkdir -p $(dir $@)
	$(hide) +$(KERNEL_MAKE_CMD) $(PRIVATE_CUSTOM_TOOL_ARGS)
	@cp $(_out_bin) $@

include $(NVIDIA_BASE)
include $(BUILD_SYSTEM)/base_rules.mk
include $(NVIDIA_POST)

include $(NVIDIA_DEFAULTS)

LOCAL_MODULE        := nvdisp-init
LOCAL_MODULE_SUFFIX := .bin
LOCAL_MODULE_PATH   := $(PRODUCT_OUT)
LOCAL_MODULE_CLASS  := EXECUTABLES

_nvdisp-init_intermediates := $(call intermediates-dir-for,$(LOCAL_MODULE_CLASS),$(LOCAL_MODULE))
_nvdisp-init_lk_bin := $(_nvdisp-init_intermediates)/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)

# Generate lk.bin with PRIVATE_CUSTOM_TOOL
# Call make in lk directory
$(_nvdisp-init_lk_bin): PRIVATE_CUSTOM_TOOL_ARGS += \
		TOP=$(BUILD_TOP) \
		TOOLCHAIN_PREFIX=$(KERNEL_TOOLCHAIN)/$(KERNEL_TOOLCHAIN_PREFIX) \
		BUILDROOT=$(abspath $(_nvdisp-init_intermediates)) \
		PROJECT=$(_cboot_project) \
		NOECHO=$(hide) \
		DEBUG=2 \
		NVDISP_INIT_ONLY=true \
		-C $(_cboot_path)

# Pass the PLATFORM_IS_AFTER_N value to nvdisp-init build
$(_nvdisp-init_lk_bin): PRIVATE_CUSTOM_TOOL_ARGS += \
		PLATFORM_IS_AFTER_N=$(PLATFORM_IS_AFTER_N)

$(_nvdisp-init_lk_bin): PRIVATE_MODULE := $(LOCAL_MODULE)

$(_nvdisp-init_lk_bin):
	@mkdir -p $(dir $@)
	$(hide) +$(KERNEL_MAKE_CMD) $(PRIVATE_CUSTOM_TOOL_ARGS)
	@cp $(dir $@)/build-$(_cboot_project)/lk.bin $@

include $(NVIDIA_BASE)
include $(BUILD_SYSTEM)/base_rules.mk
include $(NVIDIA_POST)
