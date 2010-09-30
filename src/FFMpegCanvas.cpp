#include "FFMpegCanvas.h"
#include "FFMpegConverter.h"

FFMpegCanvas::FFMpegCanvas(int w,int h,PixelFormat fmt)
{
	m_w = w;
	m_h = h;
	m_fmt = fmt;

	int ret = avpicture_alloc(&m_picture,m_fmt,m_w,m_h);
	if (ret)
		printf("FFMpegCanvas failed to alloc avpicture! ret=%d,dim=%dx%d\n",ret,m_w,m_h);
}

FFMpegCanvas::~FFMpegCanvas(void)
{
	avpicture_free(&m_picture);
}

int FFMpegCanvas::draw(PixelFormat fmt,AVPicture *pPic, int x, int y, int imageW,int imageH)
{
	if (fmt != m_fmt)
		return -1;

	bool bFmtSupported = true;
	int x_shift = 0;
	int y_shift = 0;

	switch (fmt)
	{
		case PIX_FMT_YUVJ420P:
		case PIX_FMT_YUV420P:
			x_shift = 1;
			y_shift = 1;
			break;
		case PIX_FMT_YUVJ422P:
			x_shift = 1;
			y_shift = 0;
			break;
		case PIX_FMT_YUVJ444P:
			x_shift = 0;
			y_shift = 0;
			break;
			
		default:
			bFmtSupported = false;
	}

	if (bFmtSupported == false)
		return -2;


	unsigned char* ptrY = m_picture.data[0];
	unsigned char* ptrU = m_picture.data[1] + (y >> y_shift) * m_picture.linesize[1] + (x >> x_shift);
	unsigned char* ptrV = m_picture.data[2] + (y >> y_shift) * m_picture.linesize[2] + (x >> x_shift);


	unsigned char* ptrY2 = pPic->data[0];
	unsigned char* ptrU2 = pPic->data[1];
	unsigned char* ptrV2 = pPic->data[2];
	
	for (int h=0;h<imageH;h++)
	{
		
		memcpy(m_picture.data[0]+(y+h)*m_picture.linesize[0]+x,
			pPic->data[0]+h*pPic->linesize[0],
			imageW);
		memcpy(m_picture.data[1]+((y+h) >> y_shift)*m_picture.linesize[1]+(x >> x_shift),
			pPic->data[1]+(h >> y_shift)*pPic->linesize[1],
			imageW >> x_shift);
		memcpy(m_picture.data[2]+((y+h) >> y_shift)*m_picture.linesize[2]+(x >> x_shift),
			pPic->data[2]+(h >> y_shift)*pPic->linesize[2],
			imageW >> x_shift);
		
		/*
		memcpy(ptrY,ptrY2,imageW);
		memcpy(ptrU,ptrU2,imageW >> x_shift);
		memcpy(ptrV,ptrV2,imageW >> x_shift);
		
		ptrY += m_picture.linesize[0];
		ptrU += m_picture.linesize[1] * (1 >> y_shift);
		ptrV += m_picture.linesize[2] * (1 >> y_shift);
		
		ptrY2 += pPic->linesize[0];
		ptrU2 += pPic->linesize[1] * (1 >> y_shift);
		ptrV2 += pPic->linesize[2] * (1 >> y_shift);
		 */
	}
	

}

AVPicture* FFMpegCanvas::getPicture()
{
	return &m_picture;	
}

AVPicture* FFMpegCanvas::copyPicture()
{
	AVPicture *pic_ret = (AVPicture*)malloc(sizeof(AVPicture));
	av_picture_copy(pic_ret,&m_picture,m_fmt,m_w,m_h);
	return pic_ret;
}

void FFMpegCanvas::clear()
{
	unsigned char* rgb = (unsigned char*)malloc(m_w * m_h * 3);
	for (int h=0;h<m_h;h++)
	{
		for (int x=0;x<m_w;x++)
		{
			rgb[h*m_w*3+x*3] = 0x00;
			rgb[h*m_w*3+x*3+1] = 0x00;
			rgb[h*m_w*3+x*3+2] = 0x00;
		}
	}
	AVPicture rgbPic;
	rgbPic.data[0] = rgb;
	rgbPic.linesize[0] = m_w * 3;
	
	FFMpegConverter conv(m_w, m_h, PIX_FMT_RGB24, m_w, m_h, m_fmt);
	
	
	AVPicture *picConverted = conv.convertVideo(&rgbPic);
	draw(m_fmt, picConverted, 0,0, m_w, m_h);
	free(rgb);	
}