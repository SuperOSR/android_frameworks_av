#define ALOG_NDEBUG 0
#define ALOG_TAG "TPlayer"
#include <utils/Log.h>

#include "tplayer.h"

#include <media/Metadata.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/MediaErrors.h>
#include <gui/IGraphicBufferProducer.h>
#include <ui/GraphicBufferMapper.h>
#include "libcedarv.h"


#include "avtimer.h"

extern "C" {
extern unsigned int cedarv_address_phy2vir(void *addr);
}

namespace android {


pthread_mutex_t gThreadMutex = PTHREAD_MUTEX_INITIALIZER;

static void lock_ve(void)
{
	pthread_mutex_lock(&gThreadMutex);
}

static void unlock_ve(void)
{
	pthread_mutex_unlock(&gThreadMutex);
}

static void* MainThread(void* arg);

TPlayer::TPlayer()
{
	mDecoder      = NULL;
    mStatus       = PLAYER_STATE_UNKOWN;
    mNativeWindow = NULL;
    mFpStream     = NULL;
    mThreadId     = 0;
    mStopFlag     = 0;
    mURL          = NULL;
    mFrameRate    = 30000;
    mFrameCount   = 0;

	ve_mutex_init(&mCedarVRequestCtx, CEDARV_DECODE_BACKGROUND);

	pthread_mutex_init(&mBufferMutex, NULL);
}


TPlayer::~TPlayer()
{
	int err;

	if(mThreadId != 0)
	{
		pthread_mutex_lock(&mBufferMutex);
		mStopFlag = 1;
		pthread_mutex_unlock(&mBufferMutex);
		pthread_join(mThreadId, (void**)&err);
		mThreadId = 0;
	}

	if(mDecoder != NULL)
		libcedarv_exit(mDecoder);

	if(mFpStream != NULL)
	{
		fclose(mFpStream);
		mFpStream = NULL;
	}

	if(mURL)
		free(mURL);

	ve_mutex_destroy(&mCedarVRequestCtx);

	pthread_mutex_destroy(&mBufferMutex);
}


status_t TPlayer::initCheck()
{
    return OK;
}


status_t TPlayer::setUID(uid_t uid)
{
    return OK;
}


status_t TPlayer::setDataSource(const char *url, const KeyedVector<String8, String8> *headers)
{
	if(mURL)
		free(mURL);

	mURL = (char*)malloc(strlen(url) + 1);
	if(mURL == NULL)
		return NO_MEMORY;

	strcpy(mURL, url);

	return OK;
}

// Warning: The filedescriptor passed into this method will only be valid until
// the method returns, if you want to keep it, dup it!
status_t TPlayer::setDataSource(int fd, int64_t offset, int64_t length)
{
	return ERROR_UNSUPPORTED;
}

status_t TPlayer::setDataSource(const sp<IStreamSource> &source)
{
	return ERROR_UNSUPPORTED;
}

status_t TPlayer::setParameter(int key, const Parcel &request)
{
	return OK;
}

status_t TPlayer::getParameter(int key, Parcel *reply)
{
	return ERROR_UNSUPPORTED;
}


status_t TPlayer::setVideoSurface(const sp<Surface> &surface)
{
	return OK;
}

status_t TPlayer::setVideoSurfaceTexture(const sp<IGraphicBufferProducer> &bufferProducer)
{
	mPlayer->setVideoSurfaceTexture(bufferProducer);
	return OK;
}

status_t TPlayer::prepare()
{
	mStatus = PLAYER_STATE_PREPARED;
	return OK;
}

status_t TPlayer::prepareAsync()
{
	if(prepare() == OK)
		sendEvent(MEDIA_PREPARED, 0, 0);
	else
		sendEvent(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, -1);

	return OK;
}

status_t TPlayer::start()
{
	int err;

	if(mThreadId != 0)
	{
		mStatus = PLAYER_STATE_PLAYING;
		return OK;
	}

	mStopFlag = 0;

	mStatus = PLAYER_STATE_PLAYING;
	err = pthread_create(&mThreadId, NULL, MainThread, this);
	if(err != 0 || mThreadId == 0)
	{
		mStatus = PLAYER_STATE_UNKOWN;
		return UNKNOWN_ERROR;
	}

	return OK;
}

status_t TPlayer::stop()
{
	int err;

	if(mThreadId != 0)
	{
		pthread_mutex_lock(&mBufferMutex);
		mStopFlag = 1;
		pthread_mutex_unlock(&mBufferMutex);
	}

	mStatus = PLAYER_STATE_UNKOWN;
	mFrameCount = 0;

	sendEvent(MEDIA_PLAYBACK_COMPLETE, 0, 0);

	return OK;
}

status_t TPlayer::pause()
{
	mStatus = PLAYER_STATE_SUSPEND;
	return OK;
}


bool TPlayer::isPlaying()
{
	return (mStatus == PLAYER_STATE_PLAYING);
}


status_t TPlayer::seekTo(int msec)
{
	sendEvent(MEDIA_SEEK_COMPLETE, 0, 0);
	return ERROR_UNSUPPORTED;
}


status_t TPlayer::getCurrentPosition(int *msec)
{
	int frameDuration;
	if(mFrameRate == 0)
		frameDuration = 1000*1000/3000;
	else
		frameDuration = 1000*1000/mFrameRate;

	*msec = mFrameCount * frameDuration / 1000;
	return OK;
}


status_t TPlayer::getDuration(int *msec)
{
	*msec = 5000;
	return OK;
}


status_t TPlayer::reset()
{
	return stop();
}


status_t TPlayer::setLooping(int loop)
{
	return OK;
}


player_type TPlayer::playerType()
{
    return THUMBNAIL_PLAYER;
}

status_t TPlayer::setScreen(int screen)
{
	return OK;
}

int TPlayer::getMeidaPlayerState()
{
	return mStatus;
}

status_t TPlayer::invoke(const Parcel &request, Parcel *reply)
{
    return INVALID_OPERATION;
}

void TPlayer::setAudioSink(const sp<AudioSink> &audioSink)
{
	return;
}

status_t TPlayer::getMetadata(const media::Metadata::Filter& ids, Parcel *records)
{
    using media::Metadata;
    Metadata metadata(records);
    metadata.appendBool(Metadata::kPauseAvailable, MediaExtractor::CAN_PAUSE);
    return OK;
}

int TPlayer::initDecoder()
{
	int err;
	int readBytes;
	cedarv_stream_info_t streamInfo;

	mFpStream = fopen(mURL, "rb");
	if(mFpStream == NULL)
		return UNKNOWN_ERROR;

	readBytes = fread(&mFrameRate, 1, sizeof(int), mFpStream);
	if(readBytes != sizeof(int))
	{
		fclose(mFpStream);
		mFpStream = NULL;
		return UNKNOWN_ERROR;
	}

	if(mFrameRate < 1000)
		mFrameRate *= 1000;

	if(mFrameRate > 30000 || mFrameRate == 0)
		mFrameRate = 30000;

	ve_mutex_lock(&mCedarVRequestCtx);
	mDecoder = libcedarv_init(&err);
	ve_mutex_unlock(&mCedarVRequestCtx);
	if(mDecoder == NULL)
	{
		fclose(mFpStream);
		mFpStream = NULL;
		return UNKNOWN_ERROR;
	}

	memset(&streamInfo, 0, sizeof(streamInfo));

	streamInfo.format           = CEDARV_STREAM_FORMAT_H264;
	streamInfo.frame_rate       = mFrameRate;
	streamInfo.frame_duration   = 1000*1000/mFrameRate;
	streamInfo.aspect_ratio     = 1000;

	if(mDecoder->set_vstream_info(mDecoder, &streamInfo) != 0)
	{
		ve_mutex_lock(&mCedarVRequestCtx);
		libcedarv_exit(mDecoder);
		ve_mutex_unlock(&mCedarVRequestCtx);
		fclose(mFpStream);
		return UNKNOWN_ERROR;
	}

    mFrameCount   = 0;

    return 0;
}

status_t TPlayer::generalInterface(int cmd, int int1, int int2, int int3, void *p)
{
        if(cmd == MEDIAPLAYER_CMD_QUERY_HWLAYER_RENDER)
        {
                *((int*)p) = 0;
        }

        return OK;
}

static void* MainThread(void* arg)
{
	int                  frameLen;
	int                  readBytes;
	int                  ret;
	int                  widthAlign;
	int                  heightAlign;
	void*                pGraphicBuf;
	void*                dst;
	int                  firstFrame;
	int                  decoderLocked;
	int                  decodeResult;
	TPlayer*             t;
    ANativeWindowBuffer* winBuf;
	cedarv_picture_t     picture;
	int64_t              frameDuration;
	int64_t              pts;
	AvTimer              timer;

	firstFrame = 1;
	decoderLocked = 0;
	t = (TPlayer*)arg;

	if(t->initDecoder() != 0)
		return (void*)0;

	if(t->mFrameRate == 0)
		frameDuration = 40000;
	else
		frameDuration = 1000*1000*1000/t->mFrameRate;

	//ALOGD("mFrameRate = %d", t->mFrameRate);

	//* open decoder.
	if(decoderLocked == 0)
	{
		//* lock decoder.
		ve_mutex_lock(&t->mCedarVRequestCtx);
		decoderLocked = 1;
	}

	t->mDecoder->ioctrl(t->mDecoder, CEDARV_COMMAND_SET_VBV_SIZE, 128*1024);
	t->mDecoder->ioctrl(t->mDecoder, CEDARV_COMMAND_SET_PIXEL_FORMAT, 1);
	ret = t->mDecoder->open(t->mDecoder);
	t->mDecoder->ioctrl(t->mDecoder, CEDARV_COMMAND_PLAY, 0);

	//ALOGD("open decoder return %d", ret);

	if(decoderLocked == 1)
	{
		//* unlock decoder.
		ve_mutex_unlock(&t->mCedarVRequestCtx);
		decoderLocked = 0;
	}

	while(1)
	{
		if(t->mStopFlag)
			break;

		if(t->mStatus == PLAYER_STATE_PLAYING)
		{

        	//* 1. check if there is a picture to output.
        	if(t->mDecoder->picture_ready(t->mDecoder))
        	{
        		//ALOGD("picture ready");
            	//* check if the picture size changed.
            	t->mDecoder->display_dump_picture(t->mDecoder, &picture);

            	heightAlign = (picture.display_height + 7) & (~7);
				widthAlign  = (picture.display_width + 15) & (~15);

				if(firstFrame == 1)
				{
					pthread_mutex_lock(&t->mBufferMutex);
					if(t->mStopFlag)
					{
						pthread_mutex_unlock(&t->mBufferMutex);
						break;
					}
				    native_window_set_usage(t->mNativeWindow.get(),
				                            GRALLOC_USAGE_SW_READ_NEVER  |
				                            GRALLOC_USAGE_SW_WRITE_OFTEN |
				                            GRALLOC_USAGE_HW_TEXTURE     |
				                            GRALLOC_USAGE_EXTERNAL_DISP);
				    native_window_set_scaling_mode(t->mNativeWindow.get(), NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);
				    native_window_set_buffers_geometry(t->mNativeWindow.get(), widthAlign, heightAlign, HAL_PIXEL_FORMAT_YV12);
				    native_window_set_buffers_transform(t->mNativeWindow.get(), 0);
					pthread_mutex_unlock(&t->mBufferMutex);

					firstFrame = 0;

					timer.SetTime(0);
					timer.SetSpeed(1000);
					timer.Start();
				}


				pthread_mutex_lock(&t->mBufferMutex);
				if(t->mStopFlag)
				{
					pthread_mutex_unlock(&t->mBufferMutex);
					break;
				}

				//* output a buffer.
				//ret = t->mNativeWindow->dequeueBuffer(t->mNativeWindow.get(), &winBuf);
                ret = t->mNativeWindow->dequeueBuffer_DEPRECATED(t->mNativeWindow.get(), &winBuf);
				if(ret != 0)
            	{
					pthread_mutex_unlock(&t->mBufferMutex);
            		usleep(5*1000);
					continue;
            	}

			    //t->mNativeWindow->lockBuffer(t->mNativeWindow.get(), winBuf);
                t->mNativeWindow->lockBuffer_DEPRECATED(t->mNativeWindow.get(), winBuf);

			    {
                	GraphicBufferMapper &mapper = GraphicBufferMapper::get();
                	Rect bounds(picture.width, picture.height);
				    mapper.lock(winBuf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst);
	            	t->mDecoder->display_request(t->mDecoder, &picture);
	            	memcpy(dst, (void*)cedarv_address_phy2vir(picture.y), picture.width*picture.height*3/2);
                    t->mDecoder->display_release(t->mDecoder, picture.id);
    			    mapper.unlock(winBuf->handle);
			    }

			    pthread_mutex_unlock(&t->mBufferMutex);

			    {
			    	pts = t->mFrameCount * frameDuration;
			    	if(pts > (timer.GetTime() + 1000))
			    		usleep(pts - timer.GetTime());
			    	else if(pts + 100000 < timer.GetTime())
			    		timer.SetTime(pts);
			    }

				pthread_mutex_lock(&t->mBufferMutex);
				if(t->mStopFlag)
				{
					pthread_mutex_unlock(&t->mBufferMutex);
					break;
				}
			    t->mNativeWindow->queueBuffer_DEPRECATED(t->mNativeWindow.get(), winBuf);
			    pthread_mutex_unlock(&t->mBufferMutex);

			    t->mFrameCount++;
                continue;
        	}
        	else
        	{
        		while(1)
        		{
        			if(decoderLocked == 0)
        			{
        				//* lock decoder.
        				ve_mutex_lock(&t->mCedarVRequestCtx);
                        //ALOGD("(f:%s l:%d) TPlayer", __FUNCTION__, __LINE__);
        				decoderLocked = 1;
        			}

        			decodeResult = t->mDecoder->decode(t->mDecoder);
                    //ALOGD("(f:%s l:%d) TPlayer", __FUNCTION__, __LINE__); //when hdmi plugin and plugout, thumbplayer will ...
    				if(decoderLocked == 1)
    				{
    					//* unlock decoder.
    					ve_mutex_unlock(&t->mCedarVRequestCtx);
            			decoderLocked = 0;
    				}

        			if(decodeResult == CEDARV_RESULT_FRAME_DECODED || decodeResult == CEDARV_RESULT_KEYFRAME_DECODED)
        			{
        				break;
        			}
        			else if(decodeResult == CEDARV_RESULT_NO_FRAME_BUFFER)
        			{
        				break;
        			}
        			else if(decodeResult == CEDARV_RESULT_NO_BITSTREAM)
        			{
                    	u8* pBuf0;
                    	u8* pBuf1;
                    	u32 size0;
                    	u32 size1;
                    	u32 require_size;
                    	u8* pData;
                    	cedarv_stream_data_info_t dataInfo;

                    	if(fread(&require_size, 1, sizeof(int), t->mFpStream) != sizeof(int))
                    	{
            				fseek(t->mFpStream, 0, SEEK_SET);
            				fread(&t->mFrameRate, 1, sizeof(int), t->mFpStream);
            				t->mFrameCount = 0;
            				continue;
                    	}

                    	ret = t->mDecoder->request_write(t->mDecoder, require_size, &pBuf0, &size0, &pBuf1, &size1);
                        if(ret != 0 || (size0 + size1 < require_size))
                        {
                    		if(decoderLocked == 0)
                    		{
                    			//* lock decoder.
                    			ve_mutex_lock(&t->mCedarVRequestCtx);
                    			decoderLocked = 1;
                    		}

                    		ALOGV("xxxxxxxx flush decoder.");
                    		t->mDecoder->ioctrl(t->mDecoder, CEDARV_COMMAND_FLUSH, 0);

            				if(decoderLocked == 1)
            				{
            					//* unlock decoder.
            					ve_mutex_unlock(&t->mCedarVRequestCtx);
                    			decoderLocked = 0;
            				}

            				fseek(t->mFpStream, 0, SEEK_SET);
            				fread(&t->mFrameRate, 1, sizeof(int), t->mFpStream);
            				t->mFrameCount = 0;
            				continue;
                        }

                        dataInfo.lengh = require_size;
                        dataInfo.flags = CEDARV_FLAG_FIRST_PART | CEDARV_FLAG_LAST_PART;
                        dataInfo.pts   = 0;

                        if(require_size <= size0)
                        {
                        	fread(pBuf0, 1, require_size, t->mFpStream);
                        }
                        else
                        {
                        	fread(pBuf0, 1, size0, t->mFpStream);
                        	fread(pBuf1, 1, require_size - size0, t->mFpStream);
                        }

                        t->mDecoder->update_data(t->mDecoder, &dataInfo);
                        break;
        			}
        			else if(decodeResult < 0)
        			{
                        break;
        			}
        		}
        	}

		}
		else if(t->mStatus == PLAYER_STATE_PAUSE)
		{
			usleep(10*1000);
		}
	}

	//* close decoder.
	if(decoderLocked == 0)
	{
		//* lock decoder.
		ve_mutex_lock(&t->mCedarVRequestCtx);
		decoderLocked = 1;
	}
	t->mDecoder->ioctrl(t->mDecoder, CEDARV_COMMAND_STOP, 0);
	t->mDecoder->close(t->mDecoder);
	if(decoderLocked == 1)
	{
		//* unlock decoder.
		ve_mutex_unlock(&t->mCedarVRequestCtx);
		decoderLocked = 0;
	}

	//* close native window.

	return (void*)0;
}


}  // namespace android











