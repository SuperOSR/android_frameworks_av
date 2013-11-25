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
#define LOG_TAG "WfdSink"
#include <utils/Log.h>

#include "WfdSink.h"
#include "ParsedMessage.h"
#include "RTPSource.h"

#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaErrors.h>


namespace android {

WfdSink::WfdSink(
        const sp<ANetworkSession> &netSession)
    : mState(UNDEFINED),
      mNetSession(netSession),
      mSessionID(0),
      mNextCSeq(1) {
    ALOGD("WfdSink()");
}

WfdSink::~WfdSink() {
    ALOGD("~WfdSink()");
}

static status_t PostAndAwaitResponse(
        const sp<AMessage> &msg, sp<AMessage> *response) {
    status_t err = msg->postAndAwaitResponse(response);

    if (err != OK) {
        ALOGD("1. return error code = %d", err);
        return err;
    }

    if (response == NULL || !(*response)->findInt32("err", &err)) {
        err = OK;
    }
    ALOGD("2. return error code = %d", err);
    return err;
}

status_t WfdSink::start(const char *sourceHost, int32_t sourcePort) {
    sp<AMessage> msg = new AMessage(kWhatStart, id());
    msg->setString("sourceHost", sourceHost);
    msg->setInt32("sourcePort", sourcePort);

    sp<AMessage> response;
    return PostAndAwaitResponse(msg, &response);
}

status_t WfdSink::start(const char *uri) {
    sp<AMessage> msg = new AMessage(kWhatStart, id());
    msg->setString("setupURI", uri);

    sp<AMessage> response;
    return PostAndAwaitResponse(msg, &response);
}

/*
void WfdSink::start(const char *sourceHost, int32_t sourcePort) {
    sp<AMessage> msg = new AMessage(kWhatStart, id());
    msg->setString("sourceHost", sourceHost);
    msg->setInt32("sourcePort", sourcePort);
    msg->post();
}

void WfdSink::start(const char *uri) {
    sp<AMessage> msg = new AMessage(kWhatStart, id());
    msg->setString("setupURI", uri);
    msg->post();
}
*/

//dequeue
sp<RTPSource>  WfdSink::getRTPSource() {
    return mRTPSource;
}

// static
bool WfdSink::ParseURL(
        const char *url, AString *host, int32_t *port, AString *path,
        AString *user, AString *pass) {
    host->clear();
    *port = 0;
    path->clear();
    user->clear();
    pass->clear();

    if (strncasecmp("rtsp://", url, 7)) {
        return false;
    }

    const char *slashPos = strchr(&url[7], '/');

    if (slashPos == NULL) {
        host->setTo(&url[7]);
        path->setTo("/");
    } else {
        host->setTo(&url[7], slashPos - &url[7]);
        path->setTo(slashPos);
    }

    ssize_t atPos = host->find("@");

    if (atPos >= 0) {
        // Split of user:pass@ from hostname.

        AString userPass(*host, 0, atPos);
        host->erase(0, atPos + 1);

        ssize_t colonPos = userPass.find(":");

        if (colonPos < 0) {
            *user = userPass;
        } else {
            user->setTo(userPass, 0, colonPos);
            pass->setTo(userPass, colonPos + 1, userPass.size() - colonPos - 1);
        }
    }

    const char *colonPos = strchr(host->c_str(), ':');

    if (colonPos != NULL) {
        char *end;
        unsigned long x = strtoul(colonPos + 1, &end, 10);

        if (end == colonPos + 1 || *end != '\0' || x >= 65536) {
            return false;
        }

        *port = x;

        size_t colonOffset = colonPos - host->c_str();
        size_t trailing = host->size() - colonOffset;
        host->erase(colonOffset, trailing);
    } else {
        *port = 554;
    }

    return true;
}

void WfdSink::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatStart:
        {
            uint32_t replyID;
            CHECK(msg->senderAwaitsResponse(&replyID));
            mReplyID = replyID;//record the start id, we response until error or setup sent.

            int32_t sourcePort;

            if (msg->findString("setupURI", &mSetupURI)) {
                AString path, user, pass;
                CHECK(ParseURL(
                            mSetupURI.c_str(),
                            &mRTSPHost, &sourcePort, &path, &user, &pass)
                        && user.empty() && pass.empty());
            } else {
                CHECK(msg->findString("sourceHost", &mRTSPHost));
                CHECK(msg->findInt32("sourcePort", &sourcePort));
            }
            mRTSPPort = sourcePort;

            sp<AMessage> notify = new AMessage(kWhatRTSPNotify, id());

            status_t err = mNetSession->createRTSPClient(
                    mRTSPHost.c_str(), sourcePort, notify, &mSessionID);
            //CHECK_EQ(err, (status_t)OK);
            if(err == (status_t)OK) {
                mState = CONNECTING;
                ALOGD("kWhatStart: createRTSPClient okay,not reply now");
            } else {
                //reply here
                //open fail, notify user try again.
                ALOGD("kWhatStart: createRTSPClient failed, reply");
                mState = UNDEFINED;
                sp<AMessage> response = new AMessage;
                response->setInt32("err", err);
                response->postReply(mReplyID);
                mReplyID = 0;
            }
            break;
        }

