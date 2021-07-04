#include "utilities.hpp"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <iostream>


// I guess since I'm using the enable/disable I don't need the code here
// would be less efficient to switch it all the time
int getch(void)
{
  int ch;
  // struct termios oldt;
  // struct termios newt;
  // tcgetattr(STDIN_FILENO, &oldt); /*store old settings */
  // newt = oldt; /* copy old settings to new settings */
  // newt.c_lflag &= ~(ICANON | ECHO);  // make one change to old settings in new settings 
  // tcsetattr(STDIN_FILENO, TCSANOW, &newt); /*apply the new settings immediatly */
  ch = getchar(); /* standard getchar call */
  // tcsetattr(STDIN_FILENO, TCSANOW, &oldt); /*reapply the old settings */
  return ch; /*return received char */
}

// http://stackoverflow.com/questions/448944/c-non-blocking-keyboard-input
// int kbhit(void)
// {
//     struct timeval tv = { 0L, 0L };
//     fd_set fds;
//     FD_ZERO(&fds);
//     FD_SET(0, &fds);
//     return select(1, &fds, NULL, NULL, &tv);
// }

// https://stackoverflow.com/questions/29335758/using-kbhit-and-getch-on-linux
void enable_raw_keyboard_mode()
{
    termios term;
    tcgetattr(0, &term);
    term.c_lflag &= ~(ICANON | ECHO); // Disable echo as well
    tcsetattr(0, TCSANOW, &term);
}

void disable_raw_keyboard_mode()
{
    termios term;
    tcgetattr(0, &term);
    term.c_lflag |= ICANON | ECHO;
    tcsetattr(0, TCSANOW, &term);
}

void flush_raw_keyboard_mode()
{
    tcflush(0, TCIFLUSH);
}


bool kbhit()
{
    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);
    return byteswaiting > 0;
}


// bool kbhit()
// {
//     termios term;
//     tcgetattr(0, &term);

//     termios term2 = term;
//     term2.c_lflag &= ~ICANON;
//     tcsetattr(0, TCSANOW, &term2);

//     int byteswaiting;
//     ioctl(0, FIONREAD, &byteswaiting);

//     tcsetattr(0, TCSANOW, &term);

//     return byteswaiting > 0;
// }