LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../Config.mk

include $(LOCAL_PATH)/kitkat/Android.mk

$(info CEDARX_PRODUCTOR: $(CEDARX_PRODUCTOR))
