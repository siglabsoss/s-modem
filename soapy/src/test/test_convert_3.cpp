#include <rapidcheck.h>
#include "driver/AirPacket.hpp"
#include "common/SafeQueue.hpp"
#include "common/convert.hpp"
#include "driver/VerifyHash.hpp"
#include "FileUtils.hpp"

// #include "schedule.h"


#include <algorithm>
#include <iostream>
#include <iomanip>
#include "cpp_utils.hpp"
#include "../data/packet_vectors.hpp"

using namespace std;
using namespace siglabs::file;




#define PI 3.14159


// only call if the buffer is big enough
void dspRunResiduePhase(const std::vector<uint32_t>& cfo_buf_chunk) {

    // cout << "dspRunResiduePhase " << (int) GET_RESIDUE_DUMP_ENABLED() <<  "\n";

    //// for test purpose
    // std::vector<uint32_t> cfo_buf_chunk;

     ////////////////////////////////////////////////////////////
    // double cfo_angle;
    bool residue_phase_flag = true;
    double residue_phase_estimated;
    int _cfo_symbol_num = 102400;
    double residue_phase_estimated_pre = 1.1;
    int residue_phase_counter = 0;
    double residue_phase_for_cfo;
    double cfo_estimated_sent;
    std::vector<double> residue_phase_history;
    double residue_phase_trend;
    int times_residue_phase_sent = 0;

    auto wanted = (_cfo_symbol_num)/100;//500; //_cfo_symbol_num/GET_CFO_SYMBOL_PULL_RATIO();

    auto in_size = cfo_buf_chunk.size();

    auto slice_point = in_size - wanted;
    
    const std::vector<uint32_t> tail(cfo_buf_chunk.begin() + slice_point, cfo_buf_chunk.end());

    ///////// calculate cfo
    double cfo_angle[wanted];
    double cfo_mag2;//[wanted];

    constexpr double mag_tol = 2000;

    double save_imag, save_real;
    bool save_set = false;

    ///// get cfo_angle
    uint32_t index = 0;
    for(const auto n : tail)
    {
        double n_imag;
        double n_real;

        ishort_to_double(n, n_imag, n_real);
        cfo_mag2 = (n_imag*n_imag) + (n_real*n_real);

        // cout << "mag: " << cfo_mag2 << " " << n_imag << ", " << n_real << "\n";

        if( cfo_mag2 <= mag_tol ) {
            // data was too small

            if( !save_set ) {
                cout << "first sample given to dspRunResiduePhase() had a small mag^2 of " << cfo_mag2 << " " << n_imag << ", " << n_real << "\n";
                // return;
            } else {
                // "hold data" by writing the previous good sample over the current bad samples
                n_imag = save_imag;
                n_real = save_real;
            }
        } else {

            // always save the data
            save_imag = n_imag;
            save_real = n_real;
            save_set = true;

        }

        // proceed as normal
        double atan_angle = imag_real_angle(n_imag, n_real);
        cfo_angle[index] = atan_angle;

        index++;


    }
    
    //// phase unwrap
    for(unsigned index = 1; index <wanted; index++)
    {
        if(cfo_angle[index]-cfo_angle[index-1]>=PI)
        {
            for(unsigned index1 = index; index1<wanted; index1++)
            {
                cfo_angle[index1] = cfo_angle[index1] - 2.0*PI;
            }
        }
        else if(cfo_angle[index]-cfo_angle[index-1]<PI*(-1.0))
        {
            for(unsigned index1 = index; index1<wanted; index1++)
            {
                cfo_angle[index1] = cfo_angle[index1] + 2.0*PI;
            }
        }
    }


    

    double cfo_x_mean = (wanted-1)/2.0;
    double cfo_y_mean = 0.0;
    
    uint32_t x = 0;

    double cfo_xx_sum = 0.0;

    for(unsigned index = 0; index<wanted; index++)
    {
        cfo_y_mean = cfo_y_mean + cfo_angle[index];

        cfo_xx_sum = cfo_xx_sum + (x*1.0-cfo_x_mean)*(x*1.0-cfo_x_mean);

        x=x+1;
    }

    cfo_y_mean = cfo_y_mean/wanted;

    double cfo_yx_sum = 0.0;
    
    x = 0;

    for(unsigned index = 0; index<wanted; index++)  
    {
        cfo_yx_sum = cfo_yx_sum + (cfo_angle[index]-cfo_y_mean)*(x*1.0-cfo_x_mean);

        x=x+1;
    }     

    double slope_temp = cfo_yx_sum/cfo_xx_sum;
    double intercept_temp = cfo_y_mean - slope_temp*cfo_x_mean;



    residue_phase_estimated = slope_temp*(wanted-100)+intercept_temp;

    
     


    double did_send = 0;

    if(residue_phase_flag)
    {

            if(abs(residue_phase_estimated_pre) > 0.0)
                {
                    if(abs(residue_phase_estimated)>PI/4)
                    {
                        
                       // residue_phase_estimated = residue_phase_estimated_pre;
                       // return;
                      
                    }
                } 

             // cout<<"residue phase:                                     "<<residue_phase_counter<<"    "<<residue_phase_estimated<<endl;

             // cout<<"residue phase:                                     "<<residue_phase_counter<<"    "<<slope_temp*CFO_ADJ<<endl;

             
             // if(abs(slope_temp*CFO_ADJ)>=0.15)
             // {
             //    for(int index = 0; index<2000; index++)
             //    {
             //        cout << cfo_angle[index*1]<<","<<endl;
             //    }
             // }
             


             if((residue_phase_counter>=50)&&(residue_phase_counter<=55))
             {
                residue_phase_for_cfo = 0.0;
             }
             else if((residue_phase_counter>=60)&&(residue_phase_counter<90))
             {
                residue_phase_for_cfo = residue_phase_for_cfo+residue_phase_estimated;
             }
             else if(residue_phase_counter == 90)
             {

                //  for(int index = 0; index<wanted; index++)
                // {
                //     cout << cfo_angle[index]<<","<<endl;
                // }

                residue_phase_for_cfo = residue_phase_for_cfo/30.0;
                cout << "residue phase for CFO:                   "<< residue_phase_for_cfo<<endl;

             }

             residue_phase_counter = residue_phase_counter+1;
             if(residue_phase_counter == (2*100.0))
             {
                residue_phase_counter = 0;


             }

             residue_phase_estimated_pre = residue_phase_estimated;

             // if(abs(residue_phase_estimated) > 1.5708 ) {
             //        residue_phase_estimated /= 3.0;
             // }

              if(cfo_estimated_sent<0)
              {
                   // dsp->setPartnerPhase(peer_id, residue_phase_estimated*(-1.0));
                   did_send = residue_phase_estimated*(-1.0);
              }
              else
              {
                   // dsp->setPartnerPhase(peer_id, residue_phase_estimated);
                   did_send = residue_phase_estimated;
              }
              // monitorSendResidue();

        residue_phase_history.push_back(residue_phase_estimated);

        if( residue_phase_history.size() == 10 ) {
            residue_phase_trend = 0;
            for(auto t : residue_phase_history) {
                residue_phase_trend += t;
            }

            residue_phase_trend /= residue_phase_history.size();

            // dsp->tickle(&residue_phase_trend);

            residue_phase_history.erase(residue_phase_history.begin(), residue_phase_history.end());
        }

            
        times_residue_phase_sent++;
    }


    // if( array_index == 0 && GET_RESIDUE_DUMP_ENABLED() ) {
    //     static bool fileopen = false;
    //     static ofstream ofile;
    //     if(!fileopen) {
    //         fileopen = true;
    //         std::string fname = GET_RESIDUE_DUMP_FILENAME();
    //         ofile.open(fname);
    //         cout << "OPENED " << fname << "\n";
    //     }

    //     for(auto n : tail)
    //     {
    //         ofile << HEX32_STRING(n) << "\n";
    //     }
    //         // myfile << "found_i " << found_i << " was at frame " << demod_buf_accumulate[0][found_i].second << endl;

    //     if( residue_phase_flag ) {
    //         ofile << "GG " << did_send << "\n";
    //     }

    //     // cout << "d";
    // }

}









