// Many codes in this file was borrowed from GdbConnection.cc in rr
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <stdbool.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "packets.h"

struct packet_buf
{
    uint8_t buf[PACKET_BUF_SIZE];
    int end;
} in, out;

int sock_fd;

uint8_t *inbuf_get()
{
    return in.buf;
}

int inbuf_end()
{
    return in.end;
}

void pktbuf_insert(struct packet_buf *pkt, const uint8_t *buf, ssize_t len)
{
    if (pkt->end + len >= sizeof(pkt->buf))
    {
        puts("Packet buffer overflow");
        exit(-2);
    }
    memcpy(pkt->buf + pkt->end, buf, len);
    pkt->end += len;
}

void pktbuf_erase_head(struct packet_buf *pkt, ssize_t end)
{
    memmove(pkt->buf, pkt->buf + end, pkt->end - end);
    pkt->end -= end;
}

void inbuf_erase_head(ssize_t end)
{
    pktbuf_erase_head(&in, end);
}

void pktbuf_clear(struct packet_buf *pkt)
{
    pkt->end = 0;
}

static bool poll_socket(int sock_fd, short events)
{
    struct pollfd pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = sock_fd;
    pfd.events = events;

    int ret = poll(&pfd, 1, -1);
    if (ret < 0)
    {
        perror("poll() failed");
        exit(-1);
    }
    return ret > 0;
}

static bool poll_incoming(int sock_fd)
{
    return poll_socket(sock_fd, POLLIN);
}

static void poll_outgoing(int sock_fd)
{
    poll_socket(sock_fd, POLLOUT);
}

void read_data_once()
{
    int ret;
    ssize_t nread;
    uint8_t buf[4096];

    poll_incoming(sock_fd);
    nread = read(sock_fd, buf, sizeof(buf));
    if (nread <= 0)
    {
        puts("Connection closed");
        exit(0);
    }
    pktbuf_insert(&in, buf, nread);
}

void write_flush()
{
    size_t write_index = 0;
    while (write_index < out.end)
    {
        ssize_t nwritten;
        poll_outgoing(sock_fd);
        nwritten = write(sock_fd, out.buf + write_index, out.end - write_index);
        if (nwritten < 0)
        {
            printf("Write error\n");
            exit(-2);
        }
        write_index += nwritten;
    }
    pktbuf_clear(&out);
}

void write_data_raw(const uint8_t *data, ssize_t len)
{
    pktbuf_insert(&out, data, len);
}

void write_hex(unsigned long hex)
{
    char buf[32];
    size_t len;

    len = snprintf(buf, sizeof(buf) - 1, "%02lx", hex);
    write_data_raw((uint8_t *)buf, len);
}

void write_packet_bytes(const uint8_t *data, size_t num_bytes)
{
    uint8_t checksum;
    size_t i;

    write_data_raw((uint8_t *)"$", 1);
    for (i = 0, checksum = 0; i < num_bytes; ++i)
        checksum += data[i];
    write_data_raw((uint8_t *)data, num_bytes);
    write_data_raw((uint8_t *)"#", 1);
    write_hex(checksum);
}

void write_packet(const char *data)
{
    write_packet_bytes((const uint8_t *)data, strlen(data));
}

void write_binary_packet(const char *pfx, const uint8_t *data, ssize_t num_bytes)
{
    uint8_t *buf;
    ssize_t pfx_num_chars = strlen(pfx);
    ssize_t buf_num_bytes = 0;
    int i;

    buf = malloc(2 * num_bytes + pfx_num_chars);
    memcpy(buf, pfx, pfx_num_chars);
    buf_num_bytes += pfx_num_chars;

    for (i = 0; i < num_bytes; ++i)
    {
        uint8_t b = data[i];
        switch (b)
        {
        case '#':
        case '$':
        case '}':
        case '*':
            buf[buf_num_bytes++] = '}';
            buf[buf_num_bytes++] = b ^ 0x20;
            break;
        default:
            buf[buf_num_bytes++] = b;
            break;
        }
    }
    write_packet_bytes(buf, buf_num_bytes);
    free(buf);
}

bool skip_to_packet_start()
{
    ssize_t end = -1;
    for (size_t i = 0; i < in.end; ++i)
        if (in.buf[i] == '$' || in.buf[i] == INTERRUPT_CHAR)
        {
            end = i;
            break;
        }

    if (end < 0)
    {
        pktbuf_clear(&in);
        return false;
    }

    pktbuf_erase_head(&in, end);
    assert(1 <= in.end);
    assert('$' == in.buf[0] || INTERRUPT_CHAR == in.buf[0]);
    return true;
}

void read_packet()
{
    while (!skip_to_packet_start())
        read_data_once();
    write_data_raw((uint8_t *)"+", 1);
    write_flush();
}

static int async_io_enabled;
void (*request_interrupt)(void);

/* read sock_fs non-blockingly */
bool try_input_interrupt(void)
{
    struct pollfd pfd;
    int nread;
    char buf;

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = sock_fd;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, 0);
    if (ret < 0)
    {
        perror("poll() failed");
        exit(-1);
    }

    if (!ret)
	    return false;

    nread = read(sock_fd, &buf, 1);
    assert(nread == 1 && buf == INTERRUPT_CHAR);
    return true;
}

void remote_prepare(char *name)
{
    int ret;
    char *port_str;
    int port;
    struct sockaddr_in addr;
    socklen_t len;
    char *port_end;
    const int one = 1;
    int listen_fd;

    port_str = strchr(name, ':');
    if (port_str == NULL)
        return;
    *port_str = '\0';

    port = strtoul(port_str + 1, &port_end, 10);
    if (port_str[1] == '\0' || *port_end != '\0')
        printf("Bad port argument: %s", name);

    listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
    if (listen_fd < 0)
    {
        perror("socket() failed");
        exit(-1);
    }

    ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (ret < 0)
    {
        perror("setsockopt() failed");
        exit(-1);
    }

    printf("Listening on port %d\n", port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = *name ? inet_addr(name) : INADDR_ANY;
    addr.sin_port = htons(port);

    if (addr.sin_addr.s_addr == INADDR_NONE)
    {
        printf("Bad host argument: %s", name);
        exit(-1);
    }

    ret = bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        perror("bind() failed");
        exit(-1);
    }

    ret = listen(listen_fd, 1);
    if (ret < 0)
    {
        perror("listen() failed");
        exit(-1);
    }

    sock_fd = accept(listen_fd, NULL, &len);
    if (sock_fd < 0)
    {
        perror("accept() failed");
        exit(-1);
    }

    ret = setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));
    ret = setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    close(listen_fd);
    pktbuf_clear(&in);
    pktbuf_clear(&out);
}
