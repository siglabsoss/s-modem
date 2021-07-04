/*
    Collection of usefull ComPLeX DSP functions and algorithms
*/
#define EXPORT __attribute__((visibility("default")))

#include <stdint.h>
#include <math.h>
#include "cplx.hpp"

const double qpsk_scale = 0.7071067811865476; // sqrt(2) / 2;

EXPORT
double qpsk_demod (const double a) {
    return (a > 0) ? qpsk_scale : -qpsk_scale;
}

// First Order IIR Filter
EXPORT
double iir (const double a, const double b, const double g) {
    // (1 - g) * a + g * b;
    return g * (b - a) + a;
}

EXPORT
void mmse_time_vector (const double* const src, double * dst, double * eq, const unsigned int num_points) {

    const double eq_gain = 0.01;

    double eq_re = eq[0];
    double eq_im = eq[1];

    unsigned int i = 0;
    for (; i < num_points; i++) {

        double x_re = src[2 * i];
        double x_im = src[2 * i + 1];

        // complex multiply
        double x1_re = x_re * eq_re - x_im * eq_im;
        double x1_im = x_im * eq_re + x_re * eq_im;

        double x2_re = qpsk_demod(x1_re);
        double x2_im = qpsk_demod(x1_im);

        // complex conj multiply
        double x3_re = x1_re * x2_re + x1_im * x2_im;
        double x3_im = x1_im * x2_re - x1_re * x2_im;

        // reciprocal
        double x4 = 1 / (x3_re * x3_re + x3_im * x3_im + 0.001);

        double x5_re = x4 * x3_re;
        double x5_im = x4 * -x3_im;

        eq_re = iir(eq_re, x5_re, eq_gain);
        eq_im = iir(eq_im, x5_im, eq_gain);

        // complex multiply
        double x6_re = x1_re * eq_re - x1_im * eq_im; // old EQ ???
        double x6_im = x1_im * eq_re + x1_re * eq_im;

        dst[2 * i] = x6_re;
        dst[2 * i + 1] = x6_im;
    }

    eq[0] = eq_re;
    eq[1] = eq_im;

}

EXPORT
void mmse_freq_vector (const double* const src, double * dst, double * eq, const unsigned int num_points) {
    const double eq_gain = 0.01;

    unsigned int i = 0;
    for (; i < num_points; i++) {

        const double x_re = src[2 * i];
        const double x_im = src[2 * i + 1];

        double eq_re = eq[2 * i];
        double eq_im = eq[2 * i + 1];

        // complex multiply
        const double x1_re = x_re * eq_re - x_im * eq_im;
        const double x1_im = x_im * eq_re + x_re * eq_im;

        const double x2_re = qpsk_demod(x1_re);
        const double x2_im = qpsk_demod(x1_im);

        // complex conj multiply
        const double x3_re = x1_re * x2_re + x1_im * x2_im;
        const double x3_im = x1_im * x2_re - x1_re * x2_im;

        // reciprocal
        const double x4 = 1 / (x3_re * x3_re + x3_im * x3_im + 0.001);

        const double x5_re = x4 * x3_re;
        const double x5_im = x4 * -x3_im;

        eq_re = iir(eq_re, x5_re, eq_gain);
        eq_im = iir(eq_im, x5_im, eq_gain);

        // complex multiply
        const double x6_re = x1_re * eq_re - x1_im * eq_im; // old EQ ???
        const double x6_im = x1_im * eq_re + x1_re * eq_im;

        eq[2 * i] = eq_re;
        eq[2 * i + 1] = eq_im;

        dst[2 * i] = x6_re;
        dst[2 * i + 1] = x6_im;
    }
}

EXPORT
void cmul_vector (const double* const src, double* dst, const double re, const double im, const unsigned int num_points) {

    unsigned int i = 0;
    for (; i < num_points; i++) {

        double x_re = src[2 * i];
        double x_im = src[2 * i + 1];

        double x2_re = x_re * re - x_im * im;
        double x2_im = x_im * re + x_re * im;

        dst[2 * i] = x2_re;
        dst[2 * i + 1] = x2_im;
    }
}

EXPORT
void conj_vector (const double* const src, double * dst, const double re, const double im, const unsigned int num_points) {

    unsigned int i = 0;
    for (; i < num_points; i++) {

        double x_re = src[2 * i];
        double x_im = src[2 * i + 1];

        double x2_re = x_re * re + x_im * im;
        double x2_im = x_im * re - x_re * im;

        dst[2 * i] = x2_re;
        dst[2 * i + 1] = x2_im;
    }
}

