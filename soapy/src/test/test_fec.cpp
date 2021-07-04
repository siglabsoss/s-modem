#include <rapidcheck.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "cpp_utils.hpp"
#include "random.h"
#include "tests/noise.hpp"
#include "turbofec/turbo.hpp"
#define __ENABLED

#define MAX_LEN_BITS        32768
#define MAX_LEN_BYTES       (32768/8)
#define DEFAULT_NUM_PKTS    1000
#define DEFAULT_ITER        4
#define DEFAULT_THREADS     1
#define MAX_THREADS     32

/* Maximum LTE code block size of 6144 */
#define LEN     TURBO_MAX_K

/*
 * Parameters for soft symbol generation
 *
 * Signal-to-noise ratio specified in dB and symbol amplitude,
 * which has a valid range from 0 (no signal) to 127 (saturation).
 */
#define DEFAULT_SNR 8.0
#define DEFAULT_AMP 32.0

/*
 * Command line arguments
 *
 * iter     - Number of iterations
 * threads  - Number of concurrent threads to launch for benchmark test
 * bench    - Enable the benchmarking test
 * length   - Enable length checks
 * ber      - Enable the bit-error-rate test
 * num      - Run code number if specified 
 */
struct cmd_options {
    int num_pkts;
    int iter;
    int threads;
    int bench;
    int length;
    int num;
    int ber;
    float snr;
};

/* Argument passing struct for benchmark threads */
struct benchmark_thread_arg {
    const struct lte_test_vector *test;
    const struct lte_turbo_code *code;
    struct tdecoder *tdec;
    int base;
    int num_pkts;
    int iter;
    int err;
};

const struct lte_turbo_code lte_turbo = {
    .n = 2,
    .k = 4,
    .len = LEN,
    .rgen = 013,
    .gen = 015,
};

struct lte_test_vector {
    const char *name;
    const char *spec;
    const struct lte_turbo_code *code;
    int in_len;
    int out_len;
};

const struct lte_test_vector tests[] = {
    {
        .name = "3GPP LTE turbo",
        .spec = "(N=2, K=4)",
        .code = &lte_turbo,
        .in_len  = LEN,
        .out_len = LEN * 3 + 4 * 3,
    },
    { /* end */ },
};

static void print_codes()
{
    int i = 1;
    const struct lte_test_vector *test;

    printf("\n");
    printf("Code  0:  Test all codes\n");
    for (test=tests; test->name; test++)
        printf("Code %2i:  %-18s %s\n", i++, test->name, test->spec);
    printf("\n");
}

static void fill_random(uint8_t *b, int n)
{
    int i, r, m, c;

    c = 0;
    r = rand();
    m = sizeof(int) - 1;

    for (i = 0; i < n; i++) {
        if (c++ == m) {
            r = rand();
            c = 0;
        }

        b[i] = (r >> (i % m)) & 0x01;
    }
}

/* Generate soft bits with ~2.3% crossover probability */
static int uint8_to_err(int8_t *dst, uint8_t *src, int n, float snr)
{
    int i, err = 0;

    add_noise(src, dst, n, snr, DEFAULT_AMP);

    for (i = 0; i < n; i++) {
        if ((!src[i] && (dst[i] >= 0)) || (src[i] && (dst[i] <= 0)))
            err++;
    }

    return err;
}

/* Output error input/output error rates */
static void print_error_results(const struct lte_test_vector *test,
                int iber, int ober, int fer, int num_pkts)
{
    printf("[..] Input BER.......................... %f\n",
           (float) iber / (num_pkts * test->out_len));
    printf("[..] Output BER......................... %f\n",
           (float) ober / (num_pkts * test->in_len));
    printf("[..] Output FER......................... %f\n",
           (float) fer / num_pkts);
}

/* Timed performance benchmark */
static double get_timed_results(struct timeval *tv0, struct timeval *tv1,
                    const struct lte_test_vector *test,
                int num_pkts, int threads)
{
    double elapsed;

    elapsed = (tv1->tv_sec - tv0->tv_sec);
    elapsed += (tv1->tv_usec - tv0->tv_usec) / 1e6;
    printf("[..] Elapsed time....................... %f secs\n", elapsed);
    printf("[..] Rate............................... %f Mbps\n",
           (float) threads * test->in_len * num_pkts / elapsed / 1e6);

    return elapsed;
}

