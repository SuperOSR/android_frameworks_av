LOCAL_PATH:= $(call my-dir)

ifeq ($(TARGET_BOARD_PLATFORM), fiber)
###########################################

include $(CLEAR_VARS)
LOCAL_MODULE := librotation

LOCAL_MODULE_TAGS := optional 

LOCAL_SRC_FILES := rotation.cpp 

LOCAL_SHARED_LIBRARIES := libutils libbinder

include $(BUILD_STATIC_LIBRARY)
###########################################
endif

#
# libmediaplayerservice
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    ActivityManager.cpp         \
    Crypto.cpp                  \
    Drm.cpp                     \
    HDCP.cpp                    \
    MediaPlayerFactory.cpp      \
    MediaPlayerService.cpp      \
    MediaRecorderClient.cpp     \
    MetadataRetrieverClient.cpp \
    MidiFile.cpp                \
    MidiMetadataRetriever.cpp   \
    RemoteDisplay.cpp           \
    SharedLibrary.cpp           \
    StagefrightPlayer.cpp       \
    StagefrightRecorder.cpp     \
    TestPlayerStub.cpp          \
    
ifeq ($(TARGET_BOARD_PLATFORM), fiber)
LOCAL_SRC_FILES +=              \
    CedarPlayer.cpp       		\
    CedarAPlayerWrapper.cpp		\
    SimpleMediaFormatProbe.cpp	\
    MovAvInfoDetect.cpp         \
    ThumbnailPlayer/tplayer.cpp \
    ThumbnailPlayer/avtimer.cpp
endif

LOCAL_SHARED_LIBRARIES :=       \
    libbinder                   \
    libcamera_client            \
    libcutils                   \
    liblog                      \
    libdl                       \
    libgui                      \
    libmedia                    \
    libsonivox                  \
    libstagefright              \
    libstagefright_foundation   \
    libstagefright_httplive     \
    libstagefright_omx          \
    libstagefright_wfd          \
    libutils                    \
    libvorbisidec               \

ifeq ($(TARGET_BOARD_PLATFORM), fiber)
LOCAL_SHARED_LIBRARIES +=       \
    libCedarX           	    \
    libCedarA           	    \
    libcedarxbase               \
    libcedarxosal               \
    libcedarv                   \
    libui
endif

LOCAL_STATIC_LIBRARIES :=       \
    libstagefright_nuplayer     \
    libstagefright_rtsp         \

ifeq ($(TARGET_BOARD_PLATFORM), fiber)
LOCAL_STATIC_LIBRARIES +=       \
    librotation
endif

LOCAL_C_INCLUDES :=                                                 \
    $(call include-path-for, graphics corecg)                       \
    $(TOP)/frameworks/av/media/libstagefright/include               \
    $(TOP)/frameworks/av/media/libstagefright/rtsp                  \
    $(TOP)/frameworks/av/media/libstagefright/wifi-display          \
    $(TOP)/frameworks/native/include/media/openmax                  \
    $(TOP)/external/tremolo/Tremolo                                 \

ifeq ($(TARGET_BOARD_PLATFORM), fiber)
LOCAL_C_INCLUDES +=             \
    $(TOP)/frameworks/av/media/CedarX-Projects/CedarXAndroid/IceCreamSandwich \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include/include_audio \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include/include_cedarv \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarX/include \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarA \
	$(TOP)/frameworks/av/media/CedarX-Projects/CedarA/include \
endif

ifeq ($(TARGET_BOARD_PLATFORM), fiber)
LOCAL_CFLAGS +=-DCEDARX_ANDROID_VERSION=9 -DTARGET_BOARD_FIBER
endif

LOCAL_MODULE:= libmediaplayerservice

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
