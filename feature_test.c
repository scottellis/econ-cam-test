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
 * 1.0       INITIAL RELEASE							       ANANTHAPADMANABAN
 *
 *
 *==========================================================================================================
 */


#include <sys/types.h>
#include <errno.h>
#include <sys/types.h>	
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

static int imgneed(unsigned char *imgbuf, unsigned char **out_img, int needht, int needwt, int imght, int imgwt);
static int make_bitalign(unsigned char *imgbuf, unsigned char **out_buf, int needht, int needwt);
static void form_bmp_header_info(struct camera_data *cam);

static unsigned long long start_time_ms;
static struct camera_data g_camera;

int gettime(unsigned long long *ms, int flag)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	if (flag == RESET_TIME) {
		start_time_ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
		*ms = start_time_ms;
	}
	else {
		*ms = ((tv.tv_sec * 1000) + (tv.tv_usec / 1000)) - start_time_ms;
	}

	return 0;
}

/*
 * This function will save the RGB888 by taking raw RGB565 data from the camera
 */
int save_snap(struct camera_data *cam)
{
	int ret_val;
	unsigned long long time;
	char still_file[64];
	int count;

	if (cam->save_raw_file_needed) {
		for (count = 0; ; count++) {
			sprintf(still_file, 
				"snap_%dx%d_%d.raw",
				cam->fmt_need.fmt.pix.width,
				cam->fmt_need.fmt.pix.height,
				count);

			if ((cam->save_file_ptr = fopen(still_file, "r")) == NULL) {
				cam->save_file_ptr = fopen(still_file, "w");
				
				if (cam->save_file_ptr == NULL) {
					gettime(&time, 0);
					printf("Save failed %llu ms\n", time);
					continue;
				}

				break;
			}

			fclose(cam->save_file_ptr);
			cam->save_file_ptr = NULL;
		}

		fwrite(cam->raw_read_buffer, cam->fmt.fmt.pix.sizeimage, 1, cam->save_file_ptr); 
		fclose(cam->save_file_ptr);
		cam->save_file_ptr = NULL;
		gettime(&time, 0);
		printf("Save completed %llu ms\n", time);
	}
	else {
		for (count = 0; ; count++) {
			sprintf(still_file, 
				"snap_%dx%d_%d.bmp",
				cam->fmt_need.fmt.pix.width,
				cam->fmt_need.fmt.pix.height,
				count);

			if ((cam->save_file_ptr = fopen(still_file, "r")) == NULL) {
				cam->save_file_ptr = fopen(still_file, "w");

				if (cam->save_file_ptr == NULL) {
					gettime(&time, 0);
					printf("Save failed %llu ms\n", time);
					continue;
				}

				break;
			}

			fclose(cam->save_file_ptr);
			cam->save_file_ptr = NULL;
		}

		ret_val	= convert_bmp_565_bmp_888(cam);

		if (ret_val < 0)
			return ret_val;
		
		if ((cam->fmt_need.fmt.pix.width > 1280 && cam->fmt_need.fmt.pix.width < 2048)	
			|| (cam->fmt_need.fmt.pix.height > 1024 && cam->fmt_need.fmt.pix.height < 1536)) {

			/* convert the buffer */
			ret_val	= imgneed(cam->rgb_888_buffer, 
						&cam->rgb_888_buffer,
						cam->fmt_need.fmt.pix.height,
						cam->fmt_need.fmt.pix.width,
						1536, 2048);

			if (ret_val < 0)
				return ret_val;
			
			/* make byte align */
			ret_val	= make_bitalign(cam->rgb_888_buffer,
						&cam->rgb_888_buffer,
						cam->fmt_need.fmt.pix.height,
						cam->fmt_need.fmt.pix.width);

			if (ret_val < 0)
				return ret_val;
			
			cam->fmt = cam->fmt_need;
		}

		form_bmp_header_info(cam);

		fwrite(&cam->bmp_header, sizeof(cam->bmp_header), 1, cam->save_file_ptr); 
	
		fwrite(cam->rgb_888_buffer,
			cam->fmt.fmt.pix.width * cam->fmt.fmt.pix.height * 3,
			1,
			cam->save_file_ptr); 

		fclose(cam->save_file_ptr);

		gettime(&time, 0);
		printf("Save completed %llu ms\n", time);
	}

	return 0;
}
/************************************************************************************************************
 *  
 *  MODULE TYPE	:	THREAD					MODULE ID	: 	
 *
 *  Name	:	stream_video
 *  Parameter1	:	void * data
 *
 *  Returns	:	void data - camera structure data 
 *  Description	: 	
 *  Comments	:  	 
 ************************************************************************************************************/

