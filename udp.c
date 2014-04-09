/*!  \brief Gerencia os aplicativos UDP */

#include "udp.h"
#include "dhcpc.h"
#include "resolv.h"

void udp_appcall(void)
{
    if(appIsDhcp())
    {
      dhcpc_appcall();
    }
    resolv_appcall();
}