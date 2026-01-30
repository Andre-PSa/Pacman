#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include "files.h"
#include "board.h"
#include "errors.h"

char *get_filepath(char **data, char *directory){
    char *filename = (char*)malloc(sizeof(char) * MAX_FILENAME);
    if(!filename){
        debug(MEM_ERR_MSG);
        return NULL;
    }
    for (int i = 0; i < MAX_FILENAME;i++){
        if (isspace((*data)[i])){
            *data+=i;
            filename[i]='\0';
            break;
        }
        filename[i]=(*data)[i];
    }
    int pathsize = (strlen(filename) + strlen(directory) + 2)*sizeof(char);
    char *path = (char *)malloc(pathsize);
    snprintf(path,pathsize,"%s/%s",directory,filename);
    return path;
}

void filter_file_data(char *data){
    char *datareader = data;
    char *datawriter = data;

    while (datareader[0]!='\0') {
        char *line_start = datareader;
        char *line_end = strchr(datareader, '\n');
        if (!line_end) line_end = datareader + strlen(datareader);
        if (*line_start != '#') {
            while (datareader <= line_end) {
                *datawriter++ = *datareader++;
            }
        } else {
            // skip entire line
            datareader = line_end + 1;
        }
    }
    *datawriter = '\0'; // terminate the filtered string
}


char *read_file(char *filename){
    int  fd = open(filename,O_RDONLY);

    if (fd < 0){
        debug(OPENFILE_ERR_MSG,filename);
        return NULL;
    }

    char *readfile = (char *)malloc(STRIDE * sizeof(char) + 1);
    if (!readfile){
        debug(MEM_ERR_MSG);
        close(fd);
        return NULL;
    }
    int done = 0;
    int n = 0;
    while(1){
        int bytes_read = read(fd,readfile+done+n*STRIDE,STRIDE-done);

        if (bytes_read<0){
            debug(READFILE_ERR_MSG,filename);
            close(fd);
            free(readfile);
            return NULL;
        }
        else if(bytes_read==0){
            readfile[n * STRIDE + done]= '\0';
            break;
        }
        done+= bytes_read;
        if (done == STRIDE){
            done = 0;
            n++;
            char *tempreadfile = (char *)realloc(readfile,(n+1) * STRIDE * sizeof(char) + 1);
            if (!tempreadfile){
                debug(MEM_ERR_MSG);
                close(fd);
                free (readfile);
                return NULL;
            }
            readfile = tempreadfile;
        }
    }
    close(fd);
    filter_file_data(readfile);
    return readfile;
}

int get_lvl_files(char *directory,board_t *board){
    DIR *dir = opendir(directory);
    if (!dir){
        debug(OPENDIR_ERR_MSG,directory);
        return ERROR;
    }

    struct dirent *entry;
    char **files = NULL;
    int file_count = 0;
    int dirlen = strlen(directory);

    while ((entry = readdir(dir)) != NULL) {
        int len = strlen(entry->d_name);
        if (strcmp(entry->d_name + (len - 4), ".lvl") == 0) {
            char **tempfiles = realloc(files, (file_count + 1) * sizeof(char *));
            
            if (!tempfiles) {
                for (int i = 0; i < file_count; i++) free(files[i]);
                free(files);
                closedir(dir);
                debug(MEM_ERR_MSG);
                return ERROR;
            }
            files = tempfiles;
            files[file_count] = (char *)malloc((dirlen + len + 2) * sizeof(char));
            snprintf(files[file_count],(dirlen + len + 2) * sizeof(char),"%s/%s",directory, entry->d_name);
            file_count++;
        }
    }

    closedir(dir);
    board->levels = files;
    return file_count;
}
