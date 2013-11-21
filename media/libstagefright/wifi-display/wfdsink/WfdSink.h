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

#ifndef WFD_SINK_H_

#define WFD_SINK_H_

#include "ANetworkSession.h"

#include <gui/Surface.h>
#include <media/stagefright/foundation/AHandler.h>

namespace android {

struct ParsedMessage;
struct RTPSource;

// Represents the RTSP client acting as a wifi display sink.
// Connects to a wifi display source and renders the incoming
// transport stream using a MediaPlayer instance.
struct WfdSink : public AHandler {
    WfdSink(
            const sp<ANetworkSession> &netSession);

    status_t start(const char *sourceHost, int32_t sourcePort);
    status_t start(const char *uri);

    //dequeue
    sp<RTPSource> getRTPSource();

protected:
    virtual ~WfdSink();
    virtual void onMessageReceived(const sp<AMessage> &msg);

private:
    enum State {
        UNDEFINED,
        CONNECTING,
        CONNECTED,
        PAUSED,
        PLAYING,
    };

    enum {
        kWhatStart,
        kWhatRTSPNotify,
        kWhatStop,
    };

    struct ResponseID {
        int32_t mSessionID;
        int32_t mCSeq;

        bool operator<(const ResponseID &other) const {
            return mSessionID < other.mSessionID
                || (mSessionID == other.mSessionID
                        && mCSeq < other.mCSeq);
        }
    };

    typedef status_t (WfdSink::*HandleRTSPResponseFunc)(
            int32_t sessionID, const sp<ParsedMessage> &msg);

    static const bool sUseTCPInterleaving = false;

    State mState;
    sp<ANetworkSession> mNetSession;
    AString mSetupURI;
    AString mRTSPHost;
    int32_t mRTSPPort;
    int32_t mRTPPort;
    int32_t mSessionID;

    int32_t mNextCSeq;

    uint32_t mReplyID;
    uint32_t mHeartBeat;

    KeyedVector<ResponseID, HandleRTSPResponseFunc> mResponseHandlers;

    sp<RTPSource> mRTPSource;
    AString mPlaybackSessionID;
    int32_t mPlaybackSessionTimeoutSecs;

    status_t sendM2(int32_t sessionID);
    status_t sendDescribe(int32_t sessionID, const char *uri);
    status_t sendSetup(int32_t sessionID, const char *uri);
    status_t sendPlay(int32_t sessionID, const char *uri);

    status_t onReceiveM2Response(
            int32_t sessionID, const sp<ParsedMessage> &msg);

    status_t onReceiveDescribeResponse(
            int32_t sessionID, const sp<ParsedMessage> &msg);

    status_t onReceiveSetupResponse(
            int32_t sessionID, const sp<ParsedMessage> &msg);

    status_t configureTransport(const sp<ParsedMessage> &msg);

    status_t onReceivePlayResponse(
            int32_t sessionID, const sp<ParsedMessage> &msg);

    void registerResponseHandler(
            int32_t sessionID, int32_t cseq, HandleRTSPResponseFunc func);

    void onReceiveClientData(const sp<AMessage> &msg);

    void onOptionsRequest(
            int32_t sessionID,
            int32_t cseq,
            const sp<ParsedMessage> &data);

    void onGetParameterRequest(
            int32_t sessionID,
            int32_t cseq,
            const sp<ParsedMessage> &data);

    void onSetParameterRequest(
            int32_t sessionID,
            int32_t cseq,
            const sp<ParsedMessage> &data);

    void sendErrorResponse(
            int32_t sessionID,
            const char *errorDetail,
            int32_t cseq);

    static void AppendCommonResponse(AString *response, int32_t cseq);

    bool ParseURL(
            const char *url, AString *host, int32_t *port, AString *path,
            AString *user, AString *pass);

    DISALLOW_EVIL_CONSTRUCTORS(WfdSink);
};

}  // namespace android

#endif  // WFD_SINK_H_
