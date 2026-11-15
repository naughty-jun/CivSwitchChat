#pragma once

/* Socket-level sending and the sent-packet store used for retransmission. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "civ7_session.h"

uint64_t civ7NowMs(void);

bool civ7SendRaw(Civ7Session *s, const uint8_t *data, size_t len);
void civ7SendInitialPacket(Civ7Session *s);
void civ7SendControl(Civ7Session *s, uint8_t kind, uint64_t seq);
void civ7SendStatusPong(Civ7Session *s);

/*
 * Wraps `payload` in a data frame, remembers it for resend, and sends it.
 * When out_seq is not NULL, the assigned sequence number is written to it
 * (even if the send itself fails, since the packet is still remembered).
 */
bool civ7QueuePayload(Civ7Session *s, const uint8_t *payload, size_t payload_len, uint64_t *out_seq);

Civ7SavedPacket *civ7FindSavedPacket(Civ7SavedPacket *items, size_t count, uint64_t seq);
