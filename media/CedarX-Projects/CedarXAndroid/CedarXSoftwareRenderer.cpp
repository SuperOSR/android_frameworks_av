/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "CedarXSoftwareRenderer"
#include <CDX_Debug.h>

#include <binder/MemoryHeapBase.h>

#include <system/window.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MetaData.h>
#include <ui/GraphicBufferMapper.h>
#include <gui/IGraphicBufferProducer.h>

#include "CedarXSoftwareRenderer.h"
#include "virtual_hwcomposer.h"

extern "C" {
extern unsigned int cedarv_address_phy2vir(void *addr);
}
namespace android {

CedarXSoftwareRenderer::CedarXSoftwareRenderer(
        const sp<ANativeWindow> &nativeWindow, const sp<MetaData> &meta)
    : mYUVMode(None),
      mNativeWindow(nativeWindow) {
    int32_t tmp;
    CHECK(meta->findInt32(kKeyColorFormat, &tmp));
    //mColorFormat = (OMX_COLOR_FORMATTYPE)tmp;

    //CHECK(meta->findInt32(kKeyScreenID, &screenID));
    //CHECK(meta->findInt32(kKeyColorFormat, &halFormat));
    CHECK(meta->findInt32(kKeyWidth, &mWidth));
    CHECK(meta->findInt32(kKeyHeight, &mHeight));

    int32_t rotationDegrees;
    if (!meta->findInt32(kKeyRotation, &rotationDegrees)) {
        rotationDegrees = 0;
    }

    int halFormat;
    size_t bufWidth, bufHeight;
    size_t nGpuBufWidth, nGpuBufHeight;

    halFormat = HAL_PIXEL_FORMAT_YV12;
    bufWidth = mWidth;
    bufHeight = mHeight;
    if(bufWidth != ((mWidth + 15) & ~15))
    {
        LOGW("(f:%s, l:%d) bufWidth[%d]!=display_width[%d]", __FUNCTION__, __LINE__, ((mWidth + 15) & ~15), mWidth);
    }
    if(bufHeight != ((mHeight + 15) & ~15))
    {
        LOGW("(f:%s, l:%d) bufHeight[%d]!=display_height[%d]", __FUNCTION__, __LINE__, ((mHeight + 15) & ~15), mHeight);
    }

    CHECK(mNativeWindow != NULL);

    CHECK_EQ(0,
            native_window_set_usage(
            mNativeWindow.get(),
            GRALLOC_USAGE_SW_READ_NEVER /*| GRALLOC_USAGE_SW_WRITE_OFTEN*/
            | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP));

    CHECK_EQ(0,
            native_window_set_scaling_mode(
            mNativeWindow.get(),
            NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW));

    // Width must be multiple of 32??? 
    nGpuBufWidth = mWidth;
    nGpuBufHeight = mHeight;
  #if (1 == ADAPT_A31_GPU_RENDER)
    //A31 GPU rect width should 8 align.
    if(mWidth%8 != 0)
    {
        nGpuBufWidth = (mWidth + 7) & ~7;
        LOGD("(f:%s, l:%d) A31 gpuBufWidth[%d]mod16=[%d] ,should 8 align, change to [%d]", __FUNCTION__, __LINE__, mWidth, mWidth%8, nGpuBufWidth);
    }
  #endif
    CHECK_EQ(0, native_window_set_buffers_geometry(
                mNativeWindow.get(),
                //(bufWidth + 15) & ~15,
                //(bufHeight+ 15) & ~15,
                //(bufWidth + 15) & ~15,
                //bufHeight,
                //bufWidth,
                //bufHeight,
                nGpuBufWidth,
                nGpuBufHeight,
                halFormat));
    uint32_t transform;
    switch (rotationDegrees) {
        case 0: transform = 0; break;
        case 90: transform = HAL_TRANSFORM_ROT_90; break;
        case 180: transform = HAL_TRANSFORM_ROT_180; break;
        case 270: transform = HAL_TRANSFORM_ROT_270; break;
        default: transform = 0; break;
    }

    if (transform) {
        CHECK_EQ(0, native_window_set_buffers_transform(
                    mNativeWindow.get(), transform));
    }
    Rect crop;
    crop.left = 0;
    crop.top  = 0;
    crop.right = bufWidth;
    crop.bottom = bufHeight;
    mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_SET_CROP, &crop);
}

CedarXSoftwareRenderer::~CedarXSoftwareRenderer() {
}

static int ALIGN(int x, int y) {
    // y must be a power of 2.
    return (x + y - 1) & ~(y - 1);
}


