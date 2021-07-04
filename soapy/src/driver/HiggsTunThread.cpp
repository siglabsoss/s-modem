#include "HiggsSoapyDriver.hpp"
#include "HiggsDriverSharedInclude.hpp"
#include "vector_helpers.hpp"
#include <math.h>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <boost/functional/hash.hpp>
#include "driver/RetransmitBuffer.hpp"
#include "driver/FlushHiggs.hpp"
#include "common/convert.hpp"

using namespace std;



#define BUFSIZE 2000
#define TIMEOUT_US (10000)
#define MAXSIZE (367-32)

struct timeval timeout;
static int guard(int n, const char * err) { if (n == -1) { perror(err); exit(1); } return n; }
//This is run on the tun thread
static int tunAlloc(char *dev, int flags, bool setTimeout, fd_set* set_fd) {

  struct ifreq ifr;
  int fd, err;
  const char *clonedev = "/dev/net/tun";

  if( (fd = open(clonedev , O_RDWR)) < 0 ) {
    perror("Opening /dev/net/tun");
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = flags;

  if (*dev) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }

  if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    perror("ioctl(TUNSETIFF)");
    close(fd);
    return err;
  }

  FD_ZERO(set_fd); /* clear the set */
  FD_SET(fd, set_fd); /* add our file descriptor to the set */

  strcpy(dev, ifr.ifr_name);

  if(setTimeout) {
    int flags2 = guard(fcntl(fd, F_GETFL), "could not get file flags");
    guard(fcntl(fd, F_SETFL, flags2 | O_NONBLOCK), "could not set file flags");
    // struct timeval tv;
    // tv.tv_sec = 0;
    // tv.tv_usec = 50*1000;
    // setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
  }

  return fd;
}


void HiggsDriver::tunThreadAsRx() {
    unsigned char buffer[BUFSIZE];
    // Clear Buffer
    for(int i = 0; i < BUFSIZE; i++) {
        buffer[i] = 0;
    }

    // boost::hash<std::vector<unsigned char>> char_hasher;
    // boost::hash<std::vector<uint32_t>> word_hasher;



    while(higgsRunTunThread) {

        int nread = -1;
        // cout << "XXXXXXXXXXXXXXXXXXXXXXXXXX" << endl;
        nread = read(tun_fd, buffer, sizeof(buffer));
        // cout << "yyyyyyyyyyyyyyyyyyyyyyyyyyyy" << endl;

        // if we got a valid read
        if(nread != -1) {
            // cout << "yyyyyyyyyyyyyyyyyyyyyyyyyyyy rx tun thread got " << nread << " bytes" << endl;

            std::vector<unsigned char> char_vec = cBufferPacketToCharVector(buffer, BUFSIZE, nread);

            // std::size_t h = char_hasher(char_vec);
            // cout << "yyyyyyyyyy tunThreadAsRx() got packet with len " << nread << " char hash " << HEX32_STRING(h) << endl;

            std::vector<uint32_t> constructed = charVectorPacketToWordVector(char_vec);

            if( constructed.size() > 0 ) { // redundant check
                // which peer gets the message

                uint32_t peer_with_tun;
                if( GET_RX_TUN_TO_ARB() ) {
                    peer_with_tun = GET_PEER_ARBITER();
                } else {
                    peer_with_tun = GET_PEER_0();
                }

                auto packet = feedback_vector_packet(
                    FEEDBACK_VEC_UPLINK, 
                    constructed, 
                    feedback_peer_to_mask(peer_with_tun),
                    FEEDBACK_DST_PC);

                // cout << "As Rx Publishing packet back to tx peer id " << tx0_peer << endl;
                zmqPublishOrInternal(peer_with_tun, packet);
            }

            // after we are done, re-clear the buffer
            for(int i = 0; i < BUFSIZE; i++) {
                buffer[i] = 0;
            }
        }

        // allow processor to breath
        usleep(100);

    }

}

void HiggsDriver::tunThreadAsTx() {
    unsigned char buffer[BUFSIZE];

    for(int i = 0; i < BUFSIZE; i++) {
        buffer[i] = 0;
    }

    // struct timeval timeout;
    // timeout.tv_sec = 0;
    // timeout.tv_usec = TIMEOUT_US;

    // boost::hash<std::vector<unsigned char>> char_hasher;
    // boost::hash<std::vector<uint32_t>> word_hasher;

    // int packetSize;
    // int rd;
    while(higgsRunTunThread) {

        int nread = -1;
        nread = read(tun_fd, buffer, sizeof(buffer));

        if(nread != -1) {
            std::vector<unsigned char> char_vec = cBufferPacketToCharVector(buffer, BUFSIZE, nread);

            // std::size_t h = char_hasher(char_vec);
            // cout << "wwwwwwwww tunThreadAsTx() got packet with length " << nread << "\n";

            std::vector<uint32_t> constructed = charVectorPacketToWordVector(char_vec);


            // for( auto w : constructed ) {
            //     cout << HEX32_STRING(w) << endl;
            // }

            txTun2EventDsp.enqueue(constructed);

            // after we are done, re-clear the buffer
            for(int i = 0; i < BUFSIZE; i++) {
                buffer[i] = 0;
            }
        }

        if(demodRx2TunFifo.size()) {

            auto tunVector = demodRx2TunFifo.dequeue();

            std::vector<unsigned char> char_vec2 = wordVectorPacketToCharVector(tunVector);
            // std::size_t h = char_hasher(char_vec2);
            // cout << "tunThreadAsTx() got packet over zmq with len " << char_vec2.size() << " char hash " << HEX32_STRING(h) << endl;

            auto retval = write(tun_fd, char_vec2.data(), char_vec2.size());
            if( retval < 0 ) {
                cout << "tunThreadAsTx() write !!!!!!!!!!!!!!!!!!!!!!!!!!! returned " << retval << endl;
            }
        }

        static int counter = 0;

        if( nread == -1 ) {
            counter++;
        } else {
            counter = 0;
        }

        if( counter > 10) {
            usleep(100);
        }

    }
}

size_t HiggsDriver::txTunBufferLength() {
    return txTun2EventDsp.size();
}

// void HiggsDriver::tunThreadAsArbiter() {
    // while(higgsRunTunThread) {

    // }
// }

void HiggsDriver::tunThread() {
    cout << "tunThread() enabled" << endl;
    char tun_name[IFNAMSIZ];
    strcpy(tun_name, THE_TUN_NAME);
    tun_fd = tunAlloc(tun_name, IFF_TUN | IFF_NO_PI, true, &set_fdTun); /* tun interface */

    if( GET_RADIO_ROLE() == "rx" ) {
        tunThreadAsRx();
    }
    if( GET_RADIO_ROLE() == "tx" ) {
        tunThreadAsTx();
    }
    if( GET_RADIO_ROLE() == "arb") {
        tunThreadAsTx();
    }


    return;

}
