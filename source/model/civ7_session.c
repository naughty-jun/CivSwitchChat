#include "civ7_session.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "civ7_codec.h"
#include "civ7_receive.h"
#include "civ7_transport.h"

/* ---- Lifecycle ---- */

void civ7SessionInit(Civ7Session *s)
{
    memset(s, 0, sizeof(*s));
    s->fd = -1;
    s->fresh = true;
}

void civ7SessionClose(Civ7Session *s)
{
    if (!s)
        return;

    if (s->fd >= 0)
        close(s->fd);

    s->fd = -1;
    s->active = false;
}

static void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return;

    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static bool bindToLocalAddress(const Civ7Session *s)
{
    struct sockaddr_in local = {0};

    local.sin_family = AF_INET;
    local.sin_port = htons(CIV7_UDP_PORT);
    local.sin_addr.s_addr = s->local_ip_be;

    return bind(s->fd, (struct sockaddr *)&local, sizeof(local)) == 0;
}

static void reportErrno(int *out_errno)
{
    if (out_errno)
        *out_errno = errno;
}

/* Opens and binds a non-blocking UDP socket for `s->local_ip_be`. */
static Civ7StartResult openSocket(Civ7Session *s, int *out_errno)
{
    s->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s->fd < 0) {
        reportErrno(out_errno);
        return CIV7_START_SOCKET_FAILED;
    }

    setNonBlocking(s->fd);

    if (!bindToLocalAddress(s)) {
        reportErrno(out_errno);
        civ7SessionClose(s);
        return CIV7_START_BIND_FAILED;
    }

    return CIV7_START_OK;
}

/* Builds the initial packet, starts the timers and sends the first FireCONN. */
static Civ7StartResult beginHandshake(Civ7Session *s)
{
    s->initial_len = civ7BuildInitialPacket(s->initial, sizeof(s->initial), s->host_ip_be);
    if (s->initial_len == 0) {
        civ7SessionClose(s);
        return CIV7_START_INITIAL_PACKET_FAILED;
    }
    s->next_seq = CIV7_FIRST_QUEUED_SEQUENCE;

    s->active = true;
    s->start_ms = civ7NowMs();
    s->last_resend_request_ms = s->start_ms;
    s->last_sync_ms = s->start_ms;
    civ7SendInitialPacket(s);

    return CIV7_START_OK;
}

Civ7StartResult civ7SessionStart(Civ7Session *s, const char *local_ip, const char *host_ip, int *out_errno)
{
    if (out_errno)
        *out_errno = 0;

    civ7SessionClose(s);
    civ7SessionInit(s);

    s->local_ip_be = inet_addr(local_ip);
    s->host_ip_be = inet_addr(host_ip);
    if (s->local_ip_be == INADDR_NONE || s->host_ip_be == INADDR_NONE)
        return CIV7_START_INVALID_ADDRESS;

    Civ7StartResult result = openSocket(s, out_errno);
    if (result != CIV7_START_OK)
        return result;

    return beginHandshake(s);
}

/* ---- Periodic work ---- */

static bool isDue(bool pending, uint64_t now, uint64_t due_ms)
{
    return pending && now >= due_ms;
}

static void queueMessage(Civ7Session *s, const uint8_t *message, size_t len)
{
    if (len == 0)
        return;

    civ7QueuePayload(s, message, len, NULL);
}

static void sendRegisterMessage(Civ7Session *s)
{
    uint8_t message[CIV7_MESSAGE_BUFFER_SIZE];
    size_t len = civ7BuildRegisterMessage(message, sizeof(message), s->counter++);

    queueMessage(s, message, len);
}

static void sendAppRegisterMessage(Civ7Session *s)
{
    uint8_t message[CIV7_MESSAGE_BUFFER_SIZE];
    size_t len = civ7BuildAppRegisterMessage(message, sizeof(message), s->counter++, s->slot);

    queueMessage(s, message, len);
}

static void sendSnapshotMessage(Civ7Session *s)
{
    uint8_t message[CIV7_MESSAGE_BUFFER_SIZE];
    size_t len = civ7BuildSnapshotMessage(message, sizeof(message), s->counter++);

    queueMessage(s, message, len);
}

