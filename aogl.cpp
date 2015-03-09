#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>

#include "glew/glew.h"

#include "GLFW/glfw3.h"
#include "stb/stb_image.h"
#include "imgui/imgui.h"
#include "imgui/imguiRenderGL3.h"

#include "glm/glm.hpp"
#include "glm/vec3.hpp" // glm::vec3
#include "glm/vec4.hpp" // glm::vec4, glm::ivec4
#include "glm/mat4x4.hpp" // glm::mat4
#include "glm/gtc/matrix_transform.hpp" // glm::translate, glm::rotate, glm::scale, glm::perspective
#include "glm/gtc/type_ptr.hpp" // glm::value_ptr

#ifndef DEBUG_PRINT
#define DEBUG_PRINT 1
#endif

#if DEBUG_PRINT == 0
#define debug_print(FORMAT, ...) ((void)0)
#else
#ifdef _MSC_VER
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __func__, __FILE__, __LINE__, __VA_ARGS__)
#endif
#endif

// Font buffers
extern const unsigned char DroidSans_ttf[];
extern const unsigned int DroidSans_ttf_len;    

// Shader utils
int check_link_error(GLuint program);
int check_compile_error(GLuint shader, const char ** sourceBuffer);
GLuint compile_shader(GLenum shaderType, const char * sourceBuffer, int bufferSize);
GLuint compile_shader_from_file(GLenum shaderType, const char * fileName);

// OpenGL utils
bool checkError(const char* title);

struct Camera
{
    float radius;
    float theta;
    float phi;
    glm::vec3 o;
    glm::vec3 eye;
    glm::vec3 up;
};
void camera_defaults(Camera & c);
void camera_zoom(Camera & c, float factor);
void camera_turn(Camera & c, float phi, float theta);
void camera_pan(Camera & c, float x, float y);

struct GUIStates
{
    bool panLock;
    bool turnLock;
    bool zoomLock;
    int lockPositionX;
    int lockPositionY;
    int camera;
    double time;
    bool playing;
    static const float MOUSE_PAN_SPEED;
    static const float MOUSE_ZOOM_SPEED;
    static const float MOUSE_TURN_SPEED;
};
const float GUIStates::MOUSE_PAN_SPEED = 0.001f;
const float GUIStates::MOUSE_ZOOM_SPEED = 0.05f;
const float GUIStates::MOUSE_TURN_SPEED = 0.005f;
void init_gui_states(GUIStates & guiStates);


int main( int argc, char **argv )
{
    int width = 1024, height= 768;
    float widthf = (float) width, heightf = (float) height;
    double t;
    float fps = 0.f;

    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        exit( EXIT_FAILURE );
    }
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    int const DPI = 2; // For retina screens only
#else
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    int const DPI = 1;
# endif

    // Open a window and create its OpenGL context
    GLFWwindow * window = glfwCreateWindow(width/DPI, height/DPI, "aogl", 0, 0);
    if( ! window )
    {
        fprintf( stderr, "Failed to open GLFW window\n" );
        glfwTerminate();
        exit( EXIT_FAILURE );
    }
    glfwMakeContextCurrent(window);

    // Init glew
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
          /* Problem: glewInit failed, something is seriously wrong. */
          fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
          exit( EXIT_FAILURE );
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode( window, GLFW_STICKY_KEYS, GL_TRUE );

    // Enable vertical sync (on cards that support it)
    glfwSwapInterval( 1 );
    GLenum glerr = GL_NO_ERROR;
    glerr = glGetError();

    if (!imguiRenderGLInit(DroidSans_ttf, DroidSans_ttf_len))
    {
        fprintf(stderr, "Could not init GUI renderer.\n");
        exit(EXIT_FAILURE);
    }

    // Init viewer structures
    Camera camera;
    camera_defaults(camera);
    GUIStates guiStates;
    init_gui_states(guiStates);

    // Try to load and compile shaders
    GLuint vertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "aogl.vert");
    GLuint fragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "aogl.frag");
    GLuint geomShaderId = compile_shader_from_file(GL_GEOMETRY_SHADER, "aogl.geom");
    GLuint lightVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "light.vert");
    GLuint lightFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "light.frag");
    GLuint blitVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "blit.vert");
    GLuint blitFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "blit.frag");
    GLuint pointLightVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "pointLight.vert");
    GLuint pointLightFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "pointLight.frag");
    GLuint spotLightVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "spotLight.vert");
    GLuint spotLightFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "spotLight.frag");
    GLuint directionalLightVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "directionalLight.vert");
    GLuint directionalLightFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "directionalLight.frag");
    GLuint shadowVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "shadow.vert");
    GLuint shadowFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "shadow.frag");
    GLuint gammaVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "gamma.vert");
    GLuint gammaFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "gamma.frag");
    GLuint sobelVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "sobel.vert");
    GLuint sobelFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "sobel.frag");
    GLuint blurVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "blur.vert");
    GLuint blurFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "blur.frag");
    GLuint cocVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "coc.vert");
    GLuint cocFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "coc.frag");
    // programs
    GLuint programObject = glCreateProgram();
    GLuint programLight = glCreateProgram();
    GLuint programBlit = glCreateProgram();
    GLuint programSpotLight = glCreateProgram();
    GLuint programDirectionalLight = glCreateProgram();
    GLuint programPointLight = glCreateProgram();
    GLuint programShadow = glCreateProgram();
    GLuint programGamma = glCreateProgram();
    GLuint programSobel = glCreateProgram();
    GLuint programBlur = glCreateProgram();
    GLuint programCoc = glCreateProgram();
    // scene
    glAttachShader(programObject, vertShaderId);
    glAttachShader(programObject, fragShaderId);
    glAttachShader(programObject, geomShaderId);
    glLinkProgram(programObject);
    if (check_link_error(programObject) < 0)
        exit(1);
    // light render (GL_POINTS)
    glAttachShader(programLight, lightVertShaderId);
    glAttachShader(programLight, lightFragShaderId);
    glLinkProgram(programLight);
    if (check_link_error(programLight) < 0)
        exit(1);
    // blit screen
    glAttachShader(programBlit, blitVertShaderId);
    glAttachShader(programBlit, blitFragShaderId);
    glLinkProgram(programBlit);
    if(check_link_error(programBlit) < 0)
        exit(1);
    // pointlight
    glAttachShader(programPointLight, pointLightVertShaderId);
    glAttachShader(programPointLight, pointLightFragShaderId);
    glLinkProgram(programPointLight);
    if(check_link_error(programPointLight))
        exit(1);
    // spotlight
    glAttachShader(programSpotLight, spotLightFragShaderId);
    glAttachShader(programSpotLight, spotLightVertShaderId);
    glLinkProgram(programSpotLight);
    if(check_link_error(programSpotLight))
        exit(1);
    // directional light
    glAttachShader(programDirectionalLight, directionalLightFragShaderId);
    glAttachShader(programDirectionalLight, directionalLightVertShaderId);
    glLinkProgram(programDirectionalLight);
    if(check_link_error(programDirectionalLight))
        exit(1);
    // shadow
    glAttachShader(programShadow, shadowFragShaderId);
    glAttachShader(programShadow, shadowVertShaderId);
    glLinkProgram(programShadow);
    if(check_link_error(programShadow))
        exit(1);
    // gamma
    glAttachShader(programGamma, gammaFragShaderId);
    glAttachShader(programGamma, gammaVertShaderId);
    glLinkProgram(programGamma);
    if(check_link_error(programGamma))
        exit(1);
    // sobel
    glAttachShader(programSobel, sobelFragShaderId);
    glAttachShader(programSobel, sobelVertShaderId);
    glLinkProgram(programSobel);
    if(check_link_error(programSobel))
        exit(1);
    // blur
    glAttachShader(programBlur, blurFragShaderId);
    glAttachShader(programBlur, blurVertShaderId);
    glLinkProgram(programBlur);
    if(check_link_error(programBlur))
        exit(1);
    // Circle of Confusion
    glAttachShader(programCoc, cocFragShaderId);
    glAttachShader(programCoc, blitVertShaderId);
    glLinkProgram(programCoc);
    if(check_link_error(programCoc))
        exit(1);
