#include "app_init.h"

#define FONT_FACE_INDEX          0
#define FRAMEBUFFER_BUFFER_COUNT 2
#define BSD_SESSION_COUNT        1

/*
 * Wraps a failure from an API that does not return a Result (FreeType's
 * int-based functions) into a Result so callers can use the usual
 * R_FAILED/R_SUCCEEDED checks.
 */
static Result makeInitFailedResult(void)
{
    return MAKERESULT(Module_Libnx, LibnxError_NotFound);
}

/* ---- Font ---- */

/* Releases whatever initFont partially set up, then returns `result`. */
static Result abortFontInit(FontContext* ctx, Result result)
{
    if (ctx->ft_library)
        FT_Done_FreeType(ctx->ft_library);

    plExit();
    return result;
}

Result initFont(FontContext* ctx, LdnModel* model)
{
    PlFontData font_data = {0};

    Result rc = plInitialize(PlServiceType_User);
    if (R_FAILED(rc)) {
        ldnModelLog(model, "plInitialize failed: 0x%08X", (unsigned int)rc);
        return rc;
    }

    rc = plGetSharedFontByType(&font_data, PlSharedFontType_Standard);
    if (R_FAILED(rc)) {
        ldnModelLog(model, "plGetSharedFontByType failed: 0x%08X", (unsigned int)rc);
        plExit();
        return rc;
    }

    if (FT_Init_FreeType(&ctx->ft_library)) {
        ldnModelLog(model, "FT_Init_FreeType failed");
        return abortFontInit(ctx, makeInitFailedResult());
    }

    if (FT_New_Memory_Face(
        ctx->ft_library,
        (const FT_Byte*)font_data.address,
        (FT_Long)font_data.size,
        FONT_FACE_INDEX,
        &ctx->ft_face
    )) {
        ldnModelLog(model, "FT_New_Memory_Face failed");
        return abortFontInit(ctx, makeInitFailedResult());
    }

    ctx->initialized = true;
    ldnModelLog(model, "Font initialized");
    return 0;
}

void cleanupFont(FontContext* ctx)
{
    if (!ctx->initialized) return;
    if (ctx->ft_face)    FT_Done_Face(ctx->ft_face);
    if (ctx->ft_library) FT_Done_FreeType(ctx->ft_library);
    plExit();
    ctx->initialized = false;
}

/* ---- Framebuffer ---- */

Result initFramebuffer(FramebufferContext* ctx, u32 width, u32 height, LdnModel* model)
{
    ctx->width  = width;
    ctx->height = height;

    Result rc = framebufferCreate(
        &ctx->fb,
        nwindowGetDefault(),
        width, height,
        PIXEL_FORMAT_RGBA_8888,
        FRAMEBUFFER_BUFFER_COUNT
    );
    if (R_FAILED(rc)) {
        ldnModelLog(model, "framebufferCreate failed: 0x%08X", (unsigned int)rc);
        return rc;
    }

    rc = framebufferMakeLinear(&ctx->fb);
    if (R_FAILED(rc)) {
        ldnModelLog(model, "framebufferMakeLinear failed: 0x%08X", (unsigned int)rc);
        framebufferClose(&ctx->fb);
        return rc;
    }

    ctx->initialized = true;
    ldnModelLog(model, "Framebuffer initialized");
    return 0;
}

void cleanupFramebuffer(FramebufferContext* ctx)
{
    if (!ctx->initialized) return;
    framebufferClose(&ctx->fb);
    ctx->initialized = false;
}

/* ---- Socket ---- */

Result initSocket(SocketContext* ctx, LdnModel* model)
{
    ldnModelLog(model, "Initializing socket service...");

    SocketInitConfig config = *socketGetDefaultInitConfig();
    config.num_bsd_sessions = BSD_SESSION_COUNT;
    config.bsd_service_type = BsdServiceType_System;

    Result rc = socketInitialize(&config);
    if (R_FAILED(rc)) {
        ldnModelLog(model, "socketInitialize failed: 0x%08X", (unsigned int)rc);
        return rc;
    }

    ctx->initialized = true;
    ldnModelLog(model, "Socket service initialized");
    return 0;
}

void cleanupSocket(SocketContext* ctx)
{
    if (!ctx->initialized) return;
    socketExit();
    ctx->initialized = false;
}
