#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <log.h>
#include <net.h>

#undef SUPPORT_AF_INET6

int _net_domains[] = { AF_INET
    #ifdef SUPPORT_AF_INET6
    ,AF_INET6
#endif
}    ;

/**************************************************************************
 name	: deinit_socket
 purpose	: closing a socket (it is probably a global accessible socket)
 input	: none
 output	: none
 ***************************************************************************/
void deinit_socket(int *sock_ptr)
{
	if((NULL != sock_ptr) && (0 < (*sock_ptr))) {
		shutdown(*sock_ptr,  SHUT_RDWR);
		if(0 > close(*sock_ptr)) {
			ERRL(logi, "close(sock) FAILED:(%s)\n", strerror(errno));
		}
		*sock_ptr = -1;
	}
}
/*
 * Functions that are related to gateway finding mechanism
 */
/**************************************************************************
 name	: readNlSock
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)
{
    struct nlmsghdr *nlHdr;
    int readLen = 0, msgLen = 0;

    do {
        /* Recieve response from the kernel */
        if ((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0) {
            perror("SOCK READ: ");
            return -1;
        }

        nlHdr = (struct nlmsghdr *)bufPtr;

        /* Check if the header is valid */
        if ((NLMSG_OK(nlHdr, readLen) == 0)
            || (nlHdr->nlmsg_type == NLMSG_ERROR)) {
            perror("Error in recieved packet");
            return -1;
        }

        /* Check if the its the last message */
        if (nlHdr->nlmsg_type == NLMSG_DONE) {
            break;
        } else {
            /* Else move the pointer to buffer appropriately */
            bufPtr += readLen;
            msgLen += readLen;
        }

        /* Check if its a multi part message */
        if ((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0) {
            /* return if its not */
            break;
        }
    } while ((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));
    return msgLen;
}

/**************************************************************************
 name	: print_route2
 purpose	: For printing the routes.
 input	:
 output	: none
 *************************************************************************/
void print_route2(struct route_info *rtInfo)
{
    char tempBuf[512];

    /* Print Destination address */
    if (rtInfo->dstAddr.s_addr != 0)
        strcpy(tempBuf, inet_ntoa(rtInfo->dstAddr));
    else
        sprintf(tempBuf, "*.*.*.*\t");
    fprintf(stdout, "%s\t", tempBuf);

    /* Print Gateway address */
    if (rtInfo->gateWay.s_addr != 0)
        strcpy(tempBuf, (char *)inet_ntoa(rtInfo->gateWay));
    else
        sprintf(tempBuf, "*.*.*.*\t");
    fprintf(stdout, "%s\t", tempBuf);

    /* Print Interface Name*/
    fprintf(stdout, "%s\t", rtInfo->ifName);

    /* Print Source address */
    if (rtInfo->srcAddr.s_addr != 0)
        strcpy(tempBuf, inet_ntoa(rtInfo->srcAddr));
    else
        sprintf(tempBuf, "*.*.*.*\t");
    fprintf(stdout, "%s\n", tempBuf);
}

/**************************************************************************
 name	: parse_routes
 purpose	: For parsing the route info returned
 input	:
 output	: none
 *************************************************************************/
void parse_routes(int8_t *iface, int8_t *gateway, struct nlmsghdr *nlHdr,
    struct route_info *rtInfo)
{
    struct rtmsg *rtMsg;
    struct rtattr *rtAttr;
    int rtLen;

    rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);
    TRACEL(logi, "called parse_routes func\n");
    /* If the route is not for AF_INET or does not belong to main routing table
     then return. */
    if ((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
        return;

    /* get the rtattr field */
    rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
    rtLen = RTM_PAYLOAD(nlHdr);
    for (; RTA_OK(rtAttr, rtLen); rtAttr = RTA_NEXT(rtAttr, rtLen)) {
        switch (rtAttr->rta_type) {
        case RTA_OIF:
            if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
            break;
        case RTA_GATEWAY:
            rtInfo->gateWay.s_addr = *(u_int *)RTA_DATA(rtAttr);
            break;
        case RTA_PREFSRC:
            rtInfo->srcAddr.s_addr = *(u_int *)RTA_DATA(rtAttr);
            break;
        case RTA_DST:
            rtInfo->dstAddr.s_addr = *(u_int *)RTA_DATA(rtAttr);
            break;
        }
    }
    //printf("%s\n", inet_ntoa(rtInfo->dstAddr));

    DEBUGL(logi, "interface is %s param:%s\n", rtInfo->ifName, iface);
    if ((rtInfo->dstAddr.s_addr == 0) && (strcmp(rtInfo->ifName, iface) == 0)) {
        DEBUGL(logi, "gw: %s\n", (char *)inet_ntoa(rtInfo->gateWay));
        sprintf(gateway, "%s", (const char *)inet_ntoa(rtInfo->gateWay));
    }
    //printRoute(rtInfo);

    return;
}

/**************************************************************************
 name	: get_gateway
 purpose	: return gateway of internet interface
 input	: interface
 ip
 output	: if sucess return 0 else -1
 *************************************************************************/
int8_t get_gateway(int8_t *iface, int8_t *gateway)
{
    struct nlmsghdr *nlMsg;
    struct rtmsg *rtMsg;
    struct route_info *rtInfo;
    char msgBuf[BUFSIZE];
    uint8_t *gateway_p = NULL;  // RT

    int sock, len, msgSeq = 0;

    /* Create Socket */
    if ((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
        ERRL(logi, "Socket Creation:(%s)\n", strerror(errno));
        return 0;
    }

    memset(msgBuf, 0, BUFSIZE);

    /* point the header and the msg structure pointers into the buffer */
    nlMsg = (struct nlmsghdr *)msgBuf;
    rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);

    /* Fill in the nlmsg header*/
    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
    nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .

    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
    nlMsg->nlmsg_seq = msgSeq++;    // Sequence of the message packet.
    nlMsg->nlmsg_pid = getpid();    // PID of process sending the request.

    /* Send the request */
    if (send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0) {
        close(sock);
        return 0;
    }

    /* Read the response */
    if ((len = readNlSock(sock, msgBuf, msgSeq, getpid())) < 0) {
        close(sock);
        return 0;
    }
    /* Parse and print the response */
    rtInfo = (struct route_info *)calloc(1, sizeof(struct route_info));
    //fprintf(stdout, "Destination\tGateway\tInterface\tSource\n");
    for (; NLMSG_OK(nlMsg, len); nlMsg = NLMSG_NEXT(nlMsg, len)) {
        memset(rtInfo, 0, sizeof(struct route_info));
        parse_routes(iface, gateway, nlMsg, rtInfo);
    }
    free(rtInfo);
    close(sock);

    DEBUGL(logi, "gateway: %s\n", gateway);

    return 1;
}

/**************************************************************************
 name	: get_ip_address
 purpose	: return ip address of internet interface
 input	: interface
 ip
 output	: if sucess return 0 else -1
 *************************************************************************/
int8_t get_ip_address(int8_t *interface, int8_t *ip)
{
    int32_t s;
    struct ifconf ifconf;
    struct ifreq ifr[50];
    int32_t ifs;
    int32_t i, j;
    for (i = 0; i < sizeof(_net_domains) / sizeof(_net_domains[0]); i++) {
        s = socket(_net_domains[i], SOCK_STREAM, 0);
        if (s < 0) {
            ERRL(logi, "socket error:(%s)\n", strerror(errno));
            return -1;
        }

        ifconf.ifc_buf = (int8_t *)ifr;
        ifconf.ifc_len = sizeof(ifr);

        if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) {
            ERRL(logi, "ioctl error:(%s)\n", strerror(errno));
            close(s);
            return -1;
        }

        ifs = ifconf.ifc_len / sizeof(ifr[0]);
        for (j = 0; j < ifs; j++) {
            int8_t local_ip[INET_ADDRSTRLEN];
            struct sockaddr_in *s_in = (struct sockaddr_in *)&ifr[j].ifr_addr;

            if (!inet_ntop(_net_domains[i], &s_in->sin_addr, local_ip,
                sizeof(local_ip))) {
                ERRL(logi, "inet_ntop error:(%s)\n", strerror(errno));
                close(s);
                return -1;
            }

            if (compare_strings(interface, ifr[j].ifr_name,
                strlen(interface))) {
                memcpy(ip, local_ip, strlen(local_ip));
                trim(ip);
                close(s);
                return 0;
            }
        }
    }
    close(s);
    return -1;
}

