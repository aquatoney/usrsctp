/*
 * Copyright (C) 2011-2013 Michael Tuexen
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Usage: client remote_addr remote_port [local_port] [local_encaps_port] [remote_encaps_port]
 */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifndef _WIN32
#include <unistd.h>
#include <signal.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <io.h>
#endif
#include <usrsctp.h>
#include "programs_helper.h"
#include "pthread.h"

int done = 0;
unsigned char* payload;
unsigned int payload_len;
unsigned int single_buffer_len;
int ctrl_c = 0;

#ifdef _WIN32
typedef char* caddr_t;
#endif



static int
receive_cb(struct socket *sock, union sctp_sockstore addr, void *data,
           size_t datalen, struct sctp_rcvinfo rcv, int flags, void *ulp_info)
{
	if (data == NULL) {
		done = 1;
		usrsctp_close(sock);
	} else {
		if (flags & MSG_NOTIFICATION) {
			handle_notification((union sctp_notification *)data, datalen);
		} else {
#ifdef _WIN32
			_write(_fileno(stdout), data, (unsigned int)datalen);
#else
			if (write(fileno(stdout), data, datalen) < 0) {
				perror("write");
			}
#endif
		}
		free(data);
	}
	return (1);
}


struct sockaddr_in6 addr6;
struct sockaddr_in addr4;
struct sctp_udpencaps encaps;
struct sctp_event event;

void 
init_sock(int argc, char *argv[])
{
	memset(&event, 0, sizeof(event));
	event.se_assoc_id = SCTP_ALL_ASSOC;
	event.se_on = 1;
	if (argc > 5) {
		memset(&encaps, 0, sizeof(struct sctp_udpencaps));
		encaps.sue_address.ss_family = AF_INET6;
		encaps.sue_port = htons(atoi(argv[5]));
		// if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_REMOTE_UDP_ENCAPS_PORT, (const void*)&encaps, (socklen_t)sizeof(struct sctp_udpencaps)) < 0) {
		// 	perror("setsockopt");
		// }
	}
	memset((void *)&addr4, 0, sizeof(struct sockaddr_in));
	// memset((void *)&addr6, 0, sizeof(struct sockaddr_in6));
#ifdef HAVE_SIN_LEN
	addr4.sin_len = sizeof(struct sockaddr_in);
#endif
// #ifdef HAVE_SIN6_LEN
// 	addr6.sin6_len = sizeof(struct sockaddr_in6);
// #endif
	addr4.sin_family = AF_INET;
	// addr6.sin6_family = AF_INET6;
	addr4.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &addr4.sin_addr);
}

void*
certain_client()
{
	int loop = 0;
	struct socket *sock;
	while (!ctrl_c) {
		printf("creating sock %d\n", loop++);
		if ((sock = usrsctp_socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP, receive_cb, NULL, 0, NULL)) == NULL) {
			perror("usrsctp_socket");
		}
		if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_REMOTE_UDP_ENCAPS_PORT, (const void*)&encaps, (socklen_t)sizeof(struct sctp_udpencaps)) < 0) {
			perror("setsockopt");
		}
		printf("connecting\n");
		if (usrsctp_connect(sock, (struct sockaddr *)&addr4, sizeof(struct sockaddr_in)) < 0) {
			perror("usrsctp_connect");
		}

		printf("connected\n");
		int cursor = 0;
		while (cursor < payload_len) {
			usrsctp_sendv(sock, payload+cursor, single_buffer_len, NULL, 0, NULL, 0, SCTP_SENDV_NOINFO, 0);
			cursor += single_buffer_len;
		}

		if (!done) {
			if (usrsctp_shutdown(sock, SHUT_WR) < 0) {
				perror("usrsctp_shutdown");
			}
		}
		usrsctp_close(sock);
	}
	printf("exiting thread\n");
	pthread_exit(0);
}



void 
ctrl_c_handler(int sig) {
    printf("\nwill shut down (ctrl-c again to kill)\n");
    ctrl_c = 1;
}

int
main(int argc, char *argv[])
{
	signal(SIGINT, ctrl_c_handler);

	single_buffer_len = 1024;
	payload_len = 1024*1024;
	int max_threads = atoi(argv[6]);
	payload = (unsigned char*)malloc(sizeof(unsigned char*)*payload_len);
	pthread_t pid[3000];


	usrsctp_init(atoi(argv[4]), NULL, debug_printf_stack);
	
	usrsctp_sysctl_set_sctp_blackhole(2);
	usrsctp_sysctl_set_sctp_no_csum_on_loopback(0);

	init_sock(argc, argv);

	// while (!ctrl_c) {
	// 	// printf("\nnew client\n");
	// 	ret = certain_client(argc, argv);
	// 	// printf("client done\n");
	// 	if (ret < 0) {
	// 		perror("Ret of client");
	// 		break;
	// 	}
	// }

	int thread = 0;
	while (thread != max_threads) {
		pthread_create(&pid[thread], NULL, certain_client, NULL);
		thread++;
	}
	thread = 0;
	while (thread != max_threads) {
		pthread_join(pid[thread], NULL);
		thread++;
	}

	usrsctp_finish();

	return(0);
}