EXPORT
void evm_vector (const double* const data, double * evm, const unsigned int num_points) {

    const double filter_gain = 0.01;

    unsigned int i = 0;
    for (; i < num_points; i++) {
        const double data_re = data[2 * i];
        const double data_im = data[2 * i + 1];

        const double a_re = fabs(data_re) - qpsk_scale;
        const double a_im = fabs(data_im) - qpsk_scale;

        const double evm_now = (a_re * a_re) + (a_im * a_im);

        const double evm_update = iir(evm[i], evm_now, filter_gain);

        evm[i] = evm_update;
    }
}

// complex unpack
EXPORT
double f64_ci32_imag (const uint32_t x, const uint32_t q) {
    uint32_t qq = (1 << q);
    double qq1 = qq;
    int32_t x1 = x;
    int32_t x2 = x1 >> 16;
    double x3 = x2;
    double x4 = x3 / qq1;
    return x4;
}

EXPORT
double f64_ci32_real (const uint32_t x, const uint32_t q) {
    uint32_t x1 = x << 16;
    double x2 = f64_ci32_imag(x1, q);
    return x2;
}

/// Step is the offset into the input buffer
/// The output step is always 1
///
EXPORT
void cf64_ci32_unpack_vector (const uint32_t* const src, double * dst, const uint32_t q, const unsigned int num_points, const unsigned int step) {
    unsigned int i = 0;
    unsigned int writeout = 0;
    for (; i < num_points; i += step,writeout++) {
        const uint32_t x = src[i];
        const double x_re = f64_ci32_real(x, q);
        const double x_im = f64_ci32_imag(x, q);
        dst[2 * writeout] = x_re;
        dst[2 * writeout + 1] = x_im;
    }
}

EXPORT
uint32_t ci32_cf64_pack (const double real, const double imag, const uint32_t q) {
    const uint32_t qq = (1 << q);
    const double qq1 = qq;
    const double real1 = real * qq1;
    const double imag1 = imag * qq1;

    // add saturation logic ?

    const int32_t real2 = real1;
    const int32_t imag2 = imag1;

    const uint32_t real3 = real2 & 0xffff;
    const uint32_t imag3 = imag2 << 16;

    const uint32_t res = imag3 | real3;
    return res;
}

EXPORT
void ci32_cf64_pack_vector (const double* const src, uint32_t * dst, const uint32_t q, const unsigned int num_points) {
    unsigned int i = 0;
    for (; i < num_points; i++) {
        const double x_re = src[2 * i];
        const double x_im = src[2 * i + 1];
        const uint32_t x = ci32_cf64_pack(x_re, x_im, q);
        dst[i] = x;
    }
}

// // ci32 unpack
//
// EXPORT
// int32_t i32_ci32_imag (uint32_t x) {
//   int32_t x1 = x;
//   int32_t x2 = x1 >> 16;
//   return x2;
// }
//
// EXPORT
// int32_t i32_ci32_real (uint32_t x) {
//   uint32_t x1 = x << 16;
//   int32_t x2 = i32_ci32_imag(x1);
//   return x2;
// }
//


// // ci32 pack
//
// EXPORT
// uint32_t ci32_i32_pack (int32_t real, int32_t imag) {
//   // add saturation logic ?
//   uint32_t real1 = real & 0xffff;
//   uint32_t imag1 = imag & 0xffff;
//   uint32_t imag2 = imag1 << 16;
//   return imag2 | real1;
// }
//
// EXPORT
// uint32_t ci32_mul (uint32_t x, uint32_t y) {
//   int32_t a = i32_ci32_real(x);
//   int32_t b = i32_ci32_imag(x);
//   int32_t c = i32_ci32_real(y);
//   int32_t d = i32_ci32_imag(y);
//
//   int32_t res_real = a * c - b * d;
//   int32_t res_imag = a * d - b * c;
//   uint32_t res = ci32_i32_pack(res_real, res_imag);
//   return res;
// }
//
// EXPORT


// EXPORT
// uint32_t ci32_ci32_cmul_f64 (uint32_t x, uint32_t y) {
//   double a = f64_ci32_real(x, 0);
//   double b = f64_ci32_imag(x, 0);
//   double c = f64_ci32_real(y, 0);
//   double d = f64_ci32_imag(y, 0);
//
//   double res_real = a * c - b * d;
//   double res_imag = a * d - b * c;
//   uint32_t res = ci32_f64_pack(res_real, res_imag, 0);
//   return res;
// }
