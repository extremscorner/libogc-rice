/*-------------------------------------------------------------

video_types.h -- support header

Copyright (C) 2004
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/


#ifndef __VIDEO_TYPES_H__
#define __VIDEO_TYPES_H__

/*!
\file video_types.h
\brief support header
*/

#include <gctypes.h>

/*!
 * \addtogroup vi_defines List of defines used for the VIDEO subsystem
 * @{
 */


#define VI_DISPLAY_PIX_SZ           2		/*!< multiplier to get real pixel size in bytes */

#define VI_DISPLAY_BOTH				0x3
#define VI_DISPLAY_TV				0x2
#define VI_DISPLAY_GAMEPAD			0x1

#define VI_ASPECT_1_1				0x4
#define VI_ASPECT_3_4				0x2

/*!
 * \addtogroup vi_modetypedef VIDEO mode types
 * @{
 */

#define VI_CLOCK_27MHZ              (0 << 0)
#define VI_CLOCK_54MHZ              (1 << 0)
#define VI_STANDARD                 (0 << 1)
#define VI_ENHANCED                 (1 << 1)
#define VI_INTERLACE                (0 << 2)
#define VI_NON_INTERLACE            (1 << 2)
#define VI_MONO                     (0 << 3)
#define VI_STEREO                   (1 << 3)

/*!
 * @}
 */


/*!
 * \addtogroup vi_standardtypedef VIDEO standard types
 * @{
 */

#define VI_NTSC                     0		/*!< Video standard used in North America and Japan */
#define VI_PAL                      1		/*!< Video standard used in Europe */
#define VI_MPAL                     2		/*!< Video standard, similar to NTSC, used in Brazil */
#define VI_DEBUG                    3		/*!< Video standard, for debugging purpose, used in North America and Japan. Special decoder needed */
#define VI_DEBUG_PAL                4		/*!< Video standard, for debugging purpose, used in Europe. Special decoder needed */
#define VI_EURGB60                  5		/*!< RGB 60Hz, 480 lines mode (same timing and aspect ratio as NTSC) used in Europe */
#define VI_CUSTOM                   6

/*!
 * @}
 */


#define VI_XFBMODE_SF				0
#define VI_XFBMODE_DF				1
#define VI_XFBMODE_PSF				2


/*!
 * \addtogroup vi_fielddef VIDEO field types
 * @{
 */

#define VI_FIELD_ABOVE              1		/*!< Upper field in DS mode */
#define VI_FIELD_BELOW              0		/*!< Lower field in DS mode */

/*!
 * @}
 */


// Maximum screen space
#define VI_MAX_WIDTH_NTSC           720
#define VI_MAX_HEIGHT_NTSC          486

#define VI_MAX_WIDTH_PAL            720
#define VI_MAX_HEIGHT_PAL           576

#define VI_MAX_WIDTH_MPAL           720
#define VI_MAX_HEIGHT_MPAL          486

#define VI_MAX_WIDTH_EURGB60        VI_MAX_WIDTH_NTSC
#define VI_MAX_HEIGHT_EURGB60       VI_MAX_HEIGHT_NTSC

#define VIDEO_PadFramebufferWidth(width)     ((u16)(((u16)(width) + 15) & ~15))			/*!< macro to pad the width to a multiple of 16 */

/*!
 * @}
 */


#define VI_TVMODE(fmt, mode)        (((fmt) << 4) | (mode))

#define VI_TVMODE_NTSC_INT			VI_TVMODE(VI_NTSC,      VI_MONO | VI_INTERLACE     | VI_STANDARD | VI_CLOCK_27MHZ)
#define VI_TVMODE_NTSC_DS			VI_TVMODE(VI_NTSC,      VI_MONO | VI_NON_INTERLACE | VI_STANDARD | VI_CLOCK_27MHZ)
#define VI_TVMODE_NTSC_PROG			VI_TVMODE(VI_NTSC,      VI_MONO | VI_NON_INTERLACE | VI_ENHANCED | VI_CLOCK_54MHZ)

