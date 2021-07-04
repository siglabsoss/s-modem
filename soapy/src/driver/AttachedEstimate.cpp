#include "AttachedEstimate.hpp"
#include "RadioEstimate.hpp"
#include "TxSchedule.hpp"
#include "GenericOperator.hpp"
#include "EventDsp.hpp"


using namespace std;



void AttachedEstimate::attachedFineSyncRaw(const uint32_t word) {

    if( dsp->role != "tx") {
        return;
    }

    const bool is_upper  = word&0xff0000;
    const uint32_t value = word&0xffff;

    if( !is_upper ) {
        fine_sync_raw_lower = value;
    } else {
        const uint32_t raw = ((value << 16)&0xffff0000) | fine_sync_raw_lower;
        const int32_t rr = (int16_t)(raw&0xffff);
        const int32_t ii = (int16_t)((raw>>16)&0xffff);

        const int32_t magi = sqrt(rr*rr + ii*ii);
        const double magd = (double)magi / (0x7fff * 1.4142);
        const double _magd = floor(magd*100) / 100.0;

        // cout << "attachedFineSyncRaw: " << HEX32_STRING(raw) << "\n";
        // cout << "attachedFineSyncRaw r: " << rr << " i: " << ii << "\n";
        cout << "attachedFineSyncRaw mag: " << _magd << "\n";
    }
}

// If you add anything here, also add it to 
// dspDispatchAttachedRb_TX   under the attached_consume = true section
void AttachedEstimate::dispatchAttachedRb(const uint32_t word)
{
    // cout << "AttachedEstimate::dispatchAttachedRb() " << endl;
    uint32_t data = word & 0x00ffffff;
    uint32_t cmd_type = word & 0xff000000;
    (void)data;

    uint32_t fbError = 0;

    switch(cmd_type) {

        // case TX_FILL_LEVEL_PCCMD:
        //     handleFillLevelReply(word);
        //     break;
        case TX_UD_LATENCY_PCCMD:
            handleFillLevelReplyDuplex(word);
            break;

        case TX_USERDATA_ERROR: {
                auto msg = getErrorStringFeedbackBusParse(word, &fbError);
                cout << msg;
                if( fbError == 3 ) {
                    cout << "dispatchAttachedRb requesting pause\n";
                    soapy->txSchedule->errorDetected++;
                }
                if( fbError == 2 ) {
                    soapy->txSchedule->flushZerosEnded++;
                }
                if( fbError == 1 ) {
                    soapy->txSchedule->flushZerosStarted++;
                }
            }
            break;

        case TEST_STACK_RESULTS_PCCMD: {
            cout << getErrorStringTestStack(word);
        }
        break;

        case FEEDBACK_ALIVE:
            feedback_alive_count++;
            break;

        case GENERIC_READBACK_PCCMD:
            dispatchOp->gotWord(word);
            break;

        case LAST_USERDATA_PCCMD:
            cout << "LAST_USERDATA_PCCMD " << HEX32_STRING(data) << "\n";
            break;

        case FINE_SYNC_RAW_PCCMD:
            attachedFineSyncRaw(data);
            break;

        default:
            break;
    }
}

