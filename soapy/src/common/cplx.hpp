#pragma once

double qpsk_demod (const double a);
double iir (const double a, const double b, const double g);
void mmse_time_vector (const double* const src, double * dst, double * eq, const unsigned int num_points);
void mmse_freq_vector (const double* const src, double * dst, double * eq, const unsigned int num_points);
void cmul_vector (const double* const src, double * dst, const double re, const double im, const unsigned int num_points);
void conj_vector (const double* const src, double * dst, const double re, const double im, const unsigned int num_points);
void evm_vector (const double* const data, double * evm, const unsigned int num_points);
double f64_ci32_imag (const uint32_t x, const uint32_t q);
double f64_ci32_real (const uint32_t x, const uint32_t q);
void cf64_ci32_unpack_vector (const uint32_t* const src, double * dst, const uint32_t q, const unsigned int num_points, const unsigned int step);
uint32_t ci32_cf64_pack (const double real, const double imag, const uint32_t q);
void ci32_cf64_pack_vector (const double* const src, uint32_t * dst, const uint32_t q, const unsigned int num_points);