#define VI_TVMODE_PAL_INT			VI_TVMODE(VI_PAL,       VI_MONO | VI_INTERLACE     | VI_STANDARD | VI_CLOCK_27MHZ)
#define VI_TVMODE_PAL_DS			VI_TVMODE(VI_PAL,       VI_MONO | VI_NON_INTERLACE | VI_STANDARD | VI_CLOCK_27MHZ)
#define VI_TVMODE_PAL_PROG			VI_TVMODE(VI_PAL,       VI_MONO | VI_NON_INTERLACE | VI_ENHANCED | VI_CLOCK_54MHZ)

#define VI_TVMODE_MPAL_INT			VI_TVMODE(VI_MPAL,      VI_MONO | VI_INTERLACE     | VI_STANDARD | VI_CLOCK_27MHZ)
#define VI_TVMODE_MPAL_DS			VI_TVMODE(VI_MPAL,      VI_MONO | VI_NON_INTERLACE | VI_STANDARD | VI_CLOCK_27MHZ)
#define VI_TVMODE_MPAL_PROG			VI_TVMODE(VI_MPAL,      VI_MONO | VI_NON_INTERLACE | VI_ENHANCED | VI_CLOCK_54MHZ)

#define VI_TVMODE_DEBUG_INT			VI_TVMODE(VI_DEBUG,     VI_MONO | VI_INTERLACE     | VI_STANDARD | VI_CLOCK_27MHZ)

#define VI_TVMODE_DEBUG_PAL_INT		VI_TVMODE(VI_DEBUG_PAL, VI_MONO | VI_INTERLACE     | VI_STANDARD | VI_CLOCK_27MHZ)
#define VI_TVMODE_DEBUG_PAL_DS		VI_TVMODE(VI_DEBUG_PAL, VI_MONO | VI_NON_INTERLACE | VI_STANDARD | VI_CLOCK_27MHZ)
#define VI_TVMODE_DEBUG_PAL_PROG	VI_TVMODE(VI_DEBUG_PAL, VI_MONO | VI_NON_INTERLACE | VI_ENHANCED | VI_CLOCK_54MHZ)

#define VI_TVMODE_EURGB60_INT		VI_TVMODE(VI_EURGB60,   VI_MONO | VI_INTERLACE     | VI_STANDARD | VI_CLOCK_27MHZ)
#define VI_TVMODE_EURGB60_DS		VI_TVMODE(VI_EURGB60,   VI_MONO | VI_NON_INTERLACE | VI_STANDARD | VI_CLOCK_27MHZ)
#define VI_TVMODE_EURGB60_PROG		VI_TVMODE(VI_EURGB60,   VI_MONO | VI_NON_INTERLACE | VI_ENHANCED | VI_CLOCK_54MHZ)

#define VI_TVMODE_CUSTOM_INT		VI_TVMODE(VI_CUSTOM,    VI_MONO | VI_INTERLACE     | VI_STANDARD | VI_CLOCK_27MHZ)
#define VI_TVMODE_CUSTOM_DS			VI_TVMODE(VI_CUSTOM,    VI_MONO | VI_NON_INTERLACE | VI_STANDARD | VI_CLOCK_27MHZ)
#define VI_TVMODE_CUSTOM_PROG		VI_TVMODE(VI_CUSTOM,    VI_MONO | VI_NON_INTERLACE | VI_ENHANCED | VI_CLOCK_54MHZ)


/*!
 * \addtogroup vi_defines List of defines used for the VIDEO subsystem
 * @{
 */


/*!
 * \addtogroup gxrmode_obj VIDEO render modes
 * @{
 */

