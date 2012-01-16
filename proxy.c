/* proxy.c - Basic SOCKS client implementation
 *
 * Copyright (C) 2012 Oskar Liljeblad
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <winsock2.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "rdpvnclaunch.h"

#define PROXY_BUFSIZE 1024
#define LISTEN_PORT_LOW 20000
#define LISTEN_PORT_HIGH 29999
#define PROXY_LIFETIME_SECONDS 60

static struct sockaddr_in connect_addr;
static struct sockaddr_in proxy_addr;
static SOCKET listen_sock;

static char *wsa_errstr (void)
{
    return system_errstr_error(WSAGetLastError());
}

static int
full_recv (SOCKET fd, void *buf, int count)
{
  int total = 0;
  char *ptr = (char *) buf;
  
  while (count > 0) {
    int n_rw = recv(fd, ptr, count, 0);
    if (n_rw == SOCKET_ERROR)
      break;
    if (n_rw == 0) {
      WSASetLastError(0);
      break;
    }
    total += n_rw;
    ptr += n_rw;
    count -= n_rw;
  }

  return total;
}

static int
full_send (SOCKET fd, const void *buf, int count)
{
  int total = 0;
  const char *ptr = (const char *) buf;
  
  while (count > 0) {
    int n_rw = send(fd, ptr, count, 0);
    if (n_rw == SOCKET_ERROR)
      break;
    if (n_rw == 0) {
      WSASetLastError(WSAENOBUFS);
      break;
    }
    total += n_rw;
    ptr += n_rw;
    count -= n_rw;
  }

  return total;
}

static bool
parse_addr (const wchar_t *wstr, struct in_addr *addr)
{
  char str[16];
  int c;

  for (c = 0; c < 16 && wstr[c]; c++) {
    if (wstr[c] >= L'0' && wstr[c] <= L'9')
      str[c] = '0' + wstr[c] - L'0';
    if (wstr[c] == L'.')
      str[c] = '.';
  }
  if (wstr[c])
    return false;
  str[c] = '\0';
	
  addr->s_addr = inet_addr(str);
  return addr->s_addr != INADDR_NONE;
}

static bool
parse_port (const wchar_t *str, uint16_t *port)
{
  wchar_t *tail;
  long result;
  
  if (*str == '\0')
    return false;
  result = wcstol(str, &tail, 10);
  if (result < 0 || result > UINT16_MAX || *tail != '\0')
    return false;
  *port = htons(result);
  return true;
}

uint16_t
prepare_proxy (const wchar_t *proxy_host, const wchar_t *proxy_port, const wchar_t *connect_host, const wchar_t *connect_port)
{
  struct sockaddr_in listen_addr;
  WSADATA wsadata;

  if (WSAStartup(MAKEWORD(2,2), &wsadata) != 0)
    die("Cannot initialize socket library: %s\n", wsa_errstr());

  if (!parse_addr(proxy_host, &proxy_addr.sin_addr))
    die("Invalid IP address `%ls'\n", proxy_host);
  if (!parse_port(proxy_port, &proxy_addr.sin_port))
    die("Invalid port `%ls'\n", proxy_port);
  proxy_addr.sin_family = AF_INET;
  if (!parse_addr(connect_host, &connect_addr.sin_addr))
    die("Invalid IP address `%ls'\n", connect_host);
  if (!parse_port(connect_port, &connect_addr.sin_port))
    die("Invalid port `%ls'\n", connect_port);

  /*warn("proxy=%ls:%ls connect=%ls:%ls\n", proxy_host, proxy_port, connect_host, connect_port);*/

  listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sock == INVALID_SOCKET)
    die("Cannot create socket: %s\n", wsa_errstr());
  /*BOOL sockopt = TRUE;
  if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char *) &sockopt, sizeof(sockopt)) != 0)
    die("Cannot enable socket reuse: %s\n", wsa_errstr());
  sockopt = TRUE;
  if (setsockopt(listen_sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char *) &sockopt, sizeof(sockopt)) != 0)
    die("Cannot enable socket exclusiveness: %s\n", wsa_errstr());*/
  listen_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  listen_addr.sin_family = AF_INET;

  uint16_t port;
  for (port = LISTEN_PORT_LOW; port <= LISTEN_PORT_HIGH; port++) {
    listen_addr.sin_port = htons(port);
    if (bind(listen_sock, (struct sockaddr *) &listen_addr, sizeof(listen_addr)) == 0)
        break;
    if (WSAGetLastError() != WSAEADDRINUSE)
        die("Cannot bind to address %ls port %d: %s\n", L"127.0.0.1", port, wsa_errstr());
  }
  if (port > LISTEN_PORT_HIGH)
    die("No free port found\n");
  if (listen(listen_sock, 1) != 0)
    die("Cannot listen for connections: %s\n", wsa_errstr());

  return port;
}