void stream_video(void *data)
{
	struct camera_data *cam	= data;
	unsigned int stream_count = 0;
	/* unsigned int old_value; */

	struct timeval cur_tv;
	struct timeval prev_tv;

	unsigned int fps = 0;
	unsigned int old_stream_count = 0;

	gettimeofday(&prev_tv, NULL);
	gettimeofday(&cur_tv, NULL);

	for ( ; ; stream_count++) {
		if (cam->kill.thread.stream_thread_kill)
			break;

		memset(&cam->buf, 0, sizeof (cam->buf));
		cam->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		cam->buf.memory = V4L2_MEMORY_MMAP;
	
		if (ioctl (cam->fd_v4l2, VIDIOC_DQBUF, &cam->buf) < 0) {
			printf("VIDIOC_DQBUF failed.\n");
			continue;
		}

		/* write into frame buffer */
		if(cam->stream.bit.stream_lcd == 1)
		{
			/* Need to perform resize */
			memcpy(cam->fb_ptr, cam->buffers[cam->buf.index].start, cam->fmt.fmt.pix.sizeimage); 

			fflush(stderr);

			switch (stream_count % 4) {
			case 0:
				fprintf(stderr, "\r -> Streaming <-   \\");
				break;	
			case 1:
				fprintf(stderr, "\r -> Streaming <-   |");
				break;
			case 2:
				fprintf(stderr, "\r -> Streaming <-   /");
				break;
			case 3:
				fprintf(stderr, "\r -> Streaming <-   -");
				break;
			}

			/* check on this, who sets this flag ? 
			if (cam->flag.bit.anti_shake == 1) {
				if (stream_count % 4 == 0) {
					cam->flag.bit.comm_ctrl	= WRITE;
					cam->ctrl_anti_shake.id	= V4L2_SENS_ANTISHAKE;
					anti_shake(cam);
					printf("          Triggered ");
				}
				else {
					printf("                    ");
				}

				cam->flag.bit.comm_ctrl	= READ;
				cam->ctrl_anti_shake.id	= V4L2_SENS_ANTISHAKE_STATUS;
				anti_shake(cam);

				printf("Anti - shake value is 0x%02x %s", cam->ctrl_anti_shake.value,
					(old_value == cam->ctrl_anti_shake.value) ? "GOOD FRAME" : " BAD FRAME");

				old_value = cam->ctrl_anti_shake.value;
			}
			*/
			fflush(stdout);
		}

		if (cam->stream.bit.record_mode == 1) {
			if (cam->fp_file_record) {
				fwrite(cam->buffers[cam->buf.index].start,
					cam->fmt.fmt.pix.sizeimage,
					1,
					cam->fp_file_record);	

				fflush(stderr);

				switch (stream_count % 4) {
				case 0:
					fprintf(stderr, "\r -> Recording <-   \\");
					break;
				case 1:
					fprintf(stderr, "\r -> Recording <-   |");
					break;
				case 2:
					fprintf(stderr, "\r -> Recording <-   /");
					break;
				case 3:
					fprintf(stderr, "\r -> Recording <-   -");
					break;
				}

				fflush(stdout);		
			}
		}

		if (ioctl (cam->fd_v4l2, VIDIOC_QBUF, &cam->buf) < 0) {
			printf("VIDIOC_QBUF failed\n");
			break;
		}

		gettimeofday(&cur_tv, NULL);

		if (cur_tv.tv_sec > prev_tv.tv_sec) {
			prev_tv	= cur_tv;
			fps = stream_count - old_stream_count;
			old_stream_count = stream_count;
		}

		fprintf(stderr," Frames per second = %dfps ",fps);
	}

	cam->kill.thread.stream_thread_kill = 1;

	pthread_exit(NULL);
}

/************************************************************************************************************
 *  
 *  MODULE TYPE	:	FUNCTION				MODULE ID	:	
 *
 *  Name	:	
 *  Parameter1	:	struct camera_data *cam - Address of Base pointer of camera structure
 *
 *  Returns	:	Function result depends on return value of child functions and 
 *  			condition available in the functions based on this return value will
 *
 *  			0	- all the condition executed properly
 *  			-1	- Failed to perform specified operation
 *  Description	: 	
 *  Comments	:  	
 ************************************************************************************************************/

