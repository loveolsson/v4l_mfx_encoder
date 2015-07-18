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


  int videoStream = *stateMachine->videoStream;
  AVFormatContext *pFormatCtx = stateMachine->pFormatCtx;


  unsigned int res;
  AVPacket packet;

  int64_t lastpts = 0;

  while((res = av_read_frame(pFormatCtx,&packet)) >= 0)
  {

    if(packet.stream_index == videoStream){

      //avcodec_decode_video2(pCodecCtx,pFrame,&frameFinished,&packet);

      //if(frameFinished){
      printf("%jd, %i\n", packet.pts-lastpts, packet.data[500]);
      lastpts = packet.pts;

        for (int i = 0; i < packet.size; i++) {

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




  return 0;
}

int initInputDevice (char *format, char *filenameSrc, StateMachine *stateMachine) {
  int videoStreamId = -1;
  stateMachine->videoStream = &videoStreamId;
  AVFormatContext *pFormatCtx = stateMachine->pFormatCtx;

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
      videoStreamId = i;
      break;
    }
  }

  if(videoStreamId == -1) return -14;

  printf("%i %i %i %i\n", pFormatCtx->streams[videoStreamId]->codec->width, pFormatCtx->streams[videoStreamId]->codec->height, pFormatCtx->streams[videoStreamId]->codec->framerate.num, pFormatCtx->streams[videoStreamId]->codec->framerate.den);





  return 0;


}
