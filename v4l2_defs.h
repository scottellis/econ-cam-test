/**********************************************************************************************************
 *   Copyright (C) 2007 by e-con Systems All Rights Reserved.                                             *
 *   www.e-consystems.com                                                                                 *
 *                                                                                                        *
 *   The source code contained or described herein and all documents                                      *
 *   related to the source code (Material) are owned by e-con Systems                                     *
 *                                                                                                        *
 *                                                                                                        *
 *   PROJECT	           :   COMMAND LINE V4L2 APPLICATION                                              *
 *   MODULE NAME           :   CMD_V4L2_FULL_FEATURE                                                      *
 *   MODULE VERSION        :   1.0                                                                        *
 *                                                                                                        *
 *                                                                                                        *
 *                                                                                                        *
 *   Version No	: 000-0001                                                          CODE_REV  : 1.0.0.0   *
 **********************************************************************************************************/

/*
 *==========================================================================================================
 *                                        REVISION HISTORY                                  
 *----------------------------------------------------------------------------------------------------------
 * CODE_REV  REASON FOR MODIFICATION                MODIFIED FUNCTION NAME  	            AUTHOR
 *----------------------------------------------------------------------------------------------------------
 * 1.0       -------------------------- code development --------------------          ANANTHAPADMANABAN
 *
 *
 *==========================================================================================================
 */

#ifndef V4L2_DEFS_H
#define V4L2_DEFS_H

#include <linux/videodev.h>

#define V4L2_SENS_TRIG_FOCUS			(V4L2_CID_PRIVATE_BASE + 1)
#define V4L2_SENS_FCS_OLAY			(V4L2_CID_PRIVATE_BASE + 2)

#define V4L2_SENS_FLASH				(V4L2_CID_PRIVATE_BASE + 3)
#define V4L2_SENS_FLASH_LUM			(V4L2_CID_PRIVATE_BASE + 4)
#define V4L2_SENS_FLASH_TORCH			(V4L2_CID_PRIVATE_BASE + 5)
#define V4L2_SENS_FLASH_FLASH			(V4L2_CID_PRIVATE_BASE + 6)
#define V4L2_SENS_FLASH_STROBE			(V4L2_CID_PRIVATE_BASE + 7)
#define V4L2_SENS_FLASH_FLASH_LUM		(V4L2_CID_PRIVATE_BASE + 10)
#define V4L2_SENS_FLASH_TORCH_LUM		(V4L2_CID_PRIVATE_BASE + 11)

#define V4L2_SENS_EFFECTS			(V4L2_CID_PRIVATE_BASE + 8)
#define V4L2_SENS_FOCUS_DISABLE			(V4L2_CID_PRIVATE_BASE + 9)

#define V4L2_SENS_ANTISHAKE			(V4L2_CID_PRIVATE_BASE + 12)
#define V4L2_SENS_ANTISHAKE_STATUS		(V4L2_CID_PRIVATE_BASE + 13)

#endif 
