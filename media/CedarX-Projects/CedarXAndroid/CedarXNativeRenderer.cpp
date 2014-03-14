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

#define LOG_TAG "CedarXNativeRenderer"
#include <CDX_Debug.h>

#include "CedarXNativeRenderer.h"
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MetaData.h>

#include <binder/MemoryHeapBase.h>

#include <ui/GraphicBufferMapper.h>
#include <gui/IGraphicBufferProducer.h>

namespace android {

CedarXNativeRenderer::CedarXNativeRenderer(const sp<ANativeWindow> &nativeWindow, const sp<MetaData> &meta)
    : mNativeWindow(nativeWindow)
{

    int32_t halFormat,screenID;
    size_t bufWidth, bufHeight;

    CHECK(meta->findInt32(kKeyScreenID, &screenID));
    CHECK(meta->findInt32(kKeyColorFormat, &halFormat));
    CHECK(meta->findInt32(kKeyWidth, &mWidth)); //width and height is display_width and display_height.
    CHECK(meta->findInt32(kKeyHeight, &mHeight));

    int32_t rotationDegrees;
    if (!meta->findInt32(kKeyRotation, &rotationDegrees))
        rotationDegrees = 0;
    
    bufWidth = mWidth;  //the vdec_buffer's width and height which will be told to display device .
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

    if(halFormat == HAL_PIXEL_FORMAT_YV12)
    {
        native_window_set_usage(mNativeWindow.get(),
        		                GRALLOC_USAGE_SW_READ_NEVER  |
        		                GRALLOC_USAGE_SW_WRITE_OFTEN |
        		                GRALLOC_USAGE_HW_TEXTURE     |
        		                GRALLOC_USAGE_EXTERNAL_DISP);

        // Width must be multiple of 32???
        native_window_set_buffers_geometry(mNativeWindow.get(),
        								   bufWidth,
        								   bufHeight,
                                           0x58);   //HWC_FORMAT_YUV420PLANAR

        uint32_t transform;
        switch (rotationDegrees)
        {
            case 0: transform = 0; break;
            case 90: transform = HAL_TRANSFORM_ROT_90; break;
            case 180: transform = HAL_TRANSFORM_ROT_180; break;
            case 270: transform = HAL_TRANSFORM_ROT_270; break;
            default: transform = 0; break;
        }

        if (transform)
            native_window_set_buffers_transform(mNativeWindow.get(), transform);
    }
    else
    {
        native_window_set_usage(mNativeWindow.get(),
        		                GRALLOC_USAGE_SW_READ_NEVER  |
        		                GRALLOC_USAGE_SW_WRITE_OFTEN |
        		                GRALLOC_USAGE_HW_TEXTURE     |
        		                GRALLOC_USAGE_EXTERNAL_DISP);
        // Width must be multiple of 32???
        native_window_set_buffers_geometry(mNativeWindow.get(),
            		                       bufWidth,
                                           bufHeight,
                                           halFormat);

        uint32_t transform;
        switch (rotationDegrees)
        {
            case 0: transform = 0; break;
            case 90: transform = HAL_TRANSFORM_ROT_90; break;
            case 180: transform = HAL_TRANSFORM_ROT_180; break;
            case 270: transform = HAL_TRANSFORM_ROT_270; break;
            default: transform = 0; break;
        }

        if (transform)
            native_window_set_buffers_transform(mNativeWindow.get(), transform);
    }

    mLayerShowed = 0;

}

CedarXNativeRenderer::~CedarXNativeRenderer()
{
}

void CedarXNativeRenderer::render(const void *data, size_t size, void *platformPrivate)
{
}

int CedarXNativeRenderer::control(int cmd, int para)
{
    return 0;
}

}  // namespace android

