LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	platform/tegra_shared/regulator

MODULE_SRCS += \
	$(LOCAL_DIR)/max77620.c

include make/module.mk
