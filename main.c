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
 * 1.0       INITIAL RELEASE                                                        ANANTHAPADMANABAN
 *
 * 2.0       Flash support added
 *
 *==========================================================================================================
 */


#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>	
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <asm/types.h>
#include <sys/mman.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <linux/fb.h>

#include "v4l2_defs.h"
#include "feature_test.h"

#define MENU_EXIT	100

int controls_menu(struct camera_data *cam);
int paint_main_menu();
int get_user_option(const char *prompt);


int main(int argc, char* argv[])
{
	int menu_option;
	struct camera_data *cam;
	int result;
	unsigned long long time;
	unsigned int count;
	char still_file[256];
	int read_strem_temp_gflag;

	result	= feature_test_api_init(&cam);
	if (result < 0)
		return -1;

	for(;;) {
		paint_main_menu();
		printf(" 1  Snap mode\n");
		printf(" 2  Record mode\n");
		printf(" 3  Streaming mode\n");
		printf(" 4  Controls\n");
		printf(" X  Exit\n");

		menu_option = get_user_option(NULL);

		if (menu_option == MENU_EXIT)
			break;
		
		switch (menu_option) {
		case 1:
			result	= get_camera_format(cam);
			if (result < 0)
				goto exit;
			
			cam->fmt_need = cam->fmt;
			cam->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
			cam->fmt.fmt.pix.bytesperline = cam->fmt.fmt.pix.width * 2;

			read_strem_temp_gflag = cam->stream.G_FLAG;
			cam->stream.G_FLAG = 0;
			result	= stream_off(cam);
			if (result < 0)
				goto exit;

			for(;;) {
				printf("\n=============================================\n");
				printf(" Snap Menu\n");
				printf("=============================================\n");
				printf(" 1  Snap picture\n");
				printf(" 2  Change dimensions\n");		
				printf(" 3  Set pixel format\n");
				printf(" X  Exit snap menu\n");

				if (menu_option == 2) {
					result	= get_camera_format(cam);
					if (result < 0)
						goto exit;

					printf("\nCurrent settings:\n");	
					printf("Width  : %d\n", cam->fmt.fmt.pix.width);
					printf("Height : %d\n", cam->fmt.fmt.pix.height);
				}

				printf("=============================================\n");

				menu_option = get_user_option(NULL);

				if(menu_option == MENU_EXIT) 
					break;

				switch (menu_option) {
				case 1:
					if ((cam->fmt_need.fmt.pix.width > 1280 && cam->fmt_need.fmt.pix.width < 2048) 
						|| (cam->fmt_need.fmt.pix.height > 1024 && cam->fmt_need.fmt.pix.height < 1536)) {
						cam->fmt.fmt.pix.width	= 2048;
						cam->fmt.fmt.pix.height	= 1536;
					}
					
					cam->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
					cam->fmt.fmt.pix.priv = 0;
					
					if (cam->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_SBGGR8)
						cam->fmt.fmt.pix.bytesperline = cam->fmt.fmt.pix.width;
					else
						cam->fmt.fmt.pix.bytesperline = cam->fmt.fmt.pix.width * 2;

					cam->fmt.fmt.pix.sizeimage = cam->fmt.fmt.pix.bytesperline * cam->fmt.fmt.pix.height * 2;

					result	= snap_apply_ctrl(cam);
					if (result < 0)
						goto exit;


					gettime(&time, RESET_TIME);

					result	= take_snap(cam);
					if (result < 0)
						goto exit;

					gettime(&time, 0);
					printf("Read completed %llu ms\n", time); 
					
					result	= save_snap(cam);
					if (result < 0)
						goto exit;
					
					break;

				case 2:
					cam->fmt_need.fmt.pix.width = get_user_option("Enter width");
					cam->fmt_need.fmt.pix.height = get_user_option("Enter height");
					cam->fmt.fmt.pix.width	= cam->fmt_need.fmt.pix.width;
					cam->fmt.fmt.pix.height	= cam->fmt_need.fmt.pix.height;
					break;

				case 3:
					for(;;) {						
						printf("=============================================\n");
						printf(" Format Menu\n");
						printf("=============================================\n");
						printf(" 1  Normal mode (RAW file not saved)\n");
						printf(" 2  RGB565 mode\n");
						printf(" 3  UYVY mode\n");
						printf(" 4  YUYV mode\n");
						printf(" 5  BAYER BGGR mode\n");
						printf(" X  Exit format menu\n");
						printf("=============================================\n");

						menu_option = get_user_option(NULL);

						if (menu_option == MENU_EXIT)
							break;

						switch (menu_option) {
						case 1:							
							cam->save_raw_file_needed = 0;
							cam->fmt.fmt.pix.pixelformat =	V4L2_PIX_FMT_RGB565;
							break;
						
						case 2:
							cam->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
							break;
							
						case 3:
							cam->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
							break;
							
						case 4:
							cam->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
							break;
						
						case 5:
							cam->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SBGGR8;
							break;
							
						default:
							printf("\n\t\t\tInvalid option \n");
							continue;
						}
	
						if (menu_option != 1)
							cam->save_raw_file_needed = 1;

						result	= try_fmt(cam);
						if (result < 0) {
							printf("\n\tProvided format not supported by camera \n");
							continue;
						}
					}

					break;

				default:
					printf("\nInvalid option\n");
					continue;
				}

				cam->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				cam->fmt.fmt.pix.priv = 0;
				cam->fmt.fmt.pix.sizeimage = cam->fmt.fmt.pix.width * cam->fmt.fmt.pix.height * 2;

				result	= snap_apply_ctrl(cam);
				if (result < 0)
					goto exit;
				
				cam->stream.G_FLAG = read_strem_temp_gflag;
				
				result	= stream_on(cam);
				if (result < 0)
					goto exit;				
			}
			
			break;

		case 2:
			cam->fmt.fmt.pix.width	= 320;
			cam->fmt.fmt.pix.height	= 240;

			for(;;) {
				printf("\n=============================================\n");
				printf(" Recording Menu\n");
				printf("=============================================\n");
				printf(" 1  Choose recording dimension\n");
				printf(" 2  Start recording\n");
				printf(" 3  Stop recording\n");
				printf(" X  Exit recording menu\n");
				printf("=============================================\n");

				menu_option = get_user_option(NULL);
				if (menu_option	== MENU_EXIT)
					break;

				switch (menu_option) {
				case 1:
					printf("\n=============================================\n");
					printf(" Recording Dimension Menu\n");
					printf("=============================================\n");
					printf(" 1  320 x 240\n");
					printf(" 2  640 x 480\n");
					printf(" X  Exit dimension menu\n");
					printf("=============================================\n");

					menu_option = get_user_option(NULL);
					if (menu_option	== MENU_EXIT)
						break;
					
					switch (menu_option) {
					case 1:
						cam->fmt.fmt.pix.width	= 320;
						cam->fmt.fmt.pix.height	= 240;
						break;
						
					case 2:
						cam->fmt.fmt.pix.width	= 640;
						cam->fmt.fmt.pix.height	= 480;
						break;
							
					default:
						printf("Invalid option\n");
					}
					
					break;

				case 2:
					if (cam->stream.bit.stream_lcd == 1) {
						cam->stream.bit.stream_lcd = 0;
						result	= stream_off(cam);
						if (result < 0)
							goto exit;
					}

					for (count = 0; ; count++) {
						sprintf(still_file, "video_%dx%d_%d.uyvy",
							cam->fmt.fmt.pix.width,
							cam->fmt.fmt.pix.height,
							count);
							
						if ((cam->fp_file_record = fopen(still_file, "r")) == NULL) {
							cam->fp_file_record = fopen(still_file, "w");
							if (cam->fp_file_record == NULL) {
								gettime(&time, 0);
								printf("\nRecord failed time stamp.... %llu\n", time); 
								continue;
							}
							
							break;
						}
						
						fclose(cam->fp_file_record);
						cam->fp_file_record = NULL;
					}
					
					cam->stream.bit.record_mode = 1;
					
					result	= stream_on(cam);
					if (result < 0)
						goto exit;
					
					break;

				case 3:
					cam->stream.bit.record_mode = 0;
	
					result	= stream_off(cam);
					if (result < 0)
						goto exit;
						
					if (cam->fp_file_record) {
						fclose(cam->fp_file_record);
						cam->fp_file_record = NULL;
					}
				
					break;
					
				}		
			}
			
			break;

		case 3:
			if (cam->stream.bit.record_mode	== 1) {
				cam->stream.bit.record_mode = 0;
	
				result	= stream_off(cam);
				if (result < 0)
					goto exit;
	
				if (cam->fp_file_record) {
					fclose(cam->fp_file_record);
					cam->fp_file_record = NULL;
				}
			}

			for (;;) {
				printf("\n=============================================\n");
				printf(" Streaming Menu\n");
				printf("=============================================\n");
				printf(" 1  Start Streaming\n");
				printf(" 2  Stop Streaming\n");
				printf(" X  Exit streaming menu\n");
				printf("=============================================\n");

				menu_option = get_user_option(NULL);
				if (menu_option == MENU_EXIT)
					break;

				switch (menu_option) {
				case 1:
					cam->stream.bit.stream_lcd = 1;
					result	= stream_on(cam);
					if (result < 0)
						goto exit;
						
					break;
						
				case 2:
					cam->stream.bit.stream_lcd = 0;
					result	= stream_off(cam);
					if (result < 0)
						goto exit;
						
					break;

				default:
					printf("\nInvalid option\n");
					continue;
				}
				
				cam->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				cam->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
			        cam->fmt.fmt.pix.bytesperline = cam->fmt.fmt.pix.width * 2;
				cam->fmt.fmt.pix.priv = 0;
				cam->fmt.fmt.pix.sizeimage = cam->fmt.fmt.pix.width * cam->fmt.fmt.pix.height * 2;

				result	= snap_apply_ctrl(cam);
				if (result < 0)
					goto exit;

			}
			
			break;

		case 4:
			result = controls_menu(cam);
			if (result < 0)
				goto exit;

			break;

		default:
			printf("\nInvalid option\n");
			continue;

		}
	}

exit:
	close_camera(cam);		

	return 0;
}

