LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include \
	$(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/cpc.c

include make/module.mk
