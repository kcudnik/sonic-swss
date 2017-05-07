extern "C" {
#include "sai.h"
#include "saistatus.h"
}

#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <getopt.h>

#include "orchdaemon.h"
#include "logger.h"
#include "notifications.h"
#include <sairedis.h>


using namespace std;
using namespace swss;

#define UNREFERENCED_PARAMETER(P)       (P)

/* Initialize all global api pointers */
sai_switch_api_t*           sai_switch_api;
sai_bridge_api_t*           sai_bridge_api;
sai_virtual_router_api_t*   sai_virtual_router_api;
sai_port_api_t*             sai_port_api;
sai_vlan_api_t*             sai_vlan_api;
sai_router_interface_api_t* sai_router_intfs_api;
sai_hostif_api_t*           sai_hostif_api;
sai_neighbor_api_t*         sai_neighbor_api;
sai_next_hop_api_t*         sai_next_hop_api;
sai_next_hop_group_api_t*   sai_next_hop_group_api;
sai_route_api_t*            sai_route_api;
sai_lag_api_t*              sai_lag_api;
sai_policer_api_t*          sai_policer_api;
sai_tunnel_api_t*           sai_tunnel_api;
sai_queue_api_t*            sai_queue_api;
sai_scheduler_api_t*        sai_scheduler_api;
sai_scheduler_group_api_t*  sai_scheduler_group_api;
sai_wred_api_t*             sai_wred_api;
sai_qos_map_api_t*          sai_qos_map_api;
sai_buffer_api_t*           sai_buffer_api;
sai_acl_api_t*              sai_acl_api;
sai_mirror_api_t*           sai_mirror_api;
sai_fdb_api_t*              sai_fdb_api;

/* Global variables */
map<string, string> gProfileMap = {
    { "SAI_INIT_CONFIG_FILE", "/usr/share/sai_2700.xml" },
    { "DEVICE_MAC_ADDRESS", "7c:fe:90:5e:6a:80" }
};
sai_object_id_t gVirtualRouterId;
sai_object_id_t gUnderlayIfId;
sai_object_id_t gSwitchId = SAI_NULL_OBJECT_ID;
MacAddress gMacAddress;

/* Global database mutex */
mutex gDbMutex;

const char *test_profile_get_value (
    _In_ sai_switch_profile_id_t profile_id,
    _In_ const char *variable)
{
    SWSS_LOG_ENTER();

    auto it = gProfileMap.find(variable);

    if (it == gProfileMap.end())
        return NULL;
    return it->second.c_str();
}

int test_profile_get_next_value (
    _In_ sai_switch_profile_id_t profile_id,
    _Out_ const char **variable,
    _Out_ const char **value)
{
    SWSS_LOG_ENTER();

    return -1;
}

const service_method_table_t test_services = {
    test_profile_get_value,
    test_profile_get_next_value
};

