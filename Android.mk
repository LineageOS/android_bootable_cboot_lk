LOCAL_PATH := $(call my-dir)

NVIDIA_DEFAULTS := $(CLEAR_VARS)

_cboot_project := $(TARGET_TEGRA_VERSION)

# Hardcode this since not using the Nvidia build system
PLATFORM_IS_AFTER_N := 1

include $(LOCAL_PATH)/cboot.mk
