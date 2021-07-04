#include <rapidcheck.h>
#include <queue>
#include <circular_buffer_pow2.h>

#define CIRCULAR_BUFFER_SIZE (64)

class CircularBuffer {
public:
    std::size_t buffer_size;
    std::queue<uint32_t> circular_buffer;

    /**
     * Creates a circular buffer of size 64 words.
     */
    CircularBuffer();

    /**
     * Creates a circular buffer with specified size
     *
     * @param[in] size: Initialize size of circular buffer
     */
    CircularBuffer(std::size_t size);
    ~CircularBuffer();

    /**
     * Pops data off the circular buffer
     *
     * @params[in,out] data: A pointer to return popped data
     * @returns An int of whether operation was successful. -1 if not successful
     * and 0 if successful
     */
    int pop(uint32_t *data);

    /**
     * Push data into the circular buffer
     *
     * @param[in] value: Data to push onto circular buffer
     * @returns An int of whether operation was successful. -1 if not successful
     * and 0 if successful
     */
    int push(uint32_t value);

    /**
     * Look at first element in circular buffer
     *
     * @param[in,out] data: A pointer to return peeked value
     * @returns An int of whether operation was successful. -1 if not successful
     * and 0 if successful
     */
    int peek(uint32_t *data);
};

/**
 * This method test if the circular buffer is working correctly
 */
void test_circular_buffer();

int main(int argc, char const *argv[])
{
    test_circular_buffer();
    return 0;
}

CircularBuffer::CircularBuffer() {
    buffer_size = CIRCULAR_BUFFER_SIZE;
}

CircularBuffer::CircularBuffer(std::size_t size) {
    buffer_size = size;
}

CircularBuffer::~CircularBuffer() {

}

int CircularBuffer::pop(uint32_t *data) {
    if (circular_buffer.empty()) {
        return -1;
    } else {
        *data = circular_buffer.front();
        circular_buffer.pop();
        return 0;
    }
}

int CircularBuffer::push(uint32_t value) {
    if (circular_buffer.size() < buffer_size) {
        circular_buffer.push(value);
        return 0;
    }

    return -1;
}

int CircularBuffer::peek(uint32_t *data) {
    if (circular_buffer.empty()) {
        return -1;
    } else {
        *data = circular_buffer.front();
        return 0;
    }
    
}

