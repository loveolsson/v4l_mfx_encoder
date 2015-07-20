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


int rawPacket_nextFree (StateMachine *stateMachine) {
  if ((stateMachine->rawPacketLocked + 1) % RAW_VIDEO_QUEUE_LENGTH == stateMachine->rawPacketRead) return -1;
  stateMachine->rawPacketLocked = (stateMachine->rawPacketLocked + 1) % RAW_VIDEO_QUEUE_LENGTH;
  return stateMachine->rawPacketLocked;
}

int rawPacket_nextWritten (StateMachine *stateMachine) {
  if (stateMachine->rawPacketWritten == stateMachine->rawPacketRead) return -1;
  return (stateMachine->rawPacketRead + 1) % RAW_VIDEO_QUEUE_LENGTH;
}

int rawPacket_markWritten (StateMachine *stateMachine, int written) {
  stateMachine->rawPacketWritten = written;
}

int rawPacket_markRead (StateMachine *stateMachine, int read) {
  stateMachine->rawPacketRead = read;
}


void *frameReadLoop(StateMachine *stateMachine) {
  //StateMachine stateMachine = (StateMachine *)stateMachine_;

  AVFormatContext* pFormatCtx = stateMachine->pFormatCtx;
  AVCodecContext*  pCodecCtx = stateMachine->pCodecCtx;

  int res, copyres;
  AVPacket* packet;
  int frameFinished;

  int64_t lastpts = 0;
  int next = -1;

  for (;;) {
    next = rawPacket_nextFree(stateMachine);
    if (next >= 0) {


      packet = &stateMachine->rawPacket[next];

      res = av_read_frame(pFormatCtx,packet);
      if (res < 0) break;


      if(packet->stream_index == stateMachine->videoStream){



        //if(frameFinished){
        printf("Frame time: %jd, Buffer in: %i\n", packet->pts-lastpts, next);
        lastpts = packet->pts;

        rawPacket_markWritten(stateMachine, next);
        // copyres = copyRawFrame(stateMachine, packet);
        // if (copyres == 0) {
        //   printf("Funkar: %i\n", copyres);
        // } else {
        //   printf("Frame drop: %i\n", copyres);
        // }




            }
        //av_free_packet(packet);
        // sws_freeContext(img_convert_ctx);
      } else {
        //printf("%s\n", "Frame dropped - raw buffer full");
      }

  }







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
