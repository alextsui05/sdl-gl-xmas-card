#include <stdio.h>
#include <math.h>
#include <iostream>
#include <vector>

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

    // ccw (+) rotation in radians
    void rotate(double theta)
    {
        GLfloat tx = x * cos(theta) - y * sin(theta);
        GLfloat ty = x * sin(theta) + y * cos(theta);
        x = tx;
        y = ty;
    }

    void scale(double s)
    {
        x *= s;
        y *= s;
        z *= s;
    }

    void translate(double tx, double ty)
    {
        x += tx;
        y += ty;
    }
};

struct view_context {
    GLfloat x;
    GLfloat y;
    GLfloat zoom;
    std::vector< point > verts;
    std::vector< point > star_verts;
    view_context():
        x(0),
        y(0),
        zoom(1)
    { 
        verts.push_back(point(0.0, 0.5, 0.0));
        verts.push_back(point(-0.5, -0.5, 0.0));
        verts.push_back(point(0.5, -0.5, 0.0));

        star_verts.push_back(point(0.0, 0.5, 0.0));
        star_verts.push_back(point(-0.5, -0.5, 0.0));
        star_verts.push_back(point(0.5, -0.5, 0.0));
        star_verts.push_back(point(0.0, 0.5, 0.0));
        star_verts.push_back(point(-0.5, -0.5, 0.0));
        star_verts.push_back(point(0.5, -0.5, 0.0));
        for (int i = 0; i < 6; ++i)
            star_verts[i].scale(0.1);
        for (int i = 3; i < 6; ++i)
            star_verts[i].rotate(M_PI);
        for (int i = 0; i < 3; ++i)
            star_verts[i].translate(0, 0.533);
        for (int i = 3; i < 6; ++i)
            star_verts[i].translate(0, 0.5);
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
    bool is_red;
    int count;
    bool done;
    bool initialized;
    bool fps_limited;
    GLuint shaderProgram;
    GLuint uniformColorShader;
    GLuint vao;
    GLuint vbo[2];
    GLuint vPositionId;
    GLuint vPositionId2;
    
    GLint uniformOriginX;
    GLint uniformOriginY;
    GLint uniformZoom;
    GLint uniformColor;

    view_context view;

    context(int w = 640, int h = 480):
      width(w),
      height(h),
      window(0),
      renderer(0),
      tex(0),
      color(0),
      is_red(true),
      count(0),
      done(false),
      initialized(true),
      fps_limited(true)
    {
        dest.x = 200;
        dest.y = 100;

        // initialize window + OpenGL context
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("SDL GL Christmas Card", 0, 0, width, height,
            SDL_WINDOW_OPENGL);
        glcontext = SDL_GL_CreateContext(window);
        
        // initialize OpenGL extensions
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cout << "Failed to initialize GLEW\n";
            initialized = false;
        }

        // send vertex data to the GPU... Nope, this is for OpenGL 3+
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(2, vbo);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER,
            view.verts.size() * sizeof(point),
            &(view.verts[0]),
            GL_STATIC_DRAW);
        loadPositionalShader( );
        vPositionId = glGetAttribLocation(shaderProgram, "vPosition");
        glVertexAttribPointer(vPositionId, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER,
            view.star_verts.size() * sizeof(point),
            &(view.star_verts[0]),
            GL_STATIC_DRAW);
        loadUniformColorShader( );
        vPositionId2 = glGetAttribLocation(uniformColorShader, "vPosition");
        glVertexAttribPointer(vPositionId2, 3, GL_FLOAT, GL_FALSE, 0, 0);

        //loadShaders();

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0, 0, width, height);
        SDL_GL_SwapWindow(window);

        //for (auto& vert : view.verts) {
        //    std::cout << vert.x << " ";
        //    std::cout << vert.y << " ";
        //    std::cout << vert.z << "\n";
        //}
    }

    void loadShaders() {
        loadPositionalShader();
        loadUniformColorShader();
    }

    void loadPositionalShader()
    {
        // compile the positional color shader
        GLuint vertexShader = loadShaderFromFile(GL_VERTEX_SHADER,
            "shaders/positional.vs");
        GLuint fragmentShader = loadShaderFromFile(GL_FRAGMENT_SHADER,
            "shaders/color.fs");
        shaderProgram = buildProgram(vertexShader, fragmentShader, "vPosition");

        //save location of uniform variables
        uniformOriginX = glGetUniformLocation(shaderProgram, "originX");
        uniformOriginY = glGetUniformLocation(shaderProgram, "originY");
        uniformZoom = glGetUniformLocation(shaderProgram, "zoom");
    }

    void loadUniformColorShader()
    {
        // compile the uniform color shader
        GLuint uniformColorVertexShader = loadShaderFromFile(GL_VERTEX_SHADER,
            "shaders/color.vs");
        GLuint fragmentShader2 = loadShaderFromFile(GL_FRAGMENT_SHADER,
            "shaders/color.fs");
        uniformColorShader = buildProgram(uniformColorVertexShader,
            fragmentShader2, "vPosition");

        uniformColor = glGetUniformLocation(uniformColorShader, "uColor");
    }

    operator bool() const
    {
        return initialized;
    }
};

void main_loop(void*);

void print_text()
{
    std::cout << "Merry Christmas!\n"
        << "\n"
        << "I'd like to think of this as a modern handwritten Christmas card.\n"
        << "It's written in C++ and compiled to Javascript.\n"
        << "It's an example of the wonderful world we live in.\n"
        << "But it wouldn't be as wonderful if I didn't have you as a friend.\n"
        << "Thanks for being a part of it.\n"
        << "\n"
        << "Best wishes in 2017,\n"
        << "\n"
        << "Alex\n";
}

int main()
{
  // initialize context
  context ctx;
  if (!ctx)
      return 1;

  print_text();

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
    // christmas light background effect
    if (ctx->is_red)
        glClearColor(ctx->color / 255.0f, 0, 0, 1);
    else
        glClearColor(0, ctx->color / 255.0f, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ctx->color = (ctx->color + 1) % 256;
    if (ctx->color == 0) {
        ctx->is_red = ! (ctx->is_red);
    }

    //draw the triangle (aka xmas tree)
    glUseProgram(ctx->shaderProgram);
    glUniform1f(ctx->uniformOriginX, ctx->view.x);
    glUniform1f(ctx->uniformOriginY, ctx->view.y);
    glUniform1f(ctx->uniformZoom, ctx->view.zoom);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo[0]);
    GLint vPositionId = glGetAttribLocation(ctx->shaderProgram, "vPosition");
    glVertexAttribPointer(vPositionId, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vPositionId);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // draw the star on top of the tree
    glUseProgram(ctx->uniformColorShader);
    glUniform3f(ctx->uniformColor, 1, 140 / 255.0f, 0);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo[1]);
    vPositionId = glGetAttribLocation(ctx->uniformColorShader, "vPosition");
    glVertexAttribPointer(vPositionId, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vPositionId);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    SDL_GL_SwapWindow(ctx->window);
}