int stream_off(struct camera_data *cam)
{
	int i;

	if (cam->stream.G_FLAG != 0)
		return 0;

	cam->kill.thread.stream_thread_kill = 1;
	sleep(1);

	for (i = 0; i < TEST_BUFFER_NUM; i++) {
		if (cam->buffers[i].start) {
			munmap(cam->buffers[i].start, cam->buffers[i].length);
			cam->buffers[i].start = NULL;
		}
	}

	cam->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (ioctl(cam->fd_v4l2, VIDIOC_STREAMOFF, &cam->type) < 0) {
		printf("VIDIOC_STREAMOFF error\n");
		return -1;
	}

	return 0;
}
/************************************************************************************************************
 *  
 *  MODULE TYPE	:	FUNCTION				MODULE ID	:	
 *
 *  Name	:	
 *  Parameter1	:	struct camera_data *cam - Address of Base pointer of camera structure
 *  Parameter2	:	
 *
 *  Returns	:	Function result depends on return value of child functions and 
 *  			condition available in the functions based on this return value will
 *
 *  			0	- all the condition executed properly
 *  			-1	- Failed to perform specified operation
 *  Description	: 	
 *  Comments	:  	
 ************************************************************************************************************/

int stream_on(struct camera_data *cam)
{
	int i;
	int ret_val;

	cam->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (cam->stream.G_FLAG == 0) {
		return 0;
	}
	else if (cam->stream.bit.stream_lcd == 1) {
		cam->fmt.fmt.pix.width	= cam->stream_width;
		cam->fmt.fmt.pix.height	= cam->stream_height;
		cam->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
	}
	else {
		cam->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
	}

	cam->fmt.fmt.pix.bytesperline = cam->fmt.fmt.pix.width * 2;
	cam->fmt.fmt.pix.priv = 0;
	cam->fmt.fmt.pix.sizeimage = cam->fmt.fmt.pix.width * cam->fmt.fmt.pix.height * 2;

	ret_val	= snap_apply_ctrl(cam);
	if (ret_val < 0)
		return ret_val;

	memset(&cam->req, 0, sizeof (cam->req));
	cam->req.count = TEST_BUFFER_NUM;
	cam->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	cam->req.memory = V4L2_MEMORY_MMAP;

	if (ioctl(cam->fd_v4l2, VIDIOC_REQBUFS, &cam->req) < 0) {
		printf("v4l_capture_setup: VIDIOC_REQBUFS failed\n");
		return 0;
	}

	for (i = 0; i < TEST_BUFFER_NUM; i++) {
		memset(&cam->buf,0, sizeof(cam->buf));
		cam->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		cam->buf.index = i;

		if (ioctl(cam->fd_v4l2, VIDIOC_QUERYBUF, &cam->buf) < 0) {
			printf("VIDIOC_QUERYBUF error\n");
			return -1;
		}

		cam->buffers[i].length = cam->buf.length;
		cam->buffers[i].offset = (size_t) cam->buf.m.offset;

		cam->buffers[i].start = mmap (NULL, 
						cam->buffers[i].length, 
		        			PROT_READ | PROT_WRITE, 
						MAP_SHARED, 
	                			cam->fd_v4l2, 
						cam->buffers[i].offset);

		printf("\n buffers phyaddr %x buffers[i].start %p\n", 
			cam->buffers[i].offset, cam->buffers[i].start);

		memset(cam->buffers[i].start, 0, cam->buffers[i].length);

		if (ioctl (cam->fd_v4l2, VIDIOC_QBUF, &cam->buf) < 0) {
			printf("VIDIOC_QBUF error\n");
			return -1;
		}

	}

	cam->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl (cam->fd_v4l2, VIDIOC_STREAMON, &cam->type) < 0) {
		printf("VIDIOC_STREAMON error\n");
		return -1;
	}

	cam->kill.thread.stream_thread_kill = 0;

	pthread_create(&cam->stream_tid, NULL,	(void * (*)(void *))stream_video, (void *)cam);

	return 0;
}

/************************************************************************************************************
 *  
 *  MODULE TYPE	:	FUNCTION				MODULE ID	:	FEATURE_TEST_03
 *
 *  Name	:	feature_test_api_init
 *  Parameter1	:	struct camera_data **cam - Address of Base pointer of camera structure
 *  Parameter2	:	
 *
 *  Returns	:	Function result depends on return value of child functions and 
 *  			condition available in the functions based on this return value will
 *
 *  			0	- all the condition executed properly
 *  			-1	- Failed to perform specified operation
 *  Description	: 	
 *  Comments	:  	This function initialize the feature test api module
 ************************************************************************************************************/
