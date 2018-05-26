CFLAGS = --std=c++11 -g -ftree-vectorize -O3
INC=-I.seen/src
LINK=.seen/lib/libseen.a

ifeq ($(shell uname),Darwin)
	GLFW_LINK +=-lpthread -lm -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
	GLFW_LINK += -lopencv_videoio
else
	GLFW_LINK +=-lglfw3 -lGL -lX11 -lXi -lXrandr -lXxf86vm -lXinerama -lXcursor -lrt -lm -pthread -ldl
	CFLAGS += -D_XOPEN_SOURCE=500
endif

.seen:
	git clone https://github.com/mrpossoms/Seen .seen

.seen/lib/libseen.a: .seen
	make -C .seen static

all: .seen/lib/libseen.a
	g++ $(INC) $(CFLAGS) main.cpp -o stereo -lpng $(GLFW_LINK) $(LINK)

clean:
	rm stereo	