void
handle_proxy (void)
{
    char data[PROXY_BUFSIZE];
    int data_len;
    int write_len;
    SOCKET client_sock;
    SOCKET proxy_sock;
    fd_set read_fds;
	int rc;

	/* XXX: handle multiple concurrent connections.
	 * SOCKET client_socks[MAX_CONCURRENT_CONNECTION];
	 * SOCKET proxy_socks[MAX_CONCURRENT_CONNECTION];
	 * use INVALID_SOCKET for unused connections
	 * shut down listen socket when there are no connections and
	 * no new connections have been made in PROXY_LIFETIME_SECONDS seconds.
	 */
	for (;;) {
		struct timeval timeout = { PROXY_LIFETIME_SECONDS, 0 };

		FD_ZERO(&read_fds);
		FD_SET(listen_sock, &read_fds);
		if ((rc = select(0, &read_fds, NULL, NULL, &timeout)) == SOCKET_ERROR)
			die ("Cannot wait for input: %s\n", wsa_errstr());
		if (rc == 0)
			break;

		client_sock = accept(listen_sock, NULL, NULL);
		if (client_sock == INVALID_SOCKET)
			die("Cannot accept connection: %s\n", wsa_errstr());

		proxy_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (proxy_sock == INVALID_SOCKET)
			die("Cannot create socket: %s\n", wsa_errstr());
		if (connect(proxy_sock, (struct sockaddr *) &proxy_addr, sizeof(proxy_addr)) != 0)
			die("Cannot connect to proxy: %s\n", wsa_errstr());

		data[0] = 0x04;
		data[1] = 0x01;
		data[2] = connect_addr.sin_port & 0xFF;
		data[3] = connect_addr.sin_port >> 8;
		data[4] = connect_addr.sin_addr.s_addr & 0xFF;
		data[5] = (connect_addr.sin_addr.s_addr >> 8) & 0xFF;
		data[6] = (connect_addr.sin_addr.s_addr >> 16) & 0xFF;
		data[7] = connect_addr.sin_addr.s_addr >> 24;
		data_len = 8 + strlen(program_name) + 1;
		memcpy(data + 8, program_name, data_len - 8);

		write_len = full_send(proxy_sock, data, data_len);
		if (write_len < 0)
			die("Cannot write to proxy: %s\n", strerror(errno));
		if (write_len < data_len)
			die("Connection to proxy unexpectedly closed\n");

		data_len = full_recv(proxy_sock, data, 8);
		if (data_len < 0)
			die("Cannot read from proxy: %s\n", strerror(errno));
		if (data_len < 8)
			die("Connection to proxy unexpectedly closed\n");
		if (data[0] != 0)
			die("Invalid response from proxy\n");
		if (data[1] != 0x5A)
			die("Proxy actively denied request\n");

		for (;;) {
			FD_ZERO(&read_fds);
			FD_SET(client_sock, &read_fds);
			FD_SET(proxy_sock, &read_fds);

			if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) == SOCKET_ERROR)
				die ("Cannot wait for input: %s\n", wsa_errstr());
			if (FD_ISSET(client_sock, &read_fds)) {
				data_len = recv(client_sock, data, sizeof(data), 0);
				if (data_len == SOCKET_ERROR)
					die("Cannot read from client: %s\n", wsa_errstr());
				if (data_len == 0)
					break;
				write_len = full_send(proxy_sock, data, data_len);
				if (write_len < 0)
					die("Cannot write to proxy: %s\n", strerror(errno));
				if (write_len < data_len)
					die("Connection to proxy unexpectedly closed\n");
			}
			if (FD_ISSET(proxy_sock, &read_fds)) {
				data_len = recv(proxy_sock, data, sizeof(data), 0);
				if (data_len == SOCKET_ERROR)
					die("Cannot read from proxy: %s\n", wsa_errstr());
				if (data_len == 0)
					break;
				write_len = full_send(client_sock, data, data_len);
				if (write_len < 0)
					die("Cannot write to client: %s\n", strerror(errno));
				if (write_len < data_len)
					die("Connection to client unexpectedly closed\n");
			}
		}

		if (closesocket(client_sock) != 0)
			die("Cannot close client connection: %s\n", wsa_errstr());
		if (closesocket(proxy_sock) != 0)
			die("Cannot close proxy connection: %s\n", wsa_errstr());
	}

	if (closesocket(listen_sock) != 0)
		die("Cannot close client connection: %s\n", wsa_errstr());
}
