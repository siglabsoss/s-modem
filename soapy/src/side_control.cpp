#include <iostream>
#include <stdint.h>
#include <unistd.h>
#include <jsonrpccpp/client/connectors/httpclient.h>



#include "common/utilities.hpp"
#include "gen/sidecontroljsonclient.h"

using namespace jsonrpc;
using namespace std;


// void advance(SideControlJsonClient& client, uint32_t val) {
//     try
//     {
//         client.advanceTiming(10);
//     }
//     catch (JsonRpcException e)
//     {
//         cerr << e.what() << endl;
//     }
// }


int main(void) {
    cout << "Side Control boot" << endl;
    enable_raw_keyboard_mode();

    HttpClient httpclient("http://localhost:10004");
    SideControlJsonClient client(httpclient);
    
    char c;
    bool should_break = false;
    while(!should_break) {
        if(kbhit()) {
            c = getch();
            cout << "you pressed: " << c << endl;
            switch(c) {

                // move RX side samples advance
                case 'q':
                    cout << "Advance 100" << endl;
                    client.advanceTiming(100);
                    break;
                case 'w':
                    cout << "Advance 10" << endl;
                    client.advanceTiming(10);
                    break;
                case 'e':
                    cout << "Advance 1" << endl;
                    client.advanceTiming(1);
                    break;

                // move RX side samples backwards
                case 'a':
                    cout << "Backwards 100" << endl;
                    client.advanceTiming(1280-100);
                    break;
                case 's':
                    cout << "Backwards 10" << endl;
                    client.advanceTiming(1280-10);
                    break;
                case 'd':
                    cout << "Backwards 1" << endl;
                    client.advanceTiming(1280-1);
                    break;


                // request RX to measure sync (now automatic)
                case 'p':
                    cout << "Trigger Symbol Sync" << endl;
                    client.triggerSymbolSync();
                    break;
                case 'l':
                    cout << "Trigger TX Based Symbol Sync" << endl;
                    client.triggerTxSymbolSync();
                    break;
                case 'o':
                    cout << "Trigger Fine Sync" << endl;
                    client.triggerFineSync();
                    break;

                // 2 resets
                case 'r':
                    cout << "Reset Experiment" << endl;
                    client.resetExperiment();
                    break;
                case 't':
                    cout << "Reset BER and tracking" << endl;
                    client.resetBER();
                    break;


                // eq options
                case 'y':
                    cout << "Send Channel EQ" << endl;
                    client.sendChannelEq();
                    break;
                case 'h':
                    cout << "Reset Tx EQ" << endl;
                    client.resetChannelEq();
                    break;
                case 'n':
                    cout << "Zero Tx EQ" << endl;
                    client.zeroChannelEq();
                    break;

                // data / pilot modes
                // these are RADIO INDEX and not radio id
                case 'c':
                    cout << "Radio 0, data mode" << endl;
                    client.setScheduleMode(0, 3);
                    break;
                case 'v':
                    cout << "Radio 0, pilot mode mode" << endl;
                    client.setScheduleMode(0, 4);
                    break;
                case 'u':
                    cout << "Radio 1, data mode" << endl;
                    client.setScheduleMode(1, 3);
                    break;
                case 'i':
                    cout << "Radio 1, pilot mode mode" << endl;
                    client.setScheduleMode(1, 4);
                    break;

                // schedule fudge
                case 'b':
                    cout << "Radio 0, fudge schedule" << endl;
                    client.fudgeR0Schedule();
                    break;


                // exit
                case 'x':
                    cout << "Exiting" << endl;
                    should_break = true;
                default:
                    break;
            }
        }

        usleep(10*1000);

    }

    cout << "Closing" << endl;

}
