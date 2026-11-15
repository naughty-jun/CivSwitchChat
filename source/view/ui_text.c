#include "ui_draw.h"

#include <stdbool.h>

/* FreeType metrics are 26.6 fixed point. */
#define FT_FIXED_SHIFT        6
#define REPLACEMENT_CODEPOINT 0xFFFD

#define ALPHA_OPAQUE      255
#define ALPHA_TRANSPARENT 0
#define COLOR_CHANNEL_MAX 255
#define COLOR_SHIFT_RED   0
#define COLOR_SHIFT_GREEN 8
#define COLOR_SHIFT_BLUE  16
#define COLOR_CHANNEL_MASK 0xFF

/* ---- Color blending ---- */

static u32 blendChannel(u32 dst, u32 src, u8 alpha, u32 shift)
{
    u32 inverse = COLOR_CHANNEL_MAX - alpha;

    return (((src >> shift) & COLOR_CHANNEL_MASK) * alpha +
            ((dst >> shift) & COLOR_CHANNEL_MASK) * inverse) / COLOR_CHANNEL_MAX;
}

static u32 blendColor(u32 dst, u32 src, u8 alpha)
{
    if (alpha == ALPHA_OPAQUE)
        return src;

    if (alpha == ALPHA_TRANSPARENT)
        return dst;

    return RGBA8_MAXALPHA(
        blendChannel(dst, src, alpha, COLOR_SHIFT_RED),
        blendChannel(dst, src, alpha, COLOR_SHIFT_GREEN),
        blendChannel(dst, src, alpha, COLOR_SHIFT_BLUE)
    );
}

/* ---- UTF-8 decoding ---- */

#define UTF8_ASCII_LIMIT          0x80

#define UTF8_CONT_MASK            0xC0
#define UTF8_CONT_TAG             0x80
#define UTF8_CONT_PAYLOAD_MASK    0x3F
#define UTF8_CONT_BITS            6

#define UTF8_LEAD2_MASK           0xE0
#define UTF8_LEAD2_TAG            0xC0
#define UTF8_LEAD2_PAYLOAD_MASK   0x1F
#define UTF8_LEAD3_MASK           0xF0
#define UTF8_LEAD3_TAG            0xE0
#define UTF8_LEAD3_PAYLOAD_MASK   0x0F
#define UTF8_LEAD4_MASK           0xF8
#define UTF8_LEAD4_TAG            0xF0
#define UTF8_LEAD4_PAYLOAD_MASK   0x07

#define UTF8_MIN_2BYTE            0x80
#define UTF8_MIN_3BYTE            0x800
#define UTF8_MIN_4BYTE            0x10000
#define UTF8_MAX_CODEPOINT        0x10FFFF
#define UTF16_SURROGATE_FIRST     0xD800
#define UTF16_SURROGATE_LAST      0xDFFF

static bool isContinuation(unsigned char byte)
{
    return (byte & UTF8_CONT_MASK) == UTF8_CONT_TAG;
}

static u32 continuationBits(unsigned char byte)
{
    return byte & UTF8_CONT_PAYLOAD_MASK;
}

/* Each tryDecode* returns false when the sequence is malformed. */

static bool tryDecodeTwoBytes(const unsigned char* p, u32* codepoint)
{
    if ((p[0] & UTF8_LEAD2_MASK) != UTF8_LEAD2_TAG || !isContinuation(p[1]))
        return false;

    *codepoint = ((u32)(p[0] & UTF8_LEAD2_PAYLOAD_MASK) << UTF8_CONT_BITS) |
                 continuationBits(p[1]);

    return *codepoint >= UTF8_MIN_2BYTE;
}

static bool tryDecodeThreeBytes(const unsigned char* p, u32* codepoint)
{
    if ((p[0] & UTF8_LEAD3_MASK) != UTF8_LEAD3_TAG ||
        !isContinuation(p[1]) ||
        !isContinuation(p[2]))
        return false;

    *codepoint = ((u32)(p[0] & UTF8_LEAD3_PAYLOAD_MASK) << (UTF8_CONT_BITS * 2)) |
                 (continuationBits(p[1]) << UTF8_CONT_BITS) |
                 continuationBits(p[2]);

    if (*codepoint < UTF8_MIN_3BYTE)
        return false;

    return !(*codepoint >= UTF16_SURROGATE_FIRST &&
             *codepoint <= UTF16_SURROGATE_LAST);
}