/**************************************************************************
 name	: get_interface_name
 purpose	: return interface of ip address
 input	: ip
 interface
 output	: if sucess return 0 else -1
 *************************************************************************/
int8_t get_interface_name(int8_t *ip, int8_t *interface)
{
    int32_t s;
    struct ifconf ifconf;
    struct ifreq ifr[50];
    int32_t ifs;
    int32_t i, j;
    for (i = 0; i < sizeof(_net_domains) / sizeof(_net_domains[0]); i++) {
        s = socket(_net_domains[i], SOCK_STREAM, 0);
        if (s < 0) {
            ERRL(logi, "socket error\n");
            return 0;
        }

        ifconf.ifc_buf = (int8_t *)ifr;
        ifconf.ifc_len = sizeof(ifr);

        if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) {
            ERRL(logi, "ioctl error\n");
            close(s);
            return -1;
        }

        ifs = ifconf.ifc_len / sizeof(ifr[0]);
        for (j = 0; j < ifs; j++) {
            int8_t local_ip[INET_ADDRSTRLEN];
            struct sockaddr_in *s_in = (struct sockaddr_in *)&ifr[j].ifr_addr;

            if (!inet_ntop(_net_domains[i], &s_in->sin_addr, local_ip,
                sizeof(local_ip))) {
                ERRL(logi, "inet_ntop error\n");
                close(s);
                return -1;
            }
            //INFOL(logi, "ip: %s %s %s %d\n", ip, local_ip, ifr[j].ifr_name, strlen(ifr[j].ifr_name));
            if (compare_strings(ip, local_ip, strlen(ip))) {
                memcpy(interface, ifr[j].ifr_name, strlen(ifr[j].ifr_name));
                close(s);
                return 0;
            }
        }
    }
    close(s);
    return -1;
}

/**************************************************************************
 name	: get_mac_from_interface
 purpose	: return mac address of interface
 input	: usb
 mac
 output	: if sucess return 0 else -1
 *************************************************************************/
void get_mac_from_interface(const int8_t *usb, int8_t *mac)
{
    int32_t s;
    struct ifreq buffer;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    memset(&buffer, 0x00, sizeof(buffer));
    strcpy(buffer.ifr_name, usb);
    ioctl(s, SIOCGIFHWADDR, &buffer);
    close(s);
    memcpy(mac, buffer.ifr_hwaddr.sa_data, 6);
}

/**************************************************************************
 name	: get_interface_from_mac
 purpose	: return network interface from mac
 input	: mac
 usb
 output	: if found usb return 1 else return 0
 *************************************************************************/
uint8_t get_interface_from_mac(const int8_t *mac, const int8_t *usb)
{
#if 0
    uint8_t i;
    int32_t s;
    uint8_t interface[6]= {0};
    struct ifreq buffer;

    for (i=0; i<6; i++) {
        s = socket(PF_INET, SOCK_DGRAM, 0);
        memset(&buffer, 0x00, sizeof(buffer));
        memset(interface, 0, 5);
        snprintf(interface, 5, "usb%d\0",i);
        strcpy(buffer.ifr_name, interface);
        ioctl(s, SIOCGIFHWADDR, &buffer);
        close(s);
        if (compare_strings(mac, buffer.ifr_hwaddr.sa_data, 6)) {
            memcpy(usb, buffer.ifr_hwaddr.sa_data, 6);
            return 1;
        }
    }
#endif
    return 0;
}

/**************************************************************************
 name	: list_ip_address
 purpose	: list all ip address and interface of local
 input	: none
 output	: none
 ***************************************************************************/