int main() {

     rc::check("Test Char", []() {


        std::vector<std::vector<unsigned char>> all = {v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12};

        auto rcopy = v12;

        const auto bump = *rc::gen::inRange(0u, 140u);

        rcopy[3] += bump;
        for(int i = 0; i < bump; i++) {
            const unsigned char c = *rc::gen::inRange(0u, 256u); // not inclusive
            rcopy.push_back(c);
        }

        all.push_back(rcopy);

        int idx = 0;
        for(auto x : all) {


            // cout << "idx: " << idx << endl;

            std::vector<uint32_t> constructed1 = charVectorPacketToWordVector(x);
            // cout << HEX32_STRING(constructed1[0]) << endl;

            int erase;
            auto got1 = packetToBytes(constructed1, erase);

            RC_ASSERT(got1 == x);
            idx++;
        }



        

        // std::vector<uint32_t> constructed2 = charVectorPacketToWordVector(v2);


        // auto got2 = packetToBytes(constructed2, erase);

        // RC_ASSERT(got2 == v2);

        // auto packetToBytes;
      
    });


    auto in3 = file_read_hex("/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/save_residue3_strip.txt");
    constexpr double mag_tol = 2000;
    double cfo_mag2;

    while(in3.size() > 2048) {
        dspRunResiduePhase(in3);
        in3.erase(in3.begin(), in3.begin()+1024);
    }

    // for(auto n : in3) {
    //     double n_imag;
    //     double n_real;

    //     ishort_to_double(n, n_imag, n_real);
    //     cfo_mag2 = (n_imag*n_imag) + (n_real*n_real);

    //     if( cfo_mag2 <= mag_tol ) {
    //         cout << HEX32_STRING(n) << endl;
    //     }

    // }

  return 0;
}

