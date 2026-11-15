#include "ldn_model_internal.h"

#include <stdio.h>
#include <string.h>

#define LDN_CONNECT_OPTION 0
#define LDN_MAX_COMMUNICATION_VERSION 0x7FFF

/* ---- Preconditions ---- */

static bool isStationAvailable(const LdnModel* model)
{
    return model->initialized && model->station_open;
}

bool ldnModelIsValidNetworkIndex(const LdnModel* model, s32 network_index)
{
    return network_index >= 0 &&
           network_index < model->network_count &&
           network_index < LDN_MAX_NETWORKS;
}

static bool isValidCommunicationVersion(s32 version)
{
    return version >= 0 && version <= LDN_MAX_COMMUNICATION_VERSION;
}

/* Logs and returns false unless the LDN state is Station. */
static bool confirmStationState(LdnModel* model)
{
    LdnState state = LdnState_None;
    Result rc = ldnGetState(&state);

    if (R_FAILED(rc)) {
        ldnModelLog(model, "Connect GetState failed: 0x%08X", (unsigned int)rc);
        return false;
    }

    model->state = state;

    if (state != LdnState_Station) {
        ldnModelLog(
            model,
            "Connect aborted: LDN state is %s",
            ldnModelGetStateName(state)
        );
        return false;
    }

    return true;
}

/* ---- Connection parameters ---- */

static LdnSecurityConfig buildSecurityConfig(void)
{
    LdnSecurityConfig security = {0};

    security.passphrase_size = CIV7_LDN_PASSPHRASE_SIZE;
    memcpy(security.passphrase, CIV7_LDN_PASSPHRASE, CIV7_LDN_PASSPHRASE_SIZE);

    return security;
}

static LdnUserConfig buildUserConfig(void)
{
    LdnUserConfig user = {0};

    snprintf(user.user_name, sizeof(user.user_name), "%s", CIV7_PLAYER_NAME);

    return user;
}

/* ---- Waiting for the connection ---- */

/*
 * Polls until the station is connected, leaves the Station state, or the
 * query fails. The caller inspects model->state and the returned Result.
 */
static Result waitForStationConnected(LdnModel* model)
{
    Result rc = 0;

    for (int i = 0; i < LDN_STATE_WAIT_COUNT; i++) {
        rc = ldnGetState(&model->state);

        /* Stop as soon as we leave Station, whether that means success or not. */
        if (R_FAILED(rc) || model->state != LdnState_Station)
            return rc;

        svcSleepThread(LDN_STATE_WAIT_NS);
    }

    return rc;
}

/* Logs and returns false unless the station ended up connected. */
static bool confirmStationConnected(LdnModel* model, Result wait_result)
{
    if (R_FAILED(wait_result)) {
        ldnModelLog(
            model,
            "Connection state check failed: 0x%08X",
            (unsigned int)wait_result
        );
        return false;
    }

    if (model->state != LdnState_StationConnected) {
        ldnModelLog(
            model,
            "Connection not established: State=%s",
            ldnModelGetStateName(model->state)
        );
        return false;
    }

    return true;
}

/* ---- Connecting ---- */

/* Runs the LDN part of joining. Returns true once the station is connected. */
static bool connectToNetwork(LdnModel* model, s32 network_index)
{
    if (!isStationAvailable(model)) {
        ldnModelLog(model, "Connect aborted: Station unavailable");
        return false;
    }

    if (!ldnModelIsValidNetworkIndex(model, network_index)) {
        ldnModelLog(
            model,
            "Connect aborted: Invalid network index %d",
            (int)network_index
        );
        return false;
    }

    if (!confirmStationState(model))
        return false;

    const LdnNetworkInfo* network = &model->networks[network_index];

    /*
     * Do not reject the scanned entry solely because node_count is zero.
     * Keep the scan-provided node[0] candidate visible in the log and let
     * ldnConnect report whether the entry contains usable connection data.
     */
    s32 version = network->nodes[0].local_communication_version;
    ldnModelLog(
        model,
        "Scan entry: node_count=%u node0_version=%d node0_ip=0x%08X",
        (unsigned int)network->node_count,
        (int)version,
        (unsigned int)network->nodes[0].ip_addr.addr
    );

    if (!isValidCommunicationVersion(version)) {
        ldnModelLog(
            model,
            "Connect aborted: Invalid communication version %d",
            (int)version
        );
        return false;
    }

    LdnSecurityConfig security = buildSecurityConfig();
    LdnUserConfig user = buildUserConfig();

    ldnModelLog(model, "Connecting to network %d...", (int)(network_index + 1));
    ldnModelLog(
        model,
        "Connection parameters: Version=%d, Option=%d",
        (int)version,
        LDN_CONNECT_OPTION
    );
    ldnModelLog(
        model,
        "LDN passphrase configured: %u bytes",
        (unsigned int)security.passphrase_size
    );

    Result rc = ldnConnect(&security, &user, version, LDN_CONNECT_OPTION, network);
    if (R_FAILED(rc)) {
        ldnModelLog(model, "ldnConnect failed: 0x%08X", (unsigned int)rc);
        return false;
    }

    ldnModelLog(model, "ldnConnect returned success");

    Result wait_result = waitForStationConnected(model);
    return confirmStationConnected(model, wait_result);
}

/* ---- Entry point ---- */

void ldnModelConnect(LdnModel* model, s32 network_index)
{
    if (!connectToNetwork(model, network_index)) {
        model->session_phase = LDN_SESSION_LDN_FAILED;
        return;
    }

    ldnModelLog(model, "LDN connection established");
    ldnModelLogConnectedInfo(model);
    ldnModelStartCiv7Session(model, &model->networks[network_index]);
}
