#include "board.h"
#include "display.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>

#define CONTINUE_PLAY 0
#define NEXT_LEVEL 1
#define QUIT_GAME 2
#define LOAD_BACKUP 3
#define CREATE_BACKUP 4

#define MAX_PIPE_PATH_LENGTH 40
#define MAX_REGISTER_REQUEST_LENGTH 81
#define MAX_REGISTER_ANSWER_LENGTH 3

typedef struct{
    int indiceLeitura;
    int indiceEscrita;
    char (*buffer)[MAX_REGISTER_REQUEST_LENGTH];
    sem_t bufferCheio;
    sem_t MsgparaLer;
    pthread_mutex_t sem_mutex;
}thread_buffer;

typedef struct{
    int max_games;
    thread_buffer* buffer;
    pthread_t *client_threads;
    char *level_directory;
}server_t;

typedef struct {
    board_t *board;
    int ghost_index;
} ghost_thread_arg_t;

typedef struct{
    board_t *board;
    int request_pipe_fd;
} pacman_thread_arg_t;

typedef struct{
    board_t *board;
    int answer_pipe_fd;
} board_thread_arg_t;

typedef struct {
    char op_code;           
    int width;              
    int height;             
    int tempo;              
    int victory;            
    int end_game;         
    int accumulated_points; 
    char board_data[];      
} __attribute__((packed)) msg_update_t;

typedef struct{
    int max_games;
    char* level_directory;
    thread_buffer* buffer;
} client_thread_arg_t;

typedef struct{
    int return_value;
    int client_id;
}client_return_t;


volatile sig_atomic_t thread_shutdown = 0;


void* pacman_thread(void *arg) {
    pacman_thread_arg_t *args = (pacman_thread_arg_t*) arg;
    board_t *board = args->board;
    int request_pipe_fd = args->request_pipe_fd;
    free(args);
    pacman_t* pacman = &board->pacmans[0];

    int *retval = malloc(sizeof(int));
    struct pollfd fds[1];
    fds[0].fd = request_pipe_fd;
    fds[0].events = POLLIN;
    while (true) {
        if(!pacman->alive || thread_shutdown) {
            *retval = QUIT_GAME;
            return (void*) retval;
        }

        command_t* play;
        command_t c;
        char buf[MAX_REGISTER_REQUEST_LENGTH];
        int bytes_read;
        int poll_result = poll(fds, 1, board->tempo * (1 + pacman->passo));
        if(poll_result<=0) continue;
        if (!(fds[0].revents & POLLIN)) continue;

        bytes_read = read(request_pipe_fd,buf,MAX_REGISTER_REQUEST_LENGTH);
        if (bytes_read < 0){
            debug("Error reading from request pipe\n");
            *retval = QUIT_GAME;
            return (void *) retval;
        }
        switch (buf[0]){
            case '2':{
                *retval = QUIT_GAME;
                return (void *) retval;
            }
            case '3':{
                c.command = buf[1];
                break;
            }
        }

        if(c.command == '\0') {
            continue;
        }

        c.turns = 1;
        play = &c;

        // QUIT
        if (play->command == 'Q') {
            *retval = QUIT_GAME;
            return (void*) retval;
        }

        pthread_rwlock_rdlock(&board->state_lock);

        int result = move_pacman(board, 0, play);
        if (result == REACHED_PORTAL) {
            // Next level
            *retval = NEXT_LEVEL;
            break;
        }

        if(result == DEAD_PACMAN) {
            *retval = QUIT_GAME;
            break;
        }

        pthread_rwlock_unlock(&board->state_lock);
    }
    pthread_rwlock_unlock(&board->state_lock);
    return (void*) retval;
}

void* ghost_thread(void *arg) {
    ghost_thread_arg_t *ghost_arg = (ghost_thread_arg_t*) arg;
    board_t *board = ghost_arg->board;
    int ghost_ind = ghost_arg->ghost_index;
    free(ghost_arg);
    ghost_t* ghost = &board->ghosts[ghost_ind];

    while (true) {
        sleep_ms(board->tempo * (1 + ghost->passo));

        pthread_rwlock_rdlock(&board->state_lock);
        if (board->thread_shutdown) {
            pthread_rwlock_unlock(&board->state_lock);
            pthread_exit(NULL);
        }
        
        move_ghost(board, ghost_ind, &ghost->moves[ghost->current_move%ghost->n_moves]);
        pthread_rwlock_unlock(&board->state_lock);
    }
}

