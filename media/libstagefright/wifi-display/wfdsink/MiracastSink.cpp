/*
 * Copyright 2012, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MiracastSink.h"

#include "RTPJitterBuffer.h"
#include "RTPSource.h"
#include "WfdSink.h"

#include <binder/ProcessState.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/foundation/ABuffer.h>

#define LOG_NDEBUG 0
#define LOG_TAG "mirro"
#include <utils/Log.h>

namespace android {

//wfd_read is onging, when wfd_close happen.
static Mutex mLock;

WFD_SINK *wfd_open(const char *url) {

    ProcessState::self()->startThreadPool();
    DataSource::RegisterDefaultSniffers();

    AString connectToHost;
    int32_t connectToPort = -1;
    AString uri;
    status_t err;

    const char *purl = strstr(url, "wfd://");
    if(purl == NULL) {
        ALOGE("invalid url[%s]", url);
        return NULL;
    }

    const char *pstart = strstr(url, "//");
    const char *colonPos = strrchr(pstart, ':');
    if (colonPos == NULL) {
        connectToHost = "172.168.49.1";
        connectToPort = 7236;
        ALOGD("colon pos is null.");
    } else {
        connectToHost.setTo(pstart+2, colonPos - (pstart+2));
        ALOGD("host[%s]; port[%s]", pstart+2, colonPos);
        char *end;
        connectToPort = strtol(colonPos + 1, &end, 10);
        if (connectToPort < 1 || connectToPort > 65535) {
            ALOGD("port [%d]\n", connectToPort);
            return NULL;
        }
    }

    if (connectToPort < 0 && uri.empty()) {
        fprintf(stderr,
                "You need to select either source host or uri.\n");
        return NULL;
    }

    if (connectToPort >= 0 && !uri.empty()) {
        fprintf(stderr,
                "You need to either connect to a wfd host or an rtsp url, "
                "not both.\n");
        return NULL;
    }


    // 1. sink handle
    WFD_SINK *handle = new WFD_SINK;
    ALOGD("netSession start");
    // 2. network session
    handle->mNetSession = new ANetworkSession;
    handle->mNetSession->start();

    // 3. looper
    handle->mLooper = new ALooper;
    handle->mLooper->setName("miracast_sink_looper");
    // 4. sink
    handle->mSink = new WfdSink(handle->mNetSession);
    handle->mLooper->registerHandler(handle->mSink);
    
    ALOGD("looper start");
    handle->mLooper->start( /* true runOnCallingThread */);  

    ALOGD("mSink start");
    // 5. connect succeed or failed, sync or Async, should return value.
    if (connectToPort >= 0) {
        err = handle->mSink->start(connectToHost.c_str(), connectToPort);
    } else {
        err = handle->mSink->start(uri.c_str());
    }
    ALOGD("mSink start end..");
    if(err != (status_t)OK) {
        ALOGD("open failed, stop looper and netsession, delete handle.");
        handle->mLooper->stop();
        handle->mNetSession->stop();
        delete handle;
        return NULL;
    }

    ALOGD("looper return");
    return handle;
}

int wfd_close(WFD_SINK *handle) {
    Mutex::Autolock autoLock(mLock);
    ALOGD("wfd_close");
    if(handle != NULL) {
        ALOGD("mLooper->stop.");
        handle->mLooper->stop();
        ALOGD("mNetSession->stop.");
        handle->mNetSession->stop();
        ALOGD("delete handle.");
        delete handle;
        handle = NULL;
    }
    return 0;
}

int wfd_read(WFD_SINK *handle, unsigned char *pData, unsigned dLen) {
    Mutex::Autolock autoLock(mLock);
    if(handle == NULL || pData == NULL || dLen == 0) {
        ALOGE("input parameter invalid!");
        return -1;
    }

    if(handle->mSink == NULL) {
        ALOGE("mSink invalid,logic err!");
        return -1;
    }

    if(handle->mSink->getRTPSource() != NULL) {
        sp<RTPJitterBuffer> jitbuffer = 
                    handle->mSink->getRTPSource()->getRTPJitterBuffer();
        if(jitbuffer == NULL) {
            ALOGD("jitter buffer invalid.");
            return -1;
        }
        
        if(jitbuffer->getTotalBytesQueued() < WFD_SINK_RTP_READ_LIMITED) {
            return 0;
        }

    	int copySize = 0;
    	int leftSize = dLen;
    	int qLen = 0;
    	unsigned char *ptr = pData;
        //ignore 188*4 = 752; 188*7 = 1316
        //leftSize >= 1316 reason: the max buffer len is 1316.
    	while(jitbuffer->getTotalBytesQueued() >= 752 && leftSize >= 1316) {
            sp<ABuffer> srcBuffer = jitbuffer->dequeueBuffer();
            if (srcBuffer == NULL) {
                break;
            }
            qLen = srcBuffer->size();
            CHECK_LE(qLen, leftSize);// make sure (srcBuffer->size() <= leftSize)
            CHECK_EQ((qLen % 188), 0u);

    		memcpy(ptr, srcBuffer->data(), qLen);
    		copySize += qLen;
    		leftSize -= qLen;
    		ptr += qLen;
    	}
    	return copySize;
    }
    return 0;
}

//////////////////////////////////////////////////////////////
//typedef int (*RTSPStatusCB)(void *pdata, int status);
//static int register_wfd_status_CB(int id, RTSPStatusCB statuscb) {
//    ALOGD("register_wfd_status_CB");
//    return 0;
//}
//////////////////////////////////////////////////////////////

} // namespace android
