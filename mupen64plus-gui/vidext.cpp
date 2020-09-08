#include "vidext.h"
#include "common.h"
#include "workerthread.h"
#include "mainwindow.h"
#include <stdio.h>
#include <QDesktopWidget>
#include <QScreen>
#include <SDL2/SDL.h>

static int init;
static int needs_toggle;
static int set_volume;
static QSurfaceFormat format;
QThread* rendering_thread;

// TEMP
static int screen_mode_w = -1;
static int screen_mode_h = -1;
static int screen_mode_refresh = -1;

m64p_error qtVidExtFuncInit(void)
{
    init = 0;
    set_volume = 1;
    format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::DeprecatedFunctions, 1);
    format.setDepthBufferSize(24);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setMajorVersion(2);
    format.setMinorVersion(1);
    if (w->getGLES())
        format.setRenderableType(QSurfaceFormat::OpenGLES);

    rendering_thread = QThread::currentThread();
    return M64ERR_SUCCESS;
}

m64p_error qtVidExtFuncQuit(void)
{
    init = 0;
    w->getWorkerThread()->toggleFS(M64VIDEO_WINDOWED);
    if (w->getOGLWindow() != nullptr) {
        w->getOGLWindow()->doneCurrent();
        w->getOGLWindow()->context()->moveToThread(QApplication::instance()->thread());
        w->getWorkerThread()->deleteOGLWindow();
    }
    return M64ERR_SUCCESS;
}

#include <iostream>
m64p_error qtVidExtFuncListModes(m64p_2d_size *SizeArray, int *NumSizes)
{
    SDL_Init(SDL_INIT_VIDEO);

    int mode_count = 0;
    int modes_length = SDL_GetNumDisplayModes(0);
    SDL_DisplayMode display_mode;

    if (modes_length < 1)
        return M64ERR_NOT_INIT;

    for (int i = 0; (i < modes_length && mode_count < *NumSizes); i++)
    {
        if (SDL_GetDisplayMode(0, i, &display_mode) < 0)
            return M64ERR_NOT_INIT;

        m64p_2d_size prevMode = SizeArray[mode_count - 1];
        if ((prevMode.uiWidth == display_mode.w) &&
            (prevMode.uiHeight == display_mode.h))
            continue;

        std::cout << "adding: " << display_mode.h << " x " << display_mode.w << " @ " << std::endl;

        SizeArray[mode_count].uiHeight = display_mode.h;
        SizeArray[mode_count].uiWidth = display_mode.w;
        mode_count++;
    }

    *NumSizes = mode_count;

    return M64ERR_SUCCESS;
}

#include <iostream>
m64p_error qtVidExtFuncListRates(m64p_2d_size Size, int *NumRates, int *Rates)
{
    if (!SDL_WasInit(SDL_INIT_VIDEO))
        return M64ERR_NOT_INIT;

    int modeCount = SDL_GetNumDisplayModes(0);
    SDL_DisplayMode displayMode;

    if (modeCount < 1)
        return M64ERR_SYSTEM_FAIL;

    int rateCount = 0;
    for (int i = 0; (i < modeCount) && (rateCount < *NumRates); i++)
    {
        if (SDL_GetDisplayMode(0, i, &displayMode) < 0)
            return M64ERR_SYSTEM_FAIL;

        // skip when we're not at the right resolution
        if (displayMode.w != Size.uiWidth ||
            displayMode.h != Size.uiHeight)
            continue;

        Rates[rateCount] = displayMode.refresh_rate;
        rateCount++;
    }

    *NumRates = rateCount;

    return M64ERR_SUCCESS;
}

m64p_error qtVidExtFuncSetMode(int Width, int Height, int, int ScreenMode, int)
{
    if(!init)
    {
        w->getWorkerThread()->createOGLWindow(&format);
        while (!w->getOGLWindow()->isValid()) {}
    }

    w->getWorkerThread()->resizeMainWindow(Width, Height);

    if(!init)
        w->getOGLWindow()->makeCurrent();

    needs_toggle = ScreenMode;
    screen_mode_w = Width;
    screen_mode_h = Height;
    screen_mode_refresh = -1;

    init = 1;

    //}
    return M64ERR_SUCCESS;
}

