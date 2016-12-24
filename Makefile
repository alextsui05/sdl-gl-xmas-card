CC = emcc
CXX= em++
all: hello_tri hello_owl.c
	$(CC) hello_owl.c -O2 -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' --preload-file assets -o hello_owl.html

hello_tri: hello_tri.cpp shaders.cpp
	g++ $^ -std=c++11 -lSDL2 -lSDL2_image -lGL -o hello_tri
	$(CXX) $^ -O2 -s USE_SDL=2 -s FULL_ES2=1 -o hello_tri.html

.PHONY: clean
clean:
	rm hello_tri
