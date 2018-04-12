#include <stdio.h>
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/dhcp.h"
#include "ethernetif.h"
#include "netconf.h"
#include "tcpip.h"
#include "main.h"


typedef enum {
    DHCP_START = 0,
    DHCP_WAIT_ADDRESS,
    DHCP_ADDRESS_ASSIGNED,
    DHCP_TIMEOUT
} DHCP_State_TypeDef;

#define MAX_DHCP_TRIES 5

static struct netif xnetif;		/* network interface structure */

struct ip_addr my_ipaddr;



/**
  * @brief  Initializes the lwIP stack
  * @param  None
  * @retval None
  */
void lwip_init_all(void)
{
    struct ip_addr ipaddr;
    struct ip_addr netmask;
    struct ip_addr gw;
#ifndef USE_DHCP
    uint8_t iptab[4];
    uint8_t iptxt[20];
#endif
    /* Create tcp_ip stack thread */
    tcpip_init(NULL, NULL);

    /* IP address setting & display on STM32_evalboard LCD */
#ifdef USE_DHCP
    ipaddr.addr = 0;
    netmask.addr = 0;
    gw.addr = 0;
#else
    IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
    IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
    IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);

    iptab[0] = IP_ADDR3;
    iptab[1] = IP_ADDR2;
    iptab[2] = IP_ADDR1;
    iptab[3] = IP_ADDR0;

    printf("Static IP address: %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]);
#endif

    /* - netif_add(struct netif *netif, struct ip_addr *ipaddr,
       struct ip_addr *netmask, struct ip_addr *gw,
       void *state, err_t (* init)(struct netif *netif),
       err_t (* input)(struct pbuf *p, struct netif *netif))

       Adds your network interface to the netif_list. Allocate a struct
       netif and pass a pointer to this structure as the first argument.
       Give pointers to cleared ip_addr structures when using DHCP,
       or fill them with sane numbers otherwise. The state pointer may be NULL.

       The init function pointer must point to a initialization function for
       your ethernet netif interface. The following code illustrates it's use. */
    netif_add(&xnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

    /*  Registers the default network interface. */
    netif_set_default(&xnetif);

    /*  When the netif is fully configured this function must be called. */
    netif_set_up(&xnetif);
}

#ifdef USE_DHCP
/**
  * @brief  LwIP_DHCP_Process_Handle
  * @param  None
  * @retval None
  */
void lwip_dhcp_task(void *p)
{
    struct ip_addr netmask;
    struct ip_addr gw;
    uint32_t IPaddress;
    uint8_t iptab[4];
    uint8_t DHCP_state;
    DHCP_state = DHCP_START;

    for (;;) {
	switch (DHCP_state) {
	case DHCP_START:
	    {
		dhcp_start(&xnetif);
		IPaddress = 0;
		DHCP_state = DHCP_WAIT_ADDRESS;
		printf("Looking for  DHCP server\r\n");
		printf("please wait...\r\n");
	    }
	    break;

	case DHCP_WAIT_ADDRESS:
	    {
		/* Read the new IP address */
		IPaddress = xnetif.ip_addr.addr;

		if (IPaddress != 0) {
		    DHCP_state = DHCP_ADDRESS_ASSIGNED;

		    /* Stop DHCP */
		    dhcp_stop(&xnetif);
                    memcpy(&my_ipaddr, &xnetif.ip_addr, sizeof(struct ip_addr));


		    iptab[0] = (uint8_t) (IPaddress >> 24);
		    iptab[1] = (uint8_t) (IPaddress >> 16);
		    iptab[2] = (uint8_t) (IPaddress >> 8);
		    iptab[3] = (uint8_t) (IPaddress);
		    printf("IP address assigned\nby a DHCP server: %d.%d.%d.%d\r\n", iptab[3], iptab[2], iptab[1], iptab[0]);
		    vTaskDelete(NULL);
		} else {
		    /* DHCP timeout */
		    if (xnetif.dhcp->tries > MAX_DHCP_TRIES) {
			DHCP_state = DHCP_TIMEOUT;

			/* Stop DHCP */
			dhcp_stop(&xnetif);

			/* Static address used */
			IP4_ADDR(&my_ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
			IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
			IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
			netif_set_addr(&xnetif, &my_ipaddr, &netmask, &gw);


			printf("DHCP timeout\r\n");

			iptab[0] = IP_ADDR3;
			iptab[1] = IP_ADDR2;
			iptab[2] = IP_ADDR1;
			iptab[3] = IP_ADDR0;

			printf("Static IP address: %d.%d.%d.%d\r\n", iptab[3], iptab[2], iptab[1], iptab[0]);
                        memcpy(&my_ipaddr, &xnetif.ip_addr, sizeof(struct ip_addr));

			vTaskDelete(NULL);
		    }
		}
	    }
	    break;

	default:
	    break;
	}
	/* wait 250 ms */
	vTaskDelay(250);
    }
}
#endif				/* USE_DHCP */