m64p_error qtVidExtFuncSetMode2(int Width, int Height, int Refresh, int, int ScreenMode, int)
{
    if(!init)
    {
        w->getWorkerThread()->createOGLWindow(&format);
        while (!w->getOGLWindow()->isValid()) {}
    }

    w->getWorkerThread()->resizeMainWindow(Width, Height);

    if(!init)
        w->getOGLWindow()->makeCurrent();

    needs_toggle = ScreenMode;
    screen_mode_w = Width;
    screen_mode_h = Height;
    screen_mode_refresh = Refresh;

    init = 1;

    //}
    return M64ERR_SUCCESS;
}

m64p_function qtVidExtFuncGLGetProc(const char* Proc)
{
    if (!init) return NULL;
    return w->getOGLWindow()->context()->getProcAddress(Proc);
}

m64p_error qtVidExtFuncGLSetAttr(m64p_GLattr Attr, int Value)
{
    switch (Attr) {
    case M64P_GL_DOUBLEBUFFER:
        if (Value == 1)
            format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        else if (Value == 0)
            format.setSwapBehavior(QSurfaceFormat::SingleBuffer);
        break;
    case M64P_GL_BUFFER_SIZE:
        break;
    case M64P_GL_DEPTH_SIZE:
        format.setDepthBufferSize(Value);
        break;
    case M64P_GL_RED_SIZE:
        format.setRedBufferSize(Value);
        break;
    case M64P_GL_GREEN_SIZE:
        format.setGreenBufferSize(Value);
        break;
    case M64P_GL_BLUE_SIZE:
        format.setBlueBufferSize(Value);
        break;
    case M64P_GL_ALPHA_SIZE:
        format.setAlphaBufferSize(Value);
        break;
    case M64P_GL_SWAP_CONTROL:
        format.setSwapInterval(Value);
        break;
    case M64P_GL_MULTISAMPLEBUFFERS:
        break;
    case M64P_GL_MULTISAMPLESAMPLES:
        format.setSamples(Value);
        break;
    case M64P_GL_CONTEXT_MAJOR_VERSION:
        format.setMajorVersion(Value);
        break;
    case M64P_GL_CONTEXT_MINOR_VERSION:
        format.setMinorVersion(Value);
        break;
    case M64P_GL_CONTEXT_PROFILE_MASK:
        switch (Value) {
        case M64P_GL_CONTEXT_PROFILE_CORE:
            format.setProfile(QSurfaceFormat::CoreProfile);
            break;
        case M64P_GL_CONTEXT_PROFILE_COMPATIBILITY:
            format.setProfile(QSurfaceFormat::CompatibilityProfile);
            break;
        case M64P_GL_CONTEXT_PROFILE_ES:
            format.setRenderableType(QSurfaceFormat::OpenGLES);
            break;
        }

        break;
    }

    return M64ERR_SUCCESS;
}

