#include <stdio.h>
#include <iostream>
#include <vector>

//#include <GLES2/gl2.h>
//#include <GLES2/gl2ext.h>
#include <GL/glew.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include "shaders.h"

// 1000 ms / 60 frames == 60 frames per second == about 17 ms per frame
#define FRAME_INTERVAL 17

struct point {
    GLfloat x;
    GLfloat y;
    GLfloat z;
    point() {}
    point(double a, double b, double c):
        x((GLfloat) a),
        y((GLfloat) b),
        z((GLfloat) c)
    {}

    point(GLfloat a, GLfloat b, GLfloat c):
        x(a),
        y(b),
        z(c)
    {}
};

struct view_context {
    GLfloat x;
    GLfloat y;
    GLfloat zoom;
    std::vector< point > verts;
    view_context():
        x(0),
        y(0),
        zoom(1)
    { 
        verts.push_back(point(0.0, 0.5, 0.0));
        verts.push_back(point(-0.5, -0.5, 0.0));
        verts.push_back(point(0.5, -0.5, 0.0));
    }
};

struct context {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_GLContext glcontext;
    SDL_Texture* tex;
    SDL_Rect dest;
    int width;
    int height;
    int color;
    int count;
    bool done;
    bool initialized;
    bool fps_limited;
    GLuint shaderProgram;
    GLuint vao;
    GLuint vbo;
    GLint uniformOriginX;
    GLint uniformOriginY;
    GLint uniformZoom;

    view_context view;

    context(int w = 640, int h = 480):
      width(w),
      height(h),
      window(0),
      renderer(0),
      tex(0),
      color(0),
      count(0),
      done(false),
      initialized(true),
      fps_limited(true)
    {
        dest.x = 200;
        dest.y = 100;

        // initialize window
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("SDL GL Triangle", 0, 0, width, height,
            SDL_WINDOW_OPENGL);
        glcontext = SDL_GL_CreateContext(window);
        
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cout << "Failed to initialize GLEW\n";
            initialized = false;
        }

        // send vertex data to the GPU... Nope, this is for OpenGL 3+
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
            view.verts.size() * sizeof(point),
            &(view.verts[0]),
            GL_STATIC_DRAW);

        loadShader();

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0, 0, width, height);
        SDL_GL_SwapWindow(window);

        for (auto& vert : view.verts) {
            std::cout << vert.x << " ";
            std::cout << vert.y << " ";
            std::cout << vert.z << "\n";
        }
    }

    void loadShader() {
        const char vertexShaderSource[] =
            "attribute vec4 vPosition;		                     \n"
            "uniform float originX, originY;                     \n"
            "uniform float zoom;                                 \n"
            "varying vec3 color;                                 \n"
            "void main()                                         \n"
            "{                                                   \n"
            "   gl_Position = vPosition;                         \n"
            "   gl_Position.x = (originX + gl_Position.x) * zoom;\n"
            "   gl_Position.y = (originY + gl_Position.y) * zoom;\n"
            "   color = gl_Position.xyz + vec3(0.5);             \n"
            "}                                                   \n";

        const char fragmentShaderSource[] =
            "precision mediump float;                     \n"
            "varying vec3 color;                          \n"
            "void main()                                  \n"
            "{                                            \n"
            "  gl_FragColor = vec4 ( color, 1.0 );        \n"
            "}                                            \n";
        
        //load vertex and fragment shaders
        GLuint vertexShader = ::loadShader(GL_VERTEX_SHADER, vertexShaderSource);
        GLuint fragmentShader = ::loadShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
        shaderProgram = buildProgram(vertexShader, fragmentShader, "vPosition");

        //save location of uniform variables
        uniformOriginX = glGetUniformLocation(shaderProgram, "originX");
        uniformOriginY = glGetUniformLocation(shaderProgram, "originY");
        uniformZoom = glGetUniformLocation(shaderProgram, "zoom");
    }

    operator bool() const
    {
        return initialized;
    }
};

void main_loop(void*);

int main()
{
  // initialize context
  context ctx;
  if (!ctx)
      return 1;

  printf("you should see an image.\n");
#ifdef EMSCRIPTEN
  emscripten_set_main_loop_arg(main_loop, &ctx, -1, 1);
#else
  while (!ctx.done) {
      int t1 = SDL_GetTicks();
      main_loop(&ctx);
      int t2 = SDL_GetTicks();
      int elapsed = t2 - t1;
      if (ctx.fps_limited && elapsed < FRAME_INTERVAL) {
          int wait_time = FRAME_INTERVAL - elapsed;
          SDL_Delay(wait_time);
      }
  }
#endif

  return 0;
}

void main_loop(void* data)
{
    context* ctx = static_cast<context*>(data);
    ctx->count += 1;

    // handle input event
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if ( ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN )
            || event.type == SDL_QUIT ) {
            ctx->done = true;
            return;
        }
        if ( event.type == SDL_KEYDOWN ) {
            switch ( event.key.keysym.sym ) {
                case SDLK_1:
                    ctx->fps_limited = !(ctx->fps_limited);
                    std::cout << "FPS Limiting ";
                    if ( ctx->fps_limited ) {
                        std::cout << "enabled\n";
                    } else {
                        std::cout << "disabled\n";
                    }
                    break;
                default:
                    break;
            }
        }
    }

    // render frame
    glClearColor(ctx->color / 255.0f, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(ctx->shaderProgram);

    //set up the translation
	glUniform1f(ctx->uniformOriginX, ctx->view.x);
	glUniform1f(ctx->uniformOriginY, ctx->view.y);
	glUniform1f(ctx->uniformZoom, ctx->view.zoom);

	//set up the vertices array GLES2 way
	glEnableVertexAttribArray(0);
    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, &(ctx->view.verts[0]));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	//draw the triangle
	glDrawArrays(GL_TRIANGLES, 0, 3);

    SDL_GL_SwapWindow(ctx->window);

    ctx->color = (ctx->color + 1) % 256;
}