static void retryHandshake(Civ7Session *s, uint64_t now)
{
    if (s->joined)
        return;

    bool still_retrying = s->initial_attempts < CIV7_INITIAL_MAX_ATTEMPTS;

    if (still_retrying && now >= s->next_initial_ms) {
        civ7SendInitialPacket(s);
        return;
    }

    if (!still_retrying && now >= s->start_ms + CIV7_HANDSHAKE_TIMEOUT_MS)
        s->timed_out = true;
}

/* Sends the post-join registration sequence (version, app, snapshot) once each becomes due. */
static void sendScheduledMessages(Civ7Session *s, uint64_t now)
{
    if (isDue(s->register_pending, now, s->next_register_ms)) {
        sendRegisterMessage(s);
        s->register_pending = false;
    }
    if (isDue(s->app_register_pending, now, s->next_app_register_ms)) {
        sendAppRegisterMessage(s);
        s->app_register_pending = false;
    }
    if (isDue(s->snapshot_pending, now, s->next_snapshot_ms)) {
        sendSnapshotMessage(s);
        s->snapshot_pending = false;
    }
}

static void sendSyncIfDue(Civ7Session *s, uint64_t now)
{
    if (s->last_sync_ms + CIV7_SYNC_INTERVAL_MS > now)
        return;

    civ7SendControl(s, CIV7_KIND_SYNC, s->expected_seq);
    s->last_sync_ms = now;
}

static bool hasPendingPackets(const Civ7Session *s)
{
    for (size_t i = 0; i < CIV7_PENDING_MAX; ++i) {
        if (s->pending[i].used)
            return true;
    }
    return false;
}

static void requestResendIfDue(Civ7Session *s, uint64_t now)
{
    if (!hasPendingPackets(s) || s->last_resend_request_ms + CIV7_RESEND_REQUEST_INTERVAL_MS > now)
        return;

    civ7SendControl(s, CIV7_KIND_RESEND, s->expected_seq);
    s->last_resend_request_ms = now;
}

void civ7SessionUpdate(Civ7Session *s)
{
    if (!s || !s->active || s->fd < 0)
        return;

    uint64_t now = civ7NowMs();

    civ7ReceiveOne(s);
    if (!s->active)
        return;

    retryHandshake(s, now);
    sendScheduledMessages(s, now);
    sendSyncIfDue(s, now);
    requestResendIfDue(s, now);
}

/* ---- Chat ---- */

/* Remembers `text` under `seq` so a later resend request can be logged with it. */
static void rememberSentChat(Civ7Session *s, uint64_t seq, const char *text)
{
    Civ7SentChatRecord *slot = &s->sent_chat[seq % CIV7_SENT_MAX];

    slot->seq = seq;
    slot->used = true;
    snprintf(slot->text, sizeof(slot->text), "%s", text);
}

bool civ7SessionSendChat(Civ7Session *s, const char *text)
{
    /* joined is required: chat needs our slot, which the host assigns in NetWelcome. */
    if (!s || !s->active || !s->joined || !text)
        return false;

    size_t text_len = strlen(text);
    if (text_len == 0 || text_len > CIV7_CHAT_MAX_LENGTH)
        return false;

    uint8_t message[CIV7_PACKET_MAX];
    size_t len = civ7BuildChatMessage(message, sizeof(message), s->counter++,
                                      s->slot, text, text_len);
    if (len == 0)
        return false;

    uint64_t seq = 0;
    bool sent = civ7QueuePayload(s, message, len, &seq);
    rememberSentChat(s, seq, text);

    return sent;
}

/* ---- Resend notice ---- */

bool civ7SessionTakeResendNotice(Civ7Session *s, char *out_text, size_t out_text_size, uint64_t *out_seq)
{
    if (!s || !s->resend_notice_pending)
        return false;

    snprintf(out_text, out_text_size, "%s", s->resend_notice_text);
    if (out_seq)
        *out_seq = s->resend_notice_seq;
    s->resend_notice_pending = false;

    return true;
}
