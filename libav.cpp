#ifdef __cplusplus
extern "C" {
  #endif
  #include <libavdevice/avdevice.h>
  #include <libavformat/avformat.h>
  #ifdef __cplusplus
}
#endif

#include "struct.h"
#include "mfx.h"






void *frameReadLoop(StateMachine *stateMachine) {
  //StateMachine stateMachine = (StateMachine *)stateMachine_;

  AVFormatContext* pFormatCtx = stateMachine->pFormatCtx;
  AVCodecContext*  pCodecCtx = stateMachine->pCodecCtx;


  int res, copyres;
  AVPacket packet;
  AVFrame *pFrame = avcodec_alloc_frame();
  int frameFinished;

  int64_t lastpts = 0;

  while((res = av_read_frame(pFormatCtx,&packet)) >= 0)
  {

    if(packet.stream_index == stateMachine->videoStream){


      //if(frameFinished){
      printf("%jd, %i\n", packet.pts-lastpts, packet.data[500]);
      lastpts = packet.pts;


      copyres = copyRawFrame(stateMachine, &packet);
      if (copyres == 0) {
        printf("Funkar: %i\n", copyres);
      } else {
        printf("Frame drop: %i\n", copyres);
      }




      // struct SwsContext * img_convert_ctx;
      // img_convert_ctx = sws_getCachedContext(NULL,pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,   pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL,NULL);
      // sws_scale(img_convert_ctx, ((AVPicture*)pFrame)->data, ((AVPicture*)pFrame)->linesize, 0, pCodecCtx->height, ((AVPicture *)pFrameRGB)->data, ((AVPicture *)pFrameRGB)->linesize);
      //
      // //OpenCV
      // cv::Mat img(pFrame->height,pFrame->width,CV_8UC3,pFrameRGB->data[0]);
      // cv::imshow("display",img);
      // cvWaitKey(1);
      //
      av_free_packet(&packet);
      // sws_freeContext(img_convert_ctx);

    }

    //}

  }

  av_free_packet(&packet);
  av_free(pFrame);





  return 0;
}

int initInputDevice (char *format, char *filenameSrc, StateMachine *stateMachine) {
  stateMachine->videoStream = -1;
  AVFormatContext *pFormatCtx = stateMachine->pFormatCtx;
  AVCodecContext  *pCodecCtx;
  AVCodec * pCodec;



  avdevice_register_all();
  avcodec_register_all();

  stateMachine->iformat = av_find_input_format(format);
  AVInputFormat *iformat = stateMachine->iformat;


  printf("%s\n", format);


  if(avformat_open_input(&pFormatCtx,filenameSrc,iformat,NULL) != 0) return -12;
  printf("%s\n", "Input device open");
  if(avformat_find_stream_info(pFormatCtx, NULL) < 0)   return -13;
  av_dump_format(pFormatCtx, 0, filenameSrc, 0);

  for(unsigned int i=0; i < pFormatCtx->nb_streams; i++)
  {
    if(pFormatCtx->streams[i]->codec->coder_type==AVMEDIA_TYPE_VIDEO)
    {
      stateMachine->videoStream = i;
      break;
    }
  }

  if(stateMachine->videoStream == -1) return -14;
  pCodecCtx = pFormatCtx->streams[stateMachine->videoStream]->codec;
  stateMachine->pCodecCtx = pCodecCtx;
  pCodec =avcodec_find_decoder(pCodecCtx->codec_id);
  if(avcodec_open2(pCodecCtx,pCodec,NULL) < 0) return -16;


  printf("%i %i %i %i\n", pFormatCtx->streams[stateMachine->videoStream]->codec->width, pFormatCtx->streams[stateMachine->videoStream]->codec->height, pFormatCtx->streams[stateMachine->videoStream ]->codec->framerate.num, pFormatCtx->streams[stateMachine->videoStream]->codec->framerate.den);





  return 0;


}
