#ifndef API_H
#define API_H

#define MAX_PIPE_PATH_LENGTH 40
#define MAX_REGISTER_REQUEST_LENGTH 82
#define MAX_REGISTER_ANSWER_LENGTH 3

typedef struct {
  int width;
  int height;
  int tempo;
  int victory;
  int game_over;
  int accumulated_points;
  char* data;
} Board;

int pacman_connect(char const *req_pipe_path, char const *notif_pipe_path, char const *server_pipe_path);

void pacman_play(char command);

/// @return 0 if the disconnection was successful, 1 otherwise.
int pacman_disconnect();

Board receive_board_update(void);

#endif