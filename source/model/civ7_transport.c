#include "civ7_transport.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "civ7_codec.h"

#define MS_PER_SECOND  1000u
#define US_PER_MS      1000u

uint64_t civ7NowMs(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * MS_PER_SECOND + (uint64_t)tv.tv_usec / US_PER_MS;
}

bool civ7SendRaw(Civ7Session *s, const uint8_t *data, size_t len)
{
    struct sockaddr_in destination;

    memset(&destination, 0, sizeof(destination));
    destination.sin_family = AF_INET;
    destination.sin_port = htons(CIV7_UDP_PORT);
    destination.sin_addr.s_addr = s->host_ip_be;

    ssize_t sent = sendto(s->fd, data, len, 0,
                          (struct sockaddr *)&destination, sizeof(destination));
    return sent == (ssize_t)len;
}

void civ7SendInitialPacket(Civ7Session *s)
{
    if (!civ7SendRaw(s, s->initial, s->initial_len))
        return;

    s->initial_attempts++;
    s->next_initial_ms = civ7NowMs() + CIV7_INITIAL_RETRY_MS;
}

void civ7SendControl(Civ7Session *s, uint8_t kind, uint64_t seq)
{
    uint8_t packet[CIV7_FRAME_HEADER_SIZE];
    Civ7Writer w;

    civ7WriterInit(&w, packet, sizeof(packet));
    civ7WriteFrameHeader(&w, s->host_ip_be, kind, seq);
    civ7SendRaw(s, packet, w.length);
}

/* Status frames have no sequence number, so this builds its header by hand instead of civ7WriteFrameHeader. */
void civ7SendStatusPong(Civ7Session *s)
{
    uint8_t packet[CIV7_STATUS_PONG_FRAME_SIZE];
    Civ7Writer w;
    uint8_t ip[CIV7_IP_FIELD_SIZE];

    civ7EncodeIp(s->host_ip_be, ip);
    civ7WriterInit(&w, packet, sizeof(packet));
    civ7WriteBytes(&w, ip, sizeof(ip));
    civ7WriteU8(&w, CIV7_KIND_STATUS);
    civ7WriteU8(&w, CIV7_STATUS_PONG);
    civ7SendRaw(s, packet, w.length);
}

Civ7SavedPacket *civ7FindSavedPacket(Civ7SavedPacket *items, size_t count, uint64_t seq)
{
    for (size_t i = 0; i < count; ++i) {
        if (items[i].used && items[i].seq == seq)
            return &items[i];
    }
    return NULL;
}

/* Remembers `packet` under `seq` in the sent-packet ring, for later resends. */
static void storeSentPacket(Civ7Session *s, uint64_t seq, const uint8_t *packet, size_t len)
{
    Civ7SavedPacket *slot = &s->sent[seq % CIV7_SENT_MAX];

    slot->seq = seq;
    slot->len = len;
    slot->used = true;
    memcpy(slot->data, packet, len);
}

bool civ7QueuePayload(Civ7Session *s, const uint8_t *payload, size_t payload_len, uint64_t *out_seq)
{
    if (payload_len + CIV7_FRAME_HEADER_SIZE > CIV7_PACKET_MAX)
        return false;

    uint64_t seq = s->next_seq++;

    if (out_seq)
        *out_seq = seq;

    uint8_t packet[CIV7_PACKET_MAX];
    Civ7Writer w;

    civ7WriterInit(&w, packet, sizeof(packet));
    civ7WriteFrameHeader(&w, s->host_ip_be, CIV7_KIND_DATA, seq);
    civ7WriteBytes(&w, payload, payload_len);

    storeSentPacket(s, seq, packet, w.length);

    return civ7SendRaw(s, packet, w.length);
}
