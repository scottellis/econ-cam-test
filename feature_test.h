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

#ifndef FEATURE_TEST_H
#define FEATURE_TEST_H

#include "v4l2_defs.h"

#define MAX_IMAGE_WIDTH		2048
#define MAX_IMAGE_HEIGHT	1536
#define MAX_PATH_NAME		512

#define READ			0
#define WRITE			1
#define QUERY			2

#define RESET_TIME		1

#define REGISTER_DATA		1			
#define UNREGISTER_CLEAR_ENTRY	2
#define GET_REGISTERED_DATA	3

#define TEST_BUFFER_NUM 	2

struct camera_data {
	int fd_v4l2;

	struct v4l2_format fmt;
	struct v4l2_format fmt_need;

	struct v4l2_streamparm fps;
	struct v4l2_requestbuffers req;
	enum v4l2_buf_type type;
	struct v4l2_buffer buf;

	unsigned char *rgb_888_buffer;
	unsigned char *raw_read_buffer;
	unsigned char save_raw_file_needed;

	union {
		unsigned int Maintain_threads;
		struct {
			unsigned int stream_thread_kill	:1;
		} thread;
	} kill;

	union {
		unsigned int G_FLAG;
		struct {
			unsigned int comm_ctrl		:2;	/*  0 - read, 1 - write, 2 - query */
			unsigned int anti_shake		:1;
		} bit;
	} flag;

	struct __attribute__ ((__packed__)) {
		unsigned short type; 
		unsigned int size; 
		unsigned short reserved1; 
		unsigned short reserved2; 
		unsigned int offbits; 
		unsigned int size_header; 
		int width; 
		int height; 
		unsigned short planes; 
		unsigned short bitcount; 
		unsigned int compression; 
		unsigned int size_image; 
		int x_permeter; 
		int y_permeter; 
		unsigned int clr_used; 
		unsigned int clr_important; 
	} bmp_header;

	FILE *save_file_ptr;
	char save_path[MAX_PATH_NAME];

	struct fb_var_screeninfo vinfo;
	int fb_fd;
	char *fb_ptr;

	int stream_width;
	int stream_height;
	
	pthread_t stream_tid;

	struct testbuffer {
		unsigned char *start;
		size_t offset;
		unsigned int length;
	} buffers[TEST_BUFFER_NUM];

	union {
		unsigned int G_FLAG;
		struct {
			unsigned int record_mode	:1;
			unsigned int stream_lcd		:1;
		} bit;
	} stream;

	FILE *fp_file_record;
};	


int gettime(unsigned long long *ms, int flag);
int close_camera(struct camera_data *cam);
int save_snap(struct camera_data *cam);
int feature_test_api_init(struct camera_data **cam);
int snap_apply_ctrl(struct camera_data *cam);
int take_snap(struct camera_data *cam);
int read_snap(struct camera_data *cam);
int convert_bmp_565_bmp_888(struct camera_data *cam);
int get_camera_format(struct camera_data *cam);
int stream_on(struct camera_data *cam);
int stream_off(struct camera_data *cam);
int try_fmt(struct camera_data *cam);

int frame_rate_ctrl(struct camera_data *cam);

int read_control(int id, int *val);
int write_control(int id, int val);
int query_control(struct v4l2_queryctrl *query);


#endif /* ifndef FEATURE_TEST_H */

