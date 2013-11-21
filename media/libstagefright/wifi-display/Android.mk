LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        ANetworkSession.cpp             \
        Parameters.cpp                  \
        ParsedMessage.cpp               \
        sink/LinearRegression.cpp       \
        sink/RTPSink.cpp                \
        sink/TunnelRenderer.cpp         \
        sink/WifiDisplaySink.cpp        \
        source/Converter.cpp            \
        source/MediaPuller.cpp          \
        source/PlaybackSession.cpp      \
        source/RepeaterSource.cpp       \
        source/Sender.cpp               \
        source/TSPacketizer.cpp         \
        source/WifiDisplaySource.cpp    \
        TimeSeries.cpp                  \
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

LOCAL_MODULE:= wfd

LOCAL_MODULE_TAGS := debug

include $(BUILD_EXECUTABLE)

################################################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        udptest.cpp                 \

LOCAL_SHARED_LIBRARIES:= \
        libbinder                       \
        libgui                          \
        libmedia                        \
        libstagefright                  \
        libstagefright_foundation       \
        libstagefright_wfd              \
        libutils                        \

LOCAL_MODULE:= udptest

LOCAL_MODULE_TAGS := debug

include $(BUILD_EXECUTABLE)
