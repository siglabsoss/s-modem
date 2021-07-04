#include "HiggsSNR.hpp"
#include <future>



#include "driver/HiggsSettings.hpp"
#include "driver/EventDsp.hpp"
#include "driver/HiggsSoapyDriver.hpp"

using namespace std;


static void _handle_snr_callback(struct bufferevent *bev, void *_dsp) {
    HiggsSNR *dsp = (HiggsSNR*) _dsp;
    dsp->handle_snr_callback(bev);
}

static void _handle_snr_event(bufferevent* d, short kind, void* v) {
    cout << "_handle_snr_event" << endl;
}


///////////////////////////////////////////////////////////////
//
//  Above this line are static functions which comprise events for this class
//

HiggsSNR::HiggsSNR(HiggsDriver* s):
    HiggsEvent(s)
    ,avg_snr(0.0)
{
    settings = (soapy->settings);

    history.resize(1024);

    setupBuffers();
}


void HiggsSNR::setupBuffers() {
    // bufferevent_options(udp_payload_in_bev, BE_VOPT_THREADSAFE);
    // cout << " got to HiggsFineSync::setupBuffers() " << endl;

    // shared set of pointers that we overwrite and then copy out from
    // (copied to BevPair class)
    struct bufferevent* rsps_bev[2];

    int ret = bufferevent_pair_new(evbase, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE , rsps_bev);
    assert(ret>=0);
    snr_buf = new BevPair();
    snr_buf->set(rsps_bev).enableLocking();

    // I'm guessing we would only adjust read of out
    // and we would only care about write of in, but we only need this for interrupts when data is 
    //   written?

    if(true) {

        auto snr_watermark = SNR_PACK_SIZE*4;

        // set 0 for write because nobody writes to out
        bufferevent_setwatermark(snr_buf->out, EV_READ, snr_watermark, 0);

        // bufferevent_set_max_single_read(sfo_cfo_buf->out, (1024*3)*4 );


        // only get callbacks on read side
        bufferevent_setcb(snr_buf->out, _handle_snr_callback, NULL, _handle_snr_event, this);
        bufferevent_enable(snr_buf->out, EV_READ | EV_WRITE | EV_PERSIST);
    }
}

void HiggsSNR::handle_snr_callback(struct bufferevent *bev) {
    struct evbuffer *buf  = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(buf);
    const size_t largest_read = ( (len) / (SNR_PACK_SIZE*4) ) * (SNR_PACK_SIZE*4);
    unsigned char* temp_read = evbuffer_pullup(buf, largest_read);
    uint32_t* read_from = (uint32_t*)temp_read;
    uint32_t word_len = largest_read / 4;
    uint32_t struct_len = word_len / SNR_PACK_SIZE;

    if(word_len%SNR_PACK_SIZE!=0){

        cout<<"From HiggsSNR: the word_len is wrong!!!!!!!!      "<<word_len<<endl;
    
    }else{

        uint32_t pack_index = 0;

        while(word_len>0){

            snrCalculate(read_from + pack_index*SNR_PACK_SIZE, SNR_PACK_SIZE);


            word_len -= SNR_PACK_SIZE;

            pack_index++;
        }

    }


    // if(word_len == SNR_PACK_SIZE) {
    //     if( print_all ) {
    //         for(uint32_t index=0; index < 2; index++) {
    //             cout << index << "   " << HEX32_STRING(read_from[index])<<endl;
    //         }
    //     }

    // } else {
    //     if( print_anything ) {
    //         cout << "WRONG !!!!!!!!!!!!!"<<endl;
    //         cout << "WRONG !!!!!!!!!!!!!" << word_len << "         "<< SNR_PACK_SIZE<<endl;
    //         cout << "WRONG !!!!!!!!!!!!!"<<endl;
    //     }
    //     //return;
    // }

    

   ////////////////////////calculation snr



   /////////////////////////////////////////////////////////////////////
    
    
    evbuffer_drain(buf, largest_read);

}