/* Bit error rate test */
static int error_test(const struct lte_test_vector *test,
              int num_pkts, int iter, float snr)
{
    int i, n, l, iber = 0, ober = 0, fer = 0;
    int8_t *bs0, *bs1, *bs2;
    uint8_t *in, *bu0, *bu1, *bu2;

    in  = (uint8_t *) malloc(sizeof(uint8_t) * MAX_LEN_BITS);
    bu0 = (uint8_t *) malloc(sizeof(uint8_t) * MAX_LEN_BITS);
    bu1 = (uint8_t *) malloc(sizeof(uint8_t) * MAX_LEN_BITS);
    bu2 = (uint8_t *) malloc(sizeof(uint8_t) * MAX_LEN_BITS);
    bs0 = (int8_t *) malloc(sizeof(int8_t) * MAX_LEN_BITS);
    bs1 = (int8_t *) malloc(sizeof(int8_t) * MAX_LEN_BITS);
    bs2 = (int8_t *) malloc(sizeof(int8_t) * MAX_LEN_BITS);

    struct tdecoder *tdec = alloc_tdec();

    for (i = 0; i < num_pkts; i++) {
        fill_random(in, test->in_len);
        l = lte_turbo_encode(test->code, in, bu0, bu1, bu2);
        if (l != test->out_len) {
            printf("ERROR !\n");
            fprintf(stderr, "[!] Failed encoding length check (%i)\n",
                l);
            return -1;
        }

        iber += uint8_to_err(bs0, bu0, LEN + 4, snr);
        iber += uint8_to_err(bs1, bu1, LEN + 4, snr);
        iber += uint8_to_err(bs2, bu2, LEN + 4, snr);

        lte_turbo_decode_unpack(tdec, LEN, iter, bu0, bs0, bs1, bs2);

            for (n = 0; n < test->in_len; n++) {
            if (in[n] != bu0[n])
                ober++;
        }

        if (memcmp(in, bu0, test->in_len))
            fer++;
    }

    print_error_results(test, iber, ober, fer, num_pkts);

    free(in);
    free(bs0);
    free(bs1);
    free(bs2);
    free(bu0);
    free(bu1);
    free(bu2);

    return 0;
}

static int init_thread_arg(struct benchmark_thread_arg *arg,
               const struct lte_test_vector *test,
               int num_pkts, int iter)
{
    arg->test = test;
    arg->num_pkts = num_pkts;
    arg->iter = iter;
    arg->code = test->code;
    arg->tdec = alloc_tdec();
    arg->err = 0;

    return 0;
}

/* One benchmark benchmark thread with random valued input */
static void *thread_test(void *ptr)
{
    int i;
    int8_t *bs0, *bs1, *bs2;
    uint8_t *in, *bu0, *bu1, *bu2;
    struct benchmark_thread_arg *arg = (struct benchmark_thread_arg *) ptr;

    in  = (uint8_t *) malloc(sizeof(uint8_t) * MAX_LEN_BITS);
    bu0 = (uint8_t *) malloc(sizeof(uint8_t) * MAX_LEN_BITS);
    bu1 = (uint8_t *) malloc(sizeof(uint8_t) * MAX_LEN_BITS);
    bu2 = (uint8_t *) malloc(sizeof(uint8_t) * MAX_LEN_BITS);
    bs0 = (int8_t *) malloc(sizeof(int8_t) * MAX_LEN_BITS);
    bs1 = (int8_t *) malloc(sizeof(int8_t) * MAX_LEN_BITS);
    bs2 = (int8_t *) malloc(sizeof(int8_t) * MAX_LEN_BITS);

    for (i = 0; i < arg->num_pkts; i++) {
        lte_turbo_decode_unpack(arg->tdec, arg->code->len,
                    arg->iter, bu0, bs0, bs1, bs2);
    }

    free(in);
    free(bs0);
    free(bs1);
    free(bs2);
    free(bu0);
    free(bu1);
    free(bu2);

    pthread_exit(NULL);
}

/* Fire off benchmark threads and measure elapsed time */
static double run_benchmark(const struct lte_test_vector *test,
                struct benchmark_thread_arg *args,
                int num_threads, int num_pkts, int iter)
{
    int i, rc, err = 0;
    void *status;
    struct timeval tv0, tv1;
    pthread_t threads[MAX_THREADS];

    for (i = 0; i < num_threads; i++) {
        rc = init_thread_arg(&args[i], test, num_pkts, iter);
        if (rc < 0)
            return -1.0;
    }

    gettimeofday(&tv0, NULL);
    for (i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL,
                   thread_test, (void *) &args[i]);
    }
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], &status);
        err |= args[i].err;
    }
    gettimeofday(&tv1, NULL);

    if (err)
        return -1.0;

    return get_timed_results(&tv0, &tv1, test, num_pkts, num_threads);
}

/* Verify output lengths */
static int length_test(const struct lte_test_vector *test)
{
    return 0;
}

