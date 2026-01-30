#include "api.h"
#include "protocol.h"
#include "debug.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include  <sys/time.h>

typedef struct {
    char op_code;           // 1 byte
    int width;              // 4 bytes
    int height;             // 4 bytes
    int tempo;              // 4 bytes
    int victory;            // 4 bytes
    int end_game;          // 4 bytes
    int accumulated_points; // 4 bytes
    char board_data[];      // Tamanho variável (não ocupa espaço na struct)
} __attribute__((packed)) msg_update_t;

struct Session {
  int id;
  int tempo;
  int req_pipe;
  int notif_pipe;
  char req_pipe_path[MAX_PIPE_PATH_LENGTH + 1];
  char notif_pipe_path[MAX_PIPE_PATH_LENGTH + 1];
};

static struct Session session = {.id = -1};

int pacman_connect(char const *req_pipe_path, char const *notif_pipe_path, char const *server_pipe_path) {
  unlink(req_pipe_path);
  unlink(notif_pipe_path);
  if(mkfifo(req_pipe_path, 0777)<0){
    debug("Error making request pipe:%s\n",req_pipe_path);
    return 1;
  }
  if(mkfifo(notif_pipe_path, 0777)<0){
    debug("Error making notif pipe\n");
    return 1;
  }
  int fd_register;
  if ((fd_register = open(server_pipe_path,O_WRONLY)) < 0){
    debug("Error opening server pipe:%s\n",server_pipe_path);
    return 1;
  }

  char buffer[MAX_REGISTER_REQUEST_LENGTH + 1];
  memset(buffer,0,MAX_REGISTER_REQUEST_LENGTH + 1);
  buffer[0]='1';
  strncpy(buffer+1,req_pipe_path,MAX_PIPE_PATH_LENGTH);
  strncpy(buffer+41,notif_pipe_path,MAX_PIPE_PATH_LENGTH);
  write(fd_register,buffer,MAX_REGISTER_REQUEST_LENGTH);
  
  int fserv,fcli;
  if ((fcli = open(notif_pipe_path,O_RDONLY))<0){
    debug("Error opening notif pipe \n");
    close(fd_register);
    return 1;
  }
  char register_result[MAX_REGISTER_ANSWER_LENGTH + 1];
  read(fcli,register_result,MAX_REGISTER_ANSWER_LENGTH);
  if (register_result[1]!='0'){
    debug("Error registering client");
    close (fd_register);
    close (fcli);
    return 1;
  }
  if ((fserv = open(req_pipe_path,O_WRONLY))<0){
    debug("Error opening request pipe \n");
    close (fd_register);
    close(fcli);
    return 1;
  }
  strcpy(session.req_pipe_path,req_pipe_path);
  strcpy(session.notif_pipe_path,notif_pipe_path);
  session.notif_pipe = fcli;
  session.req_pipe = fserv;
  return 0;
}

void pacman_play(char command) {
  char buf[3];
  snprintf(buf,3,"3%c",command);
  write(session.req_pipe,buf,2);
}

int pacman_disconnect() {
  write(session.req_pipe,"2",1);
  close(session.notif_pipe);
  close(session.req_pipe);
  return 0;
}

//Helper function to intialize board from notification
static void init_board(Board *board, msg_update_t *board_info){
  board->height = board_info->height;
  board->width = board_info->width;
  board->tempo = board_info->tempo;
  board->accumulated_points = board_info->accumulated_points;
  board->victory = board_info->victory;
  board->game_over = board_info->end_game;
  board->data = (char*)malloc(board->height*board->width);
  memcpy(board->data,board_info->board_data,board->height*board->width);
}

Board receive_board_update(void) {
    Board board = {0};
    msg_update_t basic_board_info;
    read(session.notif_pipe,&basic_board_info,sizeof(msg_update_t));
    if(basic_board_info.end_game){
      board.game_over = basic_board_info.end_game;
      return board;
    }
    msg_update_t *full_board = (msg_update_t *)malloc(sizeof(msg_update_t) + basic_board_info.height*basic_board_info.width);
    if(!full_board){
      debug("Error allocating memory\n");
      return board;
    }
    memcpy(full_board,&basic_board_info,sizeof(msg_update_t));
    read(session.notif_pipe,full_board->board_data,full_board->height*full_board->width);
    init_board(&board,full_board);
    free(full_board);
    return board;
}
