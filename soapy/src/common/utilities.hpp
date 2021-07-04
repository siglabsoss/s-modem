#pragma once


int getch(void);
// http://stackoverflow.com/questions/448944/c-non-blocking-keyboard-input
// https://stackoverflow.com/questions/29335758/using-kbhit-and-getch-on-linux
bool kbhit(void);
void enable_raw_keyboard_mode();
void disable_raw_keyboard_mode();
void flush_raw_keyboard_mode();