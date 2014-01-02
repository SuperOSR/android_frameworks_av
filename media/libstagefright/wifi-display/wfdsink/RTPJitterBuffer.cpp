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

#define LOG_NDEBUG 0
#define LOG_TAG "JitterBuffer"
#include <utils/Log.h>

#include "RTPJitterBuffer.h"

#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>


namespace android {

RTPJitterBuffer::RTPJitterBuffer(
        const sp<AMessage> &notifyLost)
    : mNotifyLost(notifyLost),
      mTotalBytesQueued(0ll),
      mLastDequeuedExtSeqNo(-1),
      mFirstFailedAttemptUs(-1ll),
      mRequestedRetransmission(false) {
    ALOGD("RTPJitterBuffer()");
}

RTPJitterBuffer::~RTPJitterBuffer() {
    ALOGD("~RTPJitterBuffer()");
    stop();
}

void RTPJitterBuffer::queueBuffer(const sp<ABuffer> &buffer) {
    Mutex::Autolock autoLock(mLock);

    int32_t newExtendedSeqNo = buffer->int32Data();

    // Common case: There are no packets in the queue; this will be the first one:
    if (mPackets.empty()) {
        mPackets.push_back(buffer);
        mTotalBytesQueued += buffer->size();
        //ALOGD("++SeqNo(%d)Byte1(%lld)", newExtendedSeqNo, mTotalBytesQueued);
        return;
    }

    // Ignore this packet if its sequence number is less than the one
    // that we're looking for (in this case, it's been excessively delayed).
    if (newExtendedSeqNo < mLastDequeuedExtSeqNo) {
        // No need to let dequeueBuffer do the choices.
        ALOGD("discard SeqNo(%d)lastNo(%d)", newExtendedSeqNo, mLastDequeuedExtSeqNo);
        return;
    }

    List<sp<ABuffer> >::iterator firstIt = mPackets.begin();
    List<sp<ABuffer> >::iterator it = --mPackets.end();
    for (;;) {
        int32_t extendedSeqNo = (*it)->int32Data();

        if (extendedSeqNo < newExtendedSeqNo) {
            // The next-most common case: There are packets already in the queue; this packet arrived in order => put it at the tail:
            // Insert new packet after the one at "it".
            mPackets.insert(++it, buffer);
            mTotalBytesQueued += buffer->size();
            //ALOGD("++SeqNo(%d)Byte2(%lld)", newExtendedSeqNo, mTotalBytesQueued);
            return;
        }

        if (extendedSeqNo == newExtendedSeqNo) {
            // Duplicate packet.
            //ALOGD("Duplicate packet(%d)!!!", newExtendedSeqNo);
            return;
        }

        if (it == firstIt) {
            // Insert new packet before the first existing one.
            mPackets.insert(it, buffer);
            mTotalBytesQueued += buffer->size();
            //ALOGD("++SeqNo(%d)Byte3(%lld)", newExtendedSeqNo, mTotalBytesQueued);
            ALOGD("it == firstIt");
            return;
        }

        --it;
        ALOGD("well done!!!");
    }
}

sp<ABuffer> RTPJitterBuffer::dequeueBuffer() {
    Mutex::Autolock autoLock(mLock);

    sp<ABuffer> buffer;
    int32_t extSeqNo;

    while (!mPackets.empty()) {
        buffer = *mPackets.begin();
        extSeqNo = buffer->int32Data();
        //ALOGD("fetch SeqNo(%d)lastNo(%d)", extSeqNo, mLastDequeuedExtSeqNo);
        if (mLastDequeuedExtSeqNo < 0 || extSeqNo > mLastDequeuedExtSeqNo) {
            break;
        }

        // This is a retransmission of a packet we've already returned.

        mTotalBytesQueued -= buffer->size();
        ALOGD("drop SeqNo(%d)", buffer->int32Data());
        buffer.clear();
        extSeqNo = -1;

        mPackets.erase(mPackets.begin());
    }

    if (mPackets.empty()) {
        if (mFirstFailedAttemptUs < 0ll) {
            mFirstFailedAttemptUs = ALooper::GetNowUs();
            mRequestedRetransmission = false;
        } else {
            if(((ALooper::GetNowUs() - mFirstFailedAttemptUs) / 1E6) > 2)
                ALOGV("no packets %.2f secs;mTotalBytesQueued(%lld)",
                        (ALooper::GetNowUs() - mFirstFailedAttemptUs) / 1E6, mTotalBytesQueued);
        }
        return NULL;
    }

    if (mLastDequeuedExtSeqNo < 0 || extSeqNo == mLastDequeuedExtSeqNo + 1) {
        if (mRequestedRetransmission) {
            ALOGI("Recovered after requesting retransmission of %d",
                  extSeqNo);
        }

        mLastDequeuedExtSeqNo = extSeqNo;
        mFirstFailedAttemptUs = -1ll;
        mRequestedRetransmission = false;

        mPackets.erase(mPackets.begin());

        mTotalBytesQueued -= buffer->size();
        //ALOGD("--SeqNo(%d)Bytes(%lld)", extSeqNo, mTotalBytesQueued);
        return buffer;
    }

    if (mFirstFailedAttemptUs < 0ll) {
        mFirstFailedAttemptUs = ALooper::GetNowUs();
        ALOGI("packet available but seq error,nowNo(%d)expectNo(%d).", extSeqNo, mLastDequeuedExtSeqNo+1 );
        return NULL;
    }

    // We're willing to wait a little while to get the right packet.
    // 100ms, adjust according to 802.11n block ack time
    if (mFirstFailedAttemptUs + 100000ll > ALooper::GetNowUs()) {
        if (!mRequestedRetransmission) {
            ALOGI("Waiting for seqNo(%d) to arrive.", (mLastDequeuedExtSeqNo + 1) & 0xffff);
            /*
            //Don't send request retranmission, it is no sense for many source.
            //Just do nothing
            sp<AMessage> notify = mNotifyLost->dup();
            notify->setInt32("seqNo", (mLastDequeuedExtSeqNo + 1) & 0xffff);
            notify->post();
            */
            mRequestedRetransmission = true;
        } else {
            ALOGI("Still waiting for SeqNo(%d) to arrive.", (mLastDequeuedExtSeqNo + 1) & 0xffff);
        }

        return NULL;
    }

    ALOGI("dropping packet. extSeqNo %d didn't arrive in time",
            mLastDequeuedExtSeqNo + 1);

    // Permanent failure, we never received the packet.
    mLastDequeuedExtSeqNo = extSeqNo;
    mFirstFailedAttemptUs = -1ll;
    mRequestedRetransmission = false;

    mTotalBytesQueued -= buffer->size();

    mPackets.erase(mPackets.begin());

    ALOGD("--SeqNo(%d)Bytes(%lld) reset!!!", buffer->int32Data(), mTotalBytesQueued);
    return buffer;
}

int64_t RTPJitterBuffer::getTotalBytesQueued() {
    //Mutex::Autolock autoLock(mLock);
        return mTotalBytesQueued;
}

//Received packets from RTPSink
void RTPJitterBuffer::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatQueueBuffer:
        {
            sp<ABuffer> buffer;
            CHECK(msg->findBuffer("buffer", &buffer));
            if(mTotalBytesQueued > RTP_JITTER_BUFFER_SIZE_LIMIT) {
                ALOGI("RTP_JITTER_BUFFER_SIZE_LIMIT warnning.");
                break;
            }
            queueBuffer(buffer);

            if (mTotalBytesQueued > 0ll) {
                //do nothing
            } else {
                ALOGI("Have %lld bytes queued...", mTotalBytesQueued);
            }
            //must trigger somebody read the buffer.
            break;
        }

        default:
            TRESPASS();
    }
}

void RTPJitterBuffer::start() {
    //do nothing now
}

void RTPJitterBuffer::stop() {
    //looper()->stop();

    Mutex::Autolock autoLock(mLock);

    sp<ABuffer> buffer;
    //free mPackets
    while (!mPackets.empty()) {
        buffer = *mPackets.begin();
        buffer.clear();
        mPackets.erase(mPackets.begin());
    }
}

}  // namespace android

