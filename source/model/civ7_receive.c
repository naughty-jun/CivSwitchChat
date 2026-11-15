#include "civ7_receive.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "civ7_codec.h"
#include "civ7_transport.h"

/* ---- Game payload handling ---- */

static void markRefused(Civ7Session *s)
{
    s->refused = true;
    s->active = false;
}

static void scheduleRegistration(Civ7Session *s, const uint8_t *payload, size_t len)
{
    if (s->joined)
        return;

    s->slot = len >= CIV7_WELCOME_MIN_LENGTH
        ? civ7ReadU32(payload + CIV7_WELCOME_SLOT_OFFSET)
        : CIV7_DEFAULT_SLOT;
    if (s->slot == 0)
        s->slot = CIV7_DEFAULT_SLOT;

    uint64_t now = civ7NowMs();
    s->register_pending = true;
    s->app_register_pending = true;
    s->snapshot_pending = true;
    s->next_register_ms = now + CIV7_REGISTER_DELAY_MS;
    s->next_app_register_ms = now + CIV7_APP_REGISTER_DELAY_MS;
    s->next_snapshot_ms = now + CIV7_SNAPSHOT_DELAY_MS;
    s->joined = true;
}

/* A game payload is either a "Fire..." magic word or a Civ VII message with the usual header. */
static void processGamePayload(Civ7Session *s, const uint8_t *payload, size_t len)
{
    if (len >= CIV7_FIRE_MAGIC_SIZE) {
        uint64_t magic = civ7ReadU64(payload);

        if (magic == CIV7_FIRE_REFUSED || magic == CIV7_FIRE_CLOSE) {
            markRefused(s);
            return;
        }
        if (magic == CIV7_FIRE_REPLY) {
            scheduleRegistration(s, payload, len);
            return;
        }
    }

    if (len < CIV7_MESSAGE_HEADER_SIZE)
        return;

    uint32_t type = civ7ReadU32(payload);

    if (type == CIV7_MSG_REFUSED_A || type == CIV7_MSG_REFUSED_B) {
        markRefused(s);
        return;
    }
    if (type == CIV7_MSG_WELCOME)
        scheduleRegistration(s, payload, len);
}

/* ---- Out-of-order buffering ---- */

static void stashPending(Civ7Session *s, uint64_t seq, const uint8_t *payload, size_t len)
{
    Civ7SavedPacket *slot = &s->pending[seq % CIV7_PENDING_MAX];

    if (slot->used || len > sizeof(slot->data))
        return;

    slot->seq = seq;
    slot->len = len;
    slot->used = true;
    memcpy(slot->data, payload, len);
}

static void drainPending(Civ7Session *s)
{
    for (;;) {
        Civ7SavedPacket *slot =
            civ7FindSavedPacket(s->pending, CIV7_PENDING_MAX, s->expected_seq);
        if (!slot)
            return;

        processGamePayload(s, slot->data, slot->len);
        slot->used = false;
        s->expected_seq++;
    }
}

/*
 * The host restarts its sequence at 0, and our own first data frame can
 * arrive with any starting sequence: sync expected_seq to match in both
 * cases, dropping any out-of-order frames saved under the old sequence.
 */
static void syncExpectedSequence(Civ7Session *s, uint64_t seq)
{
    if (seq == 0) {
        s->expected_seq = 0;
        memset(s->pending, 0, sizeof(s->pending));
    } else if (s->fresh) {
        s->expected_seq = seq;
    }
    s->fresh = false;
}

static void handleDataFrame(Civ7Session *s, uint64_t seq, const uint8_t *payload, size_t len)
{
    if (seq == CIV7_SEQUENCE_SPECIAL) {
        processGamePayload(s, payload, len);
        return;
    }

    syncExpectedSequence(s, seq);

    if (seq < s->expected_seq)
        return;

    if (seq > s->expected_seq) {
        stashPending(s, seq, payload, len);
        return;
    }

    processGamePayload(s, payload, len);
    s->expected_seq++;
    drainPending(s);
}

/* ---- Control frames ---- */