        case kWhatRTSPNotify:
        {
            int32_t reason;
            CHECK(msg->findInt32("reason", &reason));

            switch (reason) {
                case ANetworkSession::kWhatError:
                {
                    int32_t sessionID;
                    CHECK(msg->findInt32("sessionID", &sessionID));

                    int32_t err;
                    CHECK(msg->findInt32("err", &err));

                    AString detail;
                    CHECK(msg->findString("detail", &detail));

                    ALOGE("An error occurred in session %d (%d, '%s/%s').",
                          sessionID,
                          err,
                          detail.c_str(),
                          strerror(-err));

//E/NetworkSession(24542): readMore on socket 10 failed w/ error -104 (Connection reset by peer)
//E/WfdSink(24542): An error occurred in session 1 (-104, 'Recv failed./Connection reset by peer').

//E/NetworkSession(16425): readMore on socket 10 failed w/ error -110 (Connection timed out)
//E/WfdSink(16425): An error occurred in session 1 (-110, 'Recv failed./Connection timed out').

//I/NetworkSession(31103): connecting socket 10 to 192.168.49.1:7236
//F/WfdSink(31103): frameworks/av/media/libstagefright/wifi-display/sink/WfdSink.cpp:144 CHECK_EQ( err,(status_t)OK) failed: -101 vs. 0

//E/NetworkSession( 2683): writeMore on socket 14 failed w/ error -111 (Connection refused)
//E/WfdSink( 2683): An error occurred in session 1 (-111, 'Connection failed/Connection refused').
//I/WfdSink( 2683): Lost control connection.
                    
                    if(mReplyID != 0) {
                        sp<AMessage> response = new AMessage;
                        response->setInt32("err", err);
                        response->postReply(mReplyID);
                    }

                    if (sessionID == mSessionID) {
                        ALOGI("Lost control connection.");

                        // The control connection is dead now.
                        mNetSession->destroySession(mSessionID);
                        mSessionID = 0;

                        looper()->stop();
                    }
                    break;
                }

                case ANetworkSession::kWhatConnected:
                {
                    ALOGI("We're now connected.");
                    mState = CONNECTED;

                    if (!mSetupURI.empty()) {
                        status_t err =
                            sendDescribe(mSessionID, mSetupURI.c_str());

                        CHECK_EQ(err, (status_t)OK);
                    }
                    break;
                }

                case ANetworkSession::kWhatData:
                {
                    onReceiveClientData(msg);
                    break;
                }

                case ANetworkSession::kWhatBinaryData:
                {
                    CHECK(sUseTCPInterleaving);

                    int32_t channel;
                    CHECK(msg->findInt32("channel", &channel));

                    sp<ABuffer> data;
                    CHECK(msg->findBuffer("data", &data));

                    mRTPSource->injectPacket(channel == 0 /* isRTP */, data);
                    break;
                }

                default:
                    TRESPASS();
            }
            break;
        }

        case kWhatStop:
        {
            looper()->stop();
            break;
        }

        default:
            TRESPASS();
    }
}

