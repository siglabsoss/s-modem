#include <rapidcheck.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "cpp_utils.hpp"

#include <string>
#include <map>
#include <mutex>
#include <fstream>
#include <iostream>
#include "convert.hpp"

#include "3rd/json.hpp"


using json = nlohmann::json;

using namespace std;


/*
 * Populates a standard vector with specified amount of random 32 bit values
 *
 * Attributes:
 *     input_data: A standard vector to populate random data
 *     data_length: Amount of random data to populate
 */
void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length);


nlohmann::json js;


void load(std::string fname) {

    cout << "About to open json file name: " << fname << endl;

    std::ifstream ifs(fname);
    ifs >> js;

}


constexpr unsigned ALL_SC_CHUNK = 1024;
double channel_angle_filtering_buffer[25][1024];  // FIXME
size_t eq_filter_index;

bool should_run_background = true;
bool _eq_use_filter = true;
std::vector<uint32_t> all_sc;
std::vector<double> channel_angle; // sized 1024

std::vector<std::vector<std::pair<double,double>>> eq_filter;








void initChannelFilter(void) {

    for(int i=0; i<1024; i++) {
        for(int j=0; j<25; j++) {
            channel_angle_filtering_buffer[j][i] = 0.0;
        }
    }

    eq_filter.resize(25);
    // const std::pair<double,double> zero = {0,0};
    for( auto& vec : eq_filter ) {
        vec.resize(1024, {0,0});
    }


    eq_filter_index = 0;
}

void updateChannelFilterIndex(void) {
    if(should_run_background && _eq_use_filter)
    {
        eq_filter_index++;
        if(eq_filter_index >= 25)
        {
            eq_filter_index = 0;

            // cout << "//// Continuous updatePartnerEq() ////" << endl;
            // updatePartnerEq();
        }
    }
}

void init(void) {
    channel_angle.resize(1024);
    initChannelFilter();
}

bool dspRunChannelEstBuffers(void) {

    if(all_sc.size() != ALL_SC_CHUNK) {
        cout << "dspRunChannelEst(5) wrong !!! " << all_sc.size() << " != " << ALL_SC_CHUNK << endl;
        return false;
    }
    // std::vector<uint32_t> all_sc;
    // all_sc = all_sc_buf.get(ALL_SC_CHUNK);

    uint32_t index = 0;
    double channel_angle_current;
    for(const auto n : all_sc)
    {
        double n_imag;
        double n_real;
        ishort_to_double(n, n_imag, n_real);
        const double atan_angle = imag_real_angle(n_imag, n_real);

        //mag_coeff = 32767.0*sqrt(n_imag*n_imag + n_real*n_real)/(n_imag*n_imag + n_real*n_real+100);  //try this one later. may affect shift in cs20 at tx.
        const bool ok = 
            (n_real<=32767.0) &&
            (n_real>=-32768.0) &&
            (n_imag<=32767.0) &&
            (n_imag>=-32768.0);

        if(ok) {
            channel_angle_current = atan_angle*-1.0;
            if(_eq_use_filter && should_run_background) {
                /// mean filtering
                channel_angle_filtering_buffer[eq_filter_index][index] = channel_angle_current;

                channel_angle_current = 0.0;
                for(int j = 0; j < 25; j++) {
                    channel_angle_current += channel_angle_filtering_buffer[j][index];
                }

                double temp_max = channel_angle_filtering_buffer[0][index];
                double temp_min = channel_angle_filtering_buffer[0][index];

                for(int j = 1; j < 25; j++) {
                    if(temp_max <  channel_angle_filtering_buffer[j][index]) {
                        temp_max = channel_angle_filtering_buffer[j][index];
                    }

                    if(temp_min >  channel_angle_filtering_buffer[j][index]) {
                        temp_min = channel_angle_filtering_buffer[j][index];
                    }
                }

                channel_angle_current = (channel_angle_current - temp_min - temp_max) / (25.0-2.0);
                //cout << index<<"            "<<eq_filter_index<<endl;
            }
            // channel_angle[index] = 1*channel_angle_current + 0*channel_angle[index];
            channel_angle[index] = channel_angle_current;

        } else {
            cout<< "EQ angle data overflow ?????????????????????????????"<<endl; 
        }

        index++;
    }

    if(index!=1024) {
        cout << "dspRunChannelEst() had illegal value of index after loop: " << index <<"\n";
    }

    return true;
}


