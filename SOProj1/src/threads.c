#include "board.h"
#include "errors.h"
#include "display.h"
#include "threads.h"
#include <stdlib.h>

//Private helper functions for max and min between two numbers
static inline int min(int a, int b) {return (a < b) ? a : b;}
static inline int max(int a, int b) {return (a > b) ? a : b;}

//Private helper function for unlocking in charged movement
static void unlock_rows_and_columns(board_t *board, int start, int finish, int fixed_coord, char direction){
    if (direction == 'y'){
        for (int i = start; i < finish; i++){
            int locked_idx = get_board_index(board,fixed_coord,i);
            pthread_rwlock_unlock(&board->board[locked_idx].pos_lock);
        }
    }
    if (direction == 'x'){
        for (int j = start; j < finish; j++){
            int locked_idx = get_board_index(board,j,fixed_coord);
            pthread_rwlock_unlock(&board->board[locked_idx].pos_lock);
        }
    }
}

// Helper private function for charged ghost movement in one direction
// Returns coordinate needed for unlocking in thread_move_ghost_charged function
static int thread_move_ghost_charged_direction(board_t* board, ghost_t* ghost, char direction, int* new_x, int* new_y) {
    int x = ghost->pos_x;
    int y = ghost->pos_y;
    *new_x = x;
    *new_y = y;
    int first_locked_idx;
    switch (direction) {
        case 'W': // Up
            if (y == 0) return INVALID_MOVE;
            *new_y = 0;
            first_locked_idx = 0;
            for (int i = 0; i < y; i++) { //From lowest board_index to highest to avoid deadlocks
                int pos_index = get_board_index(board,x,i);
                pthread_rwlock_wrlock(&board->board[pos_index].pos_lock);
                char target_content = board->board[pos_index].content;
                if (target_content == 'W' || target_content == 'M') {
                    *new_y = i + 1; 
                    unlock_rows_and_columns(board,first_locked_idx,i,x,'y');
                    first_locked_idx = i;
                }
                else if (target_content == 'P') {
                    *new_y = i;
                    unlock_rows_and_columns(board,first_locked_idx,i,x,'y');
                    first_locked_idx = i;
                }
            }
            pthread_rwlock_wrlock(&board->board[get_board_index(board,x,y)].pos_lock);
            return first_locked_idx;
            break;

        case 'S': // Down
            if (y == board->height - 1) return INVALID_MOVE;
            if(pthread_rwlock_trywrlock(&board->board[get_board_index(board,x,y)].pos_lock)==0){
            }
            else{
                pthread_rwlock_wrlock(&board->board[get_board_index(board,x,y)].pos_lock);
            }
            *new_y = board->height - 1; // In case there is no colision
            for (int i = y + 1; i < board->height; i++) { //Already set from lowest idx to highest
                int pos_index = get_board_index(board,x,i);
                pthread_rwlock_wrlock(&board->board[pos_index].pos_lock);
                char target_content = board->board[pos_index].content;
                if (target_content == 'W' || target_content == 'M') {
                    *new_y = i - 1; // stop before colision
                    return i;
                }
                if (target_content == 'P') {
                    *new_y = i;
                    return i;
                }
            }
            return board->height - 1;
            break;

        case 'A': // Left
            if (x == 0) return INVALID_MOVE;
            *new_x = 0;
            first_locked_idx = 0;
            for (int j = 0; j < x; j++) { //From lowest board_index to highest to avoid deadlocks
                int pos_index = get_board_index(board,j,y);
                pthread_rwlock_wrlock(&board->board[pos_index].pos_lock);
                char target_content = board->board[pos_index].content;
                if (target_content == 'W' || target_content == 'M') {
                    *new_x =j + 1;
                    unlock_rows_and_columns(board,first_locked_idx,j,y,'x');
                    first_locked_idx = j;
                }
                if (target_content == 'P') {
                    *new_x = j;
                    unlock_rows_and_columns(board,first_locked_idx,j,y,'x');
                    first_locked_idx = j;
                }
            }
            pthread_rwlock_wrlock(&board->board[get_board_index(board,x,y)].pos_lock);
            return first_locked_idx;
            break;

        case 'D': // Right
            if (x == board->width - 1) return INVALID_MOVE;
            pthread_rwlock_wrlock(&board->board[get_board_index(board,x,y)].pos_lock);
            *new_x = board->width - 1; // In case there is no colision
            for (int j = x + 1; j < board->width; j++) {
                int pos_index = get_board_index(board,j,y);
                pthread_rwlock_wrlock(&board->board[pos_index].pos_lock);
                char target_content = board->board[pos_index].content;
                if (target_content == 'W' || target_content == 'M') {
                    *new_x = j - 1; // stop before colision
                    return j;
                }
                if (target_content == 'P') {
                    *new_x = j;
                    return j;
                }
            }
            return board->width - 1;
            break;
        default:
            return INVALID_MOVE;
    }
} 

