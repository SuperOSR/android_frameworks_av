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

#ifndef MIRACAST_SINK_H_

#define MIRACAST_SINK_H_

#include <media/stagefright/foundation/AHandler.h>
#include "ANetworkSession.h"

namespace android {

struct WfdSink;

/* 
  if the wfd sink buffer size less than WFD_SINK_RTP_READ_LIMITED, 
  it will return 0 bytes.
  make it bigger to make sure read more data one time.
*/
#define WFD_SINK_RTP_READ_LIMITED 1316

/*
  wfd sink handle.
  no need to care about member.
  when call wfd_open successfully, it will return this handle,
  you must pass this hanlde when you want to call wfd_read or wfd_close.
*/
typedef struct WfdSinkRTP: public RefBase {
    sp<ANetworkSession> mNetSession;
    sp<ALooper> mLooper;
    sp<WfdSink> mSink;
}WFD_SINK;



/*
  in:  
     url: wfd://ip_address:port, for exmaple, wfd://192.168.49.68:7236

  return: 
     succeed return wfdsink handle; failed return NULL;

 Description:    
     it will block until succeed or failed return, this will block you about [100ms,1300ms].
     if return NULL, you may try to open the url once again.
*/
WFD_SINK *wfd_open(const char *url);



/*
    in: 
        handle: get from wfd_open
        pData:  pointer of buffer
        dLen:   buffer len
    return: 
         -1:  illegal, input parameter is invalid
         0:   data not available yet
         > 0: actually data len
    Description:
      call this function only wfd_open return succeed.
*/
int wfd_read(WFD_SINK *handle, unsigned char *pData, unsigned dLen);



/*
    in: 
        handle: get from wfd_open
    return:
          0: succeed
          !0 : failed
    Description:
        when you 
*/
int wfd_close(WFD_SINK *handle);

}  // namespace android

#endif  // MIRACAST_SINK_H_