void WfdSink::registerResponseHandler(
        int32_t sessionID, int32_t cseq, HandleRTSPResponseFunc func) {
    ResponseID id;
    id.mSessionID = sessionID;
    id.mCSeq = cseq;
    mResponseHandlers.add(id, func);
}

status_t WfdSink::sendM2(int32_t sessionID) {
    AString request = "OPTIONS * RTSP/1.0\r\n";
    AppendCommonResponse(&request, mNextCSeq);

    request.append(
            "Require: org.wfa.wfd1.0\r\n"
            "\r\n");

    status_t err =
        mNetSession->sendRequest(sessionID, request.c_str(), request.size());

    if (err != OK) {
        return err;
    }

    registerResponseHandler(
            sessionID, mNextCSeq, &WfdSink::onReceiveM2Response);

    ++mNextCSeq;

    return OK;
}

status_t WfdSink::onReceiveM2Response(
        int32_t sessionID, const sp<ParsedMessage> &msg) {
    int32_t statusCode;
    if (!msg->getStatusCode(&statusCode)) {
        return ERROR_MALFORMED;
    }

    if (statusCode != 200) {
        return ERROR_UNSUPPORTED;
    }

    return OK;
}

status_t WfdSink::onReceiveDescribeResponse(
        int32_t sessionID, const sp<ParsedMessage> &msg) {
    int32_t statusCode;
    if (!msg->getStatusCode(&statusCode)) {
        return ERROR_MALFORMED;
    }

    if (statusCode != 200) {
        return ERROR_UNSUPPORTED;
    }

    return sendSetup(sessionID, mSetupURI.c_str());
}

status_t WfdSink::onReceiveSetupResponse(
        int32_t sessionID, const sp<ParsedMessage> &msg) {
    int32_t statusCode;
    if (!msg->getStatusCode(&statusCode)) {
        return ERROR_MALFORMED;
    }

    if (statusCode != 200) {
        return ERROR_UNSUPPORTED;
    }

    if (!msg->findString("session", &mPlaybackSessionID)) {
        return ERROR_MALFORMED;
    }

    if (!ParsedMessage::GetInt32Attribute(
                mPlaybackSessionID.c_str(),
                "timeout",
                &mPlaybackSessionTimeoutSecs)) {
        mPlaybackSessionTimeoutSecs = -1;
    }

    ssize_t colonPos = mPlaybackSessionID.find(";");
    if (colonPos >= 0) {
        // Strip any options from the returned session id.
        mPlaybackSessionID.erase(
                colonPos, mPlaybackSessionID.size() - colonPos);
    }

    status_t err = configureTransport(msg);

    if (err != OK) {
        return err;
    }

    mState = PAUSED;

    return sendPlay(
            sessionID,
            !mSetupURI.empty()
                ? mSetupURI.c_str() : StringPrintf("rtsp://%s/wfd1.0/streamid=0", mRTSPHost.c_str()).c_str() );
}

status_t WfdSink::configureTransport(const sp<ParsedMessage> &msg) {
    if (sUseTCPInterleaving) {
        return OK;
    }

    AString transport;
    if (!msg->findString("transport", &transport)) {
        ALOGE("Missing 'transport' field in SETUP response.");
        return ERROR_MALFORMED;
    }

    AString sourceHost;
    if (!ParsedMessage::GetAttribute(
                transport.c_str(), "source", &sourceHost)) {
        sourceHost = mRTSPHost;
    }

    AString serverPortStr;
    if (!ParsedMessage::GetAttribute(
                transport.c_str(), "server_port", &serverPortStr)) {
        ALOGE("Missing 'server_port' in Transport field.");
        return ERROR_MALFORMED;
    }

    int rtpPort = 0, rtcpPort = 0;
    /* wfd support rtcp info, seldom use and disable it now. */
    #if WIFI_DISPLAY_SINK_DEFAULT
    if (sscanf(serverPortStr.c_str(), "%d-%d", &rtpPort, &rtcpPort) != 2
            || rtpPort <= 0 || rtpPort > 65535
            || rtcpPort <=0 || rtcpPort > 65535
            || rtcpPort != rtpPort + 1) {
        ALOGE("Invalid server_port description '%s'.",
                serverPortStr.c_str());

        return ERROR_MALFORMED;
    }

    #else
    if (sscanf(serverPortStr.c_str(), "%d", &rtpPort) != 1
            || rtpPort <= 0 || rtpPort > 65535) {
        ALOGE("Invalid server_port description '%s'.",
                serverPortStr.c_str());
        return ERROR_MALFORMED;
    }
    #endif
    if (rtpPort & 1) {
        ALOGW("Server picked an odd numbered RTP port.");
    }

    return mRTPSource->connect(sourceHost.c_str(), rtpPort, rtcpPort);
}

