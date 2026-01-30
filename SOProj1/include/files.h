#ifndef FILES_H
#define FILES_H

#include "board.h"

#define STRIDE 128

/*Gets a filepath from a string of data*/
char *get_filepath(char **data, char *directory);

/*Removes the comments from a read file*/
void filter_file_data(char *data);

/*Reads a file fully into a string*/
char *read_file(char* filename);

/*Gets the filepaths for all .lvl files in the given directory*/
int get_lvl_files(char *directory,board_t *board);

#endif