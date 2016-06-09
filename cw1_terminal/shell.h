#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
//files
#include "util.h"

#ifndef SHELL_H
#define SHELL_H

typedef char** p_string_t;

typedef struct {
  char *name;
  char *value;
} var_t;

typedef struct profile_elem{
  struct profile_elem *next;
  var_t var;
} profile_t;

typedef struct {
  p_string_t argv;
  p_string_t envp;
  int argc;
  int exitStatus;
} command_t;

typedef profile_t** env_t;

//max command length defined in shell.c
const int CMD_LENGTH;
//variable starting char defined in shell.c
char OPTN_VAR_STARTER;

//promt with cmd
void prompt(char *cmd);
void parseProfile(env_t);

//shell command
command_t* parseCommand(char *buff, env_t env);

void execCommand(command_t *cmd, env_t env);

//utilities for env variables handling
//check profile
bool checkShellEnv(env_t env);

//env var name and update + list
void updateEnvVar(env_t env, var_t var);
var_t* getEnvVar(env_t env, const char *name);
env_t createEnv();

//memory
//free memory cmd/env
void deleteCommand(command_t *cmd);
void deleteEnv(env_t env);

#endif
