#pragma once

#include <stdint.h>
#include <switch.h>

/* ---- Transport sizes ---- */
#define CIV7_UDP_PORT      62056
#define CIV7_PACKET_MAX    1500
#define CIV7_SENT_MAX      64
#define CIV7_PENDING_MAX   16

/* ---- Frame layout: [reversed IPv4 (4)] [kind (1)] [sequence (8)] [payload] ---- */
#define CIV7_IP_FIELD_SIZE      4
#define CIV7_FRAME_KIND_OFFSET  4
#define CIV7_FRAME_PREFIX_SIZE  5   /* IP + kind */
#define CIV7_SEQUENCE_SIZE      8
#define CIV7_FRAME_HEADER_SIZE  13  /* IP + kind + sequence */

/* ---- Frame kinds ---- */
#define CIV7_KIND_DATA     'F'
#define CIV7_KIND_RESEND   'R'
#define CIV7_KIND_SYNC     'V'
#define CIV7_KIND_STATUS   'S'
#define CIV7_STATUS_PING   'P'
#define CIV7_STATUS_PONG   'p'
#define CIV7_STATUS_PING_BODY_SIZE 1
#define CIV7_STATUS_PONG_FRAME_SIZE 6

/* ---- Sequence numbers ---- */
#define CIV7_SEQUENCE_SPECIAL  UINT64_MAX
#define CIV7_INITIAL_SEQUENCE  UINT64_C(0)
#define CIV7_FIRST_QUEUED_SEQUENCE 1

/* ---- Magic words found in the first 8 bytes of a game payload (ASCII "Fire..." in little-endian) ---- */
#define CIV7_FIRE_CONN     UINT64_C(0x46697265434F4E4E) /* "FireCONN": our join request */
#define CIV7_FIRE_REPLY    UINT64_C(0x4669726552504C59) /* "FireRPLY": host accepted the join */
#define CIV7_FIRE_REFUSED  UINT64_C(0x4669726552465553) /* "FireRFUS": host refused the join */
#define CIV7_FIRE_CLOSE    UINT64_C(0x46697265B9133AB7) /* host closed the lobby (not valid ASCII) */
#define CIV7_FIRE_MAGIC_SIZE 8

/* ---- Application messages ---- */
#define CIV7_MESSAGE_HEADER_SIZE   28
#define CIV7_MESSAGE_BUFFER_SIZE   512
#define CIV7_MESSAGE_FIELD_ONE     1
#define CIV7_MESSAGE_FIELD_ALL     UINT32_MAX

#define CIV7_MSG_WELCOME            0x4B0 /* NetWelcome: host accepted us, carries our slot */
#define CIV7_MSG_REGISTER_VERSION   0x4B8 /* NetRegister: announce our client version */
#define CIV7_MSG_REGISTER_APP       0x4B9 /* NetAppRegister: announce our slot to the app */
#define CIV7_MSG_REFUSED_A          0x4BA /* NetJoinRefused */
#define CIV7_MSG_REFUSED_B          0x3EC /* NetKicked */
#define CIV7_MSG_SNAPSHOT_REQUEST   0x4E9 /* NetSnapshotProcessed: ready for game state */
#define CIV7_MSG_CHAT               0x3F3 /* NetChat */

#define CIV7_WELCOME_MIN_LENGTH     44 /* NetWelcome must be at least this long to contain a slot */
#define CIV7_WELCOME_SLOT_OFFSET    40 /* byte offset of our assigned slot within NetWelcome */
#define CIV7_DEFAULT_SLOT           1  /* used when NetWelcome is too short to carry a slot */

/* ---- Identity ---- */
#define CIV7_PLAYER_NAME            "Civ7-Switch"
#define CIV7_CLIENT_VERSION         "1.0.0"
#define CIV7_INTRO_PROTOCOL_VERSION 6
#define CIV7_TEXT_BLOCK_SIZE        64
#define CIV7_TEXT_MAX_LENGTH        63

/* ---- Chat ---- */
#define CIV7_CHAT_MAX_LENGTH        400
#define CIV7_CHAT_TRAILER_A         UINT32_C(0xFFFFFFFE) /* chat_type = -2: shown as sent by the host */
#define CIV7_CHAT_TRAILER_B         UINT32_MAX           /* target = -1: sent to everyone */

/* ---- Timing (milliseconds) ---- */
#define CIV7_INITIAL_RETRY_MS         3000 /* delay between FireCONN retries while unjoined */
#define CIV7_INITIAL_MAX_ATTEMPTS     3
#define CIV7_HANDSHAKE_TIMEOUT_MS     9000 /* give up joining after this long with no reply */
/* After NetWelcome, these three messages are sent in order with increasing delay. */
#define CIV7_REGISTER_DELAY_MS        100
#define CIV7_APP_REGISTER_DELAY_MS    200
#define CIV7_SNAPSHOT_DELAY_MS        400
#define CIV7_SYNC_INTERVAL_MS         5000   /* how often we tell the peer our expected sequence */
#define CIV7_RESEND_REQUEST_INTERVAL_MS 500  /* how often we ask the peer to resend a missing packet */

/* ---- LDN ---- */
/* Passphrase Civ VII expects for its LDN lobby network. Defined in civ7_protocol.c. */
#define CIV7_LDN_PASSPHRASE_SIZE 22
extern const u8 CIV7_LDN_PASSPHRASE[CIV7_LDN_PASSPHRASE_SIZE];
/* LocalCommunicationId for Civ VII LDN networks. Used for scan filtering. */
extern const s64 CIV7_LDN_LOCAL_COMM_ID;