int controls_menu(struct camera_data *cam)
{
	int menu_option, val, result;
	struct v4l2_queryctrl query;
	
	for (;;) {
		printf("\n=============================================\n");
		printf(" Controls Menu\n");
		printf("=============================================\n");
		printf(" 1   Brightness\n");
		printf(" 2   Contrast\n");
		printf(" 3   Saturation\n");
		printf(" 4   White Balance\n");
		printf(" 5   Exposure\n");
		printf(" 6   v_flip\n");
		printf(" 7   h_mirror\n");
		printf(" 8   Sharpness\n");
		printf(" 9   Focus\n");
		printf("10   Effects\n");
		printf("11   Flash\n");
		printf("12   Anti-shake\n");
		printf("13   Frame-rate\n");
		printf(" X   Exit controls menu\n");
		printf("=============================================\n");

		menu_option = get_user_option(NULL);
		if (menu_option == MENU_EXIT)
			break;
		
		switch (menu_option) {
		case 1:
			for (;;) {
				query.id = V4L2_CID_BRIGHTNESS;
				
				if (query_control(&query) < 0)
					goto exit;

 				if (read_control(V4L2_CID_BRIGHTNESS, &val) < 0)
					goto exit;
				
				printf("\n===============================================================\n");
				printf(" Brightness Menu\n");
				printf("===============================================================\n");

				printf(" 1  Change brightness level\tRange %d to %d stepsize: %d\n",
					query.minimum, query.maximum, query.step);

				printf(" X  Exit brightness menu\n");
				printf("\n Current brightness: %d\n", val);
				printf("========================================================\n");

				menu_option = get_user_option(NULL);
				if (menu_option == MENU_EXIT)
					break;
				
				if (menu_option == 1) {
					val = get_user_option("Enter brightness value");

					if (write_control(V4L2_CID_BRIGHTNESS, val) < 0)
						goto exit;
				}	
				else {	
					printf("\nInvalid option\n");
					continue;					
				}
			}

			break;

		case 2:
			for(;;) {
				query.id = V4L2_CID_CONTRAST;
				
				if (query_control(&query) < 0)
					goto exit;

 				if (read_control(V4L2_CID_CONTRAST, &val) < 0)
					goto exit;

				printf("\n===============================================================\n");
				printf(" Contrast Menu            Range %d to %d stepsize: %d\n",
					query.minimum, query.maximum, query.step);

				printf("===============================================================\n");
				printf(" 1  Change contrast level\n");
				printf(" X  Exit contrast menu\n");
				printf("\n Current contrast: %d\n", val);
				printf("==============================================================\n");

				menu_option = get_user_option(NULL);
				if (menu_option == MENU_EXIT)
					break;
				
				if (menu_option == 1) {
					val = get_user_option("Enter contrast value");
					
					if (write_control(V4L2_CID_CONTRAST, val) < 0)
						goto exit;
				}
				else {
					printf("\nInvalid option\n");
					continue;
				}
			}

			break;

		case 3:
			for (;;) {
				query.id = V4L2_CID_SATURATION;

				if (query_control(&query) < 0)
					goto exit;

 				if (read_control(V4L2_CID_SATURATION, &val) < 0)
					goto exit;
				
				printf("\n===============================================================\n");
				printf(" Saturation Menu             Range %d to %d stepsize: %d\n",
					query.minimum, query.maximum, query.step);
				printf("===============================================================\n");
				printf(" 1  Choose saturation level\n");
				printf(" X  Exit saturation menu\n");
				
				printf("\n Current saturation: %d\n", val);
				printf("========================================================\n");

				menu_option = get_user_option(NULL);			
				if (menu_option == MENU_EXIT)
					break;
				
				if (menu_option == 1) {
					val = get_user_option("Enter saturation value");
					
					if (write_control(V4L2_CID_SATURATION, val) < 0)
						goto exit;
				}
				else {
					printf("\nInvalid option \n");
					continue;
				}
			}

			break;

		case 4:
			for(;;) {
				printf("\n================================================================\n");
				printf(" White Balance Menu               Range 3000K to 5500K \n");
				printf("================================================================\n");
				printf(" 1  Auto white balance mode\n");
				printf(" 2  Temperature control           Incandesent 3000K \n");
				printf("                                  Flouresent  4000K \n");
				printf("                                  Cloudy      5000K \n");
				printf("                                  Sunny       5500K \n");
				printf(" X  Exit white balance  menu\n");
				printf("================================================================\n");

				menu_option = get_user_option(NULL);
				if (menu_option == MENU_EXIT)
					break;

				switch (menu_option) {
				case 1:
					val = get_user_option("Enable auto white balance (0 or 1)");
					
					if (write_control(V4L2_CID_AUTO_WHITE_BALANCE, val) < 0)
						goto exit;

					break;

				case 2:
					val = get_user_option("Enter white balance temperature");
					
					if (write_control(V4L2_CID_WHITE_BALANCE_TEMPERATURE, val) < 0)
						goto exit;

					break;

				default:
					printf("\nInvalid option\n");
					continue;
				}
			}

			break;

		case 5:
			for(;;) {
				query.id = V4L2_CID_EXPOSURE;
				
				if (query_control(&query) < 0)
					goto exit;

 				if (read_control(V4L2_CID_EXPOSURE, &val) < 0)
					goto exit;
				
				printf("\n===============================================================\n");
				printf(" Exposure Menu             Range %d to %d stepsize: %d\n",
					query.minimum, query.maximum, query.step);
				printf("===============================================================\n");

				printf(" 1  Choose exposure level\n");
				printf(" X  Exit exposure menu\n");	

				printf("\n Current exposure: %d\n", val);
				printf("========================================================\n");

				menu_option = get_user_option(NULL);
				if (menu_option == MENU_EXIT)
					break;
				
				if (menu_option == 1) {
					val = get_user_option("Enter exposure value");
					
					if (write_control(V4L2_CID_EXPOSURE, val) < 0)
						goto exit;
				}		
				else {
					printf("\nInvalid input\n");
					continue;
				}
			}

			break;

		case 6:
			for (;;) {
				query.id = V4L2_CID_VFLIP;
				
				if (query_control(&query) < 0)
					goto exit;

 				if (read_control(V4L2_CID_VFLIP, &val) < 0)
					goto exit;
				
				printf("\n========================================================\n");
				printf(" Vertical Flip Menu\n");
				printf("========================================================\n");
				printf(" 1  Enable vertical flip\n");
				printf(" 2  Disable vertical flip\n");
				printf(" X  Exit vertical flip menu\n");
				
				printf("\n Current vertical flip: %d\n", val);
				printf("========================================================\n");

				menu_option = get_user_option(NULL);
				if (menu_option == MENU_EXIT)
					break;
				
				if (menu_option == 1) {
					if (write_control(V4L2_CID_VFLIP, 1) < 0)
						goto exit;
				}
				else if (menu_option == 2) {
					if (write_control(V4L2_CID_VFLIP, 0) < 0)
						goto exit;
				}
				else {
					printf("\nInvalid option\n");
					continue;
				}
			}

			break;

		case 7:
			for(;;) {
				query.id = V4L2_CID_HFLIP;

				if (query_control(&query) < 0)
					goto exit;

 				if (read_control(V4L2_CID_HFLIP, &val) < 0)
					goto exit;
				
				printf("\n=============================================================\n");
				printf("Horizontal Mirror Menu\n");
				printf("=============================================================\n");
				printf(" 1  Enable horizontal mirror\n");
				printf(" 2  Disable horizontal mirror\n");
				printf(" X  Exit horizontal mirror menu\n");
				
				printf("\n Current horizontal mirror: %d\n", val);
				printf("=============================================================\n");

				menu_option = get_user_option(NULL);
				if (menu_option == MENU_EXIT)
					break;
				
				if (menu_option == 1) {
					if (write_control(V4L2_CID_HFLIP, 1) < 0)
						goto exit;
				}
				else if (menu_option == 2) {
					if (write_control(V4L2_CID_HFLIP, 0) < 0)
						goto exit;
				}
				else {
					printf("\nInvalid option\n");
					continue;
				}
			}

			break;

		case 8:
			for (;;) {
				query.id = V4L2_CID_SHARPNESS;
				
				if (query_control(&query) < 0)
					goto exit;

				if (read_control(V4L2_CID_SHARPNESS, &val) < 0)
					goto exit;
				
				printf("\n===============================================================\n");
				printf(" Sharpness Menu             Range %d to %d stepsize: %d\n",
					query.minimum, query.maximum, query.step);
				
				printf("===============================================================\n");
				printf(" 1  Choose sharpness level\n");
				printf(" X  Exit sharpness menu\n");			
				printf("\nCurrent sharpness %d\n", val);
				printf("========================================================\n");

				menu_option = get_user_option(NULL);
				if (menu_option == MENU_EXIT)
					break;
				
				if (menu_option == 1) {
					val = get_user_option("Enter sharpness value");	
				
					if (write_control(V4L2_CID_SHARPNESS, val) < 0)
						goto exit;
				}
				else {
					printf("\nInvalid option\n");
					continue;
				}
			}

			break;

		case 9:
			for (;;) {
				printf("\n=================================================================\n");
				printf(" Focus Menu                 Range\n");
				printf("=================================================================\n");
				printf(" 1  Absolute focus mode     0 - 1023 steps\n");
				printf(" 2  Relative focus mode     +ve step size move forward\n");
				printf("                            -ve step size move backward\n");
				printf(" 3  Automatic focus\n");
				printf(" 4  Trigger focus\n");
				printf(" 5  Enable focus overlay box\n");
				printf(" X  Exit focus menu\n");
				printf("=================================================================\n");

				menu_option = get_user_option(NULL);
				if (menu_option == MENU_EXIT)
					break;
			
				switch (menu_option) {
				case 1:
					val = get_user_option("Enter focus value");
					
					if (write_control(V4L2_CID_FOCUS_ABSOLUTE, val) < 0)
						goto exit;

					break;
				
				case 2:
					val = get_user_option("Enter relative focus value");
					
					if (write_control(V4L2_CID_FOCUS_RELATIVE, val) < 0)
						goto exit;

					break;

				case 3:
					if (write_control(V4L2_CID_FOCUS_AUTO, 0) < 0)
						goto exit;
					break;

				case 4:
					if (write_control(V4L2_SENS_TRIG_FOCUS, 0) < 0)
						goto exit;

					break;

				case 5:
					val = get_user_option("Enable focus overlay box (0 or 1)");
					
					if (write_control(V4L2_SENS_FCS_OLAY, val) < 0)
						goto exit;

					break;

				default:
					printf("\nInvalid option \n");
					continue;
				}
			}

			break;

		case 10:
			for (;;) {
				if (read_control(V4L2_SENS_EFFECTS, &val) < 0)
					goto exit;

				printf("\n=========================================================\n");
				printf(" Effects Menu                Range  0 to 7\n");
				printf("                                    0.Normal\n");
				printf("                                    1.Sepia(antique)\n");
				printf("                                    2.Mono chrome\n");
				printf("                                    3.Negative\n");
				printf("                                    4.Bluish\n");
				printf("                                    5.Greenish\n");
				printf("                                    6.Reddish\n");
				printf("                                    7.Yellowish\n");
				printf("========================================================\n");
				printf(" 1  Choose effects level\n");
				printf(" X  Exit effects menu\n");
			
				printf("\n Current effects %d\n", val);
				printf("========================================================\n");

				menu_option = get_user_option(NULL);
				if (menu_option == MENU_EXIT)
					break;
				
				if (menu_option == 1) {
					val = get_user_option("Enter effect option");
				
					if (write_control(V4L2_SENS_EFFECTS, val) < 0)
						goto exit;
				}
				else {
					printf("\nInvalid option \n");
					continue;
				}
			}

			break;

		case 11:
			for (;;) {
				printf("\n========================================================\n");
				printf(" Flash Menu\n");
				printf("========================================================\n");
				printf(" 1  Disable the flash\n");
				printf(" 2  Flash mode\n");
				printf(" 3  Torch mode\n");
				printf(" 4  Lumination control\n");
				printf(" 5  Flash Lumination control\n");
				printf(" 6  Torch Lumination control\n");
				printf(" X  Exit flash menu\n");
				printf("========================================================\n");

				menu_option = get_user_option(NULL);
				if (menu_option == MENU_EXIT)
					break;
				
				switch (menu_option) {
				case 1:
					if (write_control(V4L2_SENS_FLASH, 0) < 0)
						goto exit;				
					
					break;
				
				case 2:
					if (write_control(V4L2_SENS_FLASH_FLASH, 0) < 0)
						goto exit;	

					break;

				case 3:
					if (write_control(V4L2_SENS_FLASH_TORCH, 0) < 0)
						goto exit;	

					break;

				case 4:
					val = get_user_option("Lumination value (0-128)");
					
					if (write_control(V4L2_SENS_FLASH_LUM, val) < 0)
						goto exit;

					break;

				case 5:				
					val = get_user_option("Flash lumination value (0-128)");	

					if (write_control(V4L2_SENS_FLASH_FLASH_LUM, val) < 0)
						goto exit;

					break;

				case 6:
					val = get_user_option("Torch lumination value (0-31)");

					if (write_control(V4L2_SENS_FLASH_TORCH_LUM, val) < 0)
						goto exit;

					break;

				default:
					printf("\nInvalid option\n");
					continue;
				}
			}

			break;

		case 12:
			for (;;) {
				if (read_control(V4L2_SENS_ANTISHAKE_STATUS, &val) < 0)
					goto exit;

				printf("\n========================================================\n");
				printf(" Anti-Shake Menu\n");
				printf("========================================================\n");
				printf(" 1  Enable anti-shake mode\n");
				printf(" 2  Disable anti-shake mode\n");
				printf(" X  Exit anti-shake menu\n");

				printf("\n Current anti-shake: %d\n", val);
				printf("========================================================\n");

				menu_option = get_user_option(NULL);
				if (menu_option == MENU_EXIT)
					break;
				
				switch (menu_option) {
				case 1:
					if (write_control(V4L2_SENS_ANTISHAKE, 1) < 0)
						goto exit;

					break;

				case 2:
					if (write_control(V4L2_SENS_ANTISHAKE, 0) < 0)
						goto exit;

					break;

				default:
					break;
				}					
			}

			break;

		case 13:
			for (;;) {
				cam->flag.bit.comm_ctrl	= READ;
				cam->fps.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
				result	= frame_rate_ctrl(cam);
				if (result < 0) 
					goto exit;

				printf("\n========================================================\n");
				printf(" Frame Rate Menu\n");
				printf("========================================================\n");
				printf(" 1  Choose desired frame rate\n");
				printf(" X  Exit frame rate  menu\n");
				printf("Current frame rate is %d \n",
					cam->fps.parm.capture.timeperframe.denominator 
						/ cam->fps.parm.capture.timeperframe.numerator);

				menu_option = get_user_option(NULL);
				if (menu_option == MENU_EXIT)
					break;
				
				if (menu_option == 1) {
					cam->flag.bit.comm_ctrl	= WRITE;
					cam->fps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
					cam->fps.parm.capture.timeperframe.denominator = get_user_option("Enter frame rate");
					cam->fps.parm.capture.timeperframe.numerator = 1;

					result	= frame_rate_ctrl(cam);
					if (result < 0)
						goto exit;					
				}
			}
			
			break;

		default:
			printf("\nInvalid option\n");
			continue;
		}
	}
	
	return 0;

exit:
	return -1;

}

int paint_main_menu()
{
	printf("\n");
	printf("+================================================+\n");
	printf("|  V4L2 command line application    Version 2.1  |\n");
	printf("+================================================+\n");

	return 0;
}

int get_user_option(const char *prompt)
{
	char buff[32];

	int op = -1;

	if (!prompt || !*prompt)
		printf("\nEnter option: ");
	else
		printf("\n%s: ", prompt);

	memset(buff, 0, sizeof(buff));

	if (fgets(buff, sizeof(buff), stdin)) {
		if (buff[0] == 'x' || buff[0] == 'X')
			op = MENU_EXIT;
		else
			op = atoi(buff);	
	}

	return op;
}

