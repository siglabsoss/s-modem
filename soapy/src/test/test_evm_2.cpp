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

#define EVM_TONE_NUM (32)
#define EVM_WATERMARK (EVM_FRAMES*EVM_TONE_NUM*4)

// #define EVM_CHUNK_BYTES (EVM_POINTS*EVM_TONE_NUM*4)

#define EVM_KEEP (1024)

#define EVM_DROPPING_CHUNK (EVM_KEEP*EVM_TONE_NUM)

#define EVM_INTERNAL_CHUNK (EVM_FRAMES*EVM_TONE_NUM)

#define EVM_TONE_BYTES ((EVM_TONE_NUM)*4)
// how many bytes to pull from the 
#define EVM_FRAMES (1024*4)

// how many to run at once
#define EVM_POINTS (1024)


// FIXME these are old MMSE variables that should be moved
static const unsigned int num_points = EVM_POINTS;
static constexpr unsigned int array_length = EVM_POINTS * 2;
static double input_buffer[array_length];
static double output_buffer[array_length];
static double evm_buffer[num_points];
static double eq_buffer[array_length];



// FIXME This method should be placed in a constructor
static void init_eq_buffer(double* const _eq_buffer, const unsigned int _array_length) {
    for (unsigned int i = 0; i < _array_length; i++) {
        _eq_buffer[i] = (i + 1) % 2;
    }
}




static std::vector<uint32_t> ppack (const uint32_t* const src, const unsigned int num_points, const unsigned int step) {

    unsigned int wrote = 0;
    std::vector<uint32_t> ret;
    unsigned int i = 0;
    for (; i < num_points; i += step) {
        const uint32_t x = src[i];
        // const double x_re = f64_ci32_real(x, q);
        // const double x_im = f64_ci32_imag(x, q);
        // dst[2 * i] = x_re;
        // dst[2 * i + 1] = x_im;
        ret.push_back(x);
        wrote += 2;
    }
    // std::cout << "num points " << num_points << " wrote " << wrote << "\n";
    return ret;
}

static std::vector<uint32_t> selected;

// mmse code
// see https://observablehq.com/@drom/blind-mmse
std::pair<std::vector<double>,std::vector<std::complex<double>>> handle_evm(const std::vector<uint32_t>& vec0) {




    // cout << "handle_evm got " << vec0.size() << "\n";

    const unsigned num_words = 1024 * 32;

    std::vector<double> out;
    std::vector<std::complex<double>> out2;
    // const int chunks = vec0.size() / num_points;

    const int step = EVM_TONE_NUM;
    const uint32_t q = 12;

    // for(int i = 0; i < chunks; i++ ) {
        // for( int j = 0; j < num_points; j++) {
            // const uint32_t* int_d_start = (const uint32_t*)vec0.data();

    const uint32_t* int_d_start = ((const uint32_t*)vec0.data());

    selected = ppack(int_d_start, num_words, step);

    // cout << "packa"  << endl;
    cf64_ci32_unpack_vector(int_d_start, input_buffer, q, num_words, step);
    // cout << "packb"  << endl;
    mmse_freq_vector(input_buffer, output_buffer, eq_buffer, num_points);
    // cout << "packc"  << endl;
    evm_vector(output_buffer, evm_buffer, num_points);
    // cout << "packd"  << endl;

    for (int i = 0; i < num_points; i++) {
        out.push_back(evm_buffer[i]);
        out2.emplace_back(eq_buffer[i], eq_buffer[i+1]);
        // std::cout << std::hex << "Eq Index[" << i << "]: " << eq_buffer[i] << " " << eq_buffer[i+1] << std::endl; 
        // std::cout << std::hex << "EVM Index[" << i << "]: " << evm_buffer[i] << std::endl;
    }



        // }
    // }

    // cout << "Evm ran in " << chunks << " chunks\n";

// #ifdef evmm



    // cf64_ci32_unpack_vector(int_d_start, input_buffer, q, num_points, step);
    // mmse_freq_vector(input_buffer, output_buffer, eq_buffer, num_points);
    // evm_vector(output_buffer, evm_buffer, num_points);
    // for (int i = 0; i < num_points; i++) {
    //     // std::cout << std::hex << "Eq Index[" << i << "]: " << eq_buffer[i] << " " << eq_buffer[i+1] << std::endl; 
    //     std::cout << std::hex << "EVM Index[" << i << "]: " << eq_buffer[i] << std::endl;
    // }
// #endif
    return std::pair<std::vector<double>,std::vector<std::complex<double>>>(out,out2);
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

void setup_evm() {
    init_eq_buffer(eq_buffer, array_length);
    std::cout << "Equalization vector initialized" << std::endl;
}

    




int main() {

    // cout << "hi" <<"\n";
    // return 0;
    // double gain;


    // gain = 1;
    // gain = 1;

    std::string path;
    path = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/js/test/data/evmthread_1.hex";
    // path = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/js/test/data/ideal_qpsk_1.raw";

    // path = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/save_residue3_strip.txt"

    std::vector<uint32_t> waste;
    waste = file_read_hex(path);
    
    std::vector<uint32_t> wasterot;

    double rad = 3.1415/4.0 - 0.4;
    rad = 0.4;
    rotateIShortMulti(wasterot, waste, rad);

    std::string pathout3 = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/sim/verilator/test_dyn_fft_1/rot.hex";

    file_dump_vec(wasterot, pathout3);

    



    // std::vector<std::complex<double>> ascp;
    // std::vector<std::complex<double>> rot;
    // std::vector<std::complex<double>> rotby;

    // // for(int i = 0; i < 1024; i += 32) {
    // //     cout << HEX32_STRING(waste[i]) << "\n";
    // // }

    // IShortToComplexMulti(ascp, waste);


    // const std::complex<double> s = {cos(rad),sin(rad)};

    // for(unsigned i = 0; i < ascp.size(); i++) {
    //     rotby.push_back(s);
    // }

    // complexMult(rot, ascp, rotby);

    // for(int i = 0; i < 1024; i+= 32) {
    //     auto w = rot[i];
    //     cout << "r: " << w.real() << "\n";
    // }


    // exit(0);



    // auto in3 = file_read_raw_uint8_t(path);

    // in3.resize((8)*10000);


    cout << "Evm starting\n";
    setup_evm();

    std::vector<double> result2;

    std::pair<std::vector<double>,std::vector<std::complex<double>>> result;

    for( int i = 0; i < 1; i++ ) {

        result = handle_evm(wasterot);
        cout << "Evm finished\n";

        // cout << "result size " << result.size() << "\n";

        int pp = 0;
        for(const auto d : std::get<0>(result)) {
            // if( pp % 512 == 0) {

                cout << d << "\n";
            // }
            pp++;
            result2.push_back(d);
        }

        if( i == 0 ) {
            std::string pathout4 = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/sim/verilator/test_dyn_fft_1/eq_1.hex";
            std::vector<uint32_t> eqshort;
            complexToIShortMulti(eqshort, std::get<1>(result));
            file_dump_vec(eqshort, pathout4);
        }


    }
    std::string pathout2 = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/sim/verilator/test_dyn_fft_1/evm_result_1.py";

    file_dump_double_python(result2, pathout2);



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

