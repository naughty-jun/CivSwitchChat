#pragma once

#include <switch.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "civ7_protocol.h"

typedef struct {
    uint64_t seq;
    size_t len;
    uint8_t data[CIV7_PACKET_MAX];
    bool used;
} Civ7SavedPacket;

/* Remembers the text of a sent chat line so a resend request can be logged. */
typedef struct {
    uint64_t seq;
    char text[CIV7_CHAT_MAX_LENGTH + 1];
    bool used;
} Civ7SentChatRecord;

typedef struct {
    int fd;
    bool active;
    bool joined;    /* host sent FireRPLY/NetWelcome: we are in the lobby */
    bool refused;   /* host sent FireRFUS/FireClose/NetJoinRefused/NetKicked */
    bool timed_out; /* no reply after CIV7_INITIAL_MAX_ATTEMPTS FireCONN retries */
    unsigned initial_attempts;
    uint64_t next_seq;      /* sequence number for our next queued data packet */
    uint64_t expected_seq;  /* sequence number we expect from the peer next */
    bool fresh;     /* true until the first data frame sets expected_seq */
    uint32_t counter;  /* per-message counter field required by the Civ VII message header */
    uint32_t slot;     /* our lobby slot, assigned by the host in NetWelcome */
    uint32_t local_ip_be;
    uint32_t host_ip_be;
    uint8_t initial[CIV7_PACKET_MAX]; /* the FireCONN packet, resent on request */
    size_t initial_len;
    Civ7SavedPacket sent[CIV7_SENT_MAX];       /* ring of sent data packets, for resends */
    Civ7SavedPacket pending[CIV7_PENDING_MAX]; /* out-of-order data frames, waiting to be applied */
    Civ7SentChatRecord sent_chat[CIV7_SENT_MAX];
    uint64_t start_ms;
    uint64_t next_initial_ms;
    /* NetRegister/NetAppRegister/NetSnapshotProcessed: scheduled once after joining, see *_pending below. */
    uint64_t next_register_ms;
    uint64_t next_app_register_ms;
    uint64_t next_snapshot_ms;
    uint64_t last_resend_request_ms;
    uint64_t last_sync_ms;
    bool register_pending;
    bool app_register_pending;
    bool snapshot_pending;

    /* Set when the peer asks us to resend a chat line; read via civ7SessionTakeResendNotice. */
    bool resend_notice_pending;
    uint64_t resend_notice_seq;
    char resend_notice_text[CIV7_CHAT_MAX_LENGTH + 1];
} Civ7Session;

/* Return values of civ7SessionStart (the numeric values are shown in the log). */
typedef enum {
    CIV7_START_OK = 0,
    CIV7_START_INVALID_ADDRESS = 1,
    CIV7_START_SOCKET_FAILED = 2,
    CIV7_START_BIND_FAILED = 3,
    CIV7_START_INITIAL_PACKET_FAILED = 4
} Civ7StartResult;

void civ7SessionInit(Civ7Session *s);
Civ7StartResult civ7SessionStart(Civ7Session *s, const char *local_ip, const char *host_ip, int *out_errno);
void civ7SessionUpdate(Civ7Session *s);

bool civ7SessionSendChat(Civ7Session *s, const char *text);

/*
 * Reads and clears the most recent resend notice set when the peer asked us
 * to resend a chat line (see Civ7Session's internal tracking). Returns false
 * when there is no new notice. `out_text` must be at least
 * CIV7_CHAT_MAX_LENGTH + 1 bytes. `out_seq` may be NULL.
 */
bool civ7SessionTakeResendNotice(Civ7Session *s, char *out_text, size_t out_text_size, uint64_t *out_seq);

void civ7SessionClose(Civ7Session *s);
