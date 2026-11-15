#include "ldn_model_internal.h"

bool ldnModelBeginSession(LdnModel* model, s32 network_index)
{
    if (!ldnModelIsValidNetworkIndex(model, network_index))
        return false;

    model->session_network_index = network_index;
    model->session_network = model->networks[network_index];
    model->session_phase = LDN_SESSION_CONNECTING_LDN;

    return true;
}

/* Disconnects only when the station is actually connected. */
static void disconnectStation(LdnModel* model)
{
    if (!model->initialized || !model->station_open)
        return;

    model->state_result = ldnGetState(&model->state);
    if (R_FAILED(model->state_result)) {
        ldnModelLog(
            model,
            "Leave GetState failed: 0x%08X",
            (unsigned int)model->state_result
        );
        return;
    }

    if (model->state != LdnState_StationConnected) {
        ldnModelLog(
            model,
            "Leave: disconnect skipped, LDN state is %s",
            ldnModelGetStateName(model->state)
        );
        return;
    }

    Result rc = ldnDisconnect();
    if (R_FAILED(rc)) {
        ldnModelLog(model, "ldnDisconnect failed: 0x%08X", (unsigned int)rc);
        return;
    }

    ldnModelLog(model, "ldnDisconnect returned success");

    if (ldnModelWaitForStationReady(model))
        ldnModelLog(model, "Left the lobby");
}

void ldnModelLeaveSession(LdnModel* model)
{
    ldnModelLog(model, "Leaving lobby...");

    ldnModelCloseCiv7Session(model);
    disconnectStation(model);
    model->session_phase = LDN_SESSION_IDLE;

    /* The previous scan result is stale once the connection has changed. */
    ldnModelRefresh(model);
}
