#include "csocket.hpp"
#include <iostream>

CSocket::CSocket(const char *_ip, int _tx_port, int _rx_port){
    ip = _ip;
    tx_port = _tx_port;
    rx_port = _rx_port;
    sock_fd = 0;
}

CSocket::CSocket(){
    
}

CSocket::~CSocket() {
    if (sock_fd) close(sock_fd);
}

int CSocket::create_socket(const char *_ip, int _tx_port, int _rx_port){
    memset((char *)&tx_address, 0, sizeof(tx_address));
    memset((char *)&rx_address, 0, sizeof(rx_address));

    tx_address.sin_family = AF_INET;
    tx_address.sin_port = htons(_tx_port);
    tx_address.sin_addr.s_addr = inet_addr(_ip);

    rx_address.sin_family = AF_INET;
    rx_address.sin_port = htons(_rx_port);
    rx_address.sin_addr.s_addr = htonl(INADDR_ANY);

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_fd < 0){
        std::cout << "ERROR: Socket file descriptor was not allocated\n";
        return -1;
    }
    else{
        std::cout << "Socket construction complete\n";
        return 0;
    }
}

int CSocket::bind_socket(){
    int bind_successful = -1;
    bind_successful = bind(sock_fd, (struct sockaddr*)&rx_address,
                           sizeof(rx_address));
    if(bind_successful < 0){
        std::cout << "ERROR: Unable to bind to RB receive socket\n";
        exit(1);

        return -1;
    }
    else{
        std::cout << "Socket bind successful\n";
    }
    return 0;
}

// int CSocket::send_cmd(struct cmd_send *cmd, struct cmd_reply *ack){
//     socklen_t addrlen = sizeof(rx_address);

//     std::cout << "Sending " << "("
//               << std::to_string(cmd->seq) << ", "
//               << std::to_string(cmd->r_wn) << ", "
//               << std::to_string(cmd->byte_addr) << ", "
//               << std::to_string(cmd->data) << ")" << std::endl;
//     if(sendto(sock_fd, cmd, sizeof(struct cmd_send), 0, (struct sockaddr *)&tx_address, addrlen) < 0){
//         std::cout << "Sending " << "("
//                    << std::to_string(cmd->seq) << ", "
//                    << std::to_string(cmd->r_wn) << ", "
//                    << std::to_string(cmd->byte_addr) << ", "
//                    << std::to_string(cmd->data) << ") failed" << std::endl;
//     }

//     if(recvfrom(sock_fd, ack, sizeof(struct cmd_reply), MSG_WAITALL,
//                (struct sockaddr *)&rx_address, &addrlen) < 0){
//         std::cout << "Acknowledgement " << "("
//                    << std::to_string(ack->seq) << ", "
//                    << std::to_string(ack->id) << ", "
//                    << std::to_string(ack->ack2) << ", "
//                    << std::to_string(ack->data) << ") failed" << std::endl;
//     }
//     else{
//         std::cout << "Received " << "("
//                   << std::to_string(ack->seq) << ", "
//                   << std::to_string(ack->id) << ", "
//                   << std::to_string(ack->ack2) << ", "
//                   << std::to_string(ack->data) << ")" << std::endl;
//     }

//     return 0;
// }