void stop_and_join_threads(board_t *board){
    for (int i = 0; i < board->n_ghosts;i++){
        pthread_join(board->ghost_threads[i],NULL);
    }
}

/* Handles charged ghost movement logic. */
static int thread_move_ghost_charged(board_t* board, int ghost_index, char direction) {
    ghost_t* ghost = &board->ghosts[ghost_index];
    int x = ghost->pos_x;
    int y = ghost->pos_y;
    int new_x = x;
    int new_y = y;

    ghost->charged = 0; //uncharge
    int result = thread_move_ghost_charged_direction(board, ghost, direction, &new_x, &new_y);
    if (result == INVALID_MOVE) {
        debug("DEFAULT CHARGED MOVE - direction = %c\n", direction);
        return INVALID_MOVE;
    }

    int first_locked_idx = result;
    // Get board indices
    int old_index = get_board_index(board, ghost->pos_x, ghost->pos_y);
    int new_index = get_board_index(board, new_x, new_y);

    if (board->board[new_index].content == 'P'){
        pthread_rwlock_wrlock(&board->board_lock);
        result = find_and_kill_pacman(board, new_x, new_y);
        board->end_game = true;
        pthread_rwlock_unlock(&board->board_lock);
    }
    else result = VALID_MOVE;

    // Update board - clear old position (restore what was there)
    board->board[old_index].content = ' '; // Or restore the dot if ghost was on one
    // Update board - set new position
    board->board[new_index].content = 'M';
    // Update ghost position
    ghost->pos_x = new_x;
    ghost->pos_y = new_y;
    

    if (direction == 'W' || direction == 'S'){
        unlock_rows_and_columns(board,min(y,first_locked_idx),max(y,first_locked_idx) + 1,x,'y');
    }
    else unlock_rows_and_columns(board,min(x,first_locked_idx),max(x,first_locked_idx) + 1,y,'x');
    return result;
}

/* Handles ghost movement, including collisions and locking logic. */
static int thread_move_ghost(board_t *board, int ghost_index, command_t *command){
    ghost_t* ghost = &board->ghosts[ghost_index];
    int new_x = ghost->pos_x;
    int new_y = ghost->pos_y;

    // check passo
    if (ghost->waiting > 0) {
        ghost->waiting -= 1;
        return VALID_MOVE;
    }
    ghost->waiting = ghost->passo;

    char direction = command->command;
    
    if (direction == 'R') {
        char directions[] = {'W', 'S', 'A', 'D'};
        direction = directions[rand() % 4];
    }

    // Calculate new position based on direction
    switch (direction) {
        case 'W': // Up
            new_y--;
            break;
        case 'S': // Down
            new_y++;
            break;
        case 'A': // Left
            new_x--;
            break;
        case 'D': // Right
            new_x++;
            break;
        case 'C': // Charge
            ghost->current_move += 1;
            ghost->charged = 1;
            return VALID_MOVE;
        case 'T': // Wait
            if (command->turns_left == 1) {
                ghost->current_move += 1; // move on
                command->turns_left = command->turns;
            }
            else command->turns_left -= 1;
            return VALID_MOVE;
        default:
            return INVALID_MOVE; // Invalid direction
    }

    // Logic for the WASD movement
    ghost->current_move++;
    if (ghost->charged)
        return thread_move_ghost_charged(board, ghost_index, direction);

    // Check boundaries
    if (!is_valid_position(board, new_x, new_y)) {
        return INVALID_MOVE;
    }

    // Check board position
    int new_index = get_board_index(board, new_x, new_y);
    int old_index = get_board_index(board, ghost->pos_x, ghost->pos_y);

    int first = min(new_index,old_index);
    int second = max(new_index,old_index);
    pthread_rwlock_wrlock(&board->board[first].pos_lock);
    pthread_rwlock_wrlock(&board->board[second].pos_lock);

    char target_content = board->board[new_index].content;

    // Check for walls and ghosts
    if (target_content == 'W' || target_content == 'M') {
        pthread_rwlock_unlock(&board->board[first].pos_lock);
        pthread_rwlock_unlock(&board->board[second].pos_lock);
        return INVALID_MOVE;
    }

    int result = VALID_MOVE;
    // Check for pacman
    if (target_content == 'P') {
        pthread_rwlock_wrlock(&board->board_lock);
        find_and_kill_pacman(board, new_x, new_y);
        board->end_game = true;
        pthread_rwlock_unlock(&board->board_lock);
        return DEAD_PACMAN;
    }

    // Update board - clear old position (restore what was there)
    board->board[old_index].content = ' '; // Or restore the dot if ghost was on one
    // Update board - set new position
    board->board[new_index].content = 'M';
    // Update ghost position
    
    ghost->pos_x = new_x;
    ghost->pos_y = new_y;

    pthread_rwlock_unlock(&board->board[first].pos_lock);
    pthread_rwlock_unlock(&board->board[second].pos_lock);
    
    return result;
}

