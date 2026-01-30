#include "board.h"
#include "display.h"
#include "files.h"
#include "errors.h"
#include "threads.h"
#include "save.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define CONTINUE_PLAY 0
#define NEXT_LEVEL 1

int play_pacman(board_t * game_board) {
    pacman_t* pacman = &game_board->pacmans[0];
    command_t* play;
    command_t c;
    if (pacman->n_moves == 0) { // if is user input 
        c.command = get_input();

        if(c.command == '\0')
            return CONTINUE_PLAY;

        c.turns = 1;
        play = &c;
    }
    else { // else if the moves are pre-defined in the file
        // avoid buffer overflow wrapping around with modulo of n_moves
        // this ensures that we always access a valid move for the pacman
        play = &pacman->moves[pacman->current_move%pacman->n_moves];
    }

    debug("KEY %c\n", play->command);

    if (play->command == 'Q') {
        game_board->quit_game = true;
        return QUIT_GAME;
    }

    if (play->command == 'G'){
        if (game_board->is_child){
            return CONTINUE_PLAY;
        }
        return CREATE_BACKUP;
    }

    thread_move_pacman(game_board, 0, play);
    return CONTINUE_PLAY;
}

int main(int argc, char** argv) {
    board_t game_board;
    game_board.is_child = false;
    game_board.end_game = false;
    game_board.quit_game = false;
    game_board.go_to_next_level = false;
    bool load_from_files = false;

    open_debug_file("debug.log");

    if (argc != 2) {
        printf("Usage: %s <level_directory>\n", argv[0]);
    }
    else {
        printf("Usage: %s %s\n",argv[0],argv[1]);
        load_from_files = true;
        game_board.curr_lvl = 0;
        game_board.lvl_amount = get_lvl_files(argv[1],&game_board);
        if (game_board.lvl_amount == ERROR){
            debug(READLVLFILE_ERR_MSG);
            return -1;
        }
    }
    // Random seed for any random movements
    srand((unsigned int)time(NULL));

    terminal_init();
    
    int accumulated_points = 0;

    while (!game_board.end_game && !no_more_levels(&game_board) && !game_board.quit_game) {
        if(load_from_files){
            if(load_level_from_file(&game_board,accumulated_points,argv[1])==ERROR){
                debug(LOADLVL_ERR_MSG);
                return -1;
            }
        }
        else load_level(&game_board, accumulated_points);
        draw_board(&game_board, DRAW_MENU);
        refresh_screen();
        sleep_ms(game_board.tempo);
        
        if(thread_init_ghosts(&game_board) == ERROR){
            debug(CREATETHR_ERR_MSG);
            return ERROR;
        }

        while(!game_board.end_game && !game_board.go_to_next_level && !game_board.quit_game) {
            int result = play_pacman(&game_board);
            if (result == CREATE_BACKUP){
                result = handle_save(&game_board);
                if (result == ERROR){
                    debug(SAVE_ERR_MSG);
                    return ERROR;
                }
            }

            screen_refresh(&game_board, DRAW_MENU); 
            accumulated_points = game_board.pacmans[0].points;
        }

        if (game_board.end_game){
            screen_refresh(&game_board, DRAW_GAME_OVER); 
            sleep_ms(game_board.tempo);
            if (game_board.is_child){
                child_exit(&game_board,CONTINUE_PLAY);
            }
        }

        else if(game_board.quit_game){
            screen_refresh(&game_board, DRAW_GAME_OVER); 
            sleep_ms(game_board.tempo);
            if (game_board.is_child){
                child_exit(&game_board,QUIT_GAME);
            }
        }

        else if (game_board.go_to_next_level){
            if(no_more_levels(&game_board)){
                screen_refresh(&game_board, DRAW_WIN);
                if (game_board.is_child){
                    sleep_ms(game_board.tempo);
                    child_exit(&game_board,WIN_GAME);
                }
            }
            else screen_refresh(&game_board, NEXT_LEVEL);
            sleep_ms(game_board.tempo);
            
        }
        stop_and_join_threads(&game_board);
        print_board(&game_board);
        unload_level(&game_board);
    } 

    terminal_cleanup();

    close_debug_file();
    return 0;
}
