#pragma once

#include <vector>
#include <cstdint>
#include <atomic>
#include "mapmov.h"


class EmulateMapmov
{
public:
    std::atomic<unsigned> enabled_subcarriers;
    std::atomic<uint16_t> subcarrier_allocation;
    std::atomic<uint8_t> modulation_schema;

    // EmulateMapmov();
    EmulateMapmov(uint16_t initial_subcarrier_allocation = MAPMOV_SUBCARRIER_128);

    void settings_did_change(void);

    void set_subcarrier_allocation(const uint16_t allocation_value);
    uint16_t get_subcarrier_allocation(void) const;

    void set_modulation_schema(const uint8_t schema_value);
    uint8_t get_modulation_schema(void) const;

    unsigned get_enabled_subcarriers(void) const;


    void build_enable_subcarrier_table(void);
    void build_pilot_table(void);
    void map_input_data(const std::vector<uint32_t> &input_data,
                                   std::vector<uint32_t> &mapmov_data);

    void move_input_data(const std::vector<uint32_t> &mapmov_mapped,
                                    std::vector<uint32_t> &mapmov_out);

    void receive_tx_data(std::vector<uint32_t> &rx_out,
                                    const std::vector<uint32_t> &mapmov_out);

    void rx_reverse_mover(std::vector<uint32_t> &vmem_mover_out,
                                     const std::vector<uint32_t> &rx_out) const;

    void slice_rx_data(const std::vector<uint32_t> &vmem_mover_out,
                                  std::vector<uint32_t> &sliced_out_data);

    void print_enable_subcarrier_table(const std::vector<bool> &_enable_table,
                                       const int subcarriers_enabled);


private:
    std::vector<bool> enable_table;
    std::vector<uint32_t> pilot_table;

};