static uint64_t initialSequence(const Civ7Session *s)
{
    return civ7ReadU64(s->initial + CIV7_FRAME_PREFIX_SIZE);
}

/* If `seq` was a chat line we sent, records a resend notice for the UI/log. */
static void noteIfResentChatLine(Civ7Session *s, uint64_t seq)
{
    Civ7SentChatRecord *record = &s->sent_chat[seq % CIV7_SENT_MAX];

    if (record->used && record->seq == seq) {
        s->resend_notice_pending = true;
        s->resend_notice_seq = seq;
        snprintf(s->resend_notice_text, sizeof(s->resend_notice_text), "%s", record->text);
    }
}

/*
 * Resends the initial packet (if seq is its sequence) or a saved data packet.
 * When `fallback_to_initial` is set, the initial packet is sent even when
 * `seq` does not match it and no saved packet is found for it.
 */
static void resendBySequence(Civ7Session *s, uint64_t seq, bool fallback_to_initial)
{
    if (seq == initialSequence(s)) {
        civ7SendRaw(s, s->initial, s->initial_len);
        return;
    }

    Civ7SavedPacket *saved = civ7FindSavedPacket(s->sent, CIV7_SENT_MAX, seq);
    if (saved)
        civ7SendRaw(s, saved->data, saved->len);
    else if (fallback_to_initial)
        civ7SendRaw(s, s->initial, s->initial_len);
}

static void resendSequence(Civ7Session *s, uint64_t seq)
{
    noteIfResentChatLine(s, seq);
    resendBySequence(s, seq, false);
}

/* The peer's sync tells us its next expected sequence; resend ours if it fell behind. */
static void handleSyncFrame(Civ7Session *s, uint64_t peer_next_seq)
{
    uint64_t last_sent = s->next_seq - 1;

    if (initialSequence(s) == CIV7_SEQUENCE_SPECIAL || peer_next_seq > last_sent)
        return;

    resendBySequence(s, last_sent, true);
}

/* LDN link keepalive, unrelated to the game protocol: reply to a ping with a pong. */
static void handleStatusFrame(Civ7Session *s, const uint8_t *body, size_t len)
{
    if (len == CIV7_STATUS_PING_BODY_SIZE && body[0] == CIV7_STATUS_PING)
        civ7SendStatusPong(s);
}

static bool isAddressedToLocalHost(const Civ7Session *s, const uint8_t *frame)
{
    uint8_t expected_ip[CIV7_IP_FIELD_SIZE];

    civ7EncodeIp(s->local_ip_be, expected_ip);
    return memcmp(frame, expected_ip, sizeof(expected_ip)) == 0;
}

static bool isSequencedKind(uint8_t kind)
{
    return kind == CIV7_KIND_DATA ||
           kind == CIV7_KIND_RESEND ||
           kind == CIV7_KIND_SYNC;
}

void civ7ReceiveOne(Civ7Session *s)
{
    uint8_t frame[CIV7_PACKET_MAX];
    ssize_t received = recv(s->fd, frame, sizeof(frame), 0);

    if (received < CIV7_FRAME_PREFIX_SIZE)
        return;
    if (!isAddressedToLocalHost(s, frame))
        return;

    uint8_t kind = frame[CIV7_FRAME_KIND_OFFSET];
    const uint8_t *body = frame + CIV7_FRAME_PREFIX_SIZE;
    size_t body_len = (size_t)received - CIV7_FRAME_PREFIX_SIZE;

    if (kind == CIV7_KIND_STATUS) {
        handleStatusFrame(s, body, body_len);
        return;
    }

    if (!isSequencedKind(kind) || body_len < CIV7_SEQUENCE_SIZE)
        return;

    uint64_t seq = civ7ReadU64(body);

    if (kind == CIV7_KIND_RESEND) {
        resendSequence(s, seq);
        return;
    }
    if (kind == CIV7_KIND_SYNC) {
        handleSyncFrame(s, seq);
        return;
    }

    handleDataFrame(s, seq, body + CIV7_SEQUENCE_SIZE, body_len - CIV7_SEQUENCE_SIZE);
}