static void print_help()
{
    fprintf(stdout, "Options:\n"
        "  -h    This text\n"
        "  -p    Number of packets (per thread)\n"
        "  -j    Number of threads for benchmark\n"
        "  -i    Number of turbo iterations\n"
        "  -a    Run all tests\n"
        "  -b    Run benchmark tests\n"
        "  -n    Run length checks\n"
                "  -r    Specify SNR in dB (default %2.1f dB)\n"
        "  -e    Run bit error rate tests\n"
        "  -c    Test specific code\n"
        "  -l    List supported codes\n", DEFAULT_SNR);
}

static void handle_options(int argc, char **argv, struct cmd_options *cmd)
{
    int option;

    cmd->num_pkts = DEFAULT_NUM_PKTS;
    cmd->threads = DEFAULT_THREADS;
    cmd->bench = 0;
    cmd->length = 0;
    cmd->ber = 0;
    cmd->num = 0;
    cmd->iter = DEFAULT_ITER;
    cmd->snr = DEFAULT_SNR;

    while ((option = getopt(argc, argv, "hi:bp:aesc:r:lj:")) != -1) {
        switch (option) {
        case 'h':
            print_help();
            exit(0);
            break;
        case 'p':
            cmd->num_pkts = atoi(optarg);
            if (cmd->num_pkts < 1) {
                printf("Number of packets must be at least 1\n");
                exit(0);
            }
            break;
        case 'a':
            cmd->bench = 1;
            cmd->length = 1;
            cmd->ber = 1;
            break;
        case 'b':
            cmd->bench = 1;
            break;
        case 'n':
            cmd->length = 1;
            break;
        case 'e':
            cmd->ber = 1;
            break;
        case 'c':
            cmd->num = atoi(optarg);
            if (cmd->num < 0) {
                print_codes();
                exit(0);
            }
            break;
        case 'r':
            cmd->snr = atof(optarg);
            break;
        case 'l':
            print_codes();
            exit(0);
            break;
        case 'i':
            cmd->iter = atoi(optarg);
            if (cmd->iter < 1) {
                printf("Turbo iterations must be at least 1\n");
                exit(0);
            }
            break;
        case 'j':
            cmd->threads = atoi(optarg);
            if ((cmd->threads < 1) ||
                (cmd->threads > MAX_THREADS)) {
                printf("Threads must be between 1 to %i\n",
                       MAX_THREADS);
                exit(0);
            }
            break;
        default:
            print_help();
            exit(0);
        }
    }

    if (!cmd->bench && !cmd->length && !cmd->ber) {
        cmd->length = 1;
        cmd->ber = 1;
    }
}

void print_buffer(uint8_t *input_buffer, int buffer_length) {
    for (int i = 0; i < buffer_length; i++) {
        std::cout << "Buffer index " << i << " with value " 
                  << unsigned(input_buffer[i]) << std::endl;
    }
}

void fill_cooked_data(uint8_t *input_buffer, int data_length) {
    uint32_t deadbeef = 0xdeadbeef;
    for (int i = 0; i < data_length; i++) {
        input_buffer[i] = (deadbeef >> ((i % 8) * 4)) & (0xf);
    }
}
int main(int argc, char *argv[]) {

    std::cout << std::hex;
    std::cout << "This file test Turbo Forward Error Correction (FEC)"
              << std::endl;

    int returned_len;
    const struct lte_test_vector *test_vector;
    uint8_t *input_buffer;
    uint8_t *out_buffer_0;
    uint8_t *out_buffer_1;
    uint8_t *out_buffer_2;

    /* Allocating input buffer */
    input_buffer = (uint8_t *) malloc(sizeof(uint8_t) * MAX_LEN_BITS);
    out_buffer_0 = (uint8_t *) malloc(sizeof(uint8_t) * MAX_LEN_BITS);
    out_buffer_1 = (uint8_t *) malloc(sizeof(uint8_t) * MAX_LEN_BITS);
    out_buffer_2 = (uint8_t *) malloc(sizeof(uint8_t) * MAX_LEN_BITS);

    test_vector = tests;
    // fill_random(input_buffer, test_vector->in_len);
    fill_cooked_data(input_buffer, test_vector->in_len);
    print_buffer(input_buffer, test_vector->in_len);
    returned_len = lte_turbo_encode(test_vector->code,
                                    input_buffer,
                                    out_buffer_0,
                                    out_buffer_1,
                                    out_buffer_2);
    if (returned_len == test_vector->out_len) {
        std::cout << "Input buffer of size " << test_vector->in_len
                  << " successfully encoded. Output has size: "
                  << returned_len << std::endl;
    }

    return 0;
}
