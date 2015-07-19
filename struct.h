#include "mfxdefs.h"
#include "mfxstructures.h"
#include "common_utils.h"


#define OPTION_IMPL             0x001
#define OPTION_GEOMETRY         0x002
#define OPTION_BITRATE          0x004
#define OPTION_FRAMERATE        0x008
#define OPTION_MEASURE_LATENCY  0x010

#define OPTIONS_DECODE \
    (OPTION_IMPL)

#define OPTIONS_ENCODE \
    (OPTION_IMPL | OPTION_GEOMETRY | OPTION_BITRATE | OPTION_FRAMERATE)

#define OPTIONS_VPP \
    (OPTION_IMPL | OPTION_GEOMETRY)

#define OPTIONS_TRANSCODE \
    (OPTION_IMPL | OPTION_BITRATE | OPTION_FRAMERATE)

#define MSDK_MAX_PATH 280



typedef struct MFXOptions {
    mfxIMPL impl; // OPTION_IMPL

    char SourceName[MSDK_MAX_PATH]; // OPTION_FSOURCE
    char SinkName[MSDK_MAX_PATH];   // OPTION_FSINK

    mfxU16 Width; // OPTION_GEOMETRY
    mfxU16 Height;

    mfxU16 Bitrate; // OPTION_BITRATE

    mfxU16 FrameRateN; // OPTION_FRAMERATE
    mfxU16 FrameRateD;

    bool MeasureLatency; // OPTION_MEASURE_LATENCY
} MFXOptions;


typedef struct MFXRuntimeVars {
  MFXVideoSession session;
  mfxFrameAllocator* mfxAllocator;
  mfxVideoParam* mfxEncParams;
  mfxFrameAllocResponse* mfxResponse;
  mfxU16* taskPoolSize;
  Task* pTasks;
  mfxFrameSurface1*** pmfxSurfaces;
  mfxU16* nEncSurfNum;
  mfxEncodeCtrl* ctrl;
} MFXRuntimeVars;



typedef struct StateMachine {
     int videoStream;
     AVFormatContext *pFormatCtx;
     AVCodecContext  *pCodecCtx;
     AVInputFormat *iformat;
     MFXOptions *mfxOptions;
} StateMachine;
