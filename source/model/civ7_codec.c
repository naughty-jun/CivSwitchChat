#include "civ7_codec.h"

#include <string.h>

#include "civ7_protocol.h"

#define BITS_PER_BYTE 8

/* ---- Writer ---- */

void civ7WriterInit(Civ7Writer *w, uint8_t *buffer, size_t capacity)
{
    w->data = buffer;
    w->capacity = capacity;
    w->length = 0;
    w->overflow = false;
}

bool civ7WriterOk(const Civ7Writer *w)
{
    return !w->overflow;
}

static bool reserve(Civ7Writer *w, size_t len)
{
    if (w->overflow)
        return false;

    if (w->length + len > w->capacity) {
        w->overflow = true;
        return false;
    }

    return true;
}

void civ7WriteBytes(Civ7Writer *w, const uint8_t *src, size_t len)
{
    if (len == 0 || !reserve(w, len))
        return;

    memcpy(w->data + w->length, src, len);
    w->length += len;
}

void civ7WriteZeros(Civ7Writer *w, size_t len)
{
    if (len == 0 || !reserve(w, len))
        return;

    memset(w->data + w->length, 0, len);
    w->length += len;
}

void civ7WriteU8(Civ7Writer *w, uint8_t value)
{
    civ7WriteBytes(w, &value, 1);
}

void civ7WriteU32(Civ7Writer *w, uint32_t value)
{
    uint8_t bytes[4];

    for (unsigned i = 0; i < sizeof(bytes); ++i)
        bytes[i] = (uint8_t)(value >> (i * BITS_PER_BYTE));

    civ7WriteBytes(w, bytes, sizeof(bytes));
}

void civ7WriteU64(Civ7Writer *w, uint64_t value)
{
    uint8_t bytes[8];

    for (unsigned i = 0; i < sizeof(bytes); ++i)
        bytes[i] = (uint8_t)(value >> (i * BITS_PER_BYTE));

    civ7WriteBytes(w, bytes, sizeof(bytes));
}

/* Fixed-size, zero-padded text field. Empty or oversized text is an error. */
void civ7WriteTextBlock(Civ7Writer *w, const char *text)
{
    size_t length = strlen(text);

    if (length == 0 || length > CIV7_TEXT_MAX_LENGTH) {
        w->overflow = true;
        return;
    }

    if (!reserve(w, CIV7_TEXT_BLOCK_SIZE))
        return;

    memset(w->data + w->length, 0, CIV7_TEXT_BLOCK_SIZE);
    memcpy(w->data + w->length, text, length);
    w->length += CIV7_TEXT_BLOCK_SIZE;
}

/* ---- Reader ---- */

uint32_t civ7ReadU32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

uint64_t civ7ReadU64(const uint8_t *p)
{
    uint64_t value = 0;

    for (unsigned i = 0; i < sizeof(uint64_t); ++i)
        value |= (uint64_t)p[i] << (i * BITS_PER_BYTE);

    return value;
}

/* ---- Frame ---- */

void civ7EncodeIp(uint32_t ip_be, uint8_t out[4])
{
    const uint8_t *p = (const uint8_t *)&ip_be;

    out[0] = p[3];
    out[1] = p[2];
    out[2] = p[1];
    out[3] = p[0];
}

void civ7WriteFrameHeader(Civ7Writer *w, uint32_t host_ip_be, uint8_t kind, uint64_t seq)
{
    uint8_t ip[CIV7_IP_FIELD_SIZE];

    civ7EncodeIp(host_ip_be, ip);
    civ7WriteBytes(w, ip, sizeof(ip));
    civ7WriteU8(w, kind);
    civ7WriteU64(w, seq);
}

/* ---- Messages ---- */

/* Common Civ VII message header: type, 1, 1, -1, -1, 1, counter (all u32 LE). */
static void writeMessageHeader(Civ7Writer *w, uint32_t type, uint32_t counter)
{
    civ7WriteU32(w, type);
    civ7WriteU32(w, CIV7_MESSAGE_FIELD_ONE);
    civ7WriteU32(w, CIV7_MESSAGE_FIELD_ONE);
    civ7WriteU32(w, CIV7_MESSAGE_FIELD_ALL);
    civ7WriteU32(w, CIV7_MESSAGE_FIELD_ALL);
    civ7WriteU32(w, CIV7_MESSAGE_FIELD_ONE);
    civ7WriteU32(w, counter);
}

