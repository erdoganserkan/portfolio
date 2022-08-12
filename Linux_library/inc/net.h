#ifndef NET_H_
#define NET_H_

#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdint.h>

// Related to mechanism that finds the gateway
// http://stackoverflow.com/questions/3288065/getting-gateway-to-use-for-a-given-ip-in-ansi-c
#define BUFSIZE 8192
char _gateway[255];

struct route_info
{
    struct in_addr dstAddr;
    struct in_addr srcAddr;
    struct in_addr gateWay;
    char ifName[IF_NAMESIZE];
};

extern int8_t get_ip_address(int8_t *interface, int8_t *ip);
extern int8_t get_interface_name(int8_t *ip, int8_t *interface);

// no need to use extern for these 4
extern void printGateway(void);
extern void parse_routes(int8_t *iface, int8_t *gateway, struct nlmsghdr *nlHdr,
    struct route_info *rtInfo);
extern int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId);
extern void print_route2(struct route_info *rtInfo);

void get_mac_from_interface(const int8_t *usb, int8_t *mac);
uint8_t get_interface_from_mac(const int8_t *mac, const int8_t *usb);
extern void list_ip_address(void);
extern int8_t get_netmask(int8_t *iface, int8_t *netmask);
extern int8_t get_gateway(int8_t *iface, int8_t *gateway);
extern void run_udhcpc(uint8_t *iface);
extern int32_t send_tcp_socket(int32_t sock, uint8_t *buf, int32_t len);
void deinit_socket(int *sock_ptr);

#endif