//Helper function to create update message
static void init_update_msg(msg_update_t *msg,board_t *board){
    msg->op_code = 4;
    msg ->height = board->height;
    msg->width = board->width;
    msg->tempo = board->tempo;
    msg->victory = board->victory;
    msg->end_game = board->end_game;
    msg->accumulated_points = board->pacmans[0].points;
    for (int i = 0; i < board->width*board->height; i++) {
        switch(board->board[i].content){
            case 'W': msg->board_data[i] = '#'; break;
            case 'P': msg->board_data[i] = 'C'; break;
            case 'M':{
                for (int g = 0; g < board->n_ghosts; g++) {
                    ghost_t* ghost = &board->ghosts[g];
                    if (ghost->pos_y * board->width + ghost->pos_x == i) {
                        if (ghost->charged) msg->board_data[i] = 'G';
                        else msg->board_data[i] = 'M';
                        break;
                    }
                }
                break;
            }
            default:{
                if(board->board[i].has_dot) msg->board_data[i] = '.';
                else if(board->board[i].has_portal) msg->board_data[i] = '@';
                else msg->board_data[i] = ' ';
                break;
            }
        }
    }
}

void* board_thread(void *arg){
    board_thread_arg_t *args = (board_thread_arg_t*) arg;
    board_t *board = args->board;
    int answer_pipe_fd = args->answer_pipe_fd;
    free(args);
    int update_size = sizeof(msg_update_t) + board->width*board->height*sizeof(char);
    msg_update_t *msg = (msg_update_t*)malloc(update_size);
    if (!msg){
        debug("Error allocating memory");
        return NULL;
    }
    bool end_thread = false;
    while (!end_thread){
        sleep_ms(board->tempo);
        pthread_rwlock_wrlock(&board->state_lock);
        if (board->thread_shutdown) end_thread = true;
        init_update_msg(msg,board);
        pthread_rwlock_unlock(&board->state_lock);
        int ret = write(answer_pipe_fd,msg,update_size);
        if (ret < 0) {
            debug("Error writing to answer pipe\n");
            if (errno == EPIPE){
                debug("Client disconnected during game\n");
                end_thread = true;
                pthread_rwlock_wrlock(&board->state_lock);
                board->thread_shutdown = 1;
                pthread_rwlock_unlock(&board->state_lock);
            }
        }
    }
    free(msg);
    pthread_exit(NULL);
}