int feature_test_api_init(struct camera_data **cam)
{
	if ((g_camera.fd_v4l2 = open("/dev/video0", O_RDWR, 0)) < 0)
		return -1;
        
	g_camera.rgb_888_buffer	= calloc(MAX_IMAGE_WIDTH * MAX_IMAGE_HEIGHT * 3, 1);	
	if (!g_camera.rgb_888_buffer) 
		return -1;

	g_camera.raw_read_buffer = calloc(MAX_IMAGE_WIDTH * MAX_IMAGE_HEIGHT * 2, 1);	
	if (!g_camera.raw_read_buffer)
		return -1;
	
	g_camera.fb_fd = open("/dev/fb0", O_RDWR);
	if (g_camera.fb_fd < 0) {	
		printf("Error: cannot open framebuffer device.\n");
		return -1;
	}

	if (ioctl(g_camera.fb_fd, FBIOGET_VSCREENINFO, &g_camera.vinfo)) {
		printf("Error reading variable information.\n");
		return -1; 
	}

	printf("Screen resolution is %dx%d = \n", g_camera.vinfo.xres, g_camera.vinfo.yres);
	g_camera.stream_width = g_camera.vinfo.xres;
	g_camera.stream_height = g_camera.vinfo.yres;

	g_camera.fb_ptr 
		= (char *) mmap(0, 
				g_camera.vinfo.xres * g_camera.vinfo.yres * g_camera.vinfo.bits_per_pixel,
				 PROT_READ | PROT_WRITE, MAP_SHARED, 
				g_camera.fb_fd, 
				0);

	*cam = &g_camera;

	return 0;
}

int try_fmt(struct camera_data *cam)
{
	return ioctl(cam->fd_v4l2, VIDIOC_TRY_FMT, &cam->fmt);
}

int snap_apply_ctrl(struct camera_data *cam)
{
	return ioctl(cam->fd_v4l2, VIDIOC_S_FMT, &cam->fmt);
}

int take_snap(struct camera_data *cam)
{
	int count;

	count = read(cam->fd_v4l2, cam->raw_read_buffer, cam->fmt.fmt.pix.sizeimage);

        if (count != cam->fmt.fmt.pix.sizeimage) {
		printf("Camera read returned %d bytes. Expected %d bytes\n",
			count, cam->fmt.fmt.pix.sizeimage);

                return -1;
        }

	return 0;
}


/* #define ORIGINAL_METHOD */
int convert_bmp_565_bmp_888(struct camera_data *cam)
{
	int rg, gb, i, j;

#ifdef ORIGINAL_METHOD
	int r, g, b;
#else
	int w2, w3, h_1, k, m;	
#endif

	unsigned long long time;

#ifdef ORIGINAL_METHOD	
	for(i = 0; i < cam->fmt.fmt.pix.height; i++) {
		for (j = 0; j < cam->fmt.fmt.pix.width; j++) {

			gb = cam->raw_read_buffer[(i * cam->fmt.fmt.pix.width * 2) + j * 2 + 0];
			rg = cam->raw_read_buffer[(i * cam->fmt.fmt.pix.width * 2) + j * 2 + 1];

			r = (rg & 0xF8);
			g = ((((rg & 0x7) << 3) | ((gb & 0xE0) >> 5)) << 2);
			b = ((gb & 0x1F) << 3);
			
			cam->rgb_888_buffer[(((cam->fmt.fmt.pix.height - 1) - i) * cam->fmt.fmt.pix.width * 3) + j * 3 + 0] = 0xFF & b;
			cam->rgb_888_buffer[(((cam->fmt.fmt.pix.height - 1) - i) * cam->fmt.fmt.pix.width * 3) + j * 3 + 1] = 0xFF & g;
			cam->rgb_888_buffer[(((cam->fmt.fmt.pix.height - 1) - i) * cam->fmt.fmt.pix.width * 3) + j * 3 + 2] = 0xFF & r;
		}
	}
#else
	w2 = cam->fmt.fmt.pix.width * 2;
	w3 = cam->fmt.fmt.pix.width * 3;
	h_1 = cam->fmt.fmt.pix.height - 1;

	for(i = 0; i < cam->fmt.fmt.pix.height; i++) {
		m = i * w2;
		k = (h_1 - i) * w3;
		for (j = 0; j < cam->fmt.fmt.pix.width; j++, m += 2, k += 3) {
			gb = cam->raw_read_buffer[m];
			rg = cam->raw_read_buffer[m + 1];
			cam->rgb_888_buffer[k] = (gb & 0x1F) << 3;
			cam->rgb_888_buffer[k + 1] = (((rg & 0x7) << 3) | ((gb & 0xE0) >> 5)) << 2;
			cam->rgb_888_buffer[k + 2] = 0xF8 & rg;
		}
	}
#endif


	gettime(&time, 0);
	printf("Conversion completed RGB565 -> RGB888 %llu ms\n", time);

	return 0;
}


