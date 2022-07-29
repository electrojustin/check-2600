#ifndef INPUT_H
#define INPUT_H

// Paddle control values from event thread to be read by the TIA.
// TODO: Support other types of controls.

extern bool player0_up;
extern bool player0_down;
extern bool player0_left;
extern bool player0_right;
extern bool player0_fire;

extern bool player1_up;
extern bool player1_down;
extern bool player1_left;
extern bool player1_right;
extern bool player1_fire;

#endif
