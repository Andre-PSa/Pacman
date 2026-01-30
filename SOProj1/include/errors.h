#ifndef ERROR_H
#define ERROR_H

#define ERROR -1

//Error messages
#define MEM_ERR_MSG "Memory allocation error\n"
#define OPENDIR_ERR_MSG "Error opening directory %s\n"
#define OPENFILE_ERR_MSG "Error opening file %s\n"
#define READFILE_ERR_MSG "Error reading file %s\n"
#define READLVLFILE_ERR_MSG "Error reading level files\n"
#define GETFILE_ERR_MSG "Error getting filename\n"
#define PACFILE_ERR_MSG "Error loading pacman from file %s\n"
#define GHOSTFILE_ERR_MSG "Error loading ghost from file %s\n"
#define LOADLVL_ERR_MSG "Error loading level\n"
#define FORK_ERR_MSG "Error forking game\n"
#define SAVE_ERR_MSG "Error saving game \n"
#define CHILD_ERR_MSG "Error in child process\n"
#define RESTORE_ERR_MSG "Error restoring game state\n"
#define CREATETHR_ERR_MSG "Error creating thread\n"

#endif