status_t WfdSink::onReceivePlayResponse(
        int32_t sessionID, const sp<ParsedMessage> &msg) {
    int32_t statusCode;
    status_t err = OK;
    sp<AMessage> response = new AMessage;

    if (!msg->getStatusCode(&statusCode)) {
        response->setInt32("err", BAD_VALUE);
        err = ERROR_MALFORMED;
        goto _out;
    }

    if (statusCode != 200) {
        response->setInt32("err", BAD_VALUE);
        err = ERROR_UNSUPPORTED;
        goto _out;
    }

    mState = PLAYING;
    response->setInt32("err", OK);

_out:
    if(mReplyID != 0) {
        response->postReply(mReplyID);
        mReplyID = 0;
    }

    return err;
}

void WfdSink::onReceiveClientData(const sp<AMessage> &msg) {
    int32_t sessionID;
    CHECK(msg->findInt32("sessionID", &sessionID));

    sp<RefBase> obj;
    CHECK(msg->findObject("data", &obj));

    sp<ParsedMessage> data =
        static_cast<ParsedMessage *>(obj.get());

    ALOGV("session %d received '%s'",
          sessionID, data->debugString().c_str());

    AString method;
    AString uri;
    data->getRequestField(0, &method);

    int32_t cseq;
    if (!data->findInt32("cseq", &cseq)) {
        sendErrorResponse(sessionID, "400 Bad Request", -1 /* cseq */);
        return;
    }

    if (method.startsWith("RTSP/")) {
        // This is a response.

        ResponseID id;
        id.mSessionID = sessionID;
        id.mCSeq = cseq;

        ssize_t index = mResponseHandlers.indexOfKey(id);

        if (index < 0) {
            ALOGW("Received unsolicited server response, cseq %d", cseq);
            return;
        }

        HandleRTSPResponseFunc func = mResponseHandlers.valueAt(index);
        mResponseHandlers.removeItemsAt(index);

        status_t err = (this->*func)(sessionID, data);
        CHECK_EQ(err, (status_t)OK);
    } else {
        AString version;
        data->getRequestField(2, &version);
        if (!(version == AString("RTSP/1.0"))) {
            sendErrorResponse(sessionID, "505 RTSP Version not supported", cseq);
            return;
        }

        if (method == "OPTIONS") {
            onOptionsRequest(sessionID, cseq, data);
        } else if (method == "GET_PARAMETER") {
            onGetParameterRequest(sessionID, cseq, data);
        } else if (method == "SET_PARAMETER") {
            onSetParameterRequest(sessionID, cseq, data);
        } else {
            sendErrorResponse(sessionID, "405 Method Not Allowed", cseq);
        }
    }
}

