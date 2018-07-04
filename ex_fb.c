#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <jpeglib.h>

#define FBDEVFILE "/dev/fb0"

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

int fbfd;

unsigned int get_color24(unsigned char r, unsigned char g, unsigned char b)
{
	return (unsigned int)((0x00000000) | (r << 16) | (g << 8) | b);
}

int readjpeg(FILE *ifp)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];
	int xres = 0, yres = 0;
	int row_stride;

	cinfo.err = jpeg_std_error(&jerr);

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, ifp);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	printf("output width : %d, height : %d\n", cinfo.output_width, cinfo.output_height);
	xres = cinfo.output_width;
	yres = cinfo.output_height;

	row_stride = cinfo.output_width * cinfo.output_components;

	printf("row_stride : %d\n", row_stride);

	unsigned char **imgdata = (unsigned char **)malloc(sizeof(unsigned char*));
	*imgdata = (unsigned char *)malloc(row_stride * cinfo.output_height);

	while(cinfo.output_scanline < cinfo.output_height)
	{
		row_pointer[0] = &((*imgdata)[(cinfo.output_scanline)*row_stride]);
		jpeg_read_scanlines(&cinfo, row_pointer, 1);
	}
		
	int i, j;
	int color;
	for(i = 0; i < yres; i++)
	{
		for(j = 0; j < xres; j++)
		{
			color = get_color24((*imgdata)[i*(row_stride)+(j*cinfo.output_components)],
					    (*imgdata)[i*(row_stride)+(j*cinfo.output_components)+1],
					    (*imgdata)[i*(row_stride)+(j*cinfo.output_components)+2]);

			write(fbfd, &color, sizeof(vinfo.bits_per_pixel));
		}
		//lseek(fbfd, vinfo.xres*(i+1), SEEK_SET);
	}
			
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return 0;
}
	
long fb_memloc(int xp, int yp)
{
	if((xp > vinfo.xres) || (yp > vinfo.yres))
	{
		printf("The coordinate (%d, %d) is out of range!!!\n", xp, yp);
		exit(1);
	}
	return ((int)(vinfo.xres * yp + xp) * (vinfo.bits_per_pixel/8));
}

int main(int argc, char* argv[])
{
	int ioctl_ret;
	int x, y;

	fbfd = open(FBDEVFILE, O_RDWR);

	if(fbfd < 0)
	{
		perror("Error : open fb file");
		exit(1);
	}

	ioctl_ret = ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo);
	if(ioctl_ret < 0)
	{
		perror("Error: fb dev ioctl(VSCREENINFO GET)");
		return -1;
	}
	
	ioctl_ret = ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo);
	if(ioctl_ret < 0)
	{
		perror("ERror: fbdev ioctl(FSCREENINFO GET)");
		return -1;
	}

	FILE *ifp;

	if((ifp = fopen(argv[1], "rb")) == NULL)
	{
		perror("Error: open jpeg file");
		return -1;
	}

	printf("jpeg opened\n");

	readjpeg(ifp); 

	fclose(ifp);
	
	printf("BPP : %d\n", vinfo.bits_per_pixel);

	close(fbfd);

	return 0;
}