static bool tryDecodeFourBytes(const unsigned char* p, u32* codepoint)
{
    if ((p[0] & UTF8_LEAD4_MASK) != UTF8_LEAD4_TAG ||
        !isContinuation(p[1]) ||
        !isContinuation(p[2]) ||
        !isContinuation(p[3]))
        return false;

    *codepoint = ((u32)(p[0] & UTF8_LEAD4_PAYLOAD_MASK) << (UTF8_CONT_BITS * 3)) |
                 (continuationBits(p[1]) << (UTF8_CONT_BITS * 2)) |
                 (continuationBits(p[2]) << UTF8_CONT_BITS) |
                 continuationBits(p[3]);

    return *codepoint >= UTF8_MIN_4BYTE && *codepoint <= UTF8_MAX_CODEPOINT;
}

/* Advances `input` past one character; malformed bytes yield U+FFFD. */
static u32 decodeUtf8(const unsigned char** input)
{
    const unsigned char* p = *input;
    u32 codepoint;

    if (*p < UTF8_ASCII_LIMIT) {
        *input = p + 1;
        return *p;
    }

    if (tryDecodeTwoBytes(p, &codepoint)) {
        *input = p + 2;
        return codepoint;
    }

    if (tryDecodeThreeBytes(p, &codepoint)) {
        *input = p + 3;
        return codepoint;
    }

    if (tryDecodeFourBytes(p, &codepoint)) {
        *input = p + 4;
        return codepoint;
    }

    *input = p + 1;
    return REPLACEMENT_CODEPOINT;
}

/* ---- Glyph drawing ---- */

static const u8* getBitmapRow(const FT_Bitmap* bitmap, u32 row)
{
    if (bitmap->pitch >= 0)
        return bitmap->buffer + row * bitmap->pitch;

    return bitmap->buffer + (bitmap->rows - 1 - row) * (-bitmap->pitch);
}

static void blendGlyphPixel(UiDrawContext* ctx, s32 px, s32 py, u32 color, u8 coverage)
{
    if (coverage == 0)
        return;

    if (px < 0 || px >= (s32)ctx->width)
        return;

    u32* dst = &ctx->pixels[(u32)py * ctx->stride + (u32)px];
    *dst = blendColor(*dst, color, coverage);
}

static void drawGlyph(
    UiDrawContext* ctx,
    FT_GlyphSlot slot,
    s32 x,
    s32 y,
    u32 color
)
{
    FT_Bitmap* bitmap = &slot->bitmap;

    if (bitmap->pixel_mode != FT_PIXEL_MODE_GRAY)
        return;

    for (u32 row = 0; row < bitmap->rows; row++) {
        s32 py = y + (s32)row;

        if (py < 0 || py >= (s32)ctx->height)
            continue;

        const u8* src = getBitmapRow(bitmap, row);

        for (u32 col = 0; col < bitmap->width; col++)
            blendGlyphPixel(ctx, x + (s32)col, py, color, src[col]);
    }
}

/* ---- Text drawing ---- */

void uiDrawText(
    UiDrawContext* ctx,
    s32 x, s32 y,
    const char* text,
    u32 font_size,
    u32 color,
    s32 max_width
)
{
    if (!ctx || !ctx->face || !text || font_size == 0)
        return;

    if (FT_Set_Pixel_Sizes(ctx->face, 0, font_size))
        return;

    FT_Face face = ctx->face;
    s32 start_x = x;

    s32 baseline =
        y + (face->size->metrics.ascender >> FT_FIXED_SHIFT);

    const unsigned char* p =
        (const unsigned char*)text;

    while (*p) {
        u32 codepoint = decodeUtf8(&p);

        if (codepoint == '\n') {
            x = start_x;
            baseline += face->size->metrics.height >> FT_FIXED_SHIFT;
            continue;
        }

        if (max_width > 0 && x - start_x >= max_width)
            break;

        if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER))
            continue;

        FT_GlyphSlot slot = face->glyph;

        drawGlyph(
            ctx,
            slot,
            x + slot->bitmap_left,
            baseline - slot->bitmap_top,
            color
        );

        x += slot->advance.x >> FT_FIXED_SHIFT;
        baseline += slot->advance.y >> FT_FIXED_SHIFT;
    }
}
