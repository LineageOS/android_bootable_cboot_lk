LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/regulator.c \
	$(LOCAL_DIR)/fixed-regulator.c

include make/module.mk
