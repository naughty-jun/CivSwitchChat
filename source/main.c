#include <switch.h>
#include <string.h>

#include "app_init.h"
#include "app_logger.h"
#include "app_paths.h"
#include "model/chat_file.h"
#include "model/ldn_model.h"
#include "view/ui_draw.h"
#include "view/ui_theme.h"
#include "view/ui_view.h"
#include "controller/app_controller.h"

#define MAX_PAD_PLAYERS      1
#define FRAME_SLEEP_NS       16000000LL

typedef struct {
    FontContext        font;
    FramebufferContext framebuffer;
    SocketContext      socket;
} AppResources;

static void logLogFileStatus(LdnModel* model, bool opened)
{
    if (!opened) {
        ldnModelLog(model, "Log file unavailable (memory log only)");
        return;
    }

    ldnModelLog(model, "Log file: %s", appLoggerGetPath());
}

static void initializePad(PadState* pad)
{
    padConfigureInput(MAX_PAD_PLAYERS, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(pad);
}

static Result initializeResources(AppResources* resources, LdnModel* model)
{
    Result rc = initFont(&resources->font, model);
    if (R_FAILED(rc))
        return rc;

    rc = initFramebuffer(&resources->framebuffer, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT, model);
    if (R_FAILED(rc))
        return rc;

    return initSocket(&resources->socket, model);
}

static void cleanupResources(AppResources* resources)
{
    cleanupSocket(&resources->socket);
    cleanupFramebuffer(&resources->framebuffer);
    cleanupFont(&resources->font);
}

/* Returns false when no framebuffer could be acquired. */
static bool drawFrame(
    AppResources* resources,
    const AppController* controller,
    const LdnModel* model
)
{
    u32  stride = 0;
    u32* pixels = (u32*)framebufferBegin(&resources->framebuffer.fb, &stride);
    if (!pixels)
        return false;

    UiDrawContext draw;
    uiDrawInitialize(&draw, pixels, stride,
        resources->framebuffer.width,
        resources->framebuffer.height,
        resources->font.ft_face);
    uiViewDraw(&draw, controller, model);

    framebufferEnd(&resources->framebuffer.fb);
    return true;
}

static void runMainLoop(
    AppResources* resources,
    AppController* controller,
    LdnModel* model,
    PadState* pad
)
{
    while (appletMainLoop()) {
        padUpdate(pad);

        if (!appControllerUpdate(controller, pad))
            return;

        ldnModelUpdate(model);

        if (!drawFrame(resources, controller, model))
            return;

        svcSleepThread(FRAME_SLEEP_NS);
    }
}

int main(int argc, char* argv[])
{
    /*
     * static: AppController holds up to CHAT_FILE_LINE_COUNT_MAX chat lines
     * (~250KB). Keeping these off the stack avoids a stack overflow.
     */
    static AppResources  resources;
    static LdnModel       model;
    static AppController controller;
    PadState pad;

    memset(&model, 0, sizeof(model));

    const char* nro_path = argc > 0 ? argv[0] : NULL;
    char nro_directory[CHAT_FILE_NAME_MAX];
    appPathsGetNroDirectory(nro_path, nro_directory, sizeof(nro_directory));

    bool log_file_opened = appLoggerOpen(nro_path);

    ldnModelLog(&model, "Application started (type %d)", (int)appletGetAppletType());
    logLogFileStatus(&model, log_file_opened);

    initializePad(&pad);

    if (R_FAILED(initializeResources(&resources, &model))) {
        cleanupResources(&resources);
        appLoggerClose();
        return 0;
    }

    ldnModelInitialize(&model);
    appControllerInitialize(&controller, &model, nro_directory);

    runMainLoop(&resources, &controller, &model, &pad);

    ldnModelShutdown(&model);
    cleanupResources(&resources);
    appLoggerClose();

    return 0;
}
