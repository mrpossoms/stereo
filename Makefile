CFLAGS = --std=c++11 -g -ftree-vectorize -O3

ifeq ($(shell uname),Darwin)
	GLFW_LINK +=-lpthread -lm -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
	GLFW_LINK += -lopencv_videoio
else
	GLFW_LINK +=-lglfw3 -lGL -lX11 -lXi -lXrandr -lXxf86vm -lXinerama -lXcursor -lrt -lm -pthread -ldl
	CFLAGS += -D_XOPEN_SOURCE=500
endif

all:
	g++ $(CFLAGS) main.cpp -o stereo -lseen -lpng $(GLFW_LINK)
