#include "ldn_model_internal.h"

#include <stdio.h>

#define LOGGED_NODE_MAX 8

void ldnModelFormatIpv4(char* output, size_t output_size, LdnIpv4Address address)
{
    const u8* bytes = (const u8*)&address.addr;

    snprintf(
        output,
        output_size,
        "%u.%u.%u.%u",
        (unsigned int)bytes[3],
        (unsigned int)bytes[2],
        (unsigned int)bytes[1],
        (unsigned int)bytes[0]
    );
}

void ldnModelFormatMacAddress(char* output, size_t output_size, const u8 mac_address[6])
{
    snprintf(
        output,
        output_size,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        mac_address[0],
        mac_address[1],
        mac_address[2],
        mac_address[3],
        mac_address[4],
        mac_address[5]
    );
}

static void logNetworkSummary(LdnModel* model, const LdnNetworkInfo* info)
{
    char bssid[LDN_MAC_TEXT_SIZE];

    ldnModelFormatMacAddress(bssid, sizeof(bssid), info->common.bssid.addr);

    ldnModelLog(model, "=== Connected Network ===");
    ldnModelLog(model, "SSID: %.*s", (int)info->common.ssid.len, info->common.ssid.str);
    ldnModelLog(
        model,
        "Channel=%d Nodes=%u/%d",
        (int)info->common.channel,
        (unsigned int)info->node_count,
        (int)info->node_count_max
    );
    ldnModelLog(model, "BSSID: %s", bssid);
}

static void logNode(LdnModel* model, u32 index, const LdnNodeInfo* node)
{
    char ip[LDN_IPV4_TEXT_SIZE];
    char mac[LDN_MAC_TEXT_SIZE];

    ldnModelFormatIpv4(ip, sizeof(ip), node->ip_addr);
    ldnModelFormatMacAddress(mac, sizeof(mac), node->mac_addr.addr);

    ldnModelLog(model, "Node %u: %s", (unsigned int)index, node->user_name);
    ldnModelLog(model, "  MAC %s", mac);

    ldnModelLog(
        model,
        "  IP=%s Connected=%u Version=%d",
        ip,
        (unsigned int)node->is_connected,
        (int)node->local_communication_version
    );
}

static void logNetworkInfo(LdnModel* model)
{
    LdnNetworkInfo info = {0};
    Result rc = ldnGetNetworkInfo(&info);

    if (R_FAILED(rc)) {
        ldnModelLog(model, "GetNetworkInfo failed: 0x%08X", (unsigned int)rc);
        return;
    }

    logNetworkSummary(model, &info);

    u32 node_count = info.node_count;
    if (node_count > LOGGED_NODE_MAX)
        node_count = LOGGED_NODE_MAX;

    for (u32 i = 0; i < node_count; i++)
        logNode(model, i, &info.nodes[i]);
}

static void logLocalAddress(LdnModel* model)
{
    LdnIpv4Address address = {0};
    LdnSubnetMask mask = {0};
    Result rc = ldnGetIpv4Address(&address, &mask);

    if (R_FAILED(rc)) {
        ldnModelLog(model, "GetIpv4Address failed: 0x%08X", (unsigned int)rc);
        return;
    }

    char ip[LDN_IPV4_TEXT_SIZE];
    char subnet[LDN_IPV4_TEXT_SIZE];

    ldnModelFormatIpv4(ip, sizeof(ip), address);
    ldnModelFormatIpv4(subnet, sizeof(subnet), (LdnIpv4Address){ .addr = mask.mask });

    ldnModelLog(model, "Local IP: %s", ip);
    ldnModelLog(model, "Subnet Mask: %s", subnet);
}

void ldnModelLogConnectedInfo(LdnModel* model)
{
    logNetworkInfo(model);
    logLocalAddress(model);
}
