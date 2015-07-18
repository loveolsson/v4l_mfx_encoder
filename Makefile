EXEC=hwencoder

SRCS= \
	hwencoder.cpp \
 	libav.cpp \
  mfx.cpp \
  common/common_utils.cpp \
  common/common_utils_linux.cpp \
  common/common_vaapi.cpp \
  common/cmd_options.cpp

CFLAGS=-I/usr/local/include -Icommon -I$(MFX_HOME)/include
LFLAGS=-L$(MFX_HOME)/lib/lin_x64 -lmfx -lva -lva-drm -lpthread -lrt -ldl
LIBS=-I/usr/local/include  -pthread -L/usr/local/lib -lswscale -lavdevice -lavformat -lavcodec -lX11 -lasound -lbz2 -lz -lavresample -lavutil -lm

$(EXEC): $(SRCS) Makefile
	g++ -o $(EXEC) $(SRCS) $(CFLAGS) $(LFLAGS) $(LIBS) -std=c++11

.PHONY: clean
clean:
	rm $(EXEC)
