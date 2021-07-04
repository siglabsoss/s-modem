#include <cstring>
#include <set>
#include "schifra_galois_field.hpp"
#include "schifra_galois_field_polynomial.hpp"
#include "schifra_sequential_root_generator_polynomial_creator.hpp"
#include "schifra_reed_solomon_encoder.hpp"
#include "schifra_reed_solomon_decoder.hpp"
#include "schifra_error_processes.hpp"
#include "schifra_reed_solomon_bitio.hpp"

#define CODE_LEN_255     255
#define FEC_LEN_32       32
#define FEC_LEN_48       48
#define FEC_LEN_64       64
#define FEC_LEN_80       80
#define FEC_LEN_96       96
#define FEC_LEN_128      128
#define FIELD_DESCRIPTOR 8
#define RS_REMOVE_KEY       0XCAFEBA00

class ReedSolomon
{
public:
    std::size_t field_descriptor = 8;
    std::size_t generator_polynomial_index = 120;
    std::size_t generator_polynomial_root_count = 32;
    std::size_t code_length = 255;
    std::size_t fec_length = 32;
    const std::size_t data_length;
    bool print_key = true;
    schifra::galois::field field;
    schifra::galois::field_polynomial generator_polynomial;
    schifra::reed_solomon::encoder<255, 32, 223> *encoder_255_223_32;
    schifra::reed_solomon::decoder<255, 32, 223> decoder_255_223_32;
    schifra::reed_solomon::encoder<255, 48, 207> *encoder_255_207_48;
    schifra::reed_solomon::decoder<255, 48, 207> decoder_255_207_48;
    schifra::reed_solomon::encoder<255, 64, 191> *encoder_255_191_64;
    schifra::reed_solomon::decoder<255, 64, 191> decoder_255_191_64;
    schifra::reed_solomon::encoder<255, 80, 175> *encoder_255_175_80;
    schifra::reed_solomon::decoder<255, 80, 175> decoder_255_175_80;
    schifra::reed_solomon::encoder<255, 96, 159> *encoder_255_159_96;
    schifra::reed_solomon::decoder<255, 96, 159> decoder_255_159_96;
    schifra::reed_solomon::encoder<255, 128, 127> *encoder_255_127_128;
    schifra::reed_solomon::decoder<255, 128, 127> decoder_255_127_128;
    __attribute__((deprecated)) ReedSolomon();
    ReedSolomon(const std::size_t field_descriptor,
                const std::size_t generator_polynomial_index,
                const std::size_t generator_polynomial_root_count,
                const std::size_t code_length,
                const std::size_t fec_length);
    ReedSolomon(const std::size_t code_length,
                const std::size_t fec_length);
    ~ReedSolomon();

    /**
     * This functions encodes a block of data (255 bytes) given a valid code and
     * FEC length. At the time of writing, the accepted code length is 255 and
     * the accepted FEC length is 32 or 48. To encode more than one block of
     * data, execute encode_message instead.
     *
     * @param[in] in_message: Vector of words to encode. If code and FEC length
     * is 255 and 32 bytes respectively, the maximum words within in_message
     * should be (255 - 32)/4 = 55 words
     * @param[out] out_message: Empty vector to store encoded message
     * @param[in] code_length: Code length of encoded data. Currently, only 255
     * is supported
     * @param[in] fec_length: FEC length of encoded data. Currently, only 32 and
     * 48 are supported
     * @param[in] puncture: Set true to puncture encoded block. The specific
     * bytes to puncture are set by puncture_mask
     * @returns A boolean of whether block encoding was successful 
     */
    bool encode_block(std::vector<uint32_t> &in_message,
                      std::vector<uint32_t> &out_message,
                      std::size_t code_length,
                      std::size_t fec_length,
                      bool puncture=false);