//    if (!checkError("Uniforms"))
//        exit(1);

    // Viewport 
    glViewport( 0, 0, width, height  );

    // Framebuffer object handle
    GLuint gbufferFbo;
    // Texture handles
    GLuint gbufferTextures[3];
    glGenTextures(3, gbufferTextures);
    // 2 draw buffers for color and normal
    GLuint gbufferDrawBuffers[2];

    // Create color texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create normal texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create depth texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create Framebuffer Object
    glGenFramebuffers(1, &gbufferFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFbo);
    // Initialize DrawBuffers
    gbufferDrawBuffers[0] = GL_COLOR_ATTACHMENT0;
    gbufferDrawBuffers[1] = GL_COLOR_ATTACHMENT1;
    glDrawBuffers(2, gbufferDrawBuffers);

    // Attach textures to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, gbufferTextures[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1 , GL_TEXTURE_2D, gbufferTextures[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gbufferTextures[2], 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Error on building framebuffer\n");
        exit( EXIT_FAILURE );
    }

    // Back to the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ///////////////////////
    /// Create shadow FBO
    ///////////////////////
    GLuint shadowFbo;
    glGenFramebuffers(1, &shadowFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);

    // Create a render buffer since we don't need to read shadow color
    // in a texture
    int res = 1024;
    GLuint shadowRenderBuffer;
    glGenRenderbuffers(1, &shadowRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, shadowRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, res, res);
    // Attach the renderbuffer
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, shadowRenderBuffer);

    // Texture handles
    GLuint shadowTexture;
    glGenTextures(1, &shadowTexture);
    // Create shadow texture
    glBindTexture(GL_TEXTURE_2D, shadowTexture);
    // Create empty texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, res, res, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    // Bilinear filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // 2×2 Percentage Closer Filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC,  GL_LEQUAL);
    // Color needs to be 0 outside of texture coordinates
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Attach the shadow texture to the depth attachment
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTexture, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Error on building framebuffer\n");
        exit( EXIT_FAILURE );
    }

    // Back to the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ///////////////////
    /// Texture Light
    ///////////////////

    GLuint gbufferFboLight;
    GLuint gbufferTextureLight;
    glGenTextures(1, &gbufferTextureLight);
    GLuint gbufferDrawBuffersLight;

    // Create color texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextureLight);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create Framebuffer Object
    glGenFramebuffers(1, &gbufferFboLight);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFboLight);
    // Initialize DrawBuffers
    gbufferDrawBuffersLight = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &gbufferDrawBuffersLight);

    // Attach textures to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, gbufferTextureLight, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Error on building framebuffer\n");
        exit( EXIT_FAILURE );
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ////////////
    /// FX FBO
    ////////////

    // Create Fx Framebuffer Object
    GLuint fxFbo;
    GLuint fxDrawBuffers[1];
    glGenFramebuffers(1, &fxFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fxFbo);
    fxDrawBuffers[0] = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, fxDrawBuffers);

    // Create Fx textures
    const int FX_TEXTURE_COUNT = 4;
    GLuint fxTextures[FX_TEXTURE_COUNT];
    glGenTextures(FX_TEXTURE_COUNT, fxTextures);
    for (int i = 0; i < FX_TEXTURE_COUNT; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, fxTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // Attach first fx texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[0], 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Error on building framebuffern");
        exit( EXIT_FAILURE );
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ///////////////////
    /// Init OpenGL
    ///////////////////

    int cube_triangleCount = 12;
    int cube_triangleList[] = {0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 19, 17, 20, 21, 22, 23, 24, 25, 26, };
    float cube_uvs[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f,  1.f, 0.f,  1.f, 1.f,  0.f, 1.f,  1.f, 1.f,  0.f, 0.f, 0.f, 0.f, 1.f, 1.f,  1.f, 0.f,  };
    float cube_vertices[] = {-0.5, -0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5 };
    float cube_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, };

    int plane_triangleCount = 2;
    int plane_triangleList[] = {0, 1, 2, 2, 1, 3};
    float plane_uvs[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f};
    float plane_vertices[] = {-5.0, -1.0, 5.0, 5.0, -1.0, 5.0, -5.0, -1.0, -5.0, 5.0, -1.0, -5.0};
    float plane_normals[] = {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0};

    int   quad_triangleCount = 2;
    int   quad_triangleList[] = {0, 1, 2, 2, 1, 3};
    float quad_vertices[] =  {-1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0};

    // Quad
    GLuint vaoQuad;
    glGenVertexArrays(1, &vaoQuad);
    GLuint vboQuad[2];
    glGenBuffers(2, vboQuad);

    glBindVertexArray(vaoQuad);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboQuad[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_triangleList), quad_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vboQuad[1]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    //CUBE
    GLuint vao;
    glGenVertexArrays(1, &vao);
    GLuint vbo[4];
    glGenBuffers(4, vbo);

    glBindVertexArray(vao);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_triangleList), cube_triangleList, GL_STATIC_DRAW);

    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);

    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_uvs), cube_uvs, GL_STATIC_DRAW);

    // Unbind everything
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // TRIANGLE
    GLuint vao2;
    glGenVertexArrays(1, &vao2);
    GLuint vbo2[4];
    glGenBuffers(4, vbo2);

    glBindVertexArray(vao2);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo2[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plane_triangleList), plane_triangleList, GL_STATIC_DRAW);

    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo2[1]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vertices), plane_vertices, GL_STATIC_DRAW);

    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo2[2]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_normals), plane_normals, GL_STATIC_DRAW);

    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo2[3]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_uvs), plane_uvs, GL_STATIC_DRAW);

    // Unbind everything
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Lights
    int specularPower = 30;
    // Point lights
    int pointLightCount = 0;
    float pointLightPos[0]/* = {-3,3,0,3,3,0}*/;
    float pointLightColor[0] /*= {1,0,0,0,1,0}*/;
    float pointLightIntensity[0]/* = {0.5, 0.5}*/;
    // Spot lights
    int spotLightCount = 2;
    float spotLightDirection[] = {-0.4,-1,0, 0,-1,-0.4};
    float spotLightPos[] = {1,3,0, -1,2,0};
    float spotLightColor[] = {0.5,0.5,1, 1,0.2,0.8};
    float spotLightIntensity[] = {1,1};
    float spotLightAngle[] = {60.0,70.0};
    float bias = 0.005;
    // Directional lights
    int dirLightCount = 0;
    float dirLightDirection[0] /*= {1,1,0,0,0,1}*/;
    float dirLightColor[0] /*= {0,0.5,1,1,0.5,0}*/;
    float dirLightIntensity[0] /*= {1, 1}*/;
    // Gamma
    float gamma = 1;
    // Sobel
    float factorSobel = 0.5;
    // Blur
    float sampleCount = 1;
    glm::ivec2 directionBlur(-1,0);
    // Circle of Confusion
    glm::vec3 focus(1,3,10);

    // Point of light
    float lightsPos[(pointLightCount+spotLightCount)*3];
    float lightsColor[(pointLightCount+spotLightCount)*3];

    for(int i=0; i<pointLightCount; ++i)
    {
        lightsPos[i*3] = pointLightPos[i*3];
        lightsPos[i*3+1] = pointLightPos[i*3+1];
        lightsPos[i*3+2] = pointLightPos[i*3+2];
        lightsColor[i*3] = pointLightColor[i*3];
        lightsColor[i*3+1] = pointLightColor[i*3+1];
        lightsColor[i*3+2] = pointLightColor[i*3+2];
    }
    for(int i=0; i<spotLightCount; ++i)
    {
        lightsPos[(i+pointLightCount)*3] = spotLightPos[i*3];
        lightsPos[(i+pointLightCount)*3+1] = spotLightPos[i*3+1];
        lightsPos[(i+pointLightCount)*3+2] = spotLightPos[i*3+2];
        lightsColor[(i+pointLightCount)*3] = spotLightColor[i*3];
        lightsColor[(i+pointLightCount)*3+1] = spotLightColor[i*3+1];
        lightsColor[(i+pointLightCount)*3+2] = spotLightColor[i*3+2];
    }

    // Create a Vertex Array Object
    GLuint vaoLight;
    glGenVertexArrays(1, &vaoLight);
    GLuint vboLight;
    glGenBuffers(1, &vboLight);

    glBindVertexArray(vaoLight);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vboLight);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lightsPos), lightsPos, GL_STATIC_DRAW);

    // Unbind everything        for(int i=0; i<12; ++i)
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Charger les textures
    int x;
    int y;
    int comp;
    unsigned char * diffuse = stbi_load("textures/spnza_bricks_a_diff.tga", &x, &y, &comp, 3);

    GLuint texture[2];
    glGenTextures(2, &texture[0]);

    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, diffuse);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, texture[1]);
    diffuse = stbi_load("textures/spnza_bricks_a_spec.tga", &x, &y, &comp, 3);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, diffuse);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /// Initialize uniform location
    GLuint timeLocation = glGetUniformLocation(programObject, "Time");
    GLuint cameraLocation = glGetUniformLocation(programObject, "Camera");
    GLuint specularLocation = glGetUniformLocation(programObject, "specularPower");
    GLuint diffuseLocation2 = glGetUniformLocation(programObject, "Diffuse2");
    GLuint diffuseLocation = glGetUniformLocation(programObject, "Diffuse");
    GLuint lightColorLocation = glGetUniformLocation(programLight, "lightColor");

    GLuint mvpLocation = glGetUniformLocation(programObject, "MVP");
    GLuint mvpLocation2 = glGetUniformLocation(programLight, "MVP");
    GLuint screenToWorldLocation = glGetUniformLocation(programPointLight, "ScreenToWorld");
    GLuint screenToWorldLocation2 = glGetUniformLocation(programSpotLight, "ScreenToWorld");
    GLuint screenToWorldLocation3 = glGetUniformLocation(programDirectionalLight, "ScreenToWorld");
    GLuint objectToLightScreenLocation = glGetUniformLocation(programShadow, "objectToLightScreen");
    GLuint worldToLightScreenLocation = glGetUniformLocation(programShadow, "worldToLightScreen");
    GLuint objectToLightLocation = glGetUniformLocation(programShadow, "objectToLight");

    // Blit screen shader
    GLuint isDepthLocation = glGetUniformLocation(programBlit, "isDepth");
    GLuint textureLocation = glGetUniformLocation(programBlit, "Texture");

    // Point light shader
    GLuint colorBufferLocation = glGetUniformLocation(programPointLight, "ColorBuffer");
    GLuint normalBufferLocation = glGetUniformLocation(programPointLight, "NormalBuffer");
    GLuint depthBufferLocation = glGetUniformLocation(programPointLight, "DepthBuffer");
    GLuint pointLightPosLocation = glGetUniformLocation(programPointLight, "pointLightPosition");
    GLuint pointLightColorLocation = glGetUniformLocation(programPointLight, "pointLightColor");
    GLuint pointLightIntensityLocation = glGetUniformLocation(programPointLight, "pointLightIntensity");
    GLuint cameraLocation2 = glGetUniformLocation(programPointLight, "Camera");

    // Spot light shader
    GLuint colorBufferLocation2 = glGetUniformLocation(programSpotLight, "ColorBuffer");
    GLuint normalBufferLocation2 = glGetUniformLocation(programSpotLight, "NormalBuffer");
    GLuint depthBufferLocation2 = glGetUniformLocation(programSpotLight, "DepthBuffer");
    GLuint spotLightPosLocation = glGetUniformLocation(programSpotLight, "spotLightPosition");
    GLuint spotLightDirectionLocation = glGetUniformLocation(programSpotLight, "spotLightDirection");
    GLuint spotLightColorLocation = glGetUniformLocation(programSpotLight, "spotLightColor");
    GLuint spotLightIntensityLocation = glGetUniformLocation(programSpotLight, "spotLightIntensity");
    GLuint spotLightAngleLocation = glGetUniformLocation(programSpotLight, "spotLightAngle");
    GLuint cameraLocation3 = glGetUniformLocation(programSpotLight, "Camera");
    GLuint shadowLocation = glGetUniformLocation(programSpotLight, "Shadow");
    GLuint worldToLightScreenLocation_sl = glGetUniformLocation(programSpotLight, "worldToLightScreen");
    GLuint biasLocation = glGetUniformLocation(programSpotLight, "bias");

    // Directional light shader
    GLuint colorBufferLocation3 = glGetUniformLocation(programDirectionalLight, "ColorBuffer");
    GLuint normalBufferLocation3 = glGetUniformLocation(programDirectionalLight, "NormalBuffer");
    GLuint depthBufferLocation3 = glGetUniformLocation(programDirectionalLight, "DepthBuffer");
    GLuint dirLightDirectionLocation = glGetUniformLocation(programDirectionalLight, "directionalLightDirection");
    GLuint dirLightColorLocation = glGetUniformLocation(programDirectionalLight, "directionalLightColor");
    GLuint dirLightIntensityLocation = glGetUniformLocation(programDirectionalLight, "directionalLightIntensity");
    GLuint cameraLocation4 = glGetUniformLocation(programDirectionalLight, "Camera");

    // Gamma
    GLuint gammaLocation = glGetUniformLocation(programGamma, "Gamma");
    // Sobel
    GLuint sobelLocation = glGetUniformLocation(programSobel, "Factor");
    // Blur
    GLuint sampleCountLocation = glGetUniformLocation(programBlur, "SampleCount");
    GLuint directionLocation = glGetUniformLocation(programBlur, "Direction");
    // Circle of Confusion
    GLuint focusLocation = glGetUniformLocation(programCoc, "Focus");
    GLuint screenToViewCocLocation = glGetUniformLocation(programCoc, "ScreenToView");

    glProgramUniform1i(programObject, diffuseLocation, 0);
    glProgramUniform1i(programObject, diffuseLocation2, 1);
    glProgramUniform1i(programObject, specularLocation, specularPower);
    // Blit screen shader
    glProgramUniform1i(programBlit, textureLocation, 0);
    glProgramUniform1i(programBlit, isDepthLocation, 0);
    // Point light shader
    glProgramUniform1i(programPointLight, colorBufferLocation, 0);
    glProgramUniform1i(programPointLight, normalBufferLocation, 1);
    glProgramUniform1i(programPointLight, depthBufferLocation, 2);
    // Spot light shader
    glProgramUniform1i(programSpotLight, colorBufferLocation2, 0);
    glProgramUniform1i(programSpotLight, normalBufferLocation2, 1);
    glProgramUniform1i(programSpotLight, depthBufferLocation2, 2);
    glProgramUniform1i(programSpotLight, shadowLocation, 3);
    // Directional light shader
    glProgramUniform1i(programDirectionalLight, colorBufferLocation3, 0);
    glProgramUniform1i(programDirectionalLight, normalBufferLocation3, 1);
    glProgramUniform1i(programDirectionalLight, depthBufferLocation3, 2);

    do
    {
        t = glfwGetTime();
        // Upload value
        glProgramUniform1f(programObject, timeLocation, t);

        // Mouse states
        int leftButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_LEFT );
        int rightButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_RIGHT );
        int middleButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_MIDDLE );

        if( leftButton == GLFW_PRESS )
            guiStates.turnLock = true;
        else
            guiStates.turnLock = false;

        if( rightButton == GLFW_PRESS )
            guiStates.zoomLock = true;
        else
            guiStates.zoomLock = false;

        if( middleButton == GLFW_PRESS )
            guiStates.panLock = true;
        else
            guiStates.panLock = false;

        // Camera movements
        int altPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);
        if (!altPressed && (leftButton == GLFW_PRESS || rightButton == GLFW_PRESS || middleButton == GLFW_PRESS))
        {
            double x; double y;
            glfwGetCursorPos(window, &x, &y);
            guiStates.lockPositionX = x;
            guiStates.lockPositionY = y;
        }
        if (altPressed == GLFW_PRESS)
        {
            double mousex; double mousey;
            glfwGetCursorPos(window, &mousex, &mousey);
            int diffLockPositionX = mousex - guiStates.lockPositionX;
            int diffLockPositionY = mousey - guiStates.lockPositionY;
            if (guiStates.zoomLock)
            {
                float zoomDir = 0.0;
                if (diffLockPositionX > 0)
                    zoomDir = -1.f;
                else if (diffLockPositionX < 0 )
                    zoomDir = 1.f;
                camera_zoom(camera, zoomDir * GUIStates::MOUSE_ZOOM_SPEED);
            }
            else if (guiStates.turnLock)
            {
                camera_turn(camera, diffLockPositionY * GUIStates::MOUSE_TURN_SPEED,
                            diffLockPositionX * GUIStates::MOUSE_TURN_SPEED);

            }
            else if (guiStates.panLock)
            {
                camera_pan(camera, diffLockPositionX * GUIStates::MOUSE_PAN_SPEED,
                            diffLockPositionY * GUIStates::MOUSE_PAN_SPEED);
            }
            guiStates.lockPositionX = mousex;
            guiStates.lockPositionY = mousey;
        }

        glProgramUniform3f(programObject, cameraLocation, camera.eye.x, camera.eye.y, camera.eye.z);
        glProgramUniform3f(programPointLight, cameraLocation2, camera.eye.x, camera.eye.y, camera.eye.z);
        glProgramUniform3f(programSpotLight, cameraLocation3, camera.eye.x, camera.eye.y, camera.eye.z);
        glProgramUniform3f(programDirectionalLight, cameraLocation4, camera.eye.x, camera.eye.y, camera.eye.z);

        // Enable point size control in vertex shader
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);


        /// Get camera matrices
        glm::mat4 projection = glm::perspective(45.0f, widthf / heightf, 0.1f, 100.f); 
        glm::mat4 worldToView = glm::lookAt(camera.eye, camera.o, camera.up);
        glm::mat4 objectToWorld;
        glm::mat4 mvp = projection * worldToView * objectToWorld;
        glm::mat4 inverseProjection = glm::inverse(projection);
        // Compute the inverse worldToScreen matrix
        glm::mat4 screenToWorld = glm::transpose(glm::inverse(mvp));

        // Light space matrices
        // From light space to shadow map screen space
        glm::mat4 projection2 = glm::perspective(glm::radians(spotLightAngle[0]*2.f), 1.f, 1.f, 100.f);
        // From world to light
        glm::vec3 spotLightDirectionVec(spotLightDirection[0], spotLightDirection[1], spotLightDirection[2]);
        glm::vec3 spotLightPosVec(spotLightPos[0], spotLightPos[1], spotLightPos[2]);
        glm::mat4 worldToLight = glm::lookAt(spotLightPosVec, spotLightPosVec + spotLightDirectionVec, glm::vec3(0.f, 0.f, -1.f));
        // From object to light (MV for light)
        glm::mat4 objectToLight = worldToLight * objectToWorld;
        // From object to shadow map screen space (MVP for light)
        glm::mat4 objectToLightScreen = projection2 * objectToLight;
        // From world to shadow map screen space
        glm::mat4 worldToLightScreen = projection2 * worldToLight;

        // Upload uniforms
        glProgramUniformMatrix4fv(programObject, mvpLocation, 1, 0, glm::value_ptr(mvp));
        glProgramUniformMatrix4fv(programLight, mvpLocation2, 1, 0, glm::value_ptr(mvp));
        glProgramUniformMatrix4fv(programPointLight, screenToWorldLocation, 1, 0, glm::value_ptr(screenToWorld));
        glProgramUniformMatrix4fv(programSpotLight, screenToWorldLocation2, 1, 0, glm::value_ptr(screenToWorld));
        glProgramUniformMatrix4fv(programDirectionalLight, screenToWorldLocation3, 1, 0, glm::value_ptr(screenToWorld));
        glProgramUniformMatrix4fv(programShadow, objectToLightScreenLocation, 1, 0, glm::value_ptr(objectToLightScreen));
        glProgramUniformMatrix4fv(programShadow, worldToLightScreenLocation, 1, 0, glm::value_ptr(worldToLightScreen));
        glProgramUniformMatrix4fv(programShadow, objectToLightLocation, 1, 0, glm::value_ptr(objectToLight));
        glProgramUniformMatrix4fv(programSpotLight, worldToLightScreenLocation_sl, 1, 0, glm::value_ptr(worldToLightScreen));

        //////////////
        /// shadow FBO
        //////////////
        // Default states
        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
        //Set the viewport corresponding to shadow texture resolution
        glViewport(0, 0, res, res);

        // Clear only the depth buffer
        glClear(GL_DEPTH_BUFFER_BIT);

        // Select shader
        glUseProgram(programShadow);

        // Render the scene
        glBindVertexArray(vao);
        glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, 1);
        glBindVertexArray(vao2);
        glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        // Fallback to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Revert to window size viewport
        glViewport(0, 0, width, height);

        /////////////////
        /// gbuffer FBO
        /////////////////

        glBindFramebuffer(GL_FRAMEBUFFER, gbufferFbo);

        // Clear the gbuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Select shader
        glUseProgram(programObject);

        // Render vaos
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture[1]);

        glBindVertexArray(vao);
        glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, 4);
        glBindVertexArray(vao2);
        glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        ///////////////////
        /// Main Viewport
        ///////////////////
        // Unbind the frambuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        // Enable blending
        glEnable(GL_BLEND);
        // Setup additive blending
        glBlendFunc(GL_ONE, GL_ONE);

        glViewport( 0, 0, width, height);
        glBindVertexArray(vaoQuad);
        glUseProgram(programPointLight);

        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);

        glActiveTexture(GL_TEXTURE0 + 3);
        glBindTexture(GL_TEXTURE_2D, shadowTexture);

        /////////////////////
        /// gBuffer FBO Light
        //////////////////////
        glBindFramebuffer(GL_FRAMEBUFFER, gbufferFboLight);

        // Clear the gbuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Point light
        for (int i = 0; i < pointLightCount; ++i)
        {
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

            glProgramUniform1fv(programPointLight, pointLightIntensityLocation, 1, &pointLightIntensity[i]);
            glProgramUniform3fv(programPointLight, pointLightPosLocation, 1, &pointLightPos[i*3]);
            glProgramUniform3fv(programPointLight, pointLightColorLocation, 1, &pointLightColor[i*3]);
        }


        // Spot light
        glUseProgram(programSpotLight);
        for (int i = 0; i < spotLightCount; ++i)
        {
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

            glProgramUniform3fv(programSpotLight, spotLightPosLocation, 1, &spotLightPos[i*3]);
            glProgramUniform3fv(programSpotLight, spotLightDirectionLocation, 1, &spotLightDirection[i*3]);
            glProgramUniform3fv(programSpotLight, spotLightColorLocation, 1, &spotLightColor[i*3]);
            glProgramUniform1fv(programSpotLight, spotLightIntensityLocation, 1, &spotLightIntensity[i]);
            glProgramUniform1fv(programSpotLight, spotLightAngleLocation, 1, &spotLightAngle[i]);
            glProgramUniform1fv(programSpotLight, biasLocation, 1, &bias);
        }


        // Directional light
        glUseProgram(programDirectionalLight);
        for (int i = 0; i < dirLightCount; ++i)
        {
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

            glProgramUniform3fv(programDirectionalLight, dirLightDirectionLocation, 1, &dirLightDirection[i*3]);
            glProgramUniform3fv(programDirectionalLight, dirLightColorLocation, 1, &dirLightColor[i*3]);
            glProgramUniform1fv(programDirectionalLight, dirLightIntensityLocation, 1, &dirLightIntensity[i]);
        }

        // Unbind the frambuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glActiveTexture(GL_TEXTURE0);

        ////////////
        /// Sobel
        ////////////
        glBindFramebuffer(GL_FRAMEBUFFER, fxFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[0], 0);
        glClear(GL_COLOR_BUFFER_BIT);

        // Bind the quad vao if not already bound
        glBindVertexArray(vaoQuad);
        glUseProgram(programSobel);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbufferTextureLight);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        ////////////////////
        /// Circle of confusion
        ////////////////////
        // Use circle of confusion program shader
        glUseProgram(programCoc);

        // Write into Circle of Confusion Texture
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[3], 0);
        glProgramUniformMatrix4fv(programCoc, screenToViewCocLocation, 1, 0, glm::value_ptr(inverseProjection));
        // Clear the content of  texture
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        // Read the depth texture
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        ////////////
        /// Blur
        ////////////
        // Use blur program shader
        glUseProgram(programBlur);

        // Write into Vertical Blur Texture
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[1], 0);
        // Clear the content of texture
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        directionBlur = glm::ivec2(0, 1);
        glProgramUniform2iv(programBlur, directionLocation,1, &directionBlur[0]);
        // Read the texture processed by the Sobel operator
        glBindTexture(GL_TEXTURE_2D, fxTextures[0]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        directionBlur = glm::ivec2(1, 0);
        glProgramUniform2iv(programBlur, directionLocation, 1, &directionBlur[0]);
        // Write into Horizontal Blur Texture
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[2], 0);
        // Clear the content of texture
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        // Read the texture processed by the Vertical Blur
        glBindTexture(GL_TEXTURE_2D, fxTextures[1]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        ////////////
        /// Gamma
        ////////////

        // Default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        // Set quad as vao
        glBindVertexArray(vaoQuad);

        // Gamma
        glUseProgram(programGamma);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fxTextures[2]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        //////////////////
        /// Light point
        //////////////////

        // Disable blending
        glDisable(GL_BLEND);

        // draw light point (GL_POINTS)
        glUseProgram(programLight);
        glBindVertexArray(vaoLight);
        glDrawArrays(GL_POINTS, 0, pointLightCount+spotLightCount);

        /////////////////
        /// blit screens
        /////////////////

        // Use the blit program
        glActiveTexture(GL_TEXTURE0);
        glUseProgram(programBlit);
        glBindVertexArray(vaoQuad);
        glProgramUniform1i(programBlit, isDepthLocation, 0);

        // Viewport1
        glViewport( 0, 0, width/5, height/5);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        // Viewport2
        glViewport( width/5, 0, width/5, height/5);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        // Viewport3
        glViewport( 2*width/5, 0, width/5, height/5);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
        glProgramUniform1i(programBlit, isDepthLocation, 1);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        // Viewport4
        glViewport( 3*width/5, 0, width/5, height/5);
        glBindTexture(GL_TEXTURE_2D, shadowTexture);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        // Viewport5
        glViewport( 4*width/5, 0, width/5, height/5);
        glBindTexture(GL_TEXTURE_2D, fxTextures[3]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

#if 1
        // Draw UI
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, width, height);

        unsigned char mbut = 0;
        int mscroll = 0;
        double mousex; double mousey;
        glfwGetCursorPos(window, &mousex, &mousey);
        mousex*=DPI;
        mousey*=DPI;
        mousey = height - mousey;

        if( leftButton == GLFW_PRESS )
            mbut |= IMGUI_MBUT_LEFT;

        imguiBeginFrame(mousex, mousey, mbut, mscroll);
        int logScroll = 0;
        char lineBuffer[512];
        imguiBeginScrollArea("aogl", width - 210, height - 610, 200, 600, &logScroll);
        sprintf(lineBuffer, "FPS %f", fps);
        imguiLabel(lineBuffer);
        imguiLabel("Point Light");
        for(int i=0; i<pointLightCount; ++i)
        {
            imguiSlider("Intensity", &pointLightIntensity[i], 0.0, 1.0, 0.05);
            imguiSlider("R", &pointLightColor[i*3], 0.0, 1.0, 0.05);
            imguiSlider("G", &pointLightColor[i*3+1], 0.0, 1.0, 0.05);
            imguiSlider("B", &pointLightColor[i*3+2], 0.0, 1.0, 0.05);

            // Update
            lightsColor[i*3] = pointLightColor[i*3];
            lightsColor[i*3+1] = pointLightColor[i*3+1];
            lightsColor[i*3+2] = pointLightColor[i*3+2];
        }

        imguiLabel("Spot Light");
        for(int i=0; i<spotLightCount; ++i)
        {
            imguiSlider("Intensity", &spotLightIntensity[i], 0.0, 1.0, 0.05);
            imguiSlider("R", &spotLightColor[i*3], 0.0, 1.0, 0.05);
            imguiSlider("G", &spotLightColor[i*3+1], 0.0, 1.0, 0.05);
            imguiSlider("B", &spotLightColor[i*3+2], 0.0, 1.0, 0.05);
            imguiSlider("angle", &spotLightAngle[i], 0.0, 90, 1);

            // Update
            lightsColor[(i+pointLightCount)*3] = spotLightColor[i*3];
            lightsColor[(i+pointLightCount)*3+1] = spotLightColor[i*3+1];
            lightsColor[(i+pointLightCount)*3+2] = spotLightColor[i*3+2];
        }
        imguiSlider("Bias", &bias, 0.0, 0.2, 0.001);
        imguiSlider("Gamma", &gamma, 0, 6, 0.1);
        imguiSlider("Sobel", &factorSobel, 0, 2, 0.01);
        imguiSlider("SampleCount", &sampleCount, 1, 15, 1);
        imguiSlider("Near", &focus.x, 1, 15, 1);
        imguiSlider("Focus", &focus.y, 1, 15, 1);
        imguiSlider("Far", &focus.z, 1, 50, 1);

        imguiLabel("Directional Light");
        for(int i=0; i<dirLightCount; ++i)
        {
            imguiSlider("Intensity", &dirLightIntensity[i], 0.0, 1.0, 0.05);
        }

        glProgramUniform1f(programGamma, gammaLocation, gamma);
        glProgramUniform1f(programSobel, sobelLocation, factorSobel);
        glProgramUniform1i(programBlur, sampleCountLocation, sampleCount);
        glProgramUniform3fv(programCoc, focusLocation, 1, &focus[0]);
        glProgramUniform3fv(programLight, lightColorLocation, 4, &lightsColor[0]);

        imguiEndScrollArea();
        imguiEndFrame();
        imguiRenderGLDraw(width, height);

        glDisable(GL_BLEND);
#endif

        // Check for errors
        checkError("End loop");

        glfwSwapBuffers(window);
        glfwPollEvents();

        double newTime = glfwGetTime();
        fps = 1.f/ (newTime - t);
    } // Check if the ESC key was pressed
    while( glfwGetKey( window, GLFW_KEY_ESCAPE ) != GLFW_PRESS );

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    exit( EXIT_SUCCESS );
}