m64p_error qtVidExtFuncGLGetAttr(m64p_GLattr Attr, int *pValue)
{
    if (!init) return M64ERR_NOT_INIT;
    QSurfaceFormat::SwapBehavior SB = w->getOGLWindow()->format().swapBehavior();
    switch (Attr) {
    case M64P_GL_DOUBLEBUFFER:
        if (SB == QSurfaceFormat::SingleBuffer)
            *pValue = 0;
        else
            *pValue = 1;
        break;
    case M64P_GL_BUFFER_SIZE:
        *pValue = w->getOGLWindow()->format().alphaBufferSize() + w->getOGLWindow()->format().redBufferSize() + w->getOGLWindow()->format().greenBufferSize() + w->getOGLWindow()->format().blueBufferSize();
        break;
    case M64P_GL_DEPTH_SIZE:
        *pValue = w->getOGLWindow()->format().depthBufferSize();
        break;
    case M64P_GL_RED_SIZE:
        *pValue = w->getOGLWindow()->format().redBufferSize();
        break;
    case M64P_GL_GREEN_SIZE:
        *pValue = w->getOGLWindow()->format().greenBufferSize();
        break;
    case M64P_GL_BLUE_SIZE:
        *pValue = w->getOGLWindow()->format().blueBufferSize();
        break;
    case M64P_GL_ALPHA_SIZE:
        *pValue = w->getOGLWindow()->format().alphaBufferSize();
        break;
    case M64P_GL_SWAP_CONTROL:
        *pValue = w->getOGLWindow()->format().swapInterval();
        break;
    case M64P_GL_MULTISAMPLEBUFFERS:
        break;
    case M64P_GL_MULTISAMPLESAMPLES:
        *pValue = w->getOGLWindow()->format().samples();
        break;
    case M64P_GL_CONTEXT_MAJOR_VERSION:
        *pValue = w->getOGLWindow()->format().majorVersion();
        break;
    case M64P_GL_CONTEXT_MINOR_VERSION:
        *pValue = w->getOGLWindow()->format().minorVersion();
        break;
    case M64P_GL_CONTEXT_PROFILE_MASK:
        switch (w->getOGLWindow()->format().profile()) {
        case QSurfaceFormat::CoreProfile:
            *pValue = M64P_GL_CONTEXT_PROFILE_CORE;
            break;
        case QSurfaceFormat::CompatibilityProfile:
            *pValue = M64P_GL_CONTEXT_PROFILE_COMPATIBILITY;
            break;
        case QSurfaceFormat::NoProfile:
            *pValue = M64P_GL_CONTEXT_PROFILE_COMPATIBILITY;
            break;
        }
        break;
    }
    return M64ERR_SUCCESS;
}

m64p_error qtVidExtFuncGLSwapBuf(void)
{
    if (set_volume) {
        int value;
        (*CoreDoCommand)(M64CMD_CORE_STATE_QUERY, M64CORE_EMU_STATE, &value);
        if (value == M64EMU_RUNNING) {
            int volume = w->getSettings()->value("volume").toInt();
            (*CoreDoCommand)(M64CMD_CORE_STATE_SET, M64CORE_AUDIO_VOLUME, &volume);
            set_volume = 0;
        }
    }

    if (needs_toggle) {
        int value;
        (*CoreDoCommand)(M64CMD_CORE_STATE_QUERY, M64CORE_EMU_STATE, &value);
        if (value > M64EMU_STOPPED) {
            w->getWorkerThread()->toggleFS(needs_toggle, screen_mode_w, screen_mode_h, screen_mode_refresh);
            needs_toggle = 0;
        }
    }

    if (QThread::currentThread() == rendering_thread) {
        w->getOGLWindow()->context()->swapBuffers(w->getOGLWindow());
        w->getOGLWindow()->context()->makeCurrent(w->getOGLWindow());
    }
    return M64ERR_SUCCESS;
}

m64p_error qtVidExtFuncSetCaption(const char * _title)
{
    std::string title = _title;
    w->getWorkerThread()->setTitle(title);
    return M64ERR_SUCCESS;
}

m64p_error qtVidExtFuncToggleFS(void)
{
    w->getWorkerThread()->toggleFS(M64VIDEO_NONE);
    return M64ERR_SUCCESS;
}

m64p_error qtVidExtFuncResizeWindow(int width, int height)
{
    int response = M64VIDEO_NONE;
    (*CoreDoCommand)(M64CMD_CORE_STATE_QUERY, M64CORE_VIDEO_MODE, &response);
    if (response == M64VIDEO_WINDOWED)
        w->getWorkerThread()->resizeMainWindow(width, height);
    return M64ERR_SUCCESS;
}

uint32_t qtVidExtFuncGLGetDefaultFramebuffer(void)
{
    return 0;
}
