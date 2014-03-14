LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#include frameworks/${AV_BASE_PATH}/media/libstagefright/codecs/common/Config.mk
include $(LOCAL_PATH)/../Config.mk

LOCAL_SRC_FILES:=                         \
		CedarXAudioPlayer.cpp			  \
        CedarXMetadataRetriever.cpp		  \
        CedarXMetaData.cpp				  \
        CedarXSoftwareRenderer.cpp		  \
        CedarXRecorder.cpp				  \
		CedarXPlayer.cpp				  \
		CedarXNativeRenderer.cpp


LOCAL_C_INCLUDES:= \
	$(JNI_H_INCLUDE) \
	$(TOP)/frameworks/${AV_BASE_PATH}/include/media/stagefright \
    $(CEDARX_TOP)/include \
    $(CEDARX_TOP)/libutil \
    $(CEDARX_TOP)/include/include_cedarv \
    $(CEDARX_TOP)/include/include_audio \
    ${CEDARX_TOP}/include/include_camera \
    ${CEDARX_TOP}/include/include_omx \
    $(TOP)/frameworks/${AV_BASE_PATH}/media/libstagefright/include \
    $(TOP)/frameworks/${AV_BASE_PATH} \
    $(TOP)/frameworks/${AV_BASE_PATH}/include \
    $(TOP)/external/openssl/include \
    $(TOP)/frameworks/native/include/media/hardware


LOCAL_SHARED_LIBRARIES := \
        libcedarxosal	  \
        libbinder         \
        libmedia          \
        libutils          \
        libcutils         \
        libui             \
        libgui			  \
        libcamera_client \
        libstagefright_foundation \
        libicuuc \
		libskia 

ifeq ($(CEDARX_DEBUG_FRAMEWORK),Y)
LOCAL_STATIC_LIBRARIES += \
	libcedarxplayer \
	libcedarxcomponents \
	libcedarxdemuxers \
	libcedarxsftdemux \
	libcedarxrender \
	libsub \
	libsub_inline \
	libmuxers \
	libmp4_muxer \
	libawts_muxer \
	libraw_muxer \
	libmpeg2ts_muxer \
	libjpgenc	\
	libuserdemux \
	libcedarxwvmdemux \
	libcedarxdrmsniffer

LOCAL_STATIC_LIBRARIES += \
	libcedarxstream \

LOCAL_STATIC_LIBRARIES += \
	libcedarx_rtsp
	
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/libcedarxwvmdemux.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libsub.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libsub_inline.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libiconv.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libmuxers.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libmp4_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libawts_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libraw_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libmpeg2ts_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libjpgenc.a	\
	$(CEDARX_TOP)/../CedarAndroidLib/libuserdemux.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libcedarxdrmsniffer.a \

LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/libcedarx_rtsp.a
endif

#audio postprocess lib
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/libPostProcessOBJ.a

ifeq ($(CEDARX_DEBUG_DEMUXER),Y)
LOCAL_STATIC_LIBRARIES += \
	libdemux_asf \
	libdemux_avi \
	libdemux_flv \
	libdemux_mkv \
	libdemux_ogg \
	libdemux_mov \
	libdemux_mpg \
	libdemux_ts \
	libdemux_pmp \
	libdemux_idxsub \
	libdemux_awts
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/libdemux_asf.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libdemux_avi.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libdemux_mkv.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libdemux_ogg.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libdemux_mov.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libdemux_mpg.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libdemux_ts.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libdemux_pmp.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libdemux_idxsub.a \
	$(CEDARX_TOP)/../CedarAndroidLib/libdemux_awts.a
endif
	
ifeq ($(CEDARX_DEBUG_FRAMEWORK),Y)
LOCAL_SHARED_LIBRARIES += libcedarxbase libswdrm libthirdpartstream
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/libcedarxbase.so \
	$(CEDARX_TOP)/../CedarAndroidLib/libcedarxosal.so \
	$(CEDARX_TOP)/../CedarAndroidLib/libswdrm.so \
	$(CEDARX_TOP)/../CedarAndroidLib/libthirdpartstream.so
endif

ifeq ($(CEDARX_DEBUG_CEDARV),Y)
LOCAL_SHARED_LIBRARIES += libcedarv libcedarxosal libve libcedarv_base libcedarv_adapter libaw_h264enc
else
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/libcedarv.so \
	$(CEDARX_TOP)/../CedarAndroidLib/libcedarxosal.so \
	$(CEDARX_TOP)/../CedarAndroidLib/libve.so \
	$(CEDARX_TOP)/../CedarAndroidLib/libcedarv_base.so \
	$(CEDARX_TOP)/../CedarAndroidLib/libcedarv_adapter.so \
	$(CEDARX_TOP)/../CedarAndroidLib/libaw_h264enc.so
endif

LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/libaacenc.a


ifeq ($(CEDARX_ENABLE_MEMWATCH),Y)
LOCAL_STATIC_LIBRARIES += libmemwatch
endif

LOCAL_SHARED_LIBRARIES += libstagefright_foundation libstagefright libdrmframework

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

LOCAL_CFLAGS += -D__ANDROID_VERSION_2_3_4
LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)
LOCAL_MODULE_TAGS := optional

ifeq ($(TARGET_BOARD_PLATFORM), fiber)
    LOCAL_CFLAGS += -DTARGET_BOARD_FIBER
endif

LOCAL_MODULE:= libCedarX

include $(BUILD_SHARED_LIBRARY)

#include $(call all-makefiles-under,$(LOCAL_PATH))