void list_ip_address(void)
{
    int32_t s;
    struct ifconf ifconf;
    struct ifreq ifr[50];
    int32_t ifs;
    int32_t i, j;
    for (i = 0; i < sizeof(_net_domains) / sizeof(_net_domains[0]); i++) {
        s = socket(_net_domains[i], SOCK_STREAM, 0);
        if (s < 0) {
            ERRL(logi, "socket error\n");
            return;
        }

        ifconf.ifc_buf = (int8_t *)ifr;
        ifconf.ifc_len = sizeof(ifr);

        if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) {
            ERRL(logi, "ioctl error\n");
            close(s);
            return;
        }

        ifs = ifconf.ifc_len / sizeof(ifr[0]);
        DEBUGL(logi, "interface count:%d\n", ifs);
        for (j = 0; j < ifs; j++) {
            int8_t local_ip[INET_ADDRSTRLEN];
            struct sockaddr_in *s_in = (struct sockaddr_in *)&ifr[j].ifr_addr;

            if (!inet_ntop(_net_domains[i], &s_in->sin_addr, local_ip,
                sizeof(local_ip))) {
                ERRL(logi, "inet_ntop error\n");
                close(s);
                return;
            }
            DEBUGL(logi, "%s - %s\n", ifr[i].ifr_name, local_ip);
        }
    }
    close(s);
}

/**************************************************************************
 name	: get_netmask
 purpose	: It return netmask of the network interface
 output	: none
 *************************************************************************/
int8_t get_netmask(int8_t *iface, int8_t *netmask)
{
    int fd;
    struct ifreq ifr;

    if (netmask == NULL) {
    	ERRL(logi, "NULL PARAMETER\n");
        return 0;
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(0 > fd) {
    	ERRL(logi, "Socket Creation FAILED:(%s)\n", strerror(errno));
    	return 0;
    }

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want netmask address attached to iface */
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);

    //get the netmask ip
    ioctl(fd, SIOCGIFNETMASK, &ifr);

    close(fd);

    /* display & return the result */
    DEBUGL(logi, "getting dhcp netmask address...\n");
    DEBUGL(logi, "%s\n",
        inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    memcpy(netmask,
        (uint8_t *)(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr)),
        sizeof(uint8_t) * IP_LENGTH);

    return 1;
}

/**************************************************************************
 name	: run_udhcpc
 purpose	:
 input	: none
 output	: none
 ***************************************************************************/
void run_udhcpc(uint8_t *iface)
{
    uint8_t i, j;
    uint8_t line[10] = { 0 };
    uint8_t lock_file[50] = { 0 };
    uint8_t command[150] = { 0 };
    uint8_t exist = 0;
    uint32_t pid;
    uint8_t *ptr;

    snprintf(lock_file, 50, "/var/run/connman/udhcpc.%s.pid\0", iface);
    if (file_exist(lock_file) == FILE_EXIST) {
        FILE *file = fopen(lock_file, "r");
        ptr = fgets(line, 10, file);
        pid = atoi(line);
        fclose(file);
        if (pid) {
            if (kill(pid, SIGTERM) < 0) {
                ERRL(logi, "udhcpc pid not killed(sig:%u, pid:%d):(%s)\n", SIGTERM, pid, strerror(errno));
                if (kill(pid, SIGKILL) < 0) {
                    ERRL(logi, "udhcpc pid not killed(sig:%u, pid:%d):(%s)\n", SIGKILL, pid, strerror(errno));
                }
            }
        }
    }

    memset(command, 0, 150);
    snprintf(command, 150,
        "/sbin/udhcpc -b -i %s -p /var/run/connman/udhcpc.%s.pid -T 1 -A 10",
        iface, iface);
    system(command);
}

/**************************************************************************
 name	: send_tcp_socket
 purpose	:
 input	: none
 output	: none
 *************************************************************************/
int32_t send_tcp_socket(int32_t sock, uint8_t *buf, int32_t len)
{
    int32_t total = 0;
    int32_t left = len;
    int32_t i;

    while ((total < len)) {
        i = send(sock, buf + total, left, /*MSG_NOSIGNAL*/0);
        if (i == -1)
            break;
        total += i;
        left -= i;
    }

    return i == -1 ? -1 : 0; // return -1 on failure, 0 on success
}
