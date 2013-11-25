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

#ifndef RTP_JITTER_BUFFER_H_

#define RTP_JITTER_BUFFER_H_

#include <media/stagefright/foundation/AHandler.h>

namespace android {

struct ABuffer;

// This class reassembles incoming RTP packets into the correct order
// and sends the resulting transport stream to a mediaplayer instance
// for playback.
struct RTPJitterBuffer : public AHandler {
    RTPJitterBuffer(
            const sp<AMessage> &notifyLost);

    sp<ABuffer> dequeueBuffer(); //read
    int64_t getTotalBytesQueued();

    enum {
        kWhatQueueBuffer,
    };

    #define RTP_JITTER_BUFFER_SIZE_LIMIT (1024*1024*96) //96Mbytes

protected:
    virtual void onMessageReceived(const sp<AMessage> &msg);
    virtual ~RTPJitterBuffer();

private:
    mutable Mutex mLock;

    sp<AMessage> mNotifyLost;

    List<sp<ABuffer> > mPackets;
    int64_t mTotalBytesQueued;

    //jitter buffer, disorder, timeout 100ms
    int32_t mLastDequeuedExtSeqNo;
    int64_t mFirstFailedAttemptUs;
    bool mRequestedRetransmission;


    void start();//initial
    void stop();//looper,mPackets.

    void queueBuffer(const sp<ABuffer> &buffer);//inject

    DISALLOW_EVIL_CONSTRUCTORS(RTPJitterBuffer);
};

}  // namespace android

#endif  // RTP_JITTER_BUFFER_H_
