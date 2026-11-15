#pragma once

#include <stddef.h>
#include <stdint.h>
#include <switch.h>
#include "civ7_session.h"

#define LDN_MAX_NETWORKS 24
#define LDN_LOG_CAPACITY 64
#define LDN_LOG_LENGTH 128
#define LDN_MAC_TEXT_SIZE 18

/* Progress of joining a lobby, from pressing A to being in the lobby. */
typedef enum {
    LDN_SESSION_IDLE = 0,
    LDN_SESSION_CONNECTING_LDN,
    LDN_SESSION_LDN_FAILED,
    LDN_SESSION_HANDSHAKING,
    LDN_SESSION_UDP_FAILED,
    LDN_SESSION_JOINED,
    LDN_SESSION_REFUSED,
    LDN_SESSION_TIMED_OUT,
    LDN_SESSION_LDN_DISCONNECTED /* The wireless LDN link itself dropped (e.g. host closed the lobby). */
} LdnSessionPhase;

typedef struct {
    bool initialized;
    bool station_open;

    Result state_result;
    Result scan_result;

    LdnState state;

    bool scan_attempted;
    bool scan_success;

    s32 network_count;
    LdnNetworkInfo networks[LDN_MAX_NETWORKS];

    char logs[LDN_LOG_CAPACITY][LDN_LOG_LENGTH];
    u32 log_start;
    u32 log_count;

    LdnSessionPhase session_phase;
    s32 session_network_index;
    LdnNetworkInfo session_network;
    uint64_t session_next_node_poll_ms;

    Civ7Session civ7;
    bool civ7_started;
    bool civ7_join_logged;
    bool civ7_failure_logged;
} LdnModel;

void ldnModelInitialize(LdnModel* model);
void ldnModelRefresh(LdnModel* model);
void ldnModelConnect(LdnModel* model, s32 network_index);

/*
 * Records the chosen network and marks the session as connecting, without
 * blocking. Returns false for an invalid index. Call ldnModelConnect next.
 */
bool ldnModelBeginSession(LdnModel* model, s32 network_index);

/* Closes the Civ VII session, disconnects from LDN and rescans. */
void ldnModelLeaveSession(LdnModel* model);
void ldnModelShutdown(LdnModel* model);
void ldnModelUpdate(LdnModel* model);
bool ldnModelSendChat(LdnModel* model, const char* text);

void ldnModelLog(LdnModel* model, const char* format, ...);

/* Monotonic milliseconds, shared with the Civ VII transport's clock. */
uint64_t ldnModelNowMs(void);

u32 ldnModelGetLogCount(const LdnModel* model);
const char* ldnModelGetLogLine(const LdnModel* model, u32 index);

const char* ldnModelGetStateName(LdnState state);

/* Writes "AA:BB:CC:DD:EE:FF" for a 6-byte address. */
void ldnModelFormatMacAddress(char* output, size_t output_size, const u8 mac_address[6]);