bool HiggsSNR::snrCalculate(uint32_t* raw_data, uint32_t length){


    double temp_snr = 0.0;

    const bool print_anything = print_all | print_avg;


    // if( print_anything ){
    //     for(auto index = 0; index < length; index++){
    //         cout<<index<<"               "<<HEX32_STRING(raw_data[index])<<endl;
    //     }

    // }



    if((raw_data[0]&0xfffff000)!=0xfffff000) {
        if( print_anything ) {
            cout << "WRONG !!!!!!!!!!!!!"<<endl;
            cout << "WRONG !!!!!!!!!!!!!" <<HEX32_STRING(raw_data[0])<<endl;
            cout << "WRONG !!!!!!!!!!!!!"<<endl;
        }
        return false;
    } else {


        if( (raw_data[0]&0xfff) > 1024 ) {
            cout << "HiggsSNR got illegally high value for subcarrier index: " << (raw_data[0]&0xfff) << "\n";
            return false;
        }

        const uint32_t sc_index = raw_data[0]&0x3ff;

        if(print_anything){
            cout << ((0<10)?" ": "") << 0 << "   " << HEX32_STRING(raw_data[0])<<endl;
            cout << ((1<10)?" ": "") << 1 << "   " << HEX32_STRING(raw_data[1])<<endl;

        }

        const uint32_t temp_benchmark = raw_data[1];
        
        if(temp_benchmark == 0){
            return false;
        }

        const int16_t temp_benchmark_r = (temp_benchmark&0xffff);
        const int16_t temp_benchmark_i = ((temp_benchmark>>16)&0xffff);

        const double temp_qpsk_scale = sqrt((temp_benchmark_r*1.0)*(temp_benchmark_r*1.0) + (temp_benchmark_i*1.0)*(temp_benchmark_i*1.0))/(sqrt(2));

        for(auto index = 2; index < length; index++) {

            const uint32_t temp_data = raw_data[index];
            
            if(temp_data == 0){
                return false;
            }

            const int16_t temp_data_r = (temp_data&0xffff);
            const int16_t temp_data_i = ((temp_data>>16)&0xffff);

            double noise_r;
            double noise_i;

            if( expect_single_qpsk ) {
                noise_r = (temp_data_r*1.0) - temp_qpsk_scale;
                noise_i = (temp_data_i*1.0) - temp_qpsk_scale;
            } else {
                noise_r = fabs(temp_data_r*1.0) - temp_qpsk_scale;
                noise_i = fabs(temp_data_i*1.0) - temp_qpsk_scale;
            }

            const double temp_snr_per_ofdm = 2*temp_qpsk_scale*temp_qpsk_scale/(noise_r*noise_r+noise_i*noise_i);

            if( print_all ) {
                cout << ((index<10)?" ": "") << index << "   " << HEX32_STRING(raw_data[index])<<"   " << temp_snr_per_ofdm << endl;
            }

            temp_snr += temp_snr_per_ofdm;

        }

        double tone_bandwidth = 30517.57; // in Hz

        double fb_qpsk = 25600*2;  // bits per second
        double fb_16qam = 25600*4;
        double fb_64qam = 25600*6;

        double adj_qpsk = 10*log10(fb_qpsk/tone_bandwidth);
        double adj_16qam = 10*log10(fb_16qam/tone_bandwidth);
        double adj_64qam = 10*log10(fb_64qam/tone_bandwidth);

        double min_ebn0_qpsk  = 6.5;    //ber 10^(-4)
        double min_ebn0_16qam = 10.5;
        double min_ebn0_64qam = 14.5;

        temp_snr /= (length-2)*1.0;
        const double as_db = 10.0*log10(temp_snr);

        {
            std::unique_lock<std::mutex> lock(history_mutex);
            history[sc_index].push_back(as_db);
            if( history[sc_index].size() > max_history ) {
                history[sc_index].erase(history[sc_index].begin());
            }
        }

        if( print_avg ) {
            cout << std::setfill(' ') << std::setw(20)<<"5ms ave SNR for data tone " << sc_index << ": " 
                 << std::setw(20)<< temp_snr 
                 << std::setw(20)<< as_db <<"dB"
                 // << std::setw(30)<<"Eb/N0(QPSK): "
                 // << std::setw(10)<<as_db-adj_qpsk <<"("<<min_ebn0_qpsk<<")"<<"dB"
                 // << std::setw(30)<<"Eb/N0(16QAM): "
                 // << std::setw(10)<<as_db-adj_16qam<<"("<<min_ebn0_16qam<<")"<<"dB"
                 // << std::setw(30)<<"Eb/N0(64QAM): "
                 // << std::setw(10)<<as_db-adj_64qam<<"("<<min_ebn0_64qam<<")"<<"dB"
                 <<endl;
        }
        
        SetSNR(temp_snr);

        //avg_snr = temp_snr;

        return true;
    }
}


void HiggsSNR::stopThreadDerivedClass() {

}





void HiggsSNR::SetSNR(double value){
    avg_snr = value;
}

double HiggsSNR::GetSNR() const {
    return avg_snr;
}


std::vector<double> HiggsSNR::GetHistory(const unsigned idx) {

    if( idx >= 1024 ) {
        return {};
    }

    std::unique_lock<std::mutex> lock(history_mutex);
    return history[idx];
}
