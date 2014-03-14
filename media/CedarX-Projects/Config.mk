#this file is used for Android compile configuration

############################################################################
CEDARX_EXT_CFLAGS :=
# manually config CEDARX_ADAPTER_VERSION according to internal version. now default set to v1.5release(V15).
CEDARX_ADAPTER_VERSION := 5
CEDARX_EXT_CFLAGS += -DCEDARX_ADAPTER_VERSION=5


CEDARX_PRODUCTOR := GENERIC
CEDARX_DEBUG_ENABLE := N
ifeq ($(CEDARX_DEBUG_ENABLE), Y)
CEDARX_DEBUG_FRAMEWORK := Y
CEDARX_DEBUG_CEDARV := Y
CEDARX_DEBUG_DEMUXER := Y
else
CEDARX_DEBUG_FRAMEWORK := N
CEDARX_DEBUG_CEDARV := N
CEDARX_DEBUG_DEMUXER := N
endif

############################################################################
AV_BASE_PATH := av
TEMP_COMPILE_DISABLE := true
CEDARX_RTSP_VERSION := 5
CEDARX_USE_SFTDEMUX := Y
CEDARX_TOP := $(TOP)/frameworks/av/media/CedarX-Projects/CedarX

############################################################################

CEDARX_ENABLE_MEMWATCH := N

CEDAR_ENCODER_VERSION := F33

CEDARX_USE_SUNXI_MEM_ALLOCATOR := Y

CEDARX_EXT_CFLAGS +=-D__OS_ANDROID
CEDARX_EXT_CFLAGS +=-D__CDX_ENABLE_SUBTITLE 
CEDARX_EXT_CFLAGS +=-D__CDX_ENABLE_DRM

ifeq ($(CEDARX_ENABLE_MEMWATCH),Y)
CEDARX_EXT_CFLAGS +=-DMEMWATCH -DMEMWATCH_STDIO -D__CDX_MEMWATCH -I${CEDARX_TOP}/libexternal/memwatch-2.71
endif
