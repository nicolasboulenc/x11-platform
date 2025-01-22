
// x11 with GL shaders

#include <stdio.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <stdlib.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glext.h>

static int DoubleBufferAttributes[] = {
    GLX_RGBA,
    GLX_RED_SIZE, 1,
    GLX_GREEN_SIZE, 1,
    GLX_BLUE_SIZE, 1,
    GLX_DEPTH_SIZE, 12,
    GLX_DOUBLEBUFFER,
    None,
};

void VerifyOrDie(int ResultStatus, const char *Message) {

    if(ResultStatus == 0) {
        fprintf(stderr, "%s", Message);
        exit(2);
    }
}

void CheckShaderCompilation(unsigned int Shader) {

    int ResultStatus;
    char Buffer[512];
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &ResultStatus);

    if(ResultStatus == 0) {
        glGetShaderInfoLog(Shader, sizeof(Buffer), NULL, Buffer);
        printf("ERROR: Shader compilation failed. -----------------------------------\n");
        printf("%s\n", Buffer);
    }
}

void CheckProgramCompilation(unsigned int Program) {

    int  ResultStatus;
    char Buffer[512];
    glGetProgramiv(Program, GL_COMPILE_STATUS, &ResultStatus);

    if(ResultStatus == 0) {
        glGetShaderInfoLog(Program, sizeof(Buffer), NULL, Buffer);
        printf("ERROR: Failed compiling program\n");
        printf("%s\n", Buffer);
    }
}


int main() {

    Display* MainDisplay = XOpenDisplay(0);
    int MainScreen = XDefaultScreen(MainDisplay);
    Window RootWindow = XDefaultRootWindow(MainDisplay);

    int Dummy;
    int ResultStatus = glXQueryExtension(MainDisplay, &Dummy, &Dummy);
    VerifyOrDie(ResultStatus != 0, "Error: X Server has not GLX extension\n");

    XVisualInfo* VisualInfo = glXChooseVisual(MainDisplay, MainScreen, DoubleBufferAttributes);
    VerifyOrDie(VisualInfo != 0, "glXChooseVisual returned 0");
    VerifyOrDie(VisualInfo->c_class == TrueColor, "No True Color support. Cannot run program without it");

    GLXContext ShareList = None;
    int IsDirectRendering = True;
    GLXContext OpenGLContext = glXCreateContext(MainDisplay, VisualInfo, ShareList, IsDirectRendering);
    VerifyOrDie(OpenGLContext != 0, "ERROR: Couldn't create rendering context\n");

    int WindowX = 0;
    int WindowY = 0;
    int WindowWidth = 800;
    int WindowHeight = 600;
    int BorderWidth = 0;
    int WindowClass = InputOutput;
    int WindowDepth = VisualInfo->depth;
    Visual* WindowVisual = VisualInfo->visual;

    int AttributeValueMask = CWBackPixel | CWEventMask | CWColormap;

    XSetWindowAttributes WindowAttributes = {};
    WindowAttributes.colormap = XCreateColormap(MainDisplay, RootWindow, VisualInfo->visual, AllocNone);
    WindowAttributes.background_pixel = 0xffafe9af;
    WindowAttributes.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | PointerMotionMask;

    Window MainWindow = XCreateWindow(MainDisplay, RootWindow, 
            WindowX, WindowY, WindowWidth, WindowHeight,
            BorderWidth, WindowDepth, WindowClass, WindowVisual,
            AttributeValueMask, &WindowAttributes);

    XStoreName(MainDisplay, MainWindow, "General app");

    glXMakeCurrent(MainDisplay, MainWindow, OpenGLContext);

    XMapWindow(MainDisplay, MainWindow);

    Atom WM_DELETE_WINDOW = XInternAtom(MainDisplay, "WM_DELETE_WINDOW", False);
    if(!XSetWMProtocols(MainDisplay, MainWindow, &WM_DELETE_WINDOW, 1)) {
        printf("Couldn't register WM_DELETE_WINDOW\n");
    }


    /* ------------------------------------------------------------------------------------------------------------ */
    /* ---------- OPENGL ------------------------------------------------------------------------------------------ */
    /* ------------------------------------------------------------------------------------------------------------ */

    float S = 0.5;
    float Vertices[] = {
          -S, -S, 0.0f,     1.0f, 0.0f, 0.0f,
        0.0f,  S, 0.0f,     0.0f, 1.0f, 0.0f,
           S, -S, 0.0f,     0.0f, 0.0f, 1.0f,
    };

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

    printf("INFO: Compiling vertex shader\n");
    const char* VertexShaderSource = 
        "#version 330 core\n"
        "layout (location = 0) in vec3 Pos;"
        "layout (location = 1) in vec3 InColor;"
        "out vec3 Color;"
        ""
        "void main()"
        "{"
        "   gl_Position = vec4(Pos.x, Pos.y, 0.0f, 1.0f);"
        "   Color = InColor;"
        "}\0";
    unsigned int VertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(VertexShader, 1, &VertexShaderSource, NULL);
    glCompileShader(VertexShader);
    CheckShaderCompilation(VertexShader);

    printf("INFO: Compiling fragment shader\n");
    const char* FragmentShaderSource = 
        "#version 330 core\n"
        "out vec4 FragColor;"
        "in vec3 Color;"
        "void main()"
        "{"
        /* "   FragColor = vec4(1.0f, 0.5f, 0.5f, 1.0f);" */
        "   FragColor = vec4(Color, 1.0f);"
        "}\0";
    unsigned int FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(FragmentShader, 1, &FragmentShaderSource, NULL);
    glCompileShader(FragmentShader);
    CheckShaderCompilation(FragmentShader);

    printf("INFO: Compiling program\n");
    unsigned int ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, VertexShader);
    glAttachShader(ShaderProgram, FragmentShader);
    glLinkProgram(ShaderProgram);

    CheckProgramCompilation(ShaderProgram);

    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);

    glUseProgram(ShaderProgram);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void *)(3*sizeof(float)));
    glEnableVertexAttribArray(1);


    /* ------------------------------------------------------------------------------------------------------------ */
    /* ------------------------------------------------------------------------------------------------------------ */
    /* ------------------------------------------------------------------------------------------------------------ */

    int IsProgramRunning = 1;
    while(IsProgramRunning) {

        while(XPending(MainDisplay)) {
            XEvent GeneralEvent = {};
            XNextEvent(MainDisplay, &GeneralEvent);
            switch(GeneralEvent.type) {
                case ClientMessage: 
                {
                    IsProgramRunning = 0;
                } break;
            }
        }

        {
            glClearColor(0.0, 0.0, 0.2, 1.0);
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

            glDrawArrays(GL_TRIANGLES, 0, 3);

            glXSwapBuffers(MainDisplay, MainWindow);
        }
    }
}