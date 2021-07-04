#include <rapidcheck.h>
// #include "driver/StatusCode.hpp"
#include "3rd/json.hpp"



#include <vector>
#include <algorithm>
#include <iostream>
#include <unistd.h> // for usleep
#include <vector>
#include <cmath>
#include <complex>
#include <iostream>
#include <eigen3/Eigen/Core>
#include <eigen3/unsupported/Eigen/FFT>
#include <boost/multiprecision/cpp_dec_float.hpp>

using namespace std;


using boost::multiprecision::cpp_dec_float_50;


void twoSinTest() {
    const double pi = 2.*std::acos(0.);
    double Fs = 1000;            // Sampling frequency
    double T = 1 / Fs;           // Sampling period
    int L = 1000;               // Length of signal
    std::vector<double> timebuf(L);
    typedef std::complex<double> C;
    std::vector<C> freqbuf;
    
    double t = 0;
    for (int i = 0; i < L; i++) {
        // time domain has 2 sin waves
        timebuf[i] = 0.7*std::sin(2 * pi * 50 * t) + std::sin(2 * pi * 120 * t);
        t += T;
    }

    Eigen::FFT<double> fft;
    fft.fwd(freqbuf, timebuf);
    cout << freqbuf.size() << endl;

    for (int i = 0; i < freqbuf.size(); i++) {
        const C &f = freqbuf[i];
        // cout << i << " " << f << endl;
        cout << f << ",";
    }
    cout << endl;
} 



int main() {

    twoSinTest();

    return 0;
}