#pragma once

/*
 * Byte-level encoding for the Civ VII transport.
 * Pure functions only: nothing here touches sockets or session state.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Bounds-checked little-endian writer. Once a write does not fit, `overflow` stays set. */
typedef struct {
    uint8_t *data;
    size_t capacity;
    size_t length;
    bool overflow;
} Civ7Writer;

void civ7WriterInit(Civ7Writer *w, uint8_t *buffer, size_t capacity);
bool civ7WriterOk(const Civ7Writer *w);

void civ7WriteU8(Civ7Writer *w, uint8_t value);
void civ7WriteU32(Civ7Writer *w, uint32_t value);
void civ7WriteU64(Civ7Writer *w, uint64_t value);
void civ7WriteBytes(Civ7Writer *w, const uint8_t *src, size_t len);
void civ7WriteZeros(Civ7Writer *w, size_t len);
void civ7WriteTextBlock(Civ7Writer *w, const char *text);

uint32_t civ7ReadU32(const uint8_t *p);
uint64_t civ7ReadU64(const uint8_t *p);

/* Civ VII transport stores IPv4 octets in reverse order. */
void civ7EncodeIp(uint32_t ip_be, uint8_t out[4]);

void civ7WriteFrameHeader(Civ7Writer *w, uint32_t host_ip_be, uint8_t kind, uint64_t seq);

/* Each builder returns the number of bytes written, or 0 on failure. */
size_t civ7BuildInitialPacket(uint8_t *out, size_t capacity, uint32_t host_ip_be);
size_t civ7BuildRegisterMessage(uint8_t *out, size_t capacity, uint32_t counter);
size_t civ7BuildAppRegisterMessage(uint8_t *out, size_t capacity, uint32_t counter, uint32_t slot);
size_t civ7BuildSnapshotMessage(uint8_t *out, size_t capacity, uint32_t counter);
size_t civ7BuildChatMessage(uint8_t *out, size_t capacity, uint32_t counter,
                            uint32_t slot, const char *text, size_t text_len);
