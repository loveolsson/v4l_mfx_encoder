/*****************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or
nondisclosure agreement with Intel Corporation and may not be copied
or disclosed except in accordance with the terms of that agreement.
Copyright(c) 2005-2014 Intel Corporation. All Rights Reserved.

*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "cmd_options.h"


void PrintHelp(CmdOptions* cmd_options)
{
    const char* program = cmd_options->ctx.program;

    if (cmd_options->ctx.usage) {
        cmd_options->ctx.usage(&(cmd_options->ctx));
    } else {
        printf("Usage: %s [options]\n", program);
    }

    printf("\n");
    printf("Options:\n");
    //    ("..............xx....
    printf("  --help        Print this help\n");
    // (...) {
    //  printf("..............xx....
    if (cmd_options->ctx.options & OPTION_IMPL) {
        printf("  -auto         Automatically chose Media SDK library implementation. This is the default.\n");
        printf("  -sw           Load SW Media SDK Library implementation\n");
        printf("  -hw           Load HW Media SDK Library implementation\n");
    }
    if (cmd_options->ctx.options & OPTION_GEOMETRY) {
        printf("  -g WxH        Mandatory. Set input video geometry, i.e. width and height\n");
    }
    if (cmd_options->ctx.options & OPTION_BITRATE) {
        printf("  -b bitrate    Mandatory. Set bitrate with which data should be encoded, in kbps.\n");
    }
    if (cmd_options->ctx.options & OPTION_FRAMERATE) {
        printf("  -f framerate  Mandatory. Set framerate with which data should be encoded in the form '-f nominator/denominator'.\n");
    }
    if (cmd_options->ctx.options & OPTION_MEASURE_LATENCY) {
        printf("  --measure-latency\n");
        printf("                Calculate and print latency statistics. This is the default.\n");
        printf("  --no-measure-latency\n");
        printf("                Do not calculate and do not print latency statistics.\n");
    }
}

void ParseOptions(int argc, char** argv, CmdOptions* cmd_options)
{
    int i;

    if (!cmd_options->ctx.program) {
        cmd_options->ctx.program = argv[0];
    }
    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--help")) {
            PrintHelp(cmd_options);
            exit(0);
        } else if ((cmd_options->ctx.options & OPTION_IMPL) && !strcmp(argv[i], "-sw")) {
            cmd_options->values.impl = MFX_IMPL_SOFTWARE;
        } else if ((cmd_options->ctx.options & OPTION_IMPL) && !strcmp(argv[i], "-hw")) {
            cmd_options->values.impl = MFX_IMPL_HARDWARE;
        } else if ((cmd_options->ctx.options & OPTION_IMPL) && !strcmp(argv[i], "-auto")) {
            cmd_options->values.impl = MFX_IMPL_AUTO;
        } else if ((cmd_options->ctx.options & OPTION_GEOMETRY) && !strcmp(argv[i], "-g")) {
            int width = 0, height = 0;
            if (++i >= argc) {
                printf("error: no argument for -g option given\n");
                exit(-1);
            }
            if ((2 != sscanf(argv[i], "%dx%d", &width, &height)) || (width <= 0) || (height <= 0)) {
                printf("error: incorrect argument for -g option given\n");
                exit(-1);
            }
            cmd_options->values.Width = (mfxU16)width;
            cmd_options->values.Height = (mfxU16)height;
        } else if ((cmd_options->ctx.options & OPTION_BITRATE) && !strcmp(argv[i], "-b")) {
            int bitrate = 0;
            if (++i >= argc) {
                printf("error: no argument for -b option given\n");
                exit(-1);
            }
            bitrate = atoi(argv[i]);
            if (bitrate <= 0) {
                printf("error: incorrect argument for -f option given\n");
                exit(-1);
            }
            cmd_options->values.Bitrate = (mfxU16)bitrate;
        } else if ((cmd_options->ctx.options & OPTION_FRAMERATE) && !strcmp(argv[i], "-f")) {
            int frN = 0, frD = 0;
            if (++i >= argc) {
                printf("error: no argument for -f option given\n");
                exit(-1);
            }
            if ((2 != sscanf(argv[i], "%d/%d", &frN, &frD)) || (frN <= 0) || (frD <= 0)) {
                printf("error: incorrect argument for -f option given\n");
                exit(-1);
            }
            cmd_options->values.FrameRateN = (mfxU16)frN;
            cmd_options->values.FrameRateD = (mfxU16)frD;
        } else if ((cmd_options->ctx.options & OPTION_MEASURE_LATENCY) && !strcmp(argv[i], "--measure-latency")) {
            cmd_options->values.MeasureLatency = true;
        } else if ((cmd_options->ctx.options & OPTION_MEASURE_LATENCY) && !strcmp(argv[i], "--no-measure-latency")) {
            cmd_options->values.MeasureLatency = false;
        } else if (argv[i][0] == '-') {
            printf("error: unsupported option '%s'\n", argv[i]);
            exit(-1);
        } else {
            // we met fist non-option, i.e. argument
            break;
        }
    }
    if (i < argc) {
        if (strlen(argv[i]) < MSDK_MAX_PATH) {
            strcpy(cmd_options->values.SourceName, argv[i]);
        } else {
            printf("error: source file name is too long\n");
            exit(-1);
        }
        ++i;
    }
    if (i < argc) {
        if (strlen(argv[i]) < MSDK_MAX_PATH) {
            strcpy(cmd_options->values.SinkName, argv[i]);
        } else {
            printf("error: destination file name is too long\n");
            exit(-1);
        }
        ++i;
    }
    if (i < argc) {
        printf("error: too many arguments\n");
        exit(-1);
    }
}
