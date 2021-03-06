/* 
 * fping: fast-ping, file-ping, favorite-ping, funky-ping
 *
 *   Ping a list of target hosts in a round robin fashion.
 *   A better ping overall.
 *
 * fping website:  http://www.fping.org
 *
 * Current maintainer of fping: David Schweikert
 * Please send suggestions and patches to: david@schweikert.ch
 *
 *
 * Original author:  Roland Schemers  <schemers@stanford.edu>
 * IPv6 Support:     Jeroen Massar    <jeroen@unfix.org / jeroen@ipng.nl>
 * Improved main loop: David Schweikert <david@schweikert.ch>
 * Debian Merge, TOS settings: Tobi Oetiker <tobi@oetiker.ch>
 * Bugfixes, byte order & senseful seq.-numbers: Stephan Fuhrmann (stephan.fuhrmann AT 1und1.de)
 *
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Stanford University.  The name of the University may not be used 
 * to endorse or promote products derived from this software without 
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "fping.h"
#include "config.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/icmp6.h>

int open_ping_socket(void) /* {{{ */
{
	struct protoent *proto;
	int s;

	/* confirm that ICMP is available on this machine */
	if ((proto = getprotobyname("ipv6-icmp")) == NULL)
		crash_and_burn("icmp: unknown protocol");

	/* create raw socket for ICMP calls (ping) */
	if ((s = socket(AF_INET6, SOCK_RAW, proto->p_proto)) < 0)
		/* try non-privileged icmp (works on Mac OSX without privileges, for example) */
		if ((s = socket(AF_INET6, SOCK_DGRAM, proto->p_proto)) < 0)
			errno_crash_and_burn("can't create raw socket (must run as root?)");

	return s;
}
/* }}} */

void init_ping_buffer(size_t ping_data_size) /* {{{ */
{
	/* allocate ping buffer */
	ping_pkt_size = ping_data_size + sizeof(struct icmp6_hdr);
	if (!(ping_buffer = (char *) calloc(1, ping_pkt_size)))
		crash_and_burn("can't malloc ping packet");
}
/* }}} */

void socket_set_src_addr(int s, FPING_INADDR src_addr) /* {{{ */
{
	struct sockaddr_in6 sa;
	memset( &sa, 0, sizeof( sa ) );
	sa.sin6_family = AF_INET6;
	sa.sin6_addr = src_addr;

	if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0)
		errno_crash_and_burn("cannot bind source address");
}
/* }}} */

int socket_sendto_ping(int s, struct sockaddr *saddr, socklen_t saddr_len, uint16_t icmp_seq_nr, uint16_t icmp_id_nr) /* {{{ */
{
	struct icmp6_hdr *icp;
	int n;

	icp = (struct icmp6_hdr *) ping_buffer;
	icp->icmp6_type  = ICMP6_ECHO_REQUEST;
	icp->icmp6_code  = 0;
	icp->icmp6_seq   = htons(icmp_seq_nr);
	icp->icmp6_id    = htons(icmp_id_nr);

	if (random_data_flag)
		for (n = sizeof(struct icmp6_hdr); n < ping_pkt_size; ++n)
			ping_buffer[n] = random() & 0xFF;

	icp->icmp6_cksum = 0; /* The IPv6 stack calculates the checksum for us... */

	n = sendto(s, icp, ping_pkt_size, 0, saddr, saddr_len);

	return n;
}
/* }}} */

/* vi:syntax=c
 */
