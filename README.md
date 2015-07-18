# v4l_mfx_encoder

My attempt to build a seriously lightweight encoder for live streaming using V4L2 and Intel Quicksync, and work on a Intel NUC NUC5i7RYH.

It built for Magewell XI100DUSB, but should work with any capture card able to deliver YUYV422 RAW, or not.

The main aim is to make one optimized for twitch, that means 4Mbit, main profile, GOP 120, and optimized for gaming.

The plan is to create a encoder that grabs from V4L2 and ALSA with libav abstraction, converts from YUYV422 to NV12 and encodes video on hardware, encodes AAC and muxes in software and then exposes a MPEGTS-feed on a TCP socket that accepts multiple connections.
Instances of avconv connects to the TCP socket and then distributes to RTMP-servers.

It depends on:
  -CentOS 7
  -Intel Media SDK and it's custom libdrm and libav
  -libav
  -avconv