void* client_thread(void *arg){
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    client_thread_arg_t *args = (client_thread_arg_t *)arg;
    char dir_path[MAX_FILENAME + 1];
    strcpy(dir_path,args->level_directory);
    thread_buffer* buffer = args->buffer;
    int max_games= args->max_games;
    free(args);

    client_return_t *return_args = (client_return_t*)malloc(sizeof(client_return_t));
    if(!return_args){
        debug("Error allocating memory\n");
        return NULL;
    }
    char request_pipe[MAX_PIPE_PATH_LENGTH + 1];
    char answer_pipe[MAX_PIPE_PATH_LENGTH + 1];
    while(!thread_shutdown){
        sem_wait(&buffer->MsgparaLer);
        return_args->return_value=-1;
        if(thread_shutdown){
            break;
        }
        pthread_mutex_lock(&buffer->sem_mutex);

        strncpy(request_pipe, &buffer->buffer[buffer->indiceLeitura][1], MAX_PIPE_PATH_LENGTH);
        request_pipe[MAX_PIPE_PATH_LENGTH] = '\0'; 

        strncpy(answer_pipe, &buffer->buffer[buffer->indiceLeitura][41], MAX_PIPE_PATH_LENGTH);
        answer_pipe[MAX_PIPE_PATH_LENGTH] = '\0';

        sscanf(answer_pipe, "/tmp/%d_notification", &return_args->client_id);

        buffer->indiceLeitura=(buffer->indiceLeitura+1)%max_games;

        pthread_mutex_unlock(&buffer->sem_mutex);
        sem_post(&buffer->bufferCheio);
        
        DIR* level_dir = opendir(dir_path);
        if (level_dir == NULL) {
            debug("Failed to open directory: %s\n", dir_path);
            return NULL;
        }

        int accumulated_points = 0;
        bool end_game = false;
        board_t game_board;

        int fcli,fserv;
        if ((fcli = open(answer_pipe,O_WRONLY)) < 0){
            debug("Error opening answer pipe\n");
            return NULL;
        }
        int ret = write(fcli,"10\0",3);
        if (ret<0){
            debug("Error writing to answer pipe\n");
            if (errno == EPIPE){
                close (fcli);
            }
            continue;
        }
        if ((fserv = open(request_pipe,O_RDONLY)) < 0){
            debug("Error opening request pipe\n");
            close (fcli);
            continue;
        }

        struct dirent* entry;
        while ((entry = readdir(level_dir)) != NULL && !end_game && !thread_shutdown) {
            if (entry->d_name[0] == '.') continue;

            char *dot = strrchr(entry->d_name, '.');
            if (!dot) continue;

            if (strcmp(dot, ".lvl") == 0) {
                load_level(&game_board, entry->d_name, dir_path, accumulated_points);
                pthread_t pacman_tid,board_tid;
                pthread_t *ghost_tids = malloc(game_board.n_ghosts * sizeof(pthread_t));
                if(!ghost_tids){
                    debug("Error allocating memory\n");
                    break;
                }
                game_board.thread_shutdown = 0;

                pacman_thread_arg_t *pac_args = malloc(sizeof(pacman_thread_arg_t));
                if (!pac_args){
                    debug("Error allocating memory\n");
                    free(ghost_tids);
                    break;
                }
                pac_args->board = &game_board;
                pac_args->request_pipe_fd = fserv;
                pthread_create(&pacman_tid, NULL, pacman_thread, (void*) pac_args);
                for (int i = 0; i < game_board.n_ghosts; i++) {
                    ghost_thread_arg_t *arg = malloc(sizeof(ghost_thread_arg_t));
                    if(!arg){
                        debug("Error allocating memory\n");
                        free(ghost_tids);
                        break;
                    }
                    arg->board = &game_board;
                    arg->ghost_index = i;
                    pthread_create(&ghost_tids[i], NULL, ghost_thread, (void*) arg);
                }

                board_thread_arg_t *board_args = malloc(sizeof(board_thread_arg_t));
                if(!board_args){
                    debug("Error allocating memory\n");
                    free(ghost_tids);
                    break;
                }
                board_args->board = &game_board;
                board_args->answer_pipe_fd = fcli;
                pthread_create(&board_tid, NULL, board_thread, (void*) board_args);

                int *retval;
                pthread_join(pacman_tid, (void**)&retval);
                int result = *retval;
                free(retval);
                pthread_rwlock_wrlock(&game_board.state_lock);
                if (result == QUIT_GAME){
                    game_board.end_game = 1;
                    end_game = true;
                }
                else{
                    game_board.victory = 1;
                }

                game_board.thread_shutdown = 1;
                pthread_rwlock_unlock(&game_board.state_lock);

                pthread_join(board_tid,NULL);
                for (int i = 0; i < game_board.n_ghosts; i++) {
                    pthread_join(ghost_tids[i], NULL);
                }

                free(ghost_tids);
                accumulated_points = game_board.pacmans[0].points;
                return_args->return_value = accumulated_points;
                print_board(&game_board);
                unload_level(&game_board);
            }
        }
        if(!end_game){//No more levels
            msg_update_t end_msg={0};
            end_msg.end_game=1;
            write(fcli,&end_msg,sizeof(msg_update_t));
        }
        close(fcli);
        char disconnect_msg[1];
        read(fserv,disconnect_msg,1);
        close(fserv);
        if (closedir(level_dir) == -1) {
            fprintf(stderr, "Failed to close directory\n");
            return NULL;
        }
    } 
    pthread_exit(return_args);
}

int handleRequest(char *buf, server_t *server){
    char opcode = buf[0];
    if(opcode == '1'){
        sem_wait(&server->buffer->bufferCheio);
        pthread_mutex_lock(&server->buffer->sem_mutex);
        memcpy(server->buffer->buffer[server->buffer->indiceEscrita], buf, MAX_REGISTER_REQUEST_LENGTH);
        server->buffer->indiceEscrita = (server->buffer->indiceEscrita + 1) % server->max_games;
        pthread_mutex_unlock(&server->buffer->sem_mutex);
        sem_post(&server->buffer->MsgparaLer);
    }
    return 0;
}