// No windows implementation of strsep
char * strsep_custom(char **stringp, const char *delim)
{
    register char *s;
    register const char *spanp;
    register int c, sc;
    char *tok;
    if ((s = *stringp) == NULL)
        return (NULL);
    for (tok = s; ; ) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
    return 0;
}

int check_compile_error(GLuint shader, const char ** sourceBuffer)
{
    // Get error log size and print it eventually
    int logLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        char * log = new char[logLength];
        glGetShaderInfoLog(shader, logLength, &logLength, log);
        char *token, *string;
        string = strdup(sourceBuffer[0]);
        int lc = 0;
        while ((token = strsep(&string, "\n")) != NULL) {
           printf("%3d : %s\n", lc, token);
           ++lc;
        }
        fprintf(stderr, "Compile : %s", log);
        delete[] log;
    }
    // If an error happend quit
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
        return -1;     
    return 0;
}

int check_link_error(GLuint program)
{
    // Get link error log size and print it eventually
    int logLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        char * log = new char[logLength];
        glGetProgramInfoLog(program, logLength, &logLength, log);
        fprintf(stderr, "Link : %s \n", log);
        delete[] log;
    }
    int status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);        
    if (status == GL_FALSE)
        return -1;
    return 0;
}


GLuint compile_shader(GLenum shaderType, const char * sourceBuffer, int bufferSize)
{
    GLuint shaderObject = glCreateShader(shaderType);
    const char * sc[1] = { sourceBuffer };
    glShaderSource(shaderObject, 
                   1, 
                   sc,
                   NULL);
    glCompileShader(shaderObject);
    check_compile_error(shaderObject, sc);
    return shaderObject;
}

