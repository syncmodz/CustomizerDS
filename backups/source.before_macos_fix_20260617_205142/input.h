#ifndef INPUT_H
#define INPUT_H

#include <3ds.h>
#include <stdbool.h>

#define INPUT_CPAD_THRESHOLD 80

typedef struct {
    u32 down;
    u32 held;
    touchPosition touch;
    bool touchDown;
    bool touchHeld;
    bool up;
    bool downNav;
    bool left;
    bool right;
    bool confirm;
    bool back;
    bool start;
    bool debug;
    int touchPX;
    int touchPY;
} AppInput;

void inputRead(AppInput* in);

#endif