bool dspRunChannelEstBuffersComplexFilter(void) {

    if(all_sc.size() != ALL_SC_CHUNK) {
        cout << "dspRunChannelEst(6) wrong !!! " << all_sc.size() << " != " << ALL_SC_CHUNK << endl;
        return false;
    }

    uint32_t index = 0;
    for(const auto n : all_sc)
    {
        double n_imag;
        double n_real;
        ishort_to_double(n, n_imag, n_real);

        //mag_coeff = 32767.0*sqrt(n_imag*n_imag + n_real*n_real)/(n_imag*n_imag + n_real*n_real+100);  //try this one later. may affect shift in cs20 at tx.
        const bool ok = 
            (n_real<=32767.0) &&
            (n_real>=-32768.0) &&
            (n_imag<=32767.0) &&
            (n_imag>=-32768.0);

        if(ok) {

            eq_filter[eq_filter_index][index] = std::make_pair(n_imag, n_real);

            double imag_sum = 0;
            double real_sum = 0;

            // channel_angle_current = atan_angle*-1.0;
            if(_eq_use_filter && should_run_background) {
                // add all real and imag to make large vector
                double a_imag;
                double a_real;
                for(unsigned j = 0; j < 25; j++) {
                    // eq_filter[eq_filter_index][index]
                    std::tie(a_imag, a_real) = eq_filter[j][index];
                    imag_sum += a_imag;
                    real_sum += a_real;
                    // cout << "imag: " << a_imag << "\n";
                }
                
                // note this is NEGATIVE result
                channel_angle[index] = -imag_real_angle(imag_sum, real_sum);
                //cout << index<<"            "<<eq_filter_index<<endl;
            } else {
                // note this is NEGATIVE result
                channel_angle[index] = -imag_real_angle(n_imag, n_real);
            }
        } else {
            cout<< "EQ angle data overflow ?????????????????????????????"<<endl; 
        }

        index++;
    }

    if(index!=1024) {
        cout << "dspRunChannelEst() had illegal value of index after loop: " << index <<"\n";
    }

    return true;
}


// load row from global json file into
void loadSc(unsigned row) {
    all_sc.resize(1024);
    for(int i = 0; i < 1024; i++) {
        all_sc[i] = js[row][i];
    }
}


void printAngle(unsigned max = 1024) {
    for(unsigned i = 0; i < max; i++) {
        const auto w = channel_angle[i];
        std::cout << std::fixed;
        std::cout << std::setprecision(3);
        if( w >= 0 ) {
            cout << " ";
        }
        cout << w << " ";
    }
    cout << "\n";
}


int main() {

    bool pass = true;

    cout << "Test Eq\n";

    load("../src/data/eq_vectors_0.json");

    init();

    for(int i = 0; i < 30; i++) {

        loadSc(i);
        // dspRunChannelEstBuffers();
        dspRunChannelEstBuffersComplexFilter();
        updateChannelFilterIndex();
        printAngle(20);
    }

    // assert(abs(2.231 - channel_angle[1]) < 0.01);
    // assert(abs(1.950 - channel_angle[3]) < 0.01);

    // for(int i = 0; i < 30; i++) {
    //     cout << js[i][1] << "\n";
    // }


//  0.000  2.231  0.003  1.950  0.007  0.502  0.025  0.791 -0.013  1.321  0.049  1.541 -0.037  0.911  0.074  1.133 -0.058  0.295  0.091  0.585
// -0.000  3.037  0.001  2.357  0.007  2.929  0.025  2.846 -0.014  2.198  0.048  1.851 -0.038  2.731  0.073  2.259 -0.059  3.011  0.090  2.967
    // cout << js[0][35] << "\n";


    return !pass;
}

void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length) {
    for(uint32_t i = 0; i < data_length; i++) {
        const uint32_t a_word = *rc::gen::inRange(0u, 0xffffffffu);
        input_data.push_back(a_word);
    }

}