GLuint compile_shader_from_file(GLenum shaderType, const char * path)
{
    FILE * shaderFileDesc = fopen( path, "rb" );
    if (!shaderFileDesc)
        return 0;
    fseek ( shaderFileDesc , 0 , SEEK_END );
    long fileSize = ftell ( shaderFileDesc );
    rewind ( shaderFileDesc );
    char * buffer = new char[fileSize + 1];
    fread( buffer, 1, fileSize, shaderFileDesc );
    buffer[fileSize] = '\0';
    GLuint shaderObject = compile_shader(shaderType, buffer, fileSize );
    delete[] buffer;
    return shaderObject;
}


bool checkError(const char* title)
{
    int error;
    if((error = glGetError()) != GL_NO_ERROR)
    {
        std::string errorString;
        switch(error)
        {
        case GL_INVALID_ENUM:
            errorString = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            errorString = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            errorString = "GL_INVALID_OPERATION";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            errorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            errorString = "GL_OUT_OF_MEMORY";
            break;
        default:
            errorString = "UNKNOWN";
            break;
        }
        fprintf(stdout, "OpenGL Error(%s): %s\n", errorString.c_str(), title);
    }
    return error == GL_NO_ERROR;
}

void camera_compute(Camera & c)
{
    c.eye.x = cos(c.theta) * sin(c.phi) * c.radius + c.o.x;   
    c.eye.y = cos(c.phi) * c.radius + c.o.y ;
    c.eye.z = sin(c.theta) * sin(c.phi) * c.radius + c.o.z;   
    c.up = glm::vec3(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
}

void camera_defaults(Camera & c)
{
    c.phi = 3.14/2.f;
    c.theta = 3.14/2.f;
    c.radius = 10.f;
    camera_compute(c);
}

void camera_zoom(Camera & c, float factor)
{
    c.radius += factor * c.radius ;
    if (c.radius < 0.1)
    {
        c.radius = 10.f;
        c.o = c.eye + glm::normalize(c.o - c.eye) * c.radius;
    }
    camera_compute(c);
}

void camera_turn(Camera & c, float phi, float theta)
{
    c.theta += 1.f * theta;
    c.phi   -= 1.f * phi;
    if (c.phi >= (2 * M_PI) - 0.1 )
        c.phi = 0.00001;
    else if (c.phi <= 0 )
        c.phi = 2 * M_PI - 0.1;
    camera_compute(c);
}

void camera_pan(Camera & c, float x, float y)
{
    glm::vec3 up(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
    glm::vec3 fwd = glm::normalize(c.o - c.eye);
    glm::vec3 side = glm::normalize(glm::cross(fwd, up));
    c.up = glm::normalize(glm::cross(side, fwd));
    c.o[0] += up[0] * y * c.radius * 2;
    c.o[1] += up[1] * y * c.radius * 2;
    c.o[2] += up[2] * y * c.radius * 2;
    c.o[0] -= side[0] * x * c.radius * 2;
    c.o[1] -= side[1] * x * c.radius * 2;
    c.o[2] -= side[2] * x * c.radius * 2;       
    camera_compute(c);
}

void init_gui_states(GUIStates & guiStates)
{
    guiStates.panLock = false;
    guiStates.turnLock = false;
    guiStates.zoomLock = false;
    guiStates.lockPositionX = 0;
    guiStates.lockPositionY = 0;
    guiStates.camera = 0;
    guiStates.time = 0.0;
    guiStates.playing = false;
}
