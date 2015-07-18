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

#include "struct.h"



int initMFX (StateMachine *stateMachine)
{
    AVFormatContext *pFormatCtx = stateMachine->pFormatCtx;
    int videoStreamId = *stateMachine->videoStream;
    MFXOptions *options = stateMachine->mfxOptions;



    mfxStatus sts = MFX_ERR_NONE;
    bool bEnableInput;  // if true, removes all YUV file reading (which is replaced by pre-initialized surface data). Workload runs for 1000 frames.
    bool bEnableOutput; // if true, removes all output bitsteam file writing and printing the progress

    // =====================================================================
    // Intel Media SDK encode pipeline setup
    // - In this example we are encoding an AVC (H.264) stream
    // - Video memory surfaces are used to store the raw frames
    //   (Note that when using HW acceleration video surfaces are prefered, for better performance)
    //

    // Read options from the command line (if any is given)
    //options->options = OPTIONS_ENCODE;
    // Set default values:
    options->impl = MFX_IMPL_HARDWARE;

    // here we parse options
    //ParseOptions(argc, argv, &options);

    options->Width = (mfxU16)pFormatCtx->streams[videoStreamId]->codec->width;
    options->Height = (mfxU16)pFormatCtx->streams[videoStreamId]->codec->height;

    options->FrameRateN = (mfxU16)pFormatCtx->streams[videoStreamId]->avg_frame_rate.num;
    options->FrameRateD = (mfxU16)pFormatCtx->streams[videoStreamId]->avg_frame_rate.den;

    options->MeasureLatency = false;


    printf("%i %i %i %i\n", options->Width, options->Height, options->FrameRateN, options->FrameRateD);

    // Initialize Intel Media SDK session
    // - MFX_IMPL_AUTO_ANY selects HW acceleration if available (on any adapter)
    // - Version 1.0 is selected for greatest backwards compatibility.
    //   If more recent API features are needed, change the version accordingly
    mfxIMPL impl = options->impl;
    mfxVersion ver = { {0, 1} };
    MFXVideoSession session;

    mfxFrameAllocator mfxAllocator;

    sts = Initialize(impl, ver, &session, &mfxAllocator);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Initialize encoder parameters
    mfxVideoParam mfxEncParams;
    memset(&mfxEncParams, 0, sizeof(mfxEncParams));
    mfxEncParams.mfx.CodecId = MFX_CODEC_AVC;
    mfxEncParams.mfx.CodecProfile = MFX_PROFILE_AVC_MAIN;
    mfxEncParams.mfx.CodecLevel = MFX_LEVEL_AVC_32;
    mfxEncParams.mfx.TargetUsage = MFX_TARGETUSAGE_BALANCED;
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
    MFXVideoENCODE mfxENC(session);

    // Validate video encode parameters (optional)
    // - In this example the validation result is written to same structure
    // - MFX_WRN_INCOMPATIBLE_VIDEO_PARAM is returned if some of the video parameters are not supported,
    //   instead the encoder will select suitable parameters closest matching the requested configuration
    sts = mfxENC.Query(&mfxEncParams, &mfxEncParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    printf("%s\n", "Init MFX");


    // Query number of required surfaces for encoder
    mfxFrameAllocRequest EncRequest;
    memset(&EncRequest, 0, sizeof(EncRequest));
    sts = mfxENC.QueryIOSurf(&mfxEncParams, &EncRequest);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    EncRequest.NumFrameSuggested = EncRequest.NumFrameSuggested + mfxEncParams.AsyncDepth;

    EncRequest.Type |= WILL_WRITE; // This line is only required for Windows DirectX11 to ensure that surfaces can be written to by the application

    // Allocate required surfaces
    mfxFrameAllocResponse mfxResponse;
    sts = mfxAllocator.Alloc(mfxAllocator.pthis, &EncRequest, &mfxResponse);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    mfxU16 nEncSurfNum = mfxResponse.NumFrameActual;

    // Allocate surface headers (mfxFrameSurface1) for decoder
    mfxFrameSurface1** pmfxSurfaces = new mfxFrameSurface1 *[nEncSurfNum];
    MSDK_CHECK_POINTER(pmfxSurfaces, MFX_ERR_MEMORY_ALLOC);
    for (int i = 0; i < nEncSurfNum; i++) {
        pmfxSurfaces[i] = new mfxFrameSurface1;
        memset(pmfxSurfaces[i], 0, sizeof(mfxFrameSurface1));
        memcpy(&(pmfxSurfaces[i]->Info), &(mfxEncParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
        pmfxSurfaces[i]->Data.MemId = mfxResponse.mids[i];      // MID (memory id) represent one video NV12 surface
        if (!bEnableInput) {
            ClearYUVSurfaceVMem(pmfxSurfaces[i]->Data.MemId);
        }
    }

    // Initialize the Media SDK encoder
    sts = mfxENC.Init(&mfxEncParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Retrieve video parameters selected by encoder.
    // - BufferSizeInKB parameter is required to set bit stream buffer size
    mfxVideoParam par;
    memset(&par, 0, sizeof(par));
    sts = mfxENC.GetVideoParam(&par);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Create task pool to improve asynchronous performance (greater GPU utilization)
    mfxU16 taskPoolSize = mfxEncParams.AsyncDepth;  // number of tasks that can be submitted, before synchronizing is required
    Task* pTasks = new Task[taskPoolSize];
    memset(pTasks, 0, sizeof(Task) * taskPoolSize);
    for (int i = 0; i < taskPoolSize; i++) {
        // Prepare Media SDK bit stream buffer
        pTasks[i].mfxBS.MaxLength = par.mfx.BufferSizeInKB * 1000;
        pTasks[i].mfxBS.Data = new mfxU8[pTasks[i].mfxBS.MaxLength];
        MSDK_CHECK_POINTER(pTasks[i].mfxBS.Data, MFX_ERR_MEMORY_ALLOC);
    }



    // ===================================
    // Start encoding the frames
    //

    mfxTime tStart, tEnd;
    mfxGetTime(&tStart);

    int nEncSurfIdx = 0;
    int nTaskIdx = 0;
    int nFirstSyncTask = 0;
    mfxU32 nFrame = 0;

    //
    // Stage 1: Main encoding loop
    //
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts) {
        nTaskIdx = GetFreeTaskIndex(pTasks, taskPoolSize);      // Find free task
        if (MFX_ERR_NOT_FOUND == nTaskIdx) {
            // No more free tasks, need to sync
            sts = session.SyncOperation(pTasks[nFirstSyncTask].syncp, 60000);
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            ////////sts = WriteBitStreamFrame(&pTasks[nFirstSyncTask].mfxBS, fSink);
            MSDK_BREAK_ON_ERROR(sts);

            pTasks[nFirstSyncTask].syncp = NULL;
            nFirstSyncTask = (nFirstSyncTask + 1) % taskPoolSize;

            ++nFrame;
            if (bEnableOutput) {
                printf("Frame number: %d\r", nFrame);
                fflush(stdout);
            }
        } else {
            // nEncSurfIdx = GetFreeSurfaceIndex(pmfxSurfaces, nEncSurfNum);   // Find free frame surface
            // MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nEncSurfIdx, MFX_ERR_MEMORY_ALLOC);
            //
            // // Surface locking required when read/write D3D surfaces
            // sts = mfxAllocator.Lock(mfxAllocator.pthis, pmfxSurfaces[nEncSurfIdx]->Data.MemId, &(pmfxSurfaces[nEncSurfIdx]->Data));
            // MSDK_BREAK_ON_ERROR(sts);
            //
            // sts = LoadRawFrame(pmfxSurfaces[nEncSurfIdx], fSource);
            // MSDK_BREAK_ON_ERROR(sts);
            //
            // sts = mfxAllocator.Unlock(mfxAllocator.pthis, pmfxSurfaces[nEncSurfIdx]->Data.MemId, &(pmfxSurfaces[nEncSurfIdx]->Data));
            // MSDK_BREAK_ON_ERROR(sts);
            //
            // for (;;) {
            //     // Encode a frame asychronously (returns immediately)
            //     sts = mfxENC.EncodeFrameAsync(NULL, pmfxSurfaces[nEncSurfIdx], &pTasks[nTaskIdx].mfxBS, &pTasks[nTaskIdx].syncp);
            //
            //     if (MFX_ERR_NONE < sts && !pTasks[nTaskIdx].syncp) {    // Repeat the call if warning and no output
            //         if (MFX_WRN_DEVICE_BUSY == sts)
            //             MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            //     } else if (MFX_ERR_NONE < sts
            //                && pTasks[nTaskIdx].syncp) {
            //         sts = MFX_ERR_NONE;     // Ignore warnings if output is available
            //         break;
            //     } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
            //         // Allocate more bitstream buffer memory here if needed...
            //         break;
            //     } else
            //         break;
            // }
        }
    }


    // Clean up resources
    //  - It is recommended to close Media SDK components first, before releasing allocated surfaces, since
    //    some surfaces may still be locked by internal Media SDK resources.

    mfxENC.Close();
    // session closed automatically on destruction

    for (int i = 0; i < nEncSurfNum; i++)
        delete pmfxSurfaces[i];
    MSDK_SAFE_DELETE_ARRAY(pmfxSurfaces);
    for (int i = 0; i < taskPoolSize; i++)
        MSDK_SAFE_DELETE_ARRAY(pTasks[i].mfxBS.Data);
    MSDK_SAFE_DELETE_ARRAY(pTasks);

    mfxAllocator.Free(mfxAllocator.pthis, &mfxResponse);


    Release();

    return 0;
}