void form_bmp_header_info(struct camera_data *cam)
{
	cam->bmp_header.type = 0x4D42;
	cam->bmp_header.size = sizeof(cam->bmp_header) 
		+ (cam->fmt.fmt.pix.width * cam->fmt.fmt.pix.height * 3);

	cam->bmp_header.reserved1 = 0;
	cam->bmp_header.reserved2 = 0;
	cam->bmp_header.offbits	= sizeof(cam->bmp_header);
	cam->bmp_header.size_header = 40;
	cam->bmp_header.width = cam->fmt.fmt.pix.width;
	cam->bmp_header.height = cam->fmt.fmt.pix.height;
	cam->bmp_header.planes = 1;
	cam->bmp_header.bitcount = 24;
	cam->bmp_header.compression = 0;
	cam->bmp_header.size_image = 0;
	cam->bmp_header.x_permeter = 0;
	cam->bmp_header.y_permeter = 0;
	cam->bmp_header.clr_used = 0;
	cam->bmp_header.clr_important = 0;
}

int read_control(int id, int *val)
{
	int ret;
	struct v4l2_control ctl;
	
	if (!val)
		return -1;

	if (!g_camera.fd_v4l2)
		return -1;

	ctl.id = id;
	ctl.value = 0;

	ret = ioctl(g_camera.fd_v4l2, VIDIOC_G_CTRL, &ctl);

	if (ret < 0)
		perror("read_setting");
	else
		*val = ctl.value;

	return ret;
}

int write_control(int id, int val)
{
	int ret;
	struct v4l2_control ctl;

	if (!g_camera.fd_v4l2)
		return -1;

	ctl.id = id;
	ctl.value = 0xffff & val;

	ret = ioctl(g_camera.fd_v4l2, VIDIOC_S_CTRL, &ctl);

	if (ret < 0)
		perror("write control");
	
	return ret;
}

int query_control(struct v4l2_queryctrl *query)
{
	int ret;

	if (!query)
		return -1;

	if (!g_camera.fd_v4l2)
		return -1;

	ret = ioctl(g_camera.fd_v4l2, VIDIOC_QUERYCTRL, query);

	if (ret < 0)
		perror("query control");

	return ret;
}

/************************************************************************************************************
 *  
 *  MODULE TYPE	:	FUNCTION				MODULE ID	:	
 *
 *  Name	:	frame_rate_ctrl
 *  Parameter1	:	struct camera_data *cam - Base pointer of camera structure
 *
 *  Returns	:	Function result depends on return value of child functions and 
 *  			condition available in the functions based on this return value will
 *
 *  			0	- all the condition executed properly
 *  			-1	- Failed to perform specified operation
 *  Description	: 	
 *  Comments	:  	 
 ************************************************************************************************************/
int frame_rate_ctrl(struct camera_data *cam)
{
	if(cam->flag.bit.comm_ctrl	== WRITE)
	{
		if(ioctl(cam->fd_v4l2, VIDIOC_S_PARM, &cam->fps) < 0)
		{
			printf("FILE %s FUNCTION %s LINE %d : change not done\n",__FILE__,__FUNCTION__,__LINE__);
			return -1;			
		}

	}else if(cam->flag.bit.comm_ctrl	== READ)
	{
		if(ioctl(cam->fd_v4l2, VIDIOC_G_PARM, &cam->fps) < 0)
		{
			printf("FILE %s FUNCTION %s LINE %d : change not done\n",__FILE__,__FUNCTION__,__LINE__);
			return -1;			
		}

	}else if(cam->flag.bit.comm_ctrl	== QUERY)
	{
	}

	return 0;
}

int get_camera_format(struct camera_data *cam)
{
	cam->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	return ioctl(cam->fd_v4l2, VIDIOC_G_FMT, &cam->fmt);
}

