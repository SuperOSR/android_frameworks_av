/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "Camera2-FrameProcessor"
#define ATRACE_TAG ATRACE_TAG_CAMERA
//#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <utils/Trace.h>

#include "FrameProcessor.h"
#include "../CameraDeviceBase.h"
#include "../Camera2Client.h"

namespace android {
namespace camera2 {

FrameProcessor::FrameProcessor(wp<CameraDeviceBase> device,
                               wp<Camera2Client> client) :
    ProFrameProcessor(device),
    mClient(client),
    mLastFrameNumberOfFaces(0) {
}

FrameProcessor::~FrameProcessor() {
}

bool FrameProcessor::processSingleFrame(CameraMetadata &frame,
                                        const sp<CameraDeviceBase> &device) {

    sp<Camera2Client> client = mClient.promote();
    if (!client.get()) {
        return false;
    }

    if (processFaceDetect(frame, client) != OK) {
        return false;
    }

    if (!ProFrameProcessor::processSingleFrame(frame, device)) {
        return false;
    }

    return true;
}

status_t FrameProcessor::processFaceDetect(const CameraMetadata &frame,
        const sp<Camera2Client> &client) {
    status_t res = BAD_VALUE;
    ATRACE_CALL();
    camera_metadata_ro_entry_t entry;
    bool enableFaceDetect;

    {
        SharedParameters::Lock l(client->getParameters());
        enableFaceDetect = l.mParameters.enableFaceDetect;
    }
    entry = frame.find(ANDROID_STATISTICS_FACE_DETECT_MODE);

    // TODO: This should be an error once implementations are compliant
    if (entry.count == 0) {
        return OK;
    }

    uint8_t faceDetectMode = entry.data.u8[0];

    camera_frame_metadata metadata;
    Vector<camera_face_t> faces;
    metadata.number_of_faces = 0;

    if (enableFaceDetect &&
        faceDetectMode != ANDROID_STATISTICS_FACE_DETECT_MODE_OFF) {

        SharedParameters::Lock l(client->getParameters());
        entry = frame.find(ANDROID_STATISTICS_FACE_RECTANGLES);
        if (entry.count == 0) {
            // No faces this frame
            /* warning: locks SharedCameraCallbacks */
            callbackFaceDetection(client, metadata);
            return OK;
        }
        metadata.number_of_faces = entry.count / 4;
        if (metadata.number_of_faces >
                l.mParameters.fastInfo.maxFaces) {
            ALOGE("%s: Camera %d: More faces than expected! (Got %d, max %d)",
                    __FUNCTION__, client->getCameraId(),
                    metadata.number_of_faces, l.mParameters.fastInfo.maxFaces);
            return res;
        }
        const int32_t *faceRects = entry.data.i32;

        entry = frame.find(ANDROID_STATISTICS_FACE_SCORES);
        if (entry.count == 0) {
            ALOGE("%s: Camera %d: Unable to read face scores",
                    __FUNCTION__, client->getCameraId());
            return res;
        }
        const uint8_t *faceScores = entry.data.u8;

        const int32_t *faceLandmarks = NULL;
        const int32_t *faceIds = NULL;

        if (faceDetectMode == ANDROID_STATISTICS_FACE_DETECT_MODE_FULL) {
            entry = frame.find(ANDROID_STATISTICS_FACE_LANDMARKS);
            if (entry.count == 0) {
                ALOGE("%s: Camera %d: Unable to read face landmarks",
                        __FUNCTION__, client->getCameraId());
                return res;
            }
            faceLandmarks = entry.data.i32;

            entry = frame.find(ANDROID_STATISTICS_FACE_IDS);

            if (entry.count == 0) {
                ALOGE("%s: Camera %d: Unable to read face IDs",
                        __FUNCTION__, client->getCameraId());
                return res;
            }
            faceIds = entry.data.i32;
        }

        faces.setCapacity(metadata.number_of_faces);

        size_t maxFaces = metadata.number_of_faces;
        for (size_t i = 0; i < maxFaces; i++) {
            if (faceScores[i] == 0) {
                metadata.number_of_faces--;
                continue;
            }
            if (faceScores[i] > 100) {
                ALOGW("%s: Face index %d with out of range score %d",
                        __FUNCTION__, i, faceScores[i]);
            }

            camera_face_t face;

            face.rect[0] = l.mParameters.arrayXToNormalized(faceRects[i*4 + 0]);
            face.rect[1] = l.mParameters.arrayYToNormalized(faceRects[i*4 + 1]);
            face.rect[2] = l.mParameters.arrayXToNormalized(faceRects[i*4 + 2]);
            face.rect[3] = l.mParameters.arrayYToNormalized(faceRects[i*4 + 3]);

            face.score = faceScores[i];
            if (faceDetectMode == ANDROID_STATISTICS_FACE_DETECT_MODE_FULL) {
                face.id = faceIds[i];
                face.left_eye[0] =
                    l.mParameters.arrayXToNormalized(faceLandmarks[i*6 + 0]);
                face.left_eye[1] =
                    l.mParameters.arrayYToNormalized(faceLandmarks[i*6 + 1]);
                face.right_eye[0] =
                    l.mParameters.arrayXToNormalized(faceLandmarks[i*6 + 2]);
                face.right_eye[1] =
                    l.mParameters.arrayYToNormalized(faceLandmarks[i*6 + 3]);
                face.mouth[0] =
                    l.mParameters.arrayXToNormalized(faceLandmarks[i*6 + 4]);
                face.mouth[1] =
                    l.mParameters.arrayYToNormalized(faceLandmarks[i*6 + 5]);
            } else {
                face.id = 0;
                face.left_eye[0] = face.left_eye[1] = -2000;
                face.right_eye[0] = face.right_eye[1] = -2000;
                face.mouth[0] = face.mouth[1] = -2000;
            }
            faces.push_back(face);
        }

        metadata.faces = faces.editArray();
    }

    /* warning: locks SharedCameraCallbacks */
    callbackFaceDetection(client, metadata);

    return OK;
}

void FrameProcessor::callbackFaceDetection(sp<Camera2Client> client,
                                     const camera_frame_metadata &metadata) {

    camera_frame_metadata *metadata_ptr =
        const_cast<camera_frame_metadata*>(&metadata);

    /**
     * Filter out repeated 0-face callbacks,
     * but not when the last frame was >0
     */
    if (metadata.number_of_faces != 0 ||
        mLastFrameNumberOfFaces != metadata.number_of_faces) {

        Camera2Client::SharedCameraCallbacks::Lock
            l(client->mSharedCameraCallbacks);
        if (l.mRemoteCallback != NULL) {
            l.mRemoteCallback->dataCallback(CAMERA_MSG_PREVIEW_METADATA,
                                            NULL,
                                            metadata_ptr);
        }
    }

    mLastFrameNumberOfFaces = metadata.number_of_faces;
}

}; // namespace camera2
}; // namespace android