void initSaiApi()
{
    SWSS_LOG_ENTER();

    sai_api_initialize(0, (service_method_table_t *)&test_services);

    sai_api_query(SAI_API_SWITCH,               (void **)&sai_switch_api);
    sai_api_query(SAI_API_BRIDGE,               (void **)&sai_bridge_api);
    sai_api_query(SAI_API_VIRTUAL_ROUTER,       (void **)&sai_virtual_router_api);
    sai_api_query(SAI_API_PORT,                 (void **)&sai_port_api);
    sai_api_query(SAI_API_FDB,                  (void **)&sai_fdb_api);
    sai_api_query(SAI_API_VLAN,                 (void **)&sai_vlan_api);
    sai_api_query(SAI_API_HOSTIF,               (void **)&sai_hostif_api);
    sai_api_query(SAI_API_MIRROR,               (void **)&sai_mirror_api);
    sai_api_query(SAI_API_ROUTER_INTERFACE,     (void **)&sai_router_intfs_api);
    sai_api_query(SAI_API_NEIGHBOR,             (void **)&sai_neighbor_api);
    sai_api_query(SAI_API_NEXT_HOP,             (void **)&sai_next_hop_api);
    sai_api_query(SAI_API_NEXT_HOP_GROUP,       (void **)&sai_next_hop_group_api);
    sai_api_query(SAI_API_ROUTE,                (void **)&sai_route_api);
    sai_api_query(SAI_API_LAG,                  (void **)&sai_lag_api);
    sai_api_query(SAI_API_POLICER,              (void **)&sai_policer_api);
    sai_api_query(SAI_API_TUNNEL,               (void **)&sai_tunnel_api);
    sai_api_query(SAI_API_QUEUE,                (void **)&sai_queue_api);
    sai_api_query(SAI_API_SCHEDULER,            (void **)&sai_scheduler_api);
    sai_api_query(SAI_API_WRED,                 (void **)&sai_wred_api);
    sai_api_query(SAI_API_QOS_MAP,              (void **)&sai_qos_map_api);
    sai_api_query(SAI_API_BUFFER,               (void **)&sai_buffer_api);
    sai_api_query(SAI_API_SCHEDULER_GROUP,      (void **)&sai_scheduler_group_api);
    sai_api_query(SAI_API_ACL,                  (void **)&sai_acl_api);

    sai_log_set(SAI_API_SWITCH,                 SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_BRIDGE,                 SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_VIRTUAL_ROUTER,         SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_PORT,                   SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_FDB,                    SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_VLAN,                   SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_HOSTIF,                 SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_MIRROR,                 SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_ROUTER_INTERFACE,       SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_NEIGHBOR,               SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_NEXT_HOP,               SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_NEXT_HOP_GROUP,         SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_ROUTE,                  SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_LAG,                    SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_POLICER,                SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_TUNNEL,                 SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_QUEUE,                  SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_SCHEDULER,              SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_WRED,                   SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_QOS_MAP,                SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_BUFFER,                 SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_SCHEDULER_GROUP,        SAI_LOG_LEVEL_NOTICE);
    sai_log_set(SAI_API_ACL,                    SAI_LOG_LEVEL_NOTICE);
}

