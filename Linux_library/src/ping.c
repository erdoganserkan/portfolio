#include <log.h>
#include <ping.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <string.h>
#include <net/if.h>

/**************************************************************************
 name	: ping
 purpose	: return the time in microsecond that a packet takes to make
 round trip from local host to remote host
 input	: target -> ip address of remote host
 interface -> ethernet interface that will be sent data
 output	: time - > microsecond
 *************************************************************************/
int32_t ping(int8_t *target, int8_t *interface)
{
    int32_t i, len, from_len, ret;
    int32_t sock;
    int32_t packet_length = 0;
    struct ifreq ifr;
    struct sockaddr_in host, from;
    struct ip *ip;
    struct hostent *hp;
    struct icmp *icp;
    uint8_t *packet;
    struct timeval tv;
    struct timeval start, end;
    uint8_t out_pack[MAXPACKET];
    int32_t time_diff;
    fd_set rfds;

    host.sin_family = AF_INET;
    host.sin_addr.s_addr = inet_addr(target);

    if (host.sin_addr.s_addr == -1) {
        hp = gethostbyname(target);
        if (hp == NULL) {
            INFOL(logi, "Ping Unknown Host\n");
            return -1;
        }
        host.sin_family = hp->h_addrtype;
        bcopy(hp->h_addr, (caddr_t) & host.sin_addr, hp->h_length);
    }

    if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        ERRL(logi, "Ping Socket Error\n");
        return -1;
    }

    strncpy(ifr.ifr_name, interface, 16);

    i = setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr));

    if (i) {
        ERRL(logi, "cannot bind to device %s\n", interface);
        close(sock);
        return -1;
    }

    icp = (struct icmp *)out_pack;
    icp->icmp_type = ICMP_ECHO;
    icp->icmp_code = 0;
    icp->icmp_cksum = 0;
    icp->icmp_seq = 12345;
    icp->icmp_id = getpid();
    icp->icmp_cksum = in_cksum((uint16_t *)icp, DEFDATALEN + ICMP_MINLEN);

    gettimeofday(&start, NULL);

    i = sendto(sock, (int8_t *)icp, DEFDATALEN + ICMP_MINLEN, 0,
        (struct sockaddr*)&host, (socklen_t)sizeof(struct sockaddr_in));
    if (i < 0 || i != (DEFDATALEN + ICMP_MINLEN)) {
        if (i < 0) {
            ERRL(logi, "Ping Send Error\n");
            close(sock);
            return -1;
        }
    }

    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    // Wait up to four seconds.
    tv.tv_sec = 4;
    tv.tv_usec = 0;

    packet_length = DEFDATALEN + MAXIPLEN + MAXICMPLEN;
    packet = (uint8_t *)malloc(packet_length);
    if (packet == NULL)
        return -1;

    while (1) {
        if ((ret = select(sock + 1, &rfds, NULL, NULL, &tv)) == -1) {
            ERRL(logi, "Ping select error\n");
            close(sock);
            return -1;
        }

        /*no response for packet*/
        if (ret == 0) {
            ERRL(logi, "No response from remote host %s\n", target);
            close(sock);
            return -1;
        }

        from_len = sizeof(struct sockaddr_in);
        if ((len = recvfrom(sock, (int8_t *)packet, packet_length, 0,
            (struct sockaddr *)&from, (socklen_t*)&from_len)) < 0) {
            ERRL(logi, "Ping receive error\n");
            close(sock);
            return -1;
        }

        ip = (struct ip *)((int8_t *)packet);

        if (len < (sizeof(struct ip) + ICMP_MINLEN)) {
            WARNL(logi, "Ping packet too short %d bytes from %s\n",
                len, target);
            close(sock);
            return -1;
        }

        icp = (struct icmp *)(packet + sizeof(struct ip));
        if (icp->icmp_type == ICMP_ECHOREPLY) {
            if (icp->icmp_seq != 12345)
                continue;
            if (icp->icmp_id != getpid())
                continue;

            gettimeofday(&end, NULL);
            time_diff = 1000000 * (end.tv_sec - start.tv_sec)
                + (end.tv_usec - start.tv_usec);

            break;
        }
    }
    close(sock);
    free(packet);
    return time_diff;
}

/**************************************************************************
 name	: in_cksum
 purpose	: calculate checksum value of icmp packet
 input	: addr -> start address of buffer
 len -> length of buffer
 output	: cksum
 *************************************************************************/
uint16_t in_cksum(uint16_t *addr, uint16_t len)
{
    uint16_t answer = 0;

    uint32_t sum = 0;
    while (len > 1) {
        sum += *addr++;
        len -= 2;
    }

    if (len == 1) {
        *(uint8_t *)&answer = *(uint8_t *)addr;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff); // add high 16 to low 16
    sum += (sum >> 16); // add carry
    answer = ~sum; // truncate to 16 bits
    return answer;
}