extern GXRModeObj TVNtsc240Ds;				/*!< Video and render mode configuration for 240 lines,singlefield NTSC mode */
extern GXRModeObj TVNtsc240DsAa;			/*!< Video and render mode configuration for 240 lines,singlefield,antialiased NTSC mode */
extern GXRModeObj TVNtsc243Ds;
extern GXRModeObj TVNtsc480Ds;
extern GXRModeObj TVNtsc240Int;				/*!< Video and render mode configuration for 240 lines,interlaced NTSC mode */
extern GXRModeObj TVNtsc240IntAa;			/*!< Video and render mode configuration for 240 lines,interlaced,antialiased NTSC mode */
extern GXRModeObj TVNtsc480Int;				/*!< Video and render mode configuration for 480 lines,interlaced NTSC mode */
extern GXRModeObj TVNtsc480IntDf;			/*!< Video and render mode configuration for 480 lines,interlaced,doublefield NTSC mode */
extern GXRModeObj TVNtsc480IntAa;			/*!< Video and render mode configuration for 480 lines,interlaced,doublefield,antialiased NTSC mode */
extern GXRModeObj TVNtsc486Int;
extern GXRModeObj TVNtsc486IntDf;
extern GXRModeObj TVNtsc480Prog;			/*!< Video and render mode configuration for 480 lines,progressive,singlefield NTSC mode */
extern GXRModeObj TVNtsc480ProgSoft;
extern GXRModeObj TVNtsc480ProgAa;
extern GXRModeObj TVNtsc486Prog;
extern GXRModeObj TVMpal240Ds;
extern GXRModeObj TVMpal240DsAa;
extern GXRModeObj TVMpal243Ds;
extern GXRModeObj TVMpal480Ds;
extern GXRModeObj TVMpal240Int;
extern GXRModeObj TVMpal240IntAa;
extern GXRModeObj TVMpal480Int;
extern GXRModeObj TVMpal480IntDf;			/*!< Video and render mode configuration for 480 lines,interlaced,doublefield,antialiased MPAL mode */
extern GXRModeObj TVMpal480IntAa;
extern GXRModeObj TVMpal486Int;
extern GXRModeObj TVMpal486IntDf;
extern GXRModeObj TVMpal480Prog;
extern GXRModeObj TVMpal480ProgSoft;
extern GXRModeObj TVMpal480ProgAa;
extern GXRModeObj TVMpal486Prog;
extern GXRModeObj TVPal264Ds;				/*!< Video and render mode configuration for 264 lines,singlefield PAL mode */
extern GXRModeObj TVPal264DsAa;				/*!< Video and render mode configuration for 264 lines,singlefield,antialiased PAL mode */
extern GXRModeObj TVPal288Ds;
extern GXRModeObj TVPal288DsAaScale;
extern GXRModeObj TVPal576Ds;
extern GXRModeObj TVPal576DsScale;
extern GXRModeObj TVPal264Int;				/*!< Video and render mode configuration for 264 lines,interlaced PAL mode */
extern GXRModeObj TVPal264IntAa;			/*!< Video and render mode configuration for 264 lines,interlaced,antialiased PAL mode */
extern GXRModeObj TVPal288Int;
extern GXRModeObj TVPal528Int;				/*!< Video and render mode configuration for 528 lines,interlaced PAL mode */
extern GXRModeObj TVPal528IntDf;			/*!< Video and render mode configuration for 528 lines,interlaced,doublefield PAL mode */
extern GXRModeObj TVPal524IntAa;			/*!< Video and render mode configuration for 524 lines,interlaced,doublefield,antialiased PAL mode */
extern GXRModeObj TVPal576Int;
extern GXRModeObj TVPal576IntScale;
extern GXRModeObj TVPal576IntDf;
extern GXRModeObj TVPal576IntDfScale;
extern GXRModeObj TVPal528Prog;
extern GXRModeObj TVPal528ProgSoft;
extern GXRModeObj TVPal524ProgAa;
extern GXRModeObj TVPal576Prog;
extern GXRModeObj TVPal576ProgScale;
extern GXRModeObj TVEurgb60Hz240Ds;
extern GXRModeObj TVEurgb60Hz240DsAa;
extern GXRModeObj TVEurgb60Hz243Ds;
extern GXRModeObj TVEurgb60Hz480Ds;
extern GXRModeObj TVEurgb60Hz240Int;
extern GXRModeObj TVEurgb60Hz240IntAa;
extern GXRModeObj TVEurgb60Hz480Int;
extern GXRModeObj TVEurgb60Hz480IntDf;
extern GXRModeObj TVEurgb60Hz480IntAa;
extern GXRModeObj TVEurgb60Hz486Int;
extern GXRModeObj TVEurgb60Hz486IntDf;
extern GXRModeObj TVEurgb60Hz480Prog;
extern GXRModeObj TVEurgb60Hz480ProgSoft;
extern GXRModeObj TVEurgb60Hz480ProgAa;
extern GXRModeObj TVEurgb60Hz486Prog;

/*!
 * @}
 */

/*!
 * @}
 */

#endif