void test_circular_buffer() {
    rc::check("Test Circular Buffer", []() {
        bool push;
        uint32_t push_value;
        int circular_success;
        int ring_success;
        uint32_t *circular_value = new uint32_t;
        unsigned int *ring_value = new unsigned int; 
        uint32_t push_counter = 0;
        uint32_t pop_counter = 0;
        std::size_t operation_count = *rc::gen::inRange(0u, 0xfffu);
        CircularBuffer circular_buffer;
        circular_buf_pow2_t __fb_ring_queue = \
        CIRBUF_POW2_STATIC_CONSTRUCTOR(__fb_ring_queue, CIRCULAR_BUFFER_SIZE);
        circular_buf_pow2_t *fb_ring_queue = &__fb_ring_queue;
        circular_buf2_initialize(\
                &__fb_ring_queue,
                CIRBUF_POW2_STORAGE_NAME(__fb_ring_queue),
                ARRAY_SIZE(CIRBUF_POW2_STORAGE_NAME(__fb_ring_queue)));

        for (std::size_t i = 0; i < operation_count; i++) {
            push = *rc::gen::inRange(0, 2);
            circular_success = 0xdeadbeef;
            *circular_value = 0xdeadbeef;
            ring_success = 0xdeadcafe;
            *ring_value = 0xdeadcafe;
            if (push) {
                push_value = *rc::gen::inRange(0u, 0xffffffffu);
                circular_success = circular_buffer.push(push_value);
                ring_success = circular_buf2_put(fb_ring_queue, push_value);
                RC_LOG() << "Push command resulted in discrepancy "
                         << "Circular Buffer: " << circular_success << " "
                         << "Ring Queue: " << ring_success << "\n";
                RC_ASSERT(circular_success == ring_success);
                push_counter++;
            } else {
                circular_success = circular_buffer.pop(circular_value);
                ring_success = circular_buf2_get(fb_ring_queue, ring_value);
                RC_LOG() << "Pop command resulted in discrepancy "
                         << "Circular Buffer: " << circular_success << " "
                         << "Ring Queue: " << ring_success << "\n";
                RC_ASSERT(circular_success == ring_success);
                if (circular_success == 0 && ring_success == 0) {
                    RC_LOG() << "Popped values are different "
                             << "Circular Buffer: " << *circular_value
                             << "Ring Queue: " << *ring_value;
                    RC_ASSERT(*circular_value == *ring_value);
                }
                pop_counter++; 
            }

            // std::cout << circular_buf2_occupancy(fb_ring_queue) << "\n";
            // std::cout << circular_buffer.circular_buffer.size() << "\n";

            RC_ASSERT(circular_buf2_occupancy(fb_ring_queue) == circular_buffer.circular_buffer.size());

        }
        std::cout << "Executing " << operation_count << " operations "
                  << "with " << push_counter << " push operations and "
                  << pop_counter << " pop operations\n";
    });


    rc::check("Peek n", []() {
        bool push;
        uint32_t push_value;
        int circular_success;
        int ring_success;
        uint32_t *circular_value = new uint32_t;
        unsigned int *ring_value = new unsigned int; 
        uint32_t push_counter = 0;
        uint32_t pop_counter = 0;
        std::size_t operation_count = *rc::gen::inRange(0u, 0xfffu);
        CircularBuffer circular_buffer;
        circular_buf_pow2_t __fb_ring_queue = \
        CIRBUF_POW2_STATIC_CONSTRUCTOR(__fb_ring_queue, CIRCULAR_BUFFER_SIZE);
        circular_buf_pow2_t *fb_ring_queue = &__fb_ring_queue;
        circular_buf2_initialize(\
                &__fb_ring_queue,
                CIRBUF_POW2_STORAGE_NAME(__fb_ring_queue),
                ARRAY_SIZE(CIRBUF_POW2_STORAGE_NAME(__fb_ring_queue)));

        for (std::size_t i = 0; i < operation_count; i++) {
            push = *rc::gen::inRange(0, 2);
            circular_success = 0xdeadbeef;
            *circular_value = 0xdeadbeef;
            ring_success = 0xdeadcafe;
            *ring_value = 0xdeadcafe;
            if (push && !circular_buf2_full(fb_ring_queue)) {
                push_value = *rc::gen::inRange(0u, 0xffffffffu);
                // std::cout << "push value: " << push_value << "\n";
                circular_success = circular_buffer.push(push_value);
                ring_success = circular_buf2_put(fb_ring_queue, push_value);
                RC_LOG() << "Push command resulted in discrepancy "
                         << "Circular Buffer: " << circular_success << " "
                         << "Ring Queue: " << ring_success << "\n";
                RC_ASSERT(circular_success == ring_success);
                push_counter++;
            } else {
                circular_success = circular_buffer.pop(circular_value);
                ring_success = circular_buf2_get(fb_ring_queue, ring_value);
                RC_LOG() << "1 Pop command resulted in discrepancy "
                         << "Circular Buffer: " << circular_success << " "
                         << "Ring Queue: " << ring_success << "\n";
                RC_ASSERT(circular_success == ring_success);

                // std::cout << "Circular Buffer: " << circular_success << " "
                         // << "Ring Queue: " << ring_success << "\n";

                if (circular_success == 0 && ring_success == 0) {
                    RC_LOG() << "1 Popped values are different "
                             << "Circular Buffer: " << *circular_value
                             << "Ring Queue: " << *ring_value;
                    RC_ASSERT(*circular_value == *ring_value);
                }
                // std::cout << "pop\n";
                pop_counter++; 
            }
        }

        RC_PRE( circular_buffer.circular_buffer.size() > 3);


        std::cout << "Executing " << operation_count << " operations "
                  << "with " << push_counter << " push operations and "
                  << pop_counter << " pop operations\n";

        unsigned peek = 0;
        uint32_t a = 0;
        uint32_t ra = 0;

        int rc;
        int rr;
        do {
            rc = circular_buffer.pop(&a);

            rr = circular_buf2_peek_n(fb_ring_queue, &ra, peek);
            RC_ASSERT(rc == rr);

            RC_ASSERT(a == ra);


            peek++;
        } while(rc == 0);

    });
}