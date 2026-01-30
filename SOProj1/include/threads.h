#ifndef THREADS_H
#define THREADS_H

#include "board.h"

/*Waits until all ghost threads have joined*/
void stop_and_join_threads(board_t *board);

/* Ghost thread main loop (moves one ghost until the game ends). */
void *ghost_logic(void *args);

/* Handles Pacman's movement, points, collisions, and portal detection. */
int thread_move_pacman(board_t *board, int pacman_index, command_t *command);

/* Creates all ghost threads and starts ghost behavior. */
int thread_init_ghosts(board_t *board);

#endif
