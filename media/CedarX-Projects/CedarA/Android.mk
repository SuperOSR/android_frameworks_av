LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../Config.mk

LOCAL_SRC_FILES:=                         \
		CedarARender.cpp \
        CedarAPlayer.cpp				  


LOCAL_C_INCLUDES:= \
	$(JNI_H_INCLUDE) \
	$(LOCAL_PATH)/include \
	${CEDARX_TOP}/ \
	${CEDARX_TOP}/libcodecs/include \
	$(TOP)/frameworks/${AV_BASE_PATH}/include/media/stagefright \
    $(TOP)/frameworks/${AV_BASE_PATH}/include/media/stagefright/openmax

LOCAL_SHARED_LIBRARIES := \
        libbinder         \
        libmedia          \
        libutils          \
        libcutils         \
        libui             \
        libCedarX

LOCAL_LDFLAGS += \
	$(LOCAL_PATH)/../CedarAndroidLib/libGetAudio_format.a \
	$(LOCAL_PATH)/../CedarAndroidLib/libaacenc.a

ifeq ($(CEDARX_DEBUG_ENABLE),Y)
LOCAL_STATIC_LIBRARIES += \
	libcedara_decoder
endif

ifeq ($(CEDARX_ENABLE_MEMWATCH),Y)
LOCAL_STATIC_LIBRARIES += libmemwatch
endif

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
        LOCAL_LDLIBS += -lpthread -ldl
        LOCAL_SHARED_LIBRARIES += libdvm
        LOCAL_CPPFLAGS += -DANDROID_SIMULATOR
endif

ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
        LOCAL_LDLIBS += -lpthread
endif

LOCAL_CFLAGS += -Wno-multichar 

LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libCedarA

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))

ifneq ($(CEDARX_DEBUG_ENABLE),N)

include $(CLEAR_VARS)


include $(LOCAL_PATH)/../../Config.mk
LOCAL_CFLAGS += -D__ENABLE_AC3DTSRAW

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	CedarADecoder.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	${CEDARX_TOP}/libutil \
	${CEDARX_TOP}/libcodecs/include \
	${CEDARX_TOP}/include \
	${CEDARX_TOP}/include/include_base

#LOCAL_SHARED_LIBRARIES := \
        libCedarX

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)   -D__ENABLE_AC3DTS

ifeq ($(CEDARX_DEBUG_FRAMEWORK),Y)
LOCAL_SHARED_LIBRARIES += libcedarxbase 
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/libcedarxbase.so
endif

ifeq ($(CEDARX_DEBUG_CEDARV),Y)
LOCAL_SHARED_LIBRARIES += libcedarxosal 
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/libcedarxosal.so
endif

LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/libac3.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libdts.a 
	
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/libac3_raw.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libdts_raw.a

LOCAL_MODULE:= libswa2

include $(BUILD_SHARED_LIBRARY)
#######################################################################################

include $(CLEAR_VARS)


include $(LOCAL_PATH)/../../Config.mk

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	CedarADecoder.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	${CEDARX_TOP}/libutil \
	${CEDARX_TOP}/libcodecs/include \
	${CEDARX_TOP}/include \
	${CEDARX_TOP}/include/include_base

#LOCAL_SHARED_LIBRARIES := \
        libCedarX

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)   -D__ENABLE_OTHER

ifeq ($(CEDARX_DEBUG_FRAMEWORK),Y)
LOCAL_SHARED_LIBRARIES += libcedarxbase 
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/libcedarxbase.so
endif

ifeq ($(CEDARX_DEBUG_CEDARV),Y)
LOCAL_SHARED_LIBRARIES += libcedarxosal 
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/libcedarxosal.so
endif

LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/libwma.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libaac.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libmp3.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libatrc.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libcook.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libsipr.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libamr.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libape.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libogg.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libflac.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libwav.a
	

LOCAL_MODULE:= libaw_audioa

include $(BUILD_SHARED_LIBRARY)

endif
