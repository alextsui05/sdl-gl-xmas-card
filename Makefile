CC = emcc
CXX= em++
all: hello_tri

hello_tri: hello_tri.cpp shaders.cpp
	g++ $^ -std=c++11 -lSDL2 -lSDL2_image -lGL -lGLEW -o hello_tri
	$(CXX) $^ -O2 -s USE_SDL=2 --preload-file shaders -o hello_tri.html

.PHONY: clean
clean:
	rm hello_tri
