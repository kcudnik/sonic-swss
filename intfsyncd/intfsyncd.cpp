#include <iostream>
#include "logger.h"
#include "select.h"
#include "netdispatcher.h"
#include "netlink.h"
#include "intfsyncd/intfsync.h"
#include "selectableevent.h"

#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include "linkcache.h"

//  while :; do for a in up down; do ip netns exec sw0srv0 ip link set $a dev sw0srv0eth0; echo $a; sleep 3; done; done
//
using namespace std;
using namespace swss;

class IfSync : public NetMsg
{
public:
    enum { MAX_ADDR_SIZE = 64 };

    virtual void onMsg(int nlmsg_type, struct nl_object *obj)
    {
        printf("on message\n");
        char addrStr[MAX_ADDR_SIZE + 1] = {0};
        struct rtnl_addr *addr = (struct rtnl_addr *)obj;
        string key;
        string scope = "global";
        string family;

        if (nlmsg_type == RTM_NEWLINK)
        {
            printf("new link\n");

            struct rtnl_link *link = (struct rtnl_link *)obj;

            int idx = rtnl_link_get_ifindex(link);

            printf("ifx %d\n", idx);

            unsigned int flags = rtnl_link_get_flags(link); // IFF_LOWER_UP and IFF_RUNNING

            printf("flags 0x%x\n", flags);

            const char * name = rtnl_link_get_name(link);

            printf("name %s\n", name);

        }
        
        if (nlmsg_type == RTM_DELLINK)
        {
            printf("del link\n");
        }

        if ((nlmsg_type != RTM_NEWADDR) && (nlmsg_type != RTM_GETADDR) &&
            (nlmsg_type != RTM_DELADDR))
            return;

        /* Don't sync local routes */
        if (rtnl_addr_get_scope(addr) != RT_SCOPE_UNIVERSE)
        {
            scope = "local";
            return;
        }

        if (rtnl_addr_get_family(addr) == AF_INET)
            family = IPV4_NAME;
        else if (rtnl_addr_get_family(addr) == AF_INET6)
            family = IPV6_NAME;
        else
            // Not supported
            return;

        key = LinkCache::getInstance().ifindexToName(rtnl_addr_get_ifindex(addr));
        key+= ":";
        nl_addr2str(rtnl_addr_get_local(addr), addrStr, MAX_ADDR_SIZE);
        key+= addrStr;
        if (nlmsg_type == RTM_DELADDR)
        {
            printf("del %s\n", key.c_str());
            return;
        }

        printf("set %s %s %s\n", key.c_str(), family.c_str(), scope.c_str());

    }
};

int main(int argc, char **argv)
{
    swss::Logger::linkToDbNative("intfsyncd");
   // DBConnector db(APPL_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);
    ///IntfSync sync(&db);
    IfSync sync;

    //NetDispatcher::getInstance().registerMessageHandler(RTM_NEWADDR, &sync);
  //  NetDispatcher::getInstance().registerMessageHandler(RTM_DELADDR, &sync);
    NetDispatcher::getInstance().registerMessageHandler(RTM_NEWLINK, &sync);
    NetDispatcher::getInstance().registerMessageHandler(RTM_DELLINK, &sync);
   // NetDispatcher::getInstance().registerMessageHandler(RTM_GETLINK, &sync);

     swss::SelectableEvent ntf_event;


    while (1)
    {
        try
        {
            NetLink netlink;
            Select s;

         //   netlink.registerGroup(RTNLGRP_IPV4_IFADDR);
          //  netlink.registerGroup(RTNLGRP_IPV6_IFADDR);
            netlink.registerGroup(RTNLGRP_LINK);
            cout << "Listens to interface messages..." << endl;
           netlink.dumpRequest(RTM_GETLINK);
          //  netlink.dumpRequest(RTM_GETADDR);

            s.addSelectable(&netlink);
            s.addSelectable(&ntf_event);

            while (true)
            {
                printf("select\n");

                Selectable *temps = NULL;

                int tempfd;
                int result = s.select(&temps, &tempfd);

                if (result == swss::Select::OBJECT)
                {
                    printf("object\n");

                   // netlink.dumpRequest(RTM_GETLINK);
                }
                else
                {

                    printf("non object %d\n", result);
                }

            }
        }
        catch (const std::exception& e)
        {
            cout << "Exception \"" << e.what() << "\" had been thrown in deamon" << endl;
            return 0;
        }
    }

    return 1;
}
