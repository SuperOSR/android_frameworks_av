LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        ANetworkSession.cpp             \
        MediaSender.cpp                 \
        Parameters.cpp                  \
        ParsedMessage.cpp               \
        rtp/RTPSender.cpp               \
        source/Converter.cpp            \
        source/MediaPuller.cpp          \
        source/PlaybackSession.cpp      \
        source/RepeaterSource.cpp       \
        source/TSPacketizer.cpp         \
        source/WifiDisplaySource.cpp    \
        VideoFormats.cpp                \
        wfdsink/XLinearRegression.cpp   \
        wfdsink/RTPSource.cpp           \
        wfdsink/RTPJitterBuffer.cpp     \
        wfdsink/WfdSink.cpp             \
        wfdsink/MiracastSink.cpp        \

LOCAL_C_INCLUDES:= \
        $(TOP)/frameworks/av/media/libstagefright \
        $(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/frameworks/av/media/libstagefright/mpeg2ts \

LOCAL_SHARED_LIBRARIES:= \
        libbinder                       \
        libcutils                       \
        liblog                          \
        libgui                          \
        libmedia                        \
        libstagefright                  \
        libstagefright_foundation       \
        libui                           \
        libutils                        \

ifeq ($(BOARD_WLAN_DEVICE), bcmdhd)
LOCAL_CFLAGS += -DBOARD_BRCM_WLAN
endif

ifeq ($(SW_BOARD_WFD_DENSITY), sw1080p)
LOCAL_CFLAGS += -DUSE_1080P
endif

ifeq ($(SW_BOARD_WFD_DENSITY), sw720p)
LOCAL_CFLAGS += -DUSE_720P
endif

ifeq ($(SW_BOARD_WFD_DENSITY), sw480p)
LOCAL_CFLAGS += -DUSE_480P
endif

LOCAL_MODULE:= libstagefright_wfd

LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)

################################################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        wfd.cpp                 \

LOCAL_SHARED_LIBRARIES:= \
        libbinder                       \
        libgui                          \
        libmedia                        \
        libstagefright                  \
        libstagefright_foundation       \
        libstagefright_wfd              \
        libutils                        \
        liblog                          \

LOCAL_MODULE:= wfd

LOCAL_MODULE_TAGS := debug

include $(BUILD_EXECUTABLE)