void WfdSink::onOptionsRequest(
        int32_t sessionID,
        int32_t cseq,
        const sp<ParsedMessage> &data) {
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("Public: org.wfa.wfd1.0, GET_PARAMETER, SET_PARAMETER\r\n");
    response.append("\r\n");

    status_t err = mNetSession->sendRequest(sessionID, response.c_str());
    CHECK_EQ(err, (status_t)OK);

    err = sendM2(sessionID);
    CHECK_EQ(err, (status_t)OK);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
static char const* MIRACAST_CAPABILITY[] =
{
    "wfd_audio_codecs", // 0-done
    "wfd_video_formats",// 1-done
    "wfd_3d_video_formats",// 2
    "wfd_content_protection",// 3-not support
    "wfd_display_edid",// 4
    "wfd_coupled_sink",// 5
    "wfd_trigger_method",// 6 -----ignore
    "wfd_presentation_URL",// 7 -----ignore
    "wfd_client_rtp_ports",// 8-done
    "wfd_route",// 9
    "wfd_I2C",// 10
    "wfd_av_format_change_timing",// 11
    "wfd_preferred_display_mode",// 12
    "wfd_uibc_capability",// 13
    "wfd_uibc_setting",// 14 -----ignore
    "wfd_standby_resume_capability",// 15
    "wfd_standby",// 16 -----ignore
    "wfd_connector_type",// 17
    "wfd_idr_request",// 18
    "",
};
static int32_t MIRACAST_CAPABILITY_LEN = sizeof(MIRACAST_CAPABILITY)/sizeof(MIRACAST_CAPABILITY[0]);

static bool checkForHeader(char const* line, char const* headerName, unsigned headerNameLength) {
    if (strncasecmp(line, headerName, headerNameLength) != 0) {
        return false;
    }
    // The line begins with the desired header name.
    // Trim off any whitespace, and return the header parameters:
    unsigned paramIndex = headerNameLength;
    while (line[paramIndex] != '\0' && (line[paramIndex] == ' ' || line[paramIndex] == '\t')) ++paramIndex;
    if (&line[paramIndex] == '\0') {
        // the header is assumed to be bad if it has no parameters
        return false;
    }

    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////

void WfdSink::onGetParameterRequest(
        int32_t sessionID,
        int32_t cseq,
        const sp<ParsedMessage> &data) {
    int32_t contentLength = 0;
    const char *content = data->getContent();
    if(content != NULL) {
        if (!data->findInt32("Content-Length", &contentLength)) {
            ALOGI("ContentLength = %d", contentLength);
        }
    }

    AString body;
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);

    if(contentLength == 0) {
        //if get parameter content-length is zero, then this is a heartbeat every 10/30 seconds.
        // timeout get in the setup respsonse
        ALOGI("[heartbeat=%d].", ++mHeartBeat);
    } else if(contentLength == 21) {
        ALOGI("wfd_uibc_capability negotiation.");
        body = "wfd_uibc_capability: none\r\n";
        response.append("Content-Type: text/parameters\r\n");
        response.append(StringPrintf("Content-Length: %d\r\n", body.size()));
        response.append("\r\n");
        response.append(body);
    } else if(contentLength >= 53) {
        /* why 53 bytes ?
        * for wfd_video_formats,wfd_audio_codecs,wfd_client_rtp_ports,is required.
        * sink capability negotiation
        */
        const char* fields = content;
        unsigned counter = 0;
        while (*fields == ' ') ++fields;
        char *field = new char[strlen(fields)+1];
        char const* capabilityStr = NULL;

        ALOGI("Capability negotiation.");

/*
coolpad:
wfd_audio_codecs: LPCM 00000002 00
wfd_video_formats: 00 00 01 11 00004000 00000000 00000000 00 0000 0000 00 none none
*/
        while (sscanf(fields, "%[^\r\n]", field) == 1) {
            ALOGI("(%d): [%s]", ++counter, field);
            if(checkForHeader(field, MIRACAST_CAPABILITY[0] , strlen(MIRACAST_CAPABILITY[0]))) {
                // 1. audio codecs, must
                //char const* sinkAudioList1 = "LPCM 00000003 00, AAC 0000000F 00";
                char const* sinkAudioList1 = "LPCM 00000003 00";//okay
                char const* sinkAudioList2 = "AAC 0000000F 00";// todo: list a supported adudio cap list,get from xml.
                char const* sinkAudioList3 = "AC3 00000007 00";
                char const* audioCodecsFmt = "wfd_audio_codecs: %s\r\n";
                body.append(StringPrintf(audioCodecsFmt, sinkAudioList1));
            } else if(checkForHeader(field, MIRACAST_CAPABILITY[1] , strlen(MIRACAST_CAPABILITY[1]))){
                // 2. video formats, must
                //char const* sinkVideoList1 = "00 00 02 02 0001DEFF 157C7FFF 00000FFF 00 0000 0000 00 none none";
                char const* sinkVideoList1 = "00 00 01 11 0001DEFF 00000000 00000000 00 0000 0000 00 none none";//okay
                char const* sinkVideoList2 = "";// to do: list a supported video formats,get from xml.
                char const* videoFormatsFmt = "wfd_video_formats: %s\r\n";
                body.append(StringPrintf(videoFormatsFmt, sinkVideoList1));
            } else if(checkForHeader(field, MIRACAST_CAPABILITY[2] , strlen(MIRACAST_CAPABILITY[2]))) {
                // 3. 3d video formats
                char const* _3dVideoCapListList1 = "none";// todo : list a supported 3d video formats,get from xml.
                char const* _3dVideoFormatsFmt = "wfd_3d_video_formats: %s\r\n";
                body.append(StringPrintf(_3dVideoFormatsFmt, _3dVideoCapListList1));
            }else if(checkForHeader(field, MIRACAST_CAPABILITY[3] , strlen(MIRACAST_CAPABILITY[3])) ) {
                // 4. content protection, "HDCP2.1 port=1189";
                // todo: if media supported this, we will create a tcp socket server listen for client and setport
                char const* cp_spec = "none";
                char const* contentProtectionFmt = "wfd_content_protection: %s\r\n";
                body.append(StringPrintf(contentProtectionFmt, cp_spec));
            }else if(checkForHeader(field, MIRACAST_CAPABILITY[4] , strlen(MIRACAST_CAPABILITY[4]))) {
                // 5. display edid "wfd_display_edid: none\r\n"
                char const* edidBlock_Payload = "none";
                char const* edidFmt = "wfd_display_edid: %s\r\n";
                body.append(StringPrintf(edidFmt, edidBlock_Payload));
            }else if(checkForHeader(field, MIRACAST_CAPABILITY[5] , strlen(MIRACAST_CAPABILITY[5]))) {
                // 6. coupled sink
                char const* coupledSinkCap = "none";
                char const* coupledSinkFmt = "wfd_coupled_sink: %s\r\n";
                body.append(StringPrintf(coupledSinkFmt, coupledSinkCap));
            } else if(checkForHeader(field, MIRACAST_CAPABILITY[6] , strlen(MIRACAST_CAPABILITY[6]))) {
                // 7. trigger method
                // should never comming
                ALOGI("trigger method should never exist.");
            } else if(checkForHeader(field, MIRACAST_CAPABILITY[7] , strlen(MIRACAST_CAPABILITY[7]))){
                // 8. presentation url
                ALOGI("presentation URL should never exist.");
            } else if(checkForHeader(field, MIRACAST_CAPABILITY[8] , strlen(MIRACAST_CAPABILITY[8]))) {
                // 9. client rtp ports, must
                // need to init RTSPSink here.
                mRTPPort = 15550;
                unsigned short port1 = 0;// to do ,for coupled sink
                char const *rtpPortsFmt = "wfd_client_rtp_ports: RTP/AVP/UDP;unicast %u %u mode=play\r\n";
                body.append(StringPrintf(rtpPortsFmt, mRTPPort, port1));
            } else if(checkForHeader(field, MIRACAST_CAPABILITY[9] , strlen(MIRACAST_CAPABILITY[9]))) {
                // 10. route
                char const* destionation = "primary";
                char const* routeFmt = "wfd_route: %s\r\n";
                body.append(StringPrintf(routeFmt, destionation));
            } else if(checkForHeader(field, MIRACAST_CAPABILITY[10] , strlen(MIRACAST_CAPABILITY[10]))) {
                // 11. I2C
                char const* i2cPort = "none";
                char const* i2cFmt = "wfd_I2C: %s\r\n";
                body.append(StringPrintf(i2cFmt, i2cPort));
            } else if(checkForHeader(field, MIRACAST_CAPABILITY[11] , strlen(MIRACAST_CAPABILITY[11]))) {
                // 12. av format change timming
                char const* pts = "none";// todo fixme
                char const* dts = "none";
                char const* avFormatChangeTimingFmt = "wfd_av_format_change_timing: %s %s\r\n";
                body.append(StringPrintf(avFormatChangeTimingFmt, pts, dts));
            } else if(checkForHeader(field, MIRACAST_CAPABILITY[12] , strlen(MIRACAST_CAPABILITY[12]))) {
                // 13. preferred display mode
                char const* dinfo = "none";
                char const* preferredDisplayModeFmt = "wfd_preferred_display_mode: %s\r\n";
                body.append(StringPrintf(preferredDisplayModeFmt, dinfo));
            } else if(checkForHeader(field, MIRACAST_CAPABILITY[13] , strlen(MIRACAST_CAPABILITY[13]))) {
                // 14. ubic capability
                char const* uibcCap = "none";
                char const* uibcCapabilityFmt = "wfd_uibc_capablility: %s\r\n";
                body.append(StringPrintf(uibcCapabilityFmt, uibcCap));
            } else if(checkForHeader(field, MIRACAST_CAPABILITY[14] , strlen(MIRACAST_CAPABILITY[14]))) {
                // 15. uibc setting
                ALOGI("uibc setting should never exist.\n");
            } else if(checkForHeader(field, MIRACAST_CAPABILITY[15] , strlen(MIRACAST_CAPABILITY[15]))) {
                // 16. standby resume capability
                char const* standbyCap = "none";
                char const* standbyCapabilityFmt = "wfd_standby_resume_capablility: %s\r\n";
                body.append(StringPrintf(standbyCapabilityFmt, standbyCap));
            } else if(checkForHeader(field, MIRACAST_CAPABILITY[16] , strlen(MIRACAST_CAPABILITY[16]))) {
                // 17. standby
                ALOGI("wfd_standby should never exist.\n");
            } else if(checkForHeader(field, MIRACAST_CAPABILITY[17] , strlen(MIRACAST_CAPABILITY[17]))) {
                // 18. connector-type
                char const* connectorTypeContent = "none";
                char const* connectorTypeFmt = "wfd_connector_type: %s\r\n";
                body.append(StringPrintf(connectorTypeFmt, connectorTypeContent));
            } else {
                ALOGD("not match wfd keyword [%s]", field);
            }
            fields += strlen(field);
            while (*fields == '\r' || *fields == '\n') ++fields; // skip over separating ';' chars
        }// end of while
        delete[] field;

        response.append("Content-Type: text/parameters\r\n");
        response.append(StringPrintf("Content-Length: %d\r\n", body.size()));
        response.append("\r\n");
        response.append(body);
    }// end of else if(contentLength >= 53)

    status_t err = mNetSession->sendRequest(sessionID, response.c_str());
    CHECK_EQ(err, (status_t)OK);
}

status_t WfdSink::sendDescribe(int32_t sessionID, const char *uri) {
    uri = "rtsp://xwgntvx.is.livestream-api.com/livestreamiphone/wgntv";
    uri = "rtsp://v2.cache6.c.youtube.com/video.3gp?cid=e101d4bf280055f9&fmt=18";

    AString request = StringPrintf("DESCRIBE %s RTSP/1.0\r\n", uri);
    AppendCommonResponse(&request, mNextCSeq);

    request.append("Accept: application/sdp\r\n");
    request.append("\r\n");

    status_t err = mNetSession->sendRequest(
            sessionID, request.c_str(), request.size());

    if (err != OK) {
        return err;
    }

    registerResponseHandler(
            sessionID, mNextCSeq, &WfdSink::onReceiveDescribeResponse);

    ++mNextCSeq;

    return OK;
}

status_t WfdSink::sendSetup(int32_t sessionID, const char *uri) {
    //move this to set_parameter and init client_port
    mRTPSource = new RTPSource(mNetSession);
    looper()->registerHandler(mRTPSource);

    status_t err = mRTPSource->init(sUseTCPInterleaving);

    if (err != OK) {
        looper()->unregisterHandler(mRTPSource->id());
        mRTPSource.clear();
        return err;
    }

    AString request = StringPrintf("SETUP %s RTSP/1.0\r\n", uri);

    AppendCommonResponse(&request, mNextCSeq);

    if (sUseTCPInterleaving) {
        request.append("Transport: RTP/AVP/TCP;interleaved=0-1\r\n");
    } else {
        int32_t rtpPort = mRTPSource->getRTPPort();
        mRTPPort = rtpPort;
        /* wfd support rtcp info, seldom use and disable it now. */
    #if WIFI_DISPLAY_SINK_DEFAULT
        request.append(
                StringPrintf(
                    "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n",
                    rtpPort, rtpPort + 1));
    #else
        request.append(
                StringPrintf(
                    "Transport: RTP/AVP/UDP;unicast;client_port=%d\r\n",
                    rtpPort));

    }
    #endif
    request.append("\r\n");

    ALOGV("request = '%s'", request.c_str());

    err = mNetSession->sendRequest(sessionID, request.c_str(), request.size());

    if (err != OK) {
        return err;
    }

    registerResponseHandler(
            sessionID, mNextCSeq, &WfdSink::onReceiveSetupResponse);

    ++mNextCSeq;

    return OK;
}

status_t WfdSink::sendPlay(int32_t sessionID, const char *uri) {
    AString request = StringPrintf("PLAY %s RTSP/1.0\r\n", uri);

    AppendCommonResponse(&request, mNextCSeq);

    request.append(StringPrintf("Session: %s\r\n", mPlaybackSessionID.c_str()));
    request.append("\r\n");

    status_t err =
        mNetSession->sendRequest(sessionID, request.c_str(), request.size());

    if (err != OK) {
        return err;
    }

    registerResponseHandler(
            sessionID, mNextCSeq, &WfdSink::onReceivePlayResponse);

    ++mNextCSeq;

    return OK;
}

void WfdSink::onSetParameterRequest(
        int32_t sessionID,
        int32_t cseq,
        const sp<ParsedMessage> &data) {
    const char *content = data->getContent();
    int32_t contentLength = 0;
    if(content != NULL) {
        if (!data->findInt32("Content-Length", &contentLength)) {
            ALOGI("ContentLength = %d", contentLength);
        }
    }

    if (strstr(content, "wfd_trigger_method: SETUP\r\n") != NULL) {
        status_t err =
            sendSetup(
                    sessionID,
                    StringPrintf("rtsp://%s/wfd1.0/streamid=0", mRTSPHost.c_str()).c_str());

        CHECK_EQ(err, (status_t)OK);

    } else if (strstr(content, "wfd_trigger_method: PLAY\r\n") != NULL) {
        ALOGD("[wfd_trigger_method: PLAY]");
        // todo, start media player again
    } else if (strstr(content, "wfd_trigger_method: PAUSE\r\n") != NULL) {
        ALOGD("[wfd_trigger_method: PAUSE]");
        // todo, pause media player
    } else if (strstr(content, "wfd_trigger_method: TEARDOWN\r\n") != NULL) {
        ALOGD("[wfd_trigger_method: TEARDOWN]");
        // after send response okay, stop media player.
    } else if (strstr(content, "wfd_uibc_setting: enable\r\n") != NULL) {
        ALOGD("[wfd_uibc_setting: enable]");
        // todo, uibc implementation
    } else if (strstr(content, "wfd_uibc_setting: disable\r\n") != NULL) {
        ALOGD("[wfd_uibc_setting: disable]");
        // todo, uibc implementaion
    } else if (strstr(content, "wfd_standby") != NULL) {
        ALOGD("[wfd_standby]");
    } else if(contentLength > 32) {
        // todo, parse audio and video and url to confirm whether sink support the format of av.
        ALOGI("source capability = [%s]", content);
    }

    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("\r\n");

    status_t err = mNetSession->sendRequest(sessionID, response.c_str());
    CHECK_EQ(err, (status_t)OK);
}

void WfdSink::sendErrorResponse(
        int32_t sessionID,
        const char *errorDetail,
        int32_t cseq) {
    AString response;
    response.append("RTSP/1.0 ");
    response.append(errorDetail);
    response.append("\r\n");

    AppendCommonResponse(&response, cseq);

    response.append("\r\n");

    status_t err = mNetSession->sendRequest(sessionID, response.c_str());
    CHECK_EQ(err, (status_t)OK);
}

// static
void WfdSink::AppendCommonResponse(AString *response, int32_t cseq) {
    time_t now = time(NULL);
    struct tm *now2 = gmtime(&now);
    char buf[128];
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %z", now2);

    if (cseq >= 0) {
        response->append(StringPrintf("CSeq: %d\r\n", cseq));
    }

    response->append("Date: ");
    response->append(buf);
    response->append("\r\n");

    response->append("User-Agent: stagefright/1.1 (Linux;Android 4.1)\r\n");
}

}  // namespace android
