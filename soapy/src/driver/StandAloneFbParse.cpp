#include "StandAloneFbParse.hpp"
#include "mapmov.h"
#include "cpp_utils.hpp"
#include "EmulateMapmov.hpp"

#include <iostream>

using namespace std;



StandAloneFbParse::StandAloneFbParse() :
enabled_subcarriers(128)
,subcarrier_allocation(MAPMOV_SUBCARRIER_128)
,emulate_mapmov(new EmulateMapmov())
{
    emulate_mapmov->set_subcarrier_allocation(subcarrier_allocation);
}


void StandAloneFbParse::set_mapmov(const uint16_t allocation_value, const uint32_t constellation) {
    emulate_mapmov->set_subcarrier_allocation(allocation_value);
    emulate_mapmov->set_modulation_schema(constellation);
    enable_decode_mapmov = true;
    enable_expand_mapmov = true;
}

void StandAloneFbParse::disable_mapmov(void) {
    enable_decode_mapmov = false;
    enable_expand_mapmov = false;
}


// seems like we have a bug, if there are no trailing zeros
// doParseFeedbackBus() won't parse the last packet
// this hack just adds zeros which you must do at the right time in order
// to get the last packet
void StandAloneFbParse::customParseFeedbackPadZeros(const unsigned count) {
    std::vector<uint32_t> zrs;
    zrs.resize(count);
    addWords(zrs);
    // auto &data = this->outs[parse_feedback_bus_fpga]->data;
    // for(unsigned i = 0; i < count; i++) {
    //     data.push_back(0);
    // }
}

void StandAloneFbParse::addWords(const std::vector<uint32_t>& words) {

    if( print1 ) {
        cout << "adding " << words.size() << " words to parser\n";
    }

    for(const auto w : words) {
        fb_data.emplace_back(w);

        if( keep_history ) {
            fb_history.emplace_back(w);
        }
    }
}

///
/// fbp prefix means "feedback bus parse"
void StandAloneFbParse::parse(void) {
    auto &data = this->fb_data;



    if( print2 ) {
        cout << "i: " << data.size() << " - " << fbp_progress << "\n";
    }

    unsigned i;
    for(i = fbp_progress; i < data.size(); /*empty*/ ) {
        const auto word = data[i];
        // cout << i << ",";

        if(print3) {
            cout << "p: " << HEX32_STRING(word) << "\n";
        }


        const feedback_frame_t* const v = (feedback_frame_t*) (((uint32_t*)data.data())+i);

        bool was_ud = false;
        bool error = false;
        uint32_t advance;
        if( enable_decode_mapmov ) {
            // convert the length, this is without the +16
            advance = feedback_word_length(
                v
                ,&error
                ,emulate_mapmov->get_enabled_subcarriers()
                ,emulate_mapmov->get_subcarrier_allocation()
                ,&was_ud);

        } else {
            advance = feedback_word_length(v, &error);
        }

        // cout << "i: " << i << " sz " << data.size() << " adv " << advance  << "\n";

        if(print4) {
            cout << "Adv: " << advance << "\n";
        }


        if(word != 0) {

            if(print2) {
                cout << "c " << (i+1) << " " << advance << " " << ((i+1)+advance) << " " << data.size() << "\n";
            }


            // std::cout << "Breaking loop at word #" << i
            //   << " because header goes beyond received words\n";
            if( advance == 1 ) {
                if( ((i+15)+advance) > data.size() ) {
                    if(print4) {
                        cout << "Break term 0 (" << data.size() << ")\n";
                    }
                    break;
                }
            } else {
                if( ( ((i)+advance) > data.size() ) ) {
                    if(print4) {
                        cout << "Break term 1 (" << data.size() << ")\n";
                    }
                    break;
                }
            }

            // cout << "\n";
            // print_feedback_generic((const feedback_frame_t*)v);
            if( !error ) {
                if(was_ud) {
                    dispatchFb(v, advance); // pass non zero value to tell it we did a userdata
                } else {
                    dispatchFb(v);
                }
            }
            if(fbp_jamming != -1) {
                if( fbp_print_jamming ) {
                    cout << "Was Jamming was for " << fbp_jamming << "\n";
                }
                fbp_jamming = -1;
            }
        } else {
            if(fbp_jamming == -1) {
                fbp_jamming = 1;
            } else {
                fbp_jamming++;
            }
        }



        if( error ) {
            std::cout << "Hard fail when parsing word #" << i << "\n";
            advance = 1;
            fbp_error_count++;
            if( fbp_error_cb ) {
                fbp_error_cb();
            }
        }

        if( advance != 1 ) {
            // If zero we want to print because that's wrong
            // If 1 we don't want to spam during a flushing section
            // If larger we want to print because they are few
            // cout << "Advance " << advance << "\n";
        }


        if(print4) {

            cout << "BUMP Adv: " << advance << "\n";
        }



        i += advance;
    }

    // cout << "\n";

    // fbp_progress += data.size() - fbp_progress;

    if( fbp_progress < data.size() ) {
        // only run this if the loop above entered
        // we use the same condition as the loop for if()
        fbp_progress += (i) - fbp_progress;
    }
}

/// calls all of the correct user functions based on what's enabled
/// pass nonzero to ud_len to signify this is a ud
void StandAloneFbParse::dispatchFb(const feedback_frame_t *v, const uint32_t ud_len) {
    const uint32_t* p = (const uint32_t*)v;
    const int blen = ((ud_len) ? (ud_len) : (v->length)) - FEEDBACK_HEADER_WORDS;

    std::vector<uint32_t> header;
    header.assign(p, p+FEEDBACK_HEADER_WORDS);

    // cout << "llen: " << blen << "\n";

    std::vector<uint32_t> body;

    if( blen > 0 ) {
        body.assign(p+FEEDBACK_HEADER_WORDS, p+FEEDBACK_HEADER_WORDS+blen);
    }

    if( fbp_raw_cb ) {
        fbp_raw_cb(v);
    }

    uint32_t type0 = v->type;
    uint32_t type1 = ((const feedback_frame_vector_t*)v)->vtype;

    // only do anything if callback is set
    if( fbp_cb ) {
        if( type0 == 2 && type1 == 5 ) {
            if(enable_expand_mapmov) {

                std::vector<uint32_t> mapmov_mapped;
                std::vector<uint32_t> mapmov_out;
                emulate_mapmov->map_input_data(body, mapmov_mapped);
                emulate_mapmov->move_input_data(mapmov_mapped, mapmov_out);



                fbp_cb(type0, type1, header, mapmov_out);
                if( print5 ) {
                    cout << "USER DATA 1\n";
                }
            } else {
                fbp_cb(type0, type1, header, body);

                if( print5 ) {
                    cout << "USER DATA 0\n";
                }
            }
        } else {
            fbp_cb(type0, type1, header, body);
        }
    }

}


void StandAloneFbParse::registerRawCb(fb_raw_cb_t cb) {
    parse_feedback_bus = true;
    fbp_raw_cb = cb;
}

void StandAloneFbParse::registerCb(fb_cb_t cb) {
    parse_feedback_bus = true;
    fbp_cb = cb;
}
void StandAloneFbParse::registerFbError(fb_error_cb_t cb) {
    fbp_error_cb = cb;
}


void StandAloneFbParse::set_both(const unsigned enabled_sc, const uint16_t allocation_value) {
    // cout << "EmulateMapmov setting " << allocation_value << "\n";

    enabled_subcarriers = enabled_sc;
    subcarrier_allocation = allocation_value;
    // settings_did_change();
}