int main(int argc, char **argv)
{
    /*
     * We want to log orch agent main entry in syslog to distinguish orchagent
     * restarts.
     */
    swss::Logger::getInstance().setMinPrio(swss::Logger::SWSS_DEBUG);

    SWSS_LOG_ENTER();

    swss::Logger::getInstance().setMinPrio(swss::Logger::SWSS_NOTICE);

    // swss::Logger::linkToDbNative("orchagent");

    int opt;
    sai_status_t status;

    bool disableRecord = false;

    while ((opt = getopt(argc, argv, "m:hR")) != -1)
    {
        switch (opt)
        {
        case 'R':
            disableRecord = true;
            break;
        case 'm':
            gMacAddress = MacAddress(optarg);
            break;
        case 'h':
            exit(EXIT_SUCCESS);
        default: /* '?' */
            exit(EXIT_FAILURE);
        }
    }

    SWSS_LOG_NOTICE("--- Starting Orchestration Agent ---");

    initSaiApi();

    SWSS_LOG_NOTICE("sai_switch_api: create a switch");

    vector<sai_attribute_t> switch_attrs;

    sai_attribute_t switch_attr;
    switch_attr.id = SAI_SWITCH_ATTR_INIT_SWITCH;
    switch_attr.value.booldata = true;
    switch_attrs.push_back(switch_attr);

    switch_attr.id = SAI_SWITCH_ATTR_FDB_EVENT_NOTIFY;
    switch_attr.value.ptr = (void *)on_fdb_event;
    switch_attrs.push_back(switch_attr);

    switch_attr.id = SAI_SWITCH_ATTR_PORT_STATE_CHANGE_NOTIFY;
    switch_attr.value.ptr = (void *)on_port_state_change;
    switch_attrs.push_back(switch_attr);

    switch_attr.id = SAI_SWITCH_ATTR_SHUTDOWN_REQUEST_NOTIFY;
    switch_attr.value.ptr = (void *)on_switch_shutdown_request;
    switch_attrs.push_back(switch_attr);

    SWSS_LOG_NOTICE("Enabling sairedis recording");

    sai_attribute_t attr;

    /*
     * NOTE: Notice that all redis attributes here are using SAI_NULL_OBJECT_ID
     * as switch id, because thsoe operations don't require actual switch to be
     * performed, and they should be executed before creating switch.
     */
    {
        attr.id = SAI_REDIS_SWITCH_ATTR_RECORD;
        attr.value.booldata = !disableRecord;

        status = sai_switch_api->set_switch_attribute(gSwitchId, &attr);

        if (status != SAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR("Failed to enable recording %d", status);
            exit(EXIT_FAILURE);
        }

        SWSS_LOG_NOTICE("Notify syncd INIT_VIEW");

        attr.id = SAI_REDIS_SWITCH_ATTR_NOTIFY_SYNCD;
        attr.value.s32 = SAI_REDIS_NOTIFY_SYNCD_INIT_VIEW;
        status = sai_switch_api->set_switch_attribute(gSwitchId, &attr);

        if (status != SAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR("Failed to notify syncd INIT_VIEW %d", status);
            exit(EXIT_FAILURE);
        }

        SWSS_LOG_NOTICE("Enable redis pipeline");

        attr.id = SAI_REDIS_SWITCH_ATTR_USE_PIPELINE;
        attr.value.booldata = true;

        sai_switch_api->set_switch_attribute(gSwitchId, &attr);
        if (status != SAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR("Failed to enable redis pipeline %d", status);
            exit(EXIT_FAILURE);
        }
    }

    status = sai_switch_api->create_switch(&gSwitchId, switch_attrs.size(), switch_attrs.data());
    if (status != SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_ERROR("Failed to create a switch %d", status);
        exit(EXIT_FAILURE);
    }

    attr.id = SAI_SWITCH_ATTR_SRC_MAC_ADDRESS;
    if (!gMacAddress)
    {
        status = sai_switch_api->get_switch_attribute(gSwitchId, 1, &attr);
        if (status != SAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR("Failed to get MAC address from switch %d", status);
            exit(EXIT_FAILURE);
        }
        else
        {
            gMacAddress = attr.value.mac;
        }
    }
    else
    {
        memcpy(attr.value.mac, gMacAddress.getMac(), 6);
        status = sai_switch_api->set_switch_attribute(gSwitchId, &attr);
        if (status != SAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR("Failed to set MAC address to switch %d", status);
            exit(EXIT_FAILURE);
        }
    }

    /* Get the default virtual router ID */
    attr.id = SAI_SWITCH_ATTR_DEFAULT_VIRTUAL_ROUTER_ID;
    status = sai_switch_api->get_switch_attribute(gSwitchId, 1, &attr);
    if (status != SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_ERROR("Fail to get switch virtual router ID %d", status);
        exit(EXIT_FAILURE);
    }

    gVirtualRouterId = attr.value.oid;
    SWSS_LOG_NOTICE("Get switch virtual router ID %lx", gVirtualRouterId);

    /* Create a loopback underlay router interface */
    vector<sai_attribute_t> underlay_intf_attrs;

    sai_attribute_t underlay_intf_attr;
    underlay_intf_attr.id = SAI_ROUTER_INTERFACE_ATTR_VIRTUAL_ROUTER_ID;
    underlay_intf_attr.value.oid = gVirtualRouterId;
    underlay_intf_attrs.push_back(underlay_intf_attr);

    underlay_intf_attr.id = SAI_ROUTER_INTERFACE_ATTR_TYPE;
    underlay_intf_attr.value.s32 = SAI_ROUTER_INTERFACE_TYPE_LOOPBACK;
    underlay_intf_attrs.push_back(underlay_intf_attr);

    status = sai_router_intfs_api->create_router_interface(&gUnderlayIfId, gSwitchId, underlay_intf_attrs.size(), underlay_intf_attrs.data());
    if (status != SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_ERROR("Failed to create underlay router interface %d", status);
        return false;
    }

    SWSS_LOG_NOTICE("Created underlay router interface ID %lx", gUnderlayIfId);

    /* Initialize orchestration components */
    DBConnector *appl_db = new DBConnector(APPL_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);
    OrchDaemon *orchDaemon = new OrchDaemon(appl_db);
    if (!orchDaemon->init())
    {
        SWSS_LOG_ERROR("Failed to initialize orchstration daemon");
        exit(EXIT_FAILURE);
    }

    try
    {
        SWSS_LOG_NOTICE("Notify syncd APPLY_VIEW");

        attr.id = SAI_REDIS_SWITCH_ATTR_NOTIFY_SYNCD;
        attr.value.s32 = SAI_REDIS_NOTIFY_SYNCD_APPLY_VIEW;
        status = sai_switch_api->set_switch_attribute(gSwitchId, &attr);

        if (status != SAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR("Failed to notify syncd APPLY_VIEW %d", status);
            exit(EXIT_FAILURE);
        }

        orchDaemon->start();
    }
    catch (char const *e)
    {
        SWSS_LOG_ERROR("Exception: %s", e);
    }
    catch (exception& e)
    {
        SWSS_LOG_ERROR("Failed due to exception: %s", e.what());
    }

    return 0;
}
