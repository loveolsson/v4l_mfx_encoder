#ifdef __cplusplus
extern "C" {
#endif
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#ifdef __cplusplus
}
#endif

#include "fcntl.h"
#include "libav.h"
#include "mfx.h"
#include <thread>

StateMachine stateMachine;


int main(int argc, char *argv[])
{

  char  *formatName = "video4linux2";
  char  *filenameSrc = "/dev/video0";

  stateMachine.pFormatCtx = avformat_alloc_context();

  MFXOptions mfxOptions;
  memset(&mfxOptions, 0, sizeof(MFXOptions));


  mfxOptions.Bitrate = (mfxU16)3500;

  stateMachine.mfxOptions = &mfxOptions;




  initInputDevice(formatName, filenameSrc, &stateMachine);

  initMFX(&stateMachine);


  std::thread frameReadThread(&frameReadLoop, &stateMachine);

  frameReadLoop(&stateMachine);


  frameReadThread.join();







    //av_free(pFrame);
    avformat_close_input(&stateMachine.pFormatCtx);

    return 0;
}
