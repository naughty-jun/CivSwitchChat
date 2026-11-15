#pragma once

#include <switch.h>
#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct {
    u32* pixels;
    u32 stride;
    u32 width;
    u32 height;
    FT_Face face;
} UiDrawContext;

void uiDrawInitialize(
    UiDrawContext* ctx,
    u32* pixels,
    u32 stride,
    u32 width,
    u32 height,
    FT_Face face
);

void uiDrawClear(UiDrawContext* ctx, u32 color);

void uiDrawFillRect(
    UiDrawContext* ctx,
    s32 x, s32 y, s32 width, s32 height,
    u32 color
);

void uiDrawRoundedRect(
    UiDrawContext* ctx,
    s32 x, s32 y, s32 width, s32 height,
    s32 radius,
    u32 color
);

void uiDrawRoundedBorder(
    UiDrawContext* ctx,
    s32 x, s32 y, s32 width, s32 height,
    s32 radius,
    s32 thickness,
    u32 border_color,
    u32 fill_color
);

void uiDrawText(
    UiDrawContext* ctx,
    s32 x, s32 y,
    const char* text,
    u32 font_size,
    u32 color,
    s32 max_width
);