void treat_sigusr1(int signum){
    if(signum != SIGUSR1) return;
    debug("SIGUSR1 received, shutting down threads\n");
    thread_shutdown=1;
}

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGUSR1, treat_sigusr1);
    if (argc != 4) {
        printf("Usage: %s <level_directory> <max_games> <registry_pipe>\n", argv[0]);
        return -1;
    }
    srand((unsigned int)time(NULL));
    open_debug_file("debug.log");

    thread_shutdown=0;

    char register_pipe[MAX_PIPE_PATH_LENGTH];
    strcpy(register_pipe,argv[3]);
    unlink(register_pipe);
    debug("Creating registry pipe:%s\n",register_pipe);
    if (mkfifo (register_pipe, 0777) < 0){
        debug("Error creating registry pipe\n");
        return 1;
    }

    server_t server = {atoi(argv[2]),NULL,NULL,argv[1]};
    server.client_threads = (pthread_t*)malloc(sizeof(pthread_t)*server.max_games);
    if (!server.client_threads){
        debug("Error allocating memory\n");
        unlink(register_pipe);
        return 1;
    }

    thread_buffer buffer = {.indiceLeitura = 0, .indiceEscrita = 0, .buffer = NULL};
    sem_init(&buffer.bufferCheio,0,server.max_games);
    sem_init(&buffer.MsgparaLer,0,0);
    pthread_mutex_init(&buffer.sem_mutex,NULL);
    buffer.buffer = malloc(sizeof(char[MAX_REGISTER_REQUEST_LENGTH])*server.max_games);
    if (!buffer.buffer){
        debug("Error allocating memory\n");
        unlink(register_pipe);
        free(server.client_threads);
        return 1;
    }
    server.buffer = &buffer;

    for (int i = 0; i < server.max_games; i++) {
        client_thread_arg_t *client_args = (client_thread_arg_t*)malloc(sizeof(client_thread_arg_t));
        if(!client_args){
            debug("Error allocating memory\n");
            unlink(register_pipe);
            free(server.client_threads);
            free(buffer.buffer);
            thread_shutdown=1;
            return 1;
        }
        client_args->level_directory = server.level_directory;
        client_args->buffer = &buffer;
        client_args->max_games = server.max_games;
        pthread_create(&server.client_threads[i], NULL, client_thread, (void*) client_args);
    }

    int fserv;
    if ((fserv = open (register_pipe,O_RDONLY)) < 0){
        debug("Error opening registry pipe\n");
        unlink(register_pipe);
        thread_shutdown=1;
    }
    
    char buf[MAX_REGISTER_REQUEST_LENGTH+1];
    while (!thread_shutdown) {
        read (fserv, buf, MAX_REGISTER_REQUEST_LENGTH);
        if(handleRequest (buf,&server)){
            debug("Error handling request\n");
            thread_shutdown=1;
            break;
        }
    }
    
    for (int i = 0; i < server.max_games; i++){
        sem_post(&server.buffer->MsgparaLer);
    }

    client_return_t leaderboard[5];
    int n_clients=0;
    memset(leaderboard, 0, sizeof(leaderboard));
    for (int i = 0; i < server.max_games; i++) {
        client_return_t *client_result;
        pthread_join(server.client_threads[i],(void**)&client_result);
        if(!client_result) continue;
        if (client_result->return_value < 0){
            free(client_result);
            continue;
        }
        n_clients++;
        for (int j = 0; j < 5; j++) {
            if (client_result->return_value > leaderboard[j].return_value) {
                for (int k = 4; k > j; k--) {
                    leaderboard[k] = leaderboard[k - 1];
                }
                leaderboard[j] = *client_result;
                break;
            }
        }
        free(client_result);
    }
    if (n_clients > 5) n_clients = 5;
    FILE *leaderboard_file = fopen("leaderboard.log","w");
    if (leaderboard_file != NULL) {
        for (int i = 0; i < n_clients; i++) {
            fprintf(leaderboard_file, "Client %d | Points: %d\n",leaderboard[i].client_id, leaderboard[i].return_value);
        }
        fclose(leaderboard_file);
    }

    close (fserv);
    unlink(register_pipe);
    free(server.client_threads);
    free(buffer.buffer);
    sem_destroy(&buffer.bufferCheio);
    sem_destroy(&buffer.MsgparaLer);
    pthread_mutex_destroy(&buffer.sem_mutex);
    close_debug_file();
    return 0;
}
