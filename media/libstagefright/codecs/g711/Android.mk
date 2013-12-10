LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_BOARD_PLATFORM), fiber)
    LOCAL_CFLAGS += -DTARGET_BOARD_FIBER
endif

include $(call all-makefiles-under,$(LOCAL_PATH))
