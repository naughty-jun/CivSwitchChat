#pragma once

/* Shared by the ldn_model_*.c files only. Not part of the public model API. */

#include "ldn_model.h"
#include "civ7_transport.h"

#define LDN_STATE_WAIT_COUNT 50
#define LDN_STATE_WAIT_NS 100000000LL
#define LDN_IPV4_TEXT_SIZE 16

void ldnModelFormatIpv4(char* output, size_t output_size, LdnIpv4Address address);

/* Logs the joined network, its nodes and the local address. */
void ldnModelLogConnectedInfo(LdnModel* model);

/* Polls until the station state is confirmed. Logs the reason when it fails. */
bool ldnModelWaitForStationReady(LdnModel* model);

bool ldnModelIsValidNetworkIndex(const LdnModel* model, s32 network_index);

void ldnModelCloseCiv7Session(LdnModel* model);

/* Starts the Civ VII UDP session towards node 0 of `network`. */
void ldnModelStartCiv7Session(LdnModel* model, const LdnNetworkInfo* network);
