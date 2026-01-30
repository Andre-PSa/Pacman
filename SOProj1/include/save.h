#ifndef SAVE_H
#define SAVE_H

#include "board.h"

#define QUIT_GAME 2
#define LOAD_BACKUP 3
#define CREATE_BACKUP 4
#define WIN_GAME 5

/*Reinitalize all locks after a fork*/
void reinit_locks(board_t *game_board);

/*Cleanly exit from the child process.*/
void child_exit(board_t *board, int exit_code);

/*Restore parent game state after child exit*/
int restore_game_state(board_t *board);

/*Handles saving the game with the fork process*/
int handle_save(board_t *board);


#endif