void CedarXSoftwareRenderer::render(const void *pObject, size_t size, void *platformPrivate)
{
    ANativeWindowBuffer *buf;
    Virtuallibhwclayerpara *pVirtuallibhwclayerpara = (Virtuallibhwclayerpara*)pObject;
    int err;

    if ((err = mNativeWindow->dequeueBuffer_DEPRECATED(mNativeWindow.get(), &buf)) != 0) {
        LOGW("Surface::dequeueBuffer returned error %d", err);
        return;
    }
    CHECK_EQ(0, mNativeWindow->lockBuffer_DEPRECATED(mNativeWindow.get(), buf));

    GraphicBufferMapper &mapper = GraphicBufferMapper::get();


    Rect bounds(mWidth, mHeight);

    void *dst;
    CHECK_EQ(0, mapper.lock(buf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst));

	int i;
	int vdecBuf_widthAlign;
	int vdecBuf_heightAlign;
	int vdecBuf_cHeightAlign;
	int vdecBuf_cWidthAlign;
	int dstCStride;
	int extraHeight;    //the interval between heightalign and display_height

	unsigned char* dstPtr;
	unsigned char* srcPtr;
	dstPtr = (unsigned char*)dst;
	srcPtr = (unsigned char*)cedarv_address_phy2vir((void*)pVirtuallibhwclayerpara->top_y);
	vdecBuf_widthAlign = (mWidth + 15) & ~15;
	vdecBuf_heightAlign = (mHeight + 15) & ~15;
	//1. yv12_y copy
	for(i=0; i<mHeight; i++)
	{
		memcpy(dstPtr, srcPtr, vdecBuf_widthAlign);
		dstPtr += buf->stride;
		srcPtr += vdecBuf_widthAlign;
	}
	//skip hw decoder's extra line of yv12_y
	extraHeight = vdecBuf_heightAlign - mHeight;
	if(extraHeight > 0)
	{
		for(i=0; i<extraHeight; i++)
		{
			srcPtr += vdecBuf_widthAlign;
		}
	}
	vdecBuf_cWidthAlign = ((mWidth + 15) & ~15)/2;
	vdecBuf_cHeightAlign = ((mHeight + 15) & ~15)/2;
	for(i=0; i<(mHeight+1)/2; i++)
	{
		memcpy(dstPtr, srcPtr, vdecBuf_cWidthAlign);
		dstPtr += buf->stride/2;
		srcPtr += vdecBuf_cWidthAlign;
	}
	//skip hw decoder's extra line of yv12_v
	extraHeight = vdecBuf_cHeightAlign - (mHeight+1)/2;
	if(extraHeight > 0)
	{
		for(i=0;i<extraHeight;i++)
		{
			srcPtr += vdecBuf_cWidthAlign;
		}
	}
	//3. yv12_u copy
	for(i=0; i<(mHeight+1)/2; i++)
	{
		memcpy(dstPtr, srcPtr, vdecBuf_cWidthAlign);
		dstPtr += buf->stride/2;
		srcPtr += vdecBuf_cWidthAlign;
	}

    CHECK_EQ(0, mapper.unlock(buf->handle));

    if ((err = mNativeWindow->queueBuffer_DEPRECATED(mNativeWindow.get(), buf)) != 0) {
        LOGW("Surface::queueBuffer returned error %d", err);
    }
    buf = NULL;

}

int CedarXSoftwareRenderer::dequeueFrame(ANativeWindowBufferCedarXWrapper *pObject)
{
    ANativeWindowBuffer *buf;
    ANativeWindowBufferCedarXWrapper *pANativeWindowBuffer = (ANativeWindowBufferCedarXWrapper*)pObject;
    int err;

    if ((err = mNativeWindow->dequeueBuffer_DEPRECATED(mNativeWindow.get(), &buf)) != 0) {
        LOGW("Surface::dequeueBuffer returned error %d", err);
        return -1;
    }
    CHECK_EQ(0, mNativeWindow->lockBuffer_DEPRECATED(mNativeWindow.get(), buf));

    GraphicBufferMapper &mapper = GraphicBufferMapper::get();


    Rect bounds(mWidth, mHeight);

    void *dst;
    void *dstPhyAddr;
    int phyaddress;
    CHECK_EQ(0, mapper.lock(buf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst));
    mapper.get_phy_addess(buf->handle, &dstPhyAddr);
    phyaddress = (int)dstPhyAddr;
    pANativeWindowBuffer->width     = buf->width;
    pANativeWindowBuffer->height    = buf->height;
    pANativeWindowBuffer->stride    = buf->stride;
    pANativeWindowBuffer->format    = buf->format;
    pANativeWindowBuffer->usage     = buf->usage;
    pANativeWindowBuffer->dst       = dst;
    pANativeWindowBuffer->dstPhy    = (void*)phyaddress;
    pANativeWindowBuffer->pObjANativeWindowBuffer = (void*)buf;
    
    return 0;
}

int CedarXSoftwareRenderer::enqueueFrame(ANativeWindowBufferCedarXWrapper *pObject)
{
    int err;
    ANativeWindowBuffer *buf = (ANativeWindowBuffer*)pObject->pObjANativeWindowBuffer;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    CHECK_EQ(0, mapper.unlock(buf->handle));

    if ((err = mNativeWindow->queueBuffer_DEPRECATED(mNativeWindow.get(), buf)) != 0) {
        LOGW("Surface::queueBuffer returned error %d", err);
    }
    buf = NULL;
    return 0;
}

}  // namespace android
