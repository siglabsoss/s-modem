#include <netinet/ip.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>


class CSocket{
public:
    struct sockaddr_in rx_address;
    struct sockaddr_in tx_address;
    const char *ip;
    int tx_port;
    int rx_port;
    int sock_fd;

    CSocket(const char *ip, int tx_port, int rx_port);
    CSocket();
    ~CSocket();
    int create_socket(const char *ip, int tx_port, int rx_port);
    int bind_socket();
    // int send_cmd(struct cmd_send *cmd, struct cmd_reply *ack);

};