  /*****************************************************************************

  INTEL CORPORATION PROPRIETARY INFORMATION
  This software is supplied under the terms of a license agreement or
  nondisclosure agreement with Intel Corporation and may not be copied
  or disclosed except in accordance with the terms of that agreement.
  Copyright(c) 2005-2014 Intel Corporation. All Rights Reserved.

  *****************************************************************************/

  #include "common_utils.h"
  #include "cmd_options.h"

  #ifdef __cplusplus
  extern "C" {
  #endif
  #include <libavdevice/avdevice.h>
  #include <libavformat/avformat.h>
  #ifdef __cplusplus
  }
  #endif
  #include "libav.h"



  void convert_yuv422_to_yuv420(uint8_t *InBuff, mfxFrameData* OutBuff, int width,int height)
  {
      int i = 0, j = 0, k = 0;
      int UOffset = width * height;
      int UVSize = (width * height) / 4;
      int line1 = 0, line2 = 0;
      int m = 0, n = 0;
      int y = 0, u = 0, v = 0;
      mfxU8* ptrY = OutBuff->Y;
      mfxU8* prtUV = OutBuff->UV;

      for (i = 0, j = 1; i < height; i += 2, j += 2)
      {
          /* Input Buffer Pointer Indexes */
          line1 = i * width * 2;
          line2 = j * width * 2;

          /* Output Buffer Pointer Indexes */
          m = width * y;
          y = y + 1;
          n = width * y;
          y = y + 1;

          /* Scan two lines at a time */
          for (k = 0; k < width*2; k += 4)
          {
              unsigned char Y1, Y2, U, V;
              unsigned char Y3, Y4, U2, V2;

              /* Read Input Buffer */
              Y1 = InBuff[line1++];
              U  = InBuff[line1++];
              Y2 = InBuff[line1++];
              V  = InBuff[line1++];

              Y3 = InBuff[line2++];
              U2 = InBuff[line2++];
              Y4 = InBuff[line2++];
              V2 = InBuff[line2++];

              /* Write Output Buffer */
              ptrY[m++] = Y1;
              ptrY[m++] = Y2;

              ptrY[n++] = Y3;
              ptrY[n++] = Y4;

              prtUV[u++] = (U + U2)/2;
              prtUV[u++] = (V + V2)/2;
          }
      }
  }

  MFXVideoSession session;
  mfxFrameAllocator mfxAllocator;
  mfxVideoParam mfxEncParams;
  mfxFrameAllocResponse mfxResponse;
  mfxU16 taskPoolSize;
  Task* pTasks;
  mfxFrameSurface1** pmfxSurfaces;
  mfxU16 nEncSurfNum;
  mfxEncodeCtrl ctrl;



  int mfxInit (StateMachine *stateMachine) {
    mfxStatus sts = MFX_ERR_NONE;
    MFXOptions *options = stateMachine->mfxOptions;
    AVFormatContext *pFormatCtxVideo = stateMachine->pFormatCtxVideo;

    // =====================================================================
    // Intel Media SDK encode pipeline setup
    // - In this example we are encoding an AVC (H.264) stream
    // - Video memory surfaces are used to store the raw frames
    //   (Note that when using HW acceleration video surfaces are prefered, for better performance)
    //

    // Read options from the command line (if any is given)
    //options.ctx.options = OPTIONS_ENCODE;
    // Set default values:

    options->Width = (mfxU16)pFormatCtxVideo->streams[stateMachine->videoStream]->codec->width;
    options->Height = (mfxU16)pFormatCtxVideo->streams[stateMachine->videoStream]->codec->height;

    options->FrameRateN = (mfxU16)pFormatCtxVideo->streams[stateMachine->videoStream]->avg_frame_rate.num;
    options->FrameRateD = (mfxU16)pFormatCtxVideo->streams[stateMachine->videoStream]->avg_frame_rate.den;


    memset(&ctrl, 0, sizeof(mfxEncodeCtrl));


    // Initialize Intel Media SDK session
    // - MFX_IMPL_AUTO_ANY selects HW acceleration if available (on any adapter)
    // - Version 1.0 is selected for greatest backwards compatibility.
    //   If more recent API features are needed, change the version accordingly
    mfxIMPL impl = MFX_IMPL_AUTO_ANY;
    mfxVersion ver = { {0, 1} };




    sts = Initialize(impl, ver, &session, &mfxAllocator);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Initialize encoder parameters
    memset(&mfxEncParams, 0, sizeof(mfxEncParams));
    mfxEncParams.mfx.CodecId = MFX_CODEC_AVC;
    mfxEncParams.mfx.CodecProfile = MFX_PROFILE_AVC_MAIN;
    mfxEncParams.mfx.CodecLevel = MFX_LEVEL_AVC_32;
    mfxEncParams.mfx.TargetUsage =  MFX_TARGETUSAGE_1;
    mfxEncParams.mfx.TargetKbps = options->Bitrate;
    mfxEncParams.mfx.RateControlMethod = MFX_RATECONTROL_CBR;
    mfxEncParams.mfx.FrameInfo.FrameRateExtN = options->FrameRateN;
    mfxEncParams.mfx.FrameInfo.FrameRateExtD = options->FrameRateD;
    mfxEncParams.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    mfxEncParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    mfxEncParams.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    mfxEncParams.mfx.FrameInfo.CropX = 0;
    mfxEncParams.mfx.FrameInfo.CropY = 0;
    mfxEncParams.mfx.FrameInfo.CropW = options->Width;
    mfxEncParams.mfx.FrameInfo.CropH = options->Height;

    mfxEncParams.mfx.GopOptFlag = MFX_GOP_STRICT;
    mfxEncParams.mfx.GopPicSize = options->FrameRateN * 2;
    mfxEncParams.mfx.GopRefDist = 0;

    printf("%i\n", mfxEncParams.mfx.GopPicSize);
    //mfxEncParams.mfx.GopRefDist = 1; // Seems to not be honored at all.
    // Width must be a multiple of 16
    // Height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    mfxEncParams.mfx.FrameInfo.Width = MSDK_ALIGN16(options->Width);
    mfxEncParams.mfx.FrameInfo.Height =
        (MFX_PICSTRUCT_PROGRESSIVE == mfxEncParams.mfx.FrameInfo.PicStruct) ?
        MSDK_ALIGN16(options->Height) :
        MSDK_ALIGN32(options->Height);

    mfxEncParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;

    // Configure Media SDK to keep more operations in flight
    // - AsyncDepth represents the number of tasks that can be submitted, before synchronizing is required
    // - The choice of AsyncDepth = 4 is quite arbitrary but has proven to result in good performance
    mfxEncParams.AsyncDepth = 4;

    // Create Media SDK encoder
//    MFXVideoENCODE mfxENC(session);

    // Validate video encode parameters (optional)
    // - In this example the validation result is written to same structure
    // - MFX_WRN_INCOMPATIBLE_VIDEO_PARAM is returned if some of the video parameters are not supported,
    //   instead the encoder will select suitable parameters closest matching the requested configuration
    sts = MFXVideoENCODE_Query(session, &mfxEncParams, &mfxEncParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Query number of required surfaces for encoder
    mfxFrameAllocRequest EncRequest;
    memset(&EncRequest, 0, sizeof(EncRequest));
    sts = MFXVideoENCODE_QueryIOSurf(session, &mfxEncParams, &EncRequest);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    EncRequest.NumFrameSuggested = EncRequest.NumFrameSuggested + mfxEncParams.AsyncDepth;

    EncRequest.Type |= WILL_WRITE; // This line is only required for Windows DirectX11 to ensure that surfaces can be written to by the application

    // Allocate required surfaces
    sts = mfxAllocator.Alloc(mfxAllocator.pthis, &EncRequest, &mfxResponse);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    nEncSurfNum = mfxResponse.NumFrameActual;

    // Allocate surface headers (mfxFrameSurface1) for decoder
    pmfxSurfaces = new mfxFrameSurface1 *[nEncSurfNum];
    MSDK_CHECK_POINTER(pmfxSurfaces, MFX_ERR_MEMORY_ALLOC);
    for (int i = 0; i < nEncSurfNum; i++) {
        pmfxSurfaces[i] = new mfxFrameSurface1;
        memset(pmfxSurfaces[i], 0, sizeof(mfxFrameSurface1));
        memcpy(&(pmfxSurfaces[i]->Info), &(mfxEncParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
        pmfxSurfaces[i]->Data.MemId = mfxResponse.mids[i];      // MID (memory id) represent one video NV12 surface
        ClearYUVSurfaceVMem(pmfxSurfaces[i]->Data.MemId);

    }

    // Initialize the Media SDK encoder
    sts = MFXVideoENCODE_Init(session, &mfxEncParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Retrieve video parameters selected by encoder.
    // - BufferSizeInKB parameter is required to set bit stream buffer size
    mfxVideoParam par;
    memset(&par, 0, sizeof(par));
    sts = MFXVideoENCODE_GetVideoParam(session, &par);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Create task pool to improve asynchronous performance (greater GPU utilization)
    taskPoolSize = mfxEncParams.AsyncDepth;  // number of tasks that can be submitted, before synchronizing is required
    pTasks = new Task[taskPoolSize];
    memset(pTasks, 0, sizeof(Task) * taskPoolSize);
    for (int i = 0; i < taskPoolSize; i++) {
        // Prepare Media SDK bit stream buffer
        pTasks[i].mfxBS.MaxLength = par.mfx.BufferSizeInKB * 1000;
        pTasks[i].mfxBS.Data = new mfxU8[pTasks[i].mfxBS.MaxLength];
        MSDK_CHECK_POINTER(pTasks[i].mfxBS.Data, MFX_ERR_MEMORY_ALLOC);
    }


}

  int mfxEncoderLoop(StateMachine *stateMachine) {
    // ===================================
    // Start encoding the frames
    //

    mfxTime tStart, tEnd;
    mfxGetTime(&tStart);

    int nEncSurfIdx = 0;
    int nTaskIdx = 0;
    int nFirstSyncTask = 0;
    mfxU32 nFrame = 0;
    int sts;
    int nextRaw = -1;
    AVPacket* rawPacket;
    //MFXVideoENCODE mfxENC = *mfxENC_;


    FILE* fSink = NULL;
        MSDK_FOPEN(fSink, "test.h264", "wb");
        MSDK_CHECK_POINTER(fSink, MFX_ERR_NULL_PTR);



    //
    // Stage 1: Main encoding loop
    //
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts) {
        nTaskIdx = GetFreeTaskIndex(pTasks, taskPoolSize);      // Find free task
        if (MFX_ERR_NOT_FOUND == nTaskIdx) {
            // No more free tasks, need to sync
            sts = session.SyncOperation(pTasks[nFirstSyncTask].syncp, 60000);
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            sts = WriteBitStreamFrame(&pTasks[nFirstSyncTask].mfxBS, fSink);
            MSDK_BREAK_ON_ERROR(sts);
                                pTasks[nFirstSyncTask].mfxBS.DataLength = 0;

            pTasks[nFirstSyncTask].syncp = NULL;
            nFirstSyncTask = (nFirstSyncTask + 1) % taskPoolSize;

            ++nFrame;
            printf("Frame number: %d\n", nFrame);
            fflush(stdout);
        } else {
            nextRaw = rawPacket_nextWritten(stateMachine);
            if (nextRaw >= 0) {

              nEncSurfIdx = GetFreeSurfaceIndex(pmfxSurfaces, nEncSurfNum);   // Find free frame surface
              MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nEncSurfIdx, MFX_ERR_MEMORY_ALLOC);

              // Surface locking required when read/write D3D surfaces
              sts = mfxAllocator.Lock(mfxAllocator.pthis, pmfxSurfaces[nEncSurfIdx]->Data.MemId, &(pmfxSurfaces[nEncSurfIdx]->Data));
              MSDK_BREAK_ON_ERROR(sts);

              rawPacket = &stateMachine->rawPacket[nextRaw];
              printf("Packet size:%i Buffer:%i\n", rawPacket->size, nextRaw);

              if (rawPacket->size == stateMachine->mfxOptions->Width * stateMachine->mfxOptions->Height * 2)
                convert_yuv422_to_yuv420(rawPacket->data, &pmfxSurfaces[nEncSurfIdx]->Data, stateMachine->mfxOptions->Width, stateMachine->mfxOptions->Height);

              rawPacket_markRead(stateMachine, nextRaw);

              av_free_packet(rawPacket);

              sts = mfxAllocator.Unlock(mfxAllocator.pthis, pmfxSurfaces[nEncSurfIdx]->Data.MemId, &(pmfxSurfaces[nEncSurfIdx]->Data));
              MSDK_BREAK_ON_ERROR(sts);

              for (;;) {
                  // Encode a frame asychronously (returns immediately)
                  sts = MFXVideoENCODE_EncodeFrameAsync(session, &ctrl, pmfxSurfaces[nEncSurfIdx], &pTasks[nTaskIdx].mfxBS, &pTasks[nTaskIdx].syncp);

                  if (MFX_ERR_NONE < sts && !pTasks[nTaskIdx].syncp) {    // Repeat the call if warning and no output
                      if (MFX_WRN_DEVICE_BUSY == sts)
                          MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
                  } else if (MFX_ERR_NONE < sts
                             && pTasks[nTaskIdx].syncp) {
                      sts = MFX_ERR_NONE;     // Ignore warnings if output is available
                      break;
                  } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                      // Allocate more bitstream buffer memory here if needed...
                      break;
                  } else
                      break;
              }
            } else { //No frame to read
              MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            }
        }
    }

    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);


    while (pTasks[nFirstSyncTask].syncp) {
        sts = session.SyncOperation(pTasks[nFirstSyncTask].syncp, 60000);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        pTasks[nFirstSyncTask].mfxBS.DataLength = 0;


        pTasks[nFirstSyncTask].syncp = NULL;
        nFirstSyncTask = (nFirstSyncTask + 1) % taskPoolSize;

        ++nFrame;
        printf("Frame number: %d\n", nFrame);
        fflush(stdout);
    }

    mfxGetTime(&tEnd);
    double elapsed = TimeDiffMsec(tEnd, tStart) / 1000;
    double fps = ((double)nFrame / elapsed);
    printf("\nExecution time: %3.2f s (%3.2f fps)\n", elapsed, fps);

    // ===================================================================
    // Clean up resources
    //  - It is recommended to close Media SDK components first, before releasing allocated surfaces, since
    //    some surfaces may still be locked by internal Media SDK resources.

    MFXVideoENCODE_Close(session);
    // session closed automatically on destruction

    for (int i = 0; i < nEncSurfNum; i++)
        delete pmfxSurfaces[i];
    MSDK_SAFE_DELETE_ARRAY(pmfxSurfaces);
    for (int i = 0; i < taskPoolSize; i++)
        MSDK_SAFE_DELETE_ARRAY(pTasks[i].mfxBS.Data);
    MSDK_SAFE_DELETE_ARRAY(pTasks);

    mfxAllocator.Free(mfxAllocator.pthis, &mfxResponse);


    return 0;
  }



  int copyRawFrame(StateMachine *stateMachine, AVPacket *packet) {
    return 0;
}
