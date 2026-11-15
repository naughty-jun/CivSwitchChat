#include "ui_draw.h"

static void drawPixel(
    UiDrawContext* ctx,
    s32 x,
    s32 y,
    u32 color
)
{
    if (x < 0 || y < 0 ||
        x >= (s32)ctx->width ||
        y >= (s32)ctx->height)
        return;

    ctx->pixels[(u32)y * ctx->stride + (u32)x] = color;
}

static s32 clampS32(s32 value, s32 min, s32 max)
{
    if (value < min)
        return min;

    if (value > max)
        return max;

    return value;
}

void uiDrawInitialize(
    UiDrawContext* ctx,
    u32* pixels,
    u32 stride,
    u32 width,
    u32 height,
    FT_Face face
)
{
    ctx->pixels = pixels;
    ctx->stride = stride / sizeof(u32);
    ctx->width = width;
    ctx->height = height;
    ctx->face = face;
}

void uiDrawClear(UiDrawContext* ctx, u32 color)
{
    for (u32 y = 0; y < ctx->height; y++) {
        u32* row = ctx->pixels + y * ctx->stride;

        for (u32 x = 0; x < ctx->width; x++)
            row[x] = color;
    }
}

void uiDrawFillRect(
    UiDrawContext* ctx,
    s32 x, s32 y, s32 width, s32 height,
    u32 color
)
{
    if (width <= 0 || height <= 0)
        return;

    s32 x0 = x < 0 ? 0 : x;
    s32 y0 = y < 0 ? 0 : y;
    s32 x1 = x + width;
    s32 y1 = y + height;

    if (x1 > (s32)ctx->width)
        x1 = ctx->width;

    if (y1 > (s32)ctx->height)
        y1 = ctx->height;

    for (s32 py = y0; py < y1; py++) {
        u32* row = ctx->pixels + py * ctx->stride;

        for (s32 px = x0; px < x1; px++)
            row[px] = color;
    }
}

/*
 * A pixel belongs to the rounded rectangle when its distance to the nearest
 * point of the inner (corner-free) rectangle is within the radius.
 */
static bool isInsideRoundedRect(
    s32 px, s32 py,
    s32 x, s32 y, s32 width, s32 height,
    s32 radius
)
{
    s32 nearest_x = clampS32(px, x + radius, x + width - radius - 1);
    s32 nearest_y = clampS32(py, y + radius, y + height - radius - 1);

    s32 dx = px - nearest_x;
    s32 dy = py - nearest_y;

    return dx * dx + dy * dy <= radius * radius;
}

void uiDrawRoundedRect(
    UiDrawContext* ctx,
    s32 x, s32 y, s32 width, s32 height,
    s32 radius,
    u32 color
)
{
    if (width <= 0 || height <= 0)
        return;

    if (radius < 0)
        radius = 0;

    if (radius > width / 2)
        radius = width / 2;

    if (radius > height / 2)
        radius = height / 2;

    s32 x0 = x < 0 ? 0 : x;
    s32 y0 = y < 0 ? 0 : y;
    s32 x1 = x + width;
    s32 y1 = y + height;

    if (x1 > (s32)ctx->width)
        x1 = ctx->width;

    if (y1 > (s32)ctx->height)
        y1 = ctx->height;

    for (s32 py = y0; py < y1; py++) {
        for (s32 px = x0; px < x1; px++) {
            if (!isInsideRoundedRect(px, py, x, y, width, height, radius))
                continue;

            drawPixel(ctx, px, py, color);
        }
    }
}

void uiDrawRoundedBorder(
    UiDrawContext* ctx,
    s32 x, s32 y, s32 width, s32 height,
    s32 radius,
    s32 thickness,
    u32 border_color,
    u32 fill_color
)
{
    if (!ctx || width <= 0 || height <= 0 || thickness <= 0)
        return;

    uiDrawRoundedRect(
        ctx, x, y, width, height,
        radius, border_color
    );

    s32 inner_width = width - thickness * 2;
    s32 inner_height = height - thickness * 2;

    if (inner_width <= 0 || inner_height <= 0)
        return;

    s32 inner_radius = radius - thickness;

    if (inner_radius < 0)
        inner_radius = 0;

    uiDrawRoundedRect(
        ctx,
        x + thickness,
        y + thickness,
        inner_width,
        inner_height,
        inner_radius,
        fill_color
    );
}
