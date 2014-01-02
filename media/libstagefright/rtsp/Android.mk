LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=       \
        AAMRAssembler.cpp           \
        AAVCAssembler.cpp           \
        AH263Assembler.cpp          \
        AMPEG2TSAssembler.cpp       \
        AMPEG4AudioAssembler.cpp    \
        AMPEG4ElementaryAssembler.cpp \
        APacketSource.cpp           \
        ARawAudioAssembler.cpp      \
        ARTPAssembler.cpp           \
        ARTPConnection.cpp          \
        ARTPSource.cpp              \
        ARTPWriter.cpp              \
        ARTSPConnection.cpp         \
        ASessionDescription.cpp     \
        SDPLoader.cpp               \

LOCAL_C_INCLUDES:= \
	$(TOP)/frameworks/av/media/libstagefright/include \
	$(TOP)/frameworks/native/include/media/openmax \
	$(TOP)/external/openssl/include

LOCAL_MODULE:= libstagefright_rtsp

ifeq ($(TARGET_BOARD_PLATFORM), fiber)
    LOCAL_CFLAGS += -DTARGET_BOARD_FIBER
endif

ifeq ($(TARGET_ARCH),arm)
    LOCAL_CFLAGS += -Wno-psabi
endif

include $(BUILD_STATIC_LIBRARY)