void *ghost_logic(void *args){
    ghost_thread_args_t *thread_args = (ghost_thread_args_t*)args;
    board_t *board = thread_args->board;
    int ghost_idx = thread_args->ghost_idx;
    free(thread_args);
    ghost_t *ghost = &board->ghosts[ghost_idx];
    while (!board->end_game && !board->go_to_next_level && !board->quit_game){
        thread_move_ghost(board,ghost_idx,&ghost->moves[ghost->current_move%ghost->n_moves]);
        sleep_ms(board->tempo);
    }
    debug("Ended ghost thread %d\n",ghost_idx);
    return NULL;
}

int thread_init_ghosts(board_t* board){
    for (int i = 0; i < board->n_ghosts;i++){
        ghost_thread_args_t *args = (ghost_thread_args_t*)malloc(sizeof(ghost_thread_args_t));
        args->board = board;
        args->ghost_idx = i;
        if(pthread_create(&board->ghost_threads[i],NULL,ghost_logic,(void*)args)!=0){
            debug(CREATETHR_ERR_MSG);
            return ERROR;
        }
    }
    return 0;
}

int thread_move_pacman(board_t* board, int pacman_index, command_t* command) {
    if (pacman_index < 0 || !board->pacmans[pacman_index].alive) {
        return DEAD_PACMAN; // Invalid or dead pacman
    }

    pacman_t* pac = &board->pacmans[pacman_index];
    int new_x = pac->pos_x;
    int new_y = pac->pos_y;

    // check passo
    if (pac->waiting > 0) {
        pac->waiting -= 1;
        return VALID_MOVE;        
    }
    pac->waiting = pac->passo;

    char direction = command->command;

    if (direction == 'R') {
        char directions[] = {'W', 'S', 'A', 'D'};
        direction = directions[rand() % 4];
    }

    // Calculate new position based on direction
    switch (direction) {
        case 'W': // Up
            new_y--;
            break;
        case 'S': // Down
            new_y++;
            break;
        case 'A': // Left
            new_x--;
            break;
        case 'D': // Right
            new_x++;
            break;
        case 'T': // Wait
            if (command->turns_left == 1) {
                pac->current_move += 1; // move on
                command->turns_left = command->turns;
            }
            else command->turns_left -= 1;
            return VALID_MOVE;
        default:
            return INVALID_MOVE; // Invalid direction
    }

    // Logic for the WASD movement
    pac->current_move+=1;

    // Check boundaries
    if (!is_valid_position(board, new_x, new_y)) {
        return INVALID_MOVE;
    }

    int new_index = get_board_index(board, new_x, new_y);
    int old_index = get_board_index(board, pac->pos_x, pac->pos_y);
    
    int first = min(new_index,old_index);
    int second = max(new_index,old_index);
    pthread_rwlock_wrlock(&board->board[first].pos_lock);
    pthread_rwlock_wrlock(&board->board[second].pos_lock);
    char target_content = board->board[new_index].content;

    int result = VALID_MOVE;
    if (board->board[new_index].has_portal) {
        pthread_rwlock_wrlock(&board->board_lock);
        if(!board->end_game) board->go_to_next_level = true;
        pthread_rwlock_unlock(&board->board_lock);
        result = REACHED_PORTAL;
    }

    // Check for walls
    if (target_content == 'W' && result == VALID_MOVE) {
        result = INVALID_MOVE;
    }

    // Check for ghosts
    if (target_content == 'M' && result == VALID_MOVE) {
        kill_pacman(board, pacman_index);
        if(!board->go_to_next_level) board->end_game = true;
        result = DEAD_PACMAN;
    }

    // Collect points
    if (board->board[new_index].has_dot && result == VALID_MOVE) {
        pac->points++;
    }
    //locking board to change content and prevent refresh
    if(result == VALID_MOVE){
        board->board[new_index].has_dot = 0;
        board->board[old_index].content = ' ';
        board->board[new_index].content = 'P';
        pac->pos_x = new_x;
        pac->pos_y = new_y;
    }

    pthread_rwlock_unlock(&board->board[first].pos_lock);
    pthread_rwlock_unlock(&board->board[second].pos_lock);
    
    return result;
}