    /**
     * This functions encodes a standard vector given a valid code and FEC
     * length. At the time of writing, the accepted code length is 255 and the
     * accepted FEC length is 32 or 48.
     *
     * @param[in] in_message: Vector of words to encode. If the size of vector
     * is not an even multiple of (code_length - fec_length)/4, it will be zero
     * padded
     * @param[out] out_message: Empty vector to store encoded message
     * @param[in] code_length: Code length of encoded data. Currently, only 255
     * is supported
     * @param[in] fec_length: FEC length of encoded data. Currently, only 32 and
     * 48 are supported
     * @param[in] puncture: Set true to puncture encoded block. The specific
     * bytes to puncture are set by puncture_mask
     * @returns A boolean of whether message encoding was successful 
     */
    bool encode_message(const std::vector<uint32_t> &in_message,
                        std::vector<uint32_t> &out_message,
                        std::size_t code_length,
                        std::size_t fec_length,
                        bool puncture=false);

    /**
     * Decodes a block of data (255 bytes) encapsulated in a standard vector.
     * An encoded standard vector is usually marked by parity bits at the end of
     * the vector. To decode more than one block of data, execute decode_message
     * instead.
     *
     * @param[in] encoded_vector: Vector of encoded data. Size should be 64
     * words
     * @param[out] decoded_vector: Empty vector to store decoded data
     * @param[in] code_length: Code length of encoded data. Currently, only 255
     * is supported
     * @param[in] fec_length: FEC length of encoded data. Currently, only 32 and
     * 48 are supported
     * @param[in] puncture: Set true to replace punctured bytes with zero
     * @returns A boolean of whether block decoding was successful
     */
    bool decode_block(std::vector<uint32_t> &encoded_vector,
                      std::vector<uint32_t> &decoded_vector,
                      const std::size_t code_length,
                      const std::size_t fec_length,
                      const bool puncture=false);

    /**
     * Decodes an encoded standard vector. An encoded standard vector is usually
     * marked by parity bits at the end of the vector.
     *
     * @param[in] encoded_vector: Vector of encoded data.
     * @param[out] decoded_vector: Empty vector to store decoded data
     * @param[in] code_length: Code length of encoded data. Currently, only 255
     * is supported
     * @param[in] fec_length: FEC length of encoded data. Currently, only 32 and
     * 48 are supported
     * @param[in] puncture: Set true to replace punctured bytes with zero
     * @returns always true FIXME
     */
    bool decode_message(std::vector<uint32_t> &encoded_vector,
                        std::vector<uint32_t> &decoded_vector,
                        const std::size_t code_length,
                        const std::size_t fec_length,
                        const bool puncture=false);

    std::vector<std::pair<uint32_t,uint32_t>> decode_message2(std::vector<uint32_t> &encoded_vector,
                        std::vector<uint32_t> &decoded_vector,
                        const std::size_t code_length,
                        const std::size_t fec_length,
                        const bool puncture=false);

    /**
     * Prints a standard container onto terminal for debugging purposes
     *
     * @param[in] input_data: Container of data to print onto terminal
     */
    template <typename T>
    void print_container(const T &input_data) {
        typename T::const_iterator it = input_data.begin();
        typename T::const_iterator end = input_data.end();
        std::cout << "[";
        for (; it != end; ++it) {
            if (it == --input_data.end()) {
                std::cout << *it;
            } else {
                std::cout << *it << ", ";
            }
        }
        std::cout << "] " << "(len = " << input_data.size() << ")\n";
    };

    /**
     * Set mask to determine which bytes to puncture in encoded block
     *
     * @param[in] mask: A standard vector with values indicating the position of
     * bytes to puncture
     */
    void set_puncture_mask(std::set<uint32_t> &mask);


    static bool settings_valid(const std::size_t code_length, const std::size_t fec_length);

private:
    std::set<uint32_t> puncture_mask;
    /**
     * Convert data from a standard vector to string representation
     *
     * @param[in] vector_message: Vector of words to convert into string
     * representation
     * @returns A string representing data from input vector
     */
    std::string vector_to_str(std::vector<uint32_t> &vector_message);

    /**
     * Convert a schifra block data to standard vector
     *
     * @param[out] output: Empty vector to place encoded data
     * @param[in] block: A schifra block with encoded data
     * @param[in] code_length: Code length
     */
    template <typename T>
    void schifra_block_to_vector(std::vector<uint32_t> &output,
                                 const T &block, std::size_t code_length);

