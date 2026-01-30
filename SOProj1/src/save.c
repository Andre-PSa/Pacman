#include "save.h"
#include "errors.h"
#include "threads.h"
#include "display.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void reinit_locks(board_t *board) {
    pthread_rwlock_init(&board->board_lock,NULL);
    for (int i = 0; i < board->height * board->width; i++) {
        pthread_rwlock_init(&board->board[i].pos_lock, NULL);
    }
}

void child_exit(board_t *board, int exit_code){
    stop_and_join_threads(board);
    print_board(board);
    unload_level(board);
    terminal_cleanup();
    close_debug_file();
    exit(exit_code);
}

int restore_game_state(board_t *board){
    terminal_cleanup();
    terminal_init();
    reinit_locks(board);
    if(thread_init_ghosts(board)==ERROR){
        debug(CREATETHR_ERR_MSG);
        return ERROR;
    }
    screen_refresh(board, DRAW_MENU);
    return 1;
}

int handle_save(board_t *board) {
    board->end_game = true; //Forces ghost threads to stop
    stop_and_join_threads(board);
    board->end_game = false;

    pid_t pid = fork();
    if (pid < 0) {
        debug(FORK_ERR_MSG);
        return ERROR;
    }
    
    if (pid == 0) {
        debug("[%d] Child process\n", getpid());
        board->is_child = true;
        reinit_locks(board);
        if(thread_init_ghosts(board) == ERROR){
            debug(CREATETHR_ERR_MSG);
            exit(ERROR);
        }
        return 0; //MUDAR ISTO
    }
    
    debug("[%d] Parent waiting for child %d\n", getpid(), pid);
    
    int child_status;
    waitpid(pid, &child_status, 0);
    
    if (WIFEXITED(child_status)) {
        int exit_code = WEXITSTATUS(child_status);
        if (exit_code == WIN_GAME) {
            debug("[%d] Child won! Parent exiting.\n", getpid());
            close_debug_file();
            exit(0);
        } else if (exit_code == QUIT_GAME) {
            debug("[%d] Child quit! Parent exiting.\n", getpid());
            close_debug_file();
            exit(0);
        }
        else if (exit_code == ERROR){
            debug(CHILD_ERR_MSG);
            return ERROR;
        }
    }
    
    if(restore_game_state(board)==ERROR){
        debug(RESTORE_ERR_MSG);
        return ERROR;
    }
    return 1;
}