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
#include "cplx.hpp"

using namespace std;
using namespace siglabs::file;




// #define EVM_CHUNK (128)


// FIXME these are old MMSE variables that should be moved
static const unsigned int num_points = 1024;
static constexpr unsigned int array_length = num_points * 2;
static double input_buffer[array_length];
static double output_buffer[array_length];
static double evm_buffer[num_points];
static double eq_buffer[array_length];
// static bool init_eq = true;
// FIXME This method should be placed in a constructor
static void init_eq_buffer(double* const _eq_buffer, const unsigned int _array_length) {
    for (unsigned int i = 0; i < _array_length; i++) {
        _eq_buffer[i] = (i + 1) % 2;
    }
}






/*
auto in3 = file_read_raw_uint8_t(path);

    cout << "sz: " << in3.size() << "\n";

    const double* as_double = (double*)in3.data();
    const unsigned double_len = in3.size() / sizeof(double); 
    const unsigned samples_len = double_len / 2;

    cout << "doubles: " << double_len << "\n";
    cout << "doubles: " << samples_len << "\n";

    bool print = false;

    if( print ) {
        for(int i = 0; i < samples_len; i++) {
            const double x_re = as_double[2 * i];
            const double x_im = as_double[2 * i + 1];
            cout << "r: " << x_re << " im: " << x_im << "\n";
        }
    }
*/

static void setup_evm() {
    init_eq_buffer(eq_buffer, array_length);
    std::cout << "Equalization vector initialized" << std::endl;
}

    


static std::vector<double> handle_evm(const std::vector<uint32_t>& vec0) {

    std::vector<double> out;
    const int chunks = vec0.size() / num_points;

    const int step = 1;
    const uint32_t q = 12;

    for(int i = 0; i < chunks; i++ ) {
        for( unsigned j = 0; j < num_points; j++) {
            // const uint32_t* int_d_start = (const uint32_t*)vec0.data();
            const uint32_t* int_d_start = ((const uint32_t*)vec0.data()) + i*chunks;

            cf64_ci32_unpack_vector(int_d_start, input_buffer, q, num_points, step);
            mmse_freq_vector(input_buffer, output_buffer, eq_buffer, num_points);
            evm_vector(output_buffer, evm_buffer, num_points);

            for (unsigned k = 0; k < num_points; k++) {
                out.push_back(evm_buffer[k]);
                // std::cout << std::hex << "Eq Index[" << i << "]: " << eq_buffer[i] << " " << eq_buffer[i+1] << std::endl; 
                // std::cout << std::hex << "EVM Index[" << i << "]: " << eq_buffer[i] << std::endl;
            }
        }
    }

    cout << "Evm ran in " << chunks << " chunks\n";

// #ifdef evmm



    // cf64_ci32_unpack_vector(int_d_start, input_buffer, q, num_points, step);
    // mmse_freq_vector(input_buffer, output_buffer, eq_buffer, num_points);
    // evm_vector(output_buffer, evm_buffer, num_points);
    // for (int i = 0; i < num_points; i++) {
    //     // std::cout << std::hex << "Eq Index[" << i << "]: " << eq_buffer[i] << " " << eq_buffer[i+1] << std::endl; 
    //     std::cout << std::hex << "EVM Index[" << i << "]: " << eq_buffer[i] << std::endl;
    // }
// #endif
    return out;
}



int main() {

    // cout << "hi" <<"\n";
    // return 0;
    double gain;


    gain = 1;
    // gain = 1;

    std::string path;
    path = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/js/test/data/single_sc_1.raw";
    // path = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/js/test/data/ideal_qpsk_1.raw";

    // path = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/save_residue3_strip.txt"

    auto in3 = file_read_raw_uint8_t(path);

    in3.resize((8)*10000);

    cout << "sz: " << in3.size() << "\n";

    const float* as_double = (float*)in3.data();
    const unsigned float_len = in3.size() / sizeof(float); 
    const unsigned samples_len = float_len / 2;

    cout << "floats: " << float_len << "\n";
    cout << "complex samples: " << samples_len << "\n";

    bool print = false;

    if( print ) {
        for(unsigned i = 0; i < samples_len; i++) {
            const float x_re = as_double[2 * i];
            const float x_im = as_double[2 * i + 1];
            cout << "r: " << x_re << " im: " << x_im << "\n";
        }
    }


    std::vector<std::complex<double>> source;

    

    for(unsigned i = 0; i < samples_len; i++) {
        const float x_re = as_double[2 * i];
        const float x_im = as_double[2 * i + 1];

        source.emplace_back(gain*x_re, gain*x_im);
    }

    cout << "\n";


    std::vector<uint32_t> vec0;
    complexToIShortMulti(vec0, source);


    if( print ) {
        for(unsigned i = 0; i < vec0.size(); i++) {
            cout << HEX32_STRING(vec0[i]) << "\n";
        }
    }

    std::string pathout = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/sim/verilator/test_dyn_fft_1/evm_0.hex";

    file_dump_vec(vec0, pathout);


    cout << "Evm starting\n";
    setup_evm();
    auto evm = handle_evm(vec0);
    cout << "Evm finished\n";

    std::string pathout2 = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/sim/verilator/test_dyn_fft_1/evm_result_0.py";

    file_dump_double_python(evm, pathout2);



    // for(int i = 0; i < 16; i++) {
    //     cout << (int)in3[i] << "\n";
    // }




    // constexpr double mag_tol = 2000;
    // double cfo_mag2;

    // while(in3.size() > 2048) {
    //     dspRunResiduePhase(in3);
    //     in3.erase(in3.begin(), in3.begin()+1024);
    // }

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

