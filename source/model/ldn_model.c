#include "ldn_model_internal.h"

#include <string.h>

const char* ldnModelGetStateName(LdnState state)
{
    switch (state) {
        case LdnState_None:
            return "None";
        case LdnState_Initialized:
            return "Initialized";
        case LdnState_AccessPoint:
            return "AccessPoint";
        case LdnState_AccessPointCreated:
            return "AccessPointCreated";
        case LdnState_Station:
            return "Station";
        case LdnState_StationConnected:
            return "StationConnected";
        case LdnState_Error:
            return "Error";
        default:
            return "Unknown";
    }
}

/* ---- Scan ---- */

static void resetScanResults(LdnModel* model)
{
    model->scan_attempted = true;
    model->scan_success = false;
    model->scan_result = 0;
    model->network_count = 0;

    memset(model->networks, 0, sizeof(model->networks));
}

static void clampNetworkCount(LdnModel* model)
{
    if (model->network_count < 0)
        model->network_count = 0;

    if (model->network_count > LDN_MAX_NETWORKS)
        model->network_count = LDN_MAX_NETWORKS;
}

static void runScan(LdnModel* model)
{
    LdnScanFilter filter = {0};

    /* Only return Civilization VII LDN networks. */
    // filter.network_id.intent_id.local_communication_id = CIV7_LDN_LOCAL_COMM_ID;
    // filter.flags = LdnScanFilterFlag_LocalCommunicationId;

    ldnModelLog(model, "Scanning LDN...");

    model->scan_result = ldnScan(
        0,
        &filter,
        model->networks,
        LDN_MAX_NETWORKS,
        &model->network_count
    );

    if (R_FAILED(model->scan_result)) {
        ldnModelLog(
            model,
            "Scan failed: 0x%08X",
            (unsigned int)model->scan_result
        );
        return;
    }

    model->scan_success = true;
    clampNetworkCount(model);

    ldnModelLog(
        model,
        "Scan completed: %d network(s)",
        (int)model->network_count
    );
}

void ldnModelRefresh(LdnModel* model)
{
    resetScanResults(model);

    if (!model->initialized || !model->station_open) {
        ldnModelLog(model, "Scan skipped: Station is unavailable");
        return;
    }

    model->state_result = ldnGetState(&model->state);
    if (R_FAILED(model->state_result)) {
        ldnModelLog(
            model,
            "GetState failed: 0x%08X",
            (unsigned int)model->state_result
        );
        return;
    }

    if (model->state != LdnState_Station) {
        ldnModelLog(
            model,
            "Scan skipped: State is %s",
            ldnModelGetStateName(model->state)
        );
        return;
    }

    runScan(model);
}

/* ---- Initialization ---- */

static void resetCiv7State(LdnModel* model)
{
    civ7SessionInit(&model->civ7);
    model->civ7_started = false;
    model->civ7_join_logged = false;
    model->civ7_failure_logged = false;
}

static bool pollStationReady(LdnModel* model)
{
    for (int i = 0; i < LDN_STATE_WAIT_COUNT; i++) {
        model->state_result = ldnGetState(&model->state);

        if (R_SUCCEEDED(model->state_result) &&
            model->state == LdnState_Station)
            return true;

        svcSleepThread(LDN_STATE_WAIT_NS);
    }

    return false;
}

static void logStationWaitFailure(LdnModel* model)
{
    if (R_FAILED(model->state_result)) {
        ldnModelLog(
            model,
            "GetState failed: 0x%08X",
            (unsigned int)model->state_result
        );
        return;
    }

    ldnModelLog(
        model,
        "Station state timeout: %s",
        ldnModelGetStateName(model->state)
    );
}

bool ldnModelWaitForStationReady(LdnModel* model)
{
    if (pollStationReady(model))
        return true;

    logStationWaitFailure(model);
    return false;
}

void ldnModelInitialize(LdnModel* model)
{
    resetCiv7State(model);
    model->state = LdnState_None;

    ldnModelLog(model, "Initializing LDN service...");
    
    Result initialize_result = ldnInitialize(LdnServiceType_User);
    model->initialized = R_SUCCEEDED(initialize_result);

    if (!model->initialized) {
        ldnModelLog(
            model,
            "LDN initialization failed: 0x%08X",
            (unsigned int)initialize_result
        );
        return;
    }

    ldnModelLog(model, "LDN initialization successful");
    ldnModelLog(model, "Opening Station...");

    Result open_station_result = ldnOpenStation();
    model->station_open = R_SUCCEEDED(open_station_result);

    if (!model->station_open) {
        ldnModelLog(
            model,
            "OpenStation failed: 0x%08X",
            (unsigned int)open_station_result
        );
        return;
    }

    ldnModelLog(model, "Station opened");

    if (!ldnModelWaitForStationReady(model))
        return;

    ldnModelLog(model, "Station state confirmed");
}

/* ---- Shutdown ---- */

void ldnModelShutdown(LdnModel* model)
{
    ldnModelCloseCiv7Session(model);

    if (!model->initialized)
        return;

    ldnExit();
    model->initialized = false;
    model->station_open = false;
}