int close_camera(struct camera_data *cam)
{
	close(cam->fd_v4l2);

	return 0;
}

/************************************************************************************************************
 *  
 *  MODULE TYPE	:	FUNCTION				MODULE ID	:	
 *
 *  Name	:	imgneed	
 *  Parameter1	:	unsigned char * imgbuf
 *  Parameter2	:	unsigned char ** out_img
 *  Parameter3	:	int needht
 *  Parameter4	:	int needwt
 *  Parameter5	:	int imght
 *  Parameter6	:	int imgwt
 *
 *  Returns	:	Function result depends on return value of child functions and 
 *  			condition available in the functions based on this return value will
 *
 *  			0	- all the condition executed properly
 *  			-1	- Failed to perform specified operation
 *  Description	: 	
 *  Comments	:  	
 ************************************************************************************************************/

/***********************************************************************************
 * imgneed is the function which will take original buffer as input 
 * and return converted small image 
 * needht = required height
 * needwt = required width
 * imght  = image height
 * imgwt  = image width
 * ********************************************************************************/

int imgneed(unsigned char * imgbuf,unsigned char ** out_img,int needht,int needwt,int imght,int imgwt)
{
	int Rw=imgwt/needwt;
	int Rh=imght/needht;
	int RemRw=imgwt-Rw*needwt;
	int RemRh=imght-Rh*needht;
	unsigned int spreadh=0,spreadw=0;
	int i,j,k,l,count=0,count1=0;
	int R=0,G=0,B=0;
	unsigned char * needbuf=calloc(needht*needwt*3,sizeof(unsigned char));
	float fraction_h,count_fraction_h=0;
	float fraction_w,count_fraction_w=0;
	fraction_h=(RemRh/(float)needht);
	fraction_w=(RemRw/(float)needwt);

	for(i=0;i<needht;i++)
	{
		count_fraction_h=count_fraction_h+fraction_h;
		if((count_fraction_h>1)&&RemRh)
		{
			spreadh=1;
			count_fraction_h=count_fraction_h-1;
			RemRh--;
		}else
		{
			spreadh=0;	
        	}
		for(l=0;l<(Rh+spreadh);l++)
        	{                 
			count=imgwt*3*count1++;
                        for(j=0;j<needwt;j++)
	 		{
				count_fraction_w=count_fraction_w+fraction_w;
				if((count_fraction_w>1)&&RemRw)
				{
					spreadw=1;
					count_fraction_w=count_fraction_w-1;
					RemRw--;
				}else
				{
					spreadw=0;	
     				}
				for(k=0;k<(Rw+spreadw);k++)
				{
//					R+=(0xff&(imgbuf[count++]))/(Rw+spreadw);
//					G+=(0xff&(imgbuf[count++]))/(Rw+spreadw);
//					B+=(0xff&(imgbuf[count++]))/(Rw+spreadw);

					R=(0xff&(imgbuf[count++]));
					G=(0xff&(imgbuf[count++]));
					B=(0xff&(imgbuf[count++]));

				}
			needbuf[((i*needwt)+j)*3]=(R);
			needbuf[((i*needwt)+j)*3+1]=G;
			needbuf[((i*needwt)+j)*3+2]=B;

//			needbuf[((i*needwt)+j)*3]+=(R/(Rh+spreadh));
//			needbuf[((i*needwt)+j)*3+1]+=(G/(Rh+spreadh));
//			needbuf[((i*needwt)+j)*3+2]+=(B/(Rh+spreadh));
			R=G=B=0;
			}
			RemRw=imgwt-Rw*needwt;
        	}
	}
	*out_img	= needbuf;
	return 0;
}

int make_bitalign(unsigned char *imgbuf, unsigned char **out_buf, int needht, int needwt)
{
	int w3 = needwt * 3;

	int pixadd = (w3 & 0x3) > 0 ? 4 - (w3 & 0x3) : 0;

	if (pixadd > 0) {
		unsigned char* imgbuf_1 = calloc(needht * (w3 + pixadd), sizeof(char));
		int i;

		for (i = 0; i < needht; i++) {
			memcpy(&imgbuf_1[i * (w3 + pixadd)], &imgbuf[i * w3], w3);
		}

		free(imgbuf);
		*out_buf = imgbuf_1;

		return 0;
	}
	else {
		*out_buf = imgbuf;
		return 0;
	}
		
	return 0;
}

