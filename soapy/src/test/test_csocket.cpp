#include <iostream>
#include <rapidcheck.h>
#include "driver/csocket.hpp"
#include "common/constants.hpp"
#include <string>

/*
 * Test whether socket can be successfully created and binded
 */
void test_socket_creation();

int main(int argc, char const *argv[])
{
    test_socket_creation();

    return 0;
}

void test_socket_creation() {
    rc::check("Test socket creation", []() {
        int create_successful = -1;
        int bind_successful = -1;
        CSocket csocket;

        create_successful = csocket.create_socket(HIGGS_IP,
                                                  TX_CMD_PORT,
                                                  RX_CMD_PORT);
        RC_ASSERT(create_successful == 0);
        bind_successful = csocket.bind_socket();
        RC_ASSERT(bind_successful == 0);
    });
}