static size_t finish(const Civ7Writer *w)
{
    return civ7WriterOk(w) ? w->length : 0;
}

/* Civ VII's game-level introduction, carried inside the FireCONN packet. */
static void writeIntro(Civ7Writer *w)
{
    civ7WriteU32(w, 0);                      /* intro_id */
    civ7WriteTextBlock(w, CIV7_PLAYER_NAME);  /* identity */
    civ7WriteTextBlock(w, CIV7_PLAYER_NAME);  /* name; same value as identity */
    civ7WriteU32(w, CIV7_INTRO_PROTOCOL_VERSION);
    civ7WriteU8(w, 0);                        /* flag */
    civ7WriteU32(w, 0);                       /* length of trailing extra data (none here) */
}

/* The first packet sent to the host: FireCONN magic, host IP, reserved fields, then the intro. */
size_t civ7BuildInitialPacket(uint8_t *out, size_t capacity, uint32_t host_ip_be)
{
    Civ7Writer w;
    uint8_t ip[CIV7_IP_FIELD_SIZE];

    civ7WriterInit(&w, out, capacity);
    civ7EncodeIp(host_ip_be, ip);

    civ7WriteFrameHeader(&w, host_ip_be, CIV7_KIND_DATA, CIV7_INITIAL_SEQUENCE);
    civ7WriteU64(&w, CIV7_FIRE_CONN);
    civ7WriteBytes(&w, ip, sizeof(ip));
    civ7WriteZeros(&w, 4); /* reserved */
    civ7WriteZeros(&w, 4); /* reserved */
    writeIntro(&w);

    return finish(&w);
}

/* Announces our client version to the host, sent once right after joining. */
size_t civ7BuildRegisterMessage(uint8_t *out, size_t capacity, uint32_t counter)
{
    Civ7Writer w;
    const char *version = CIV7_CLIENT_VERSION;
    size_t version_len = strlen(version);

    civ7WriterInit(&w, out, capacity);
    writeMessageHeader(&w, CIV7_MSG_REGISTER_VERSION, counter);
    civ7WriteU32(&w, (uint32_t)version_len);
    civ7WriteBytes(&w, (const uint8_t *)version, version_len);

    return finish(&w);
}

size_t civ7BuildAppRegisterMessage(uint8_t *out, size_t capacity, uint32_t counter, uint32_t slot)
{
    Civ7Writer w;

    civ7WriterInit(&w, out, capacity);
    writeMessageHeader(&w, CIV7_MSG_REGISTER_APP, counter);
    civ7WriteU32(&w, slot);
    civ7WriteU8(&w, 0); /* is_host; we always join as a guest */

    return finish(&w);
}

/* Tells the host we are ready to receive game state, sent after registering. */
size_t civ7BuildSnapshotMessage(uint8_t *out, size_t capacity, uint32_t counter)
{
    Civ7Writer w;

    civ7WriterInit(&w, out, capacity);
    writeMessageHeader(&w, CIV7_MSG_SNAPSHOT_REQUEST, counter);
    civ7WriteU32(&w, 0);
    civ7WriteU8(&w, 1);

    return finish(&w);
}

/* Public chat: slot, text, then trailer fields that mark it as "from host" / "to everyone". */
size_t civ7BuildChatMessage(uint8_t *out, size_t capacity, uint32_t counter,
                            uint32_t slot, const char *text, size_t text_len)
{
    Civ7Writer w;

    civ7WriterInit(&w, out, capacity);
    writeMessageHeader(&w, CIV7_MSG_CHAT, counter);
    civ7WriteU32(&w, slot);
    civ7WriteU32(&w, (uint32_t)text_len);
    civ7WriteBytes(&w, (const uint8_t *)text, text_len);
    civ7WriteU32(&w, CIV7_CHAT_TRAILER_A);
    civ7WriteU32(&w, CIV7_CHAT_TRAILER_B);

    return finish(&w);
}
