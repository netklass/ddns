/* client.c - dynamic DNS client */

/*
 * Copyright (c) 2009, Natacha Porté
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "csexp.h"
#include "log.h"
#include "message.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


/********************
 * OPTION FUNCTIONS *
 ********************/

/* client_options • structure containing all client parameters */
struct client_options {
	char		*host;
	char		*port;
	char		*name;
	size_t		 nsize;
	char		*key;
	size_t		 ksize;
	int		 interval; };

/* DEFAULT_OPT • initializer for struct client_options */
#define DEFAULT_OPT { 0, 0, 0, 0, 0, 0, 0 }


/* free_options • releases internal memory from struct client_options */
static void
free_options(struct client_options *opt) {
	if (!opt) return;
	free(opt->host);
	free(opt->port);
	free(opt->name);
	free(opt->key); }


/* parse_options • fills in a struct client_options from a S-expression */
/*	returns 0 on success, -1 on failure */
static int
parse_options(struct client_options *opt, struct sx_node *sx) {
	struct sx_node *s, *arg;
	struct client_options nopt = DEFAULT_OPT;
	const char *cmd;
	void *neo;

	/* reading value from S-expression */
	for (s = sx; s; s = s->next)
		if (!SX_IS_LIST(s)
		|| (cmd = SX_DATA(SX_CHILD(s))) == 0
		|| (arg = SX_CHILD(s)->next) == 0)
			continue;
		else if (!strcmp(cmd, "server")) {
			if (SX_IS_ATOM(arg)) {
				free(nopt.host);
				nopt.host = strdup(arg->data); }
			if (arg->next && SX_IS_ATOM(arg->next)) {
				free(nopt.port);
				nopt.port = strdup(arg->next->data); } }
		else if (!strcmp(cmd, "name")) {
			if (!SX_IS_ATOM(arg) || !arg->size) continue;
			neo = realloc(nopt.name, arg->size);
			if (!neo) continue;
			nopt.name = neo;
			nopt.nsize = arg->size;
			memcpy(nopt.name, arg->data, arg->size); }
		else if (!strcmp(cmd, "key")) {
			if (!SX_IS_ATOM(arg) || !arg->size) continue;
			neo = realloc(nopt.key, arg->size);
			if (!neo) continue;
			nopt.key = neo;
			nopt.ksize = arg->size;
			memcpy(nopt.key, arg->data, arg->size); }
		else if (!strcmp(cmd, "interval")) {
			if (SX_IS_ATOM(arg) && arg->size)
				nopt.interval = atoi(arg->data); }
		else log_c_bad_cmd(cmd);

	/* conformity checks */
	if (!nopt.host
	||  !nopt.port
	||  !nopt.name || !nopt.nsize
	||  !nopt.key  || !nopt.ksize) {
		free_options(&nopt);
		return -1; }

	/* output of the options */
	free_options(opt);
	*opt = nopt;
	return 0; }



/*********************
 * NETWORK FUNCTIONS *
 *********************/

/* connect_socket • connects the given socket to the parametered server */
/*	returns 0 on success, -1 on failure */
static int
connect_socket(int socket, struct client_options *opt) {
	struct addrinfo hints, *res;
	int ret;

	/* address lookup */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	ret = getaddrinfo(opt->host, opt->port, &hints, &res);
	if (ret) {
		log_c_getaddrinfo(opt->host, opt->port, ret);
		return -1; }
	if (res->ai_next) {
		log_c_ambiguous_addr(opt->host, opt->port, res);
		freeaddrinfo(res);
		return -1; }

	/* socket connection */
	if (connect(socket, res->ai_addr, res->ai_addrlen) < 0) {
		log_c_connect(opt->host, opt->port, res);
		freeaddrinfo(res);
		return -1; }

	/* clean-up */
	freeaddrinfo(res);
	return 0; }


/* send_message • sends the given message over the given socket */
/*	returns 0 on success, -1 on faileure */
static int
send_message(int socket, struct ddns_message *msg, const void *key,
							size_t ksize) {
	unsigned char data[MAX_MSG_SIZE];
	size_t datalen;
	ssize_t ret;

	datalen = encode_message(data, sizeof data, msg, key, ksize);
	if (!datalen) return -1;
	ret = send(socket, data, datalen, 0);
	if (ret < 0) {
		log_c_send_fail(data, datalen);
		return -1; }
	else if ((size_t)ret < datalen) {
		 log_c_send_short(data, datalen, ret);
		return -1; }
	return 0; }



/******************
 * MAIN FUNCTIONS *
 ******************/

/* client_loop • main message sending loop */
/*	returns 0 on success, -1 on faileure */
static int
client_loop(struct client_options *opt) {
	int fd;
	struct ddns_message msg;

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (!fd) {
		log_c_socket();
		return -1; }

	if (connect_socket(fd, opt) < 0) return -1;

	msg.time = time(0);
	msg.name = opt->name;
	msg.namelen = opt->nsize;
	msg.addr[0] = msg.addr[1] = msg.addr[2] = msg.addr[3] = 0;

	if (opt->interval <= 0)
		return send_message(fd, &msg, opt->key,  opt->ksize);

	for (;;) {
		msg.time = time(0);
		if (send_message(fd, &msg, opt->key, opt->ksize) < 0) return -1;
		sleep(opt->interval); }

	return 0; }


/* main */
int
main(void) {
	struct sexp arg;
	struct client_options opt = DEFAULT_OPT;

	if (sxp_file_to_sx(&arg, stdin, 1024, 1024, 64) < 0
	|| arg.nsize <= 0) {
		log_c_no_options();
		return EXIT_FAILURE; }

	if (parse_options(&opt, arg.nodes) < 0
	|| client_loop(&opt) < 0)
		 return EXIT_FAILURE;

	return 0; }

/* vim: set filetype=c: */
