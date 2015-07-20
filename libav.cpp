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
  if ((stateMachine->rawPacketWritten + 1) % RAW_VIDEO_QUEUE_LENGTH == stateMachine->rawPacketRead) return -1;
  return (stateMachine->rawPacketWritten + 1) % RAW_VIDEO_QUEUE_LENGTH;
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

  AVFormatContext* pFormatCtxVideo = stateMachine->pFormatCtxVideo;
  AVCodecContext*  pCodecCtxVideo = stateMachine->pCodecCtxVideo;

  int res, copyres;
  AVPacket* packet;
  int frameFinished;

  int64_t lastpts = 0;
  int next = -1;

  for (;;) {
    next = rawPacket_nextFree(stateMachine);
    if (next >= 0) {


      packet = &stateMachine->rawPacket[next];

      res = av_read_frame(pFormatCtxVideo,packet);
      if (res < 0) break;


      if(packet->stream_index == stateMachine->videoStream){

        printf("Frame time: %jd, Buffer in: %i\n", packet->pts-lastpts, next);
        lastpts = packet->pts;

        rawPacket_markWritten(stateMachine, next);

      }

    } else {
      printf("%s\n", "Frame dropped - raw buffer full");
    }

  }

  return 0;
}

int initInputDevice (char *formatVideo, char *pathVideo, char *formatAudio, char *pathAudio, StateMachine *stateMachine) {
  stateMachine->videoStream = -1;
  AVFormatContext *pFormatCtxVideo = stateMachine->pFormatCtxVideo;
  AVCodecContext  *pCodecCtxVideo;
  // AVCodec * pCodecVideo;

  avdevice_register_all();
  avcodec_register_all();

  stateMachine->iformatVideo = av_find_input_format(formatVideo);
  AVInputFormat *iformatVideo = stateMachine->iformatVideo;


  printf("%s\n", formatVideo);


  if(avformat_open_input(&pFormatCtxVideo, pathVideo, iformatVideo, NULL) != 0) return -12;


  printf("%s\n", "Input device open");
  if(avformat_find_stream_info(pFormatCtxVideo, NULL) < 0)   return -13;
  av_dump_format(pFormatCtxVideo, 0, pathVideo, 0);

  for(unsigned int i=0; i < pFormatCtxVideo->nb_streams; i++)
  {
    if(pFormatCtxVideo->streams[i]->codec->coder_type==AVMEDIA_TYPE_VIDEO)
    {
      stateMachine->videoStream = i;
      break;
    }
  }

  if(stateMachine->videoStream == -1) return -14;
  pCodecCtxVideo = pFormatCtxVideo->streams[stateMachine->videoStream]->codec;
  stateMachine->pCodecCtxVideo = pCodecCtxVideo;
  // pCodecVideo=avcodec_find_decoder(pCodecCtxVideo->codec_id);
  // if(avcodec_open2(pCodecCtxVideo, pCodecVideo, NULL) < 0) return -16;


  return 0;


}
