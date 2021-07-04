#include <vector>
#include <stdint.h>
#include <stddef.h>

#define REED_SOLOMON_CODE_LENGTH 255
#define M_ROWS                   4

#define INTERLEAVE_REMOVE_KEY 0XACEF12AB

class Interleaver
{
public:
    /**
     * Interleaves a stream of bytes using block interleaving. The mechanism and
     * implementation of block interleaving is described
     * <a href="https://en.wikipedia.org/wiki/Burst_error-correcting_code#Block_interleaver">
     * here</a>. The dimension for the block interleaver is default set at
     * 255 (col) x 4 (row) if this constructor is called. Typically, the column
     * dimension is the code length of an encoded block.
     * -- 
     * This constructor will create a block interleaver where the column
     * dimension is the code length of an encoded block.
     *
     * The units of code_length / col_length are in words
     */
    Interleaver(const size_t col_length = REED_SOLOMON_CODE_LENGTH);
    ~Interleaver();

    /**
     * This method interleave a stream of bytes using a block interleaver. If
     * data stream is expressed in words, the data stream size need to
     * conform to the following logic:
     * (code_length % 4) ? ((code_length / 4) + 1) * 4 : code_length;
     *
     * @param[in,out] data_stream: Stream of data to interleave. The data will
     * be modified in-place
     * @returns True if interleaving is successful, false otherwise.
     */
    bool inplace_interleave_block(std::vector<uint32_t> &data_stream);

    /**
     * This method deinterleave a stream of bytes using a block deinterleaver.
     * If data stream is expressed in words, the data stream size need to
     * conform to the following logic:
     * (code_length % 4) ? ((code_length / 4) + 1) * 4 : code_length;
     *
     * @param[in,out] data_stream: Stream of data to deinterleave. The data will
     * be modified in-place
     * @returns True if deinterleaving is successful, false otherwise.
     */
    bool inplace_deinterleave_block(std::vector<uint32_t> &data_stream);

    bool interleave(const std::vector<uint32_t> &input, std::vector<uint32_t> &output);
    bool deinterleave(const std::vector<uint32_t> &input, std::vector<uint32_t> &output);

    /**
     * Returns true if :
     * (code_length % 4) ? ((code_length / 4) + 1) * 4 : code_length;
     */
    bool length_ok(const size_t sz) const;

    static size_t calculate_words_for_code(const size_t c_len);
    static size_t calculate_blocks_for_length(const size_t len, const size_t c_len);

    bool print_on_error = true;
    bool auto_remove_tail = true;

private:
    const size_t required_stream_size = 0;
    const size_t code_length = 0;
    std::vector<std::vector<uint8_t>> block_interleaver;

    /**
     * Load data stream into block interleaver
     *
     * @param[in,out] data_stream: Stream of data to interleave.
     * @param[in] stream_size: Data stream size. This value is determined by the
     * following logic
     *     (code_length % 4) ? ((code_length / 4) + 1) * 4 : code_length; 
     */
    void load_block_interleaver(const std::vector<uint32_t> &data_stream,
                                const size_t stream_size);

    /**
     * Load data stream into block deinterleaver
     *
     * @param[in,out] data_stream: Stream of data to deinterleave.
     */
    void load_block_deinterleaver(const std::vector<uint32_t> &data_stream);

    /**
     * Print block interleaver data
     */
    void print_block_interleaver() const;
};