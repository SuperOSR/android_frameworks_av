/*
********************************************************************************
*                                    Android
*                               CedarX sub-system
*                               CedarXAndroid module
*
*          (c) Copyright 2010-2013, Allwinner Microelectronic Co., Ltd.
*                              All Rights Reserved
*
* File   : virtual_hwcomposer.h
* Version: V1.0
* By     : Eric_wang
* Date   : 2012-2-26
* Description:
********************************************************************************
*/
#ifndef _VIRTUAL_HWCOMPOSER_H_
#define _VIRTUAL_HWCOMPOSER_H_

#include <CDX_PlayerAPI.h>
#include <libcedarv.h>

namespace android {

typedef struct tag_VIRTUALLIBHWCLAYERPARA
{
    unsigned long   number;             //frame_id

    cedarv_3d_mode_e            source3dMode;   //cedarv_3d_mode_e, CEDARV_3D_MODE_DOUBLE_STREAM
    cedarx_display_3d_mode_e    displayMode;    //cedarx_display_3d_mode_e, CEDARX_DISPLAY_3D_MODE_3D
    cedarv_pixel_format_e       pixel_format;   //cedarv_pixel_format_e, CEDARV_PIXEL_FORMAT_MB_UV_COMBINE_YUV420
    
    unsigned long   top_y;              // the address of frame buffer, which contains top field luminance
    unsigned long   top_u;  //top_c;              // the address of frame buffer, which contains top field chrominance
    unsigned long   top_v;
    unsigned long   top_y2;
    unsigned long   top_u2;
    unsigned long   top_v2;

    unsigned long   size_top_y;
    unsigned long   size_top_u;
    unsigned long   size_top_v;
    unsigned long   size_top_y2;
    unsigned long   size_top_u2;
    unsigned long   size_top_v2;
    
    signed char     bProgressiveSrc;    // Indicating the source is progressive or not
    signed char     bTopFieldFirst;     // VPO should check this flag when bProgressiveSrc is FALSE
    unsigned long   flag_addr;          //dit maf flag address
    unsigned long   flag_stride;        //dit maf flag line stride
    unsigned char   maf_valid;
    unsigned char   pre_frame_valid;
}Virtuallibhwclayerpara;   //libhwclayerpara_t, [hwcomposer.h]

#if (defined(__CHIP_VERSION_F23) || defined(__CHIP_VERSION_F51) || (defined(__CHIP_VERSION_F33) && CEDARX_ANDROID_VERSION<8))
extern int convertlibhwclayerpara_NativeRendererVirtual2Arch(libhwclayerpara_t *pDes, Virtuallibhwclayerpara *pSrc);
extern int convertlibhwclayerpara_SoftwareRendererVirtual2Arch(libhwclayerpara_t *pDes, Virtuallibhwclayerpara *pSrc);
#elif ((defined(__CHIP_VERSION_F33) && CEDARX_ANDROID_VERSION>=9))
#else
    #error "Unknown chip type!"
#endif
}
#endif  /* _VIRTUAL_HWCOMPOSER_H_ */