    /**
     * Convert data in a vector to a schifra block. This step is used in
     * decoding. Data from an encoded vector is transferred to a schifra block
     * to enable decoding
     *
     * @param[in] in_vector: Vector with encoded data
     * @param[out] block: An empty schifra block to place data
     * @param[in] code_length: Code length
     */
    template <typename T>
    static void schifra_vector_to_block(const std::vector<uint32_t> &in_vector, T &block,
                                        const std::size_t code_length);

    /**
     * Creates a sequential root generator object needed to instantiate
     * Reed Solomon encoder
     */
    void create_root_generator(bool print=false);

    /**
     * Convert data from a string to vector representation
     *
     * @param[in] str_message: String message to convert to vector
     * representation
     * @param[out] out_vector: Ouput vector with string data
     */
    static void str_to_vector(const std::string str_message,
                              std::vector<uint32_t> &out_vector);

    /**
     * Print Reed Solomon data block
     *
     * @param[in] block: Reed Solomon data block
     * @param[in] code_length: Code length
     */
    template <typename T>
    void print_block(const T &block, std::size_t code_length);

    /**
     * Resizes input message so that the length is a multiple of the maximum
     * amount of words you can encode at a time. For example, if the maximum
     * amount of words you can encode at a time is 55, and the length of the
     * input message is 65, the input message will be resized to 110 with the
     * tail zero padded. The last element in the vector will indicate how many
     * zeros were added.
     *
     * @param[in,out] in_message: Input message
     * @param[in] max_word: Maximum amount of words RS can encode at once. This
     * value depends on the the code and FEC length
     * @param[in] encode_count: The amount of times needed to fully encode input
     * message
     */
    void resize_in_message(std::vector<uint32_t> &in_message,
                           std::size_t max_word,
                           std::size_t encode_count);

    /**
     * Takes a schifra block and puncture random bytes set by the puncture_mask
     * member variable. Puncturing can potentially increase the data rate.
     *
     * @param[in,out] block: A schifra block with encoded data
     * @param[in] code_length: Code length
     */
    template <typename T>
    void puncture_block(T &block, std::size_t code_length);

    /**
     * This method encodes a string message and output the results in a standard
     * vector. If punturing is enabled, the encoded message will be punctured
     * set by the puncture_mask member variable.
     *
     * @param[in] encoder: A pointer to a specific member encoder. Currently,
     * only two encoders are available where the difference is the FEC length
     * being 28 and 32 bytes.
     * @param[in] message: Message to encode in string format
     * @param[out] block: Empty schifra block to hold encoded data
     * @param[in] puncture: Set true to puncture encoded data
     * @param[in] code_length: Code length
     * @param[out] out_message: Empty vector to hold encoded data
     * @returns A boolean of whether encoding was successful or not.
     */
    template <typename T, typename U>
    bool encode(T *encoder,
                std::string message,
                U &block,
                bool puncture,
                std::size_t code_length,
                std::vector<uint32_t> &out_message);

    /**
     * This method decodes an encoded vector and output the results in a
     * standard vector. If puncturing is enabled, the punctured bytes will be
     * replaced with zeros.
     *
     * @param[in] decoder: A specific member decoder. Currently, only two
     * decoders are available where the difference is the FEC length being 28
     * and 32 bytes.
     * @param[in] encoded_vector: A vector with encoded data
     * @param[out] received_block: Empty schifra block to hold encoded data
     * @param[in] puncture: Set true to puncture encoded data
     * @param[in] code_length: Code length
     * @param[in] fec_length: FEC length
     * @param[out] decoded_vector: Empty vector to hold decoded data
     * @returns A boolean of whether decoding was successful or not.
     */
    template <typename T, typename U>
    static bool decode(T &decoder,
                const std::vector<uint32_t> &encoded_vector,
                U &received_block,
                const bool puncture,
                const std::size_t code_length,
                const std::size_t fec_length,
                std::vector<uint32_t> &decoded_vector);
};