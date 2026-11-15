#pragma once

#include <switch.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "model/ldn_model.h"

typedef struct {
    FT_Library ft_library;
    FT_Face    ft_face;
    bool       initialized;
} FontContext;

typedef struct {
    Framebuffer fb;
    u32         width;
    u32         height;
    bool        initialized;
} FramebufferContext;

typedef struct {
    bool initialized;
} SocketContext;

Result initFont(FontContext* ctx, LdnModel* model);
void   cleanupFont(FontContext* ctx);

Result initFramebuffer(FramebufferContext* ctx, u32 width, u32 height, LdnModel* model);
void   cleanupFramebuffer(FramebufferContext* ctx);

/* Initializes the standard BSD socket API (socket/bind/sendto/recv) used by the Civ VII UDP session. */
Result initSocket(SocketContext* ctx, LdnModel* model);
void   cleanupSocket(SocketContext* ctx);
