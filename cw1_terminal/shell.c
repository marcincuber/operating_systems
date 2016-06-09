#include "shell.h"

//Main shall file which contains entire configuration

//profile
const char PROFILE[] = "profile";

//command assignment
const char CMD_ASSIGN[] = "=";
const char CMD_CD[] = "cd";
const char CMD_EXIT[] = "exit";

//size of the buffer specified for command
const int CMD_LENGTH = 1024;
//starter
char OPTN_VAR_STARTER = '\0';

//parsing functions and data structures
typedef enum {
  PARSE_VAR,
  PARSE_STRING,
  PARSE_NORMAL
} parse_policy_t;

//list for commands
typedef struct argv_list {
    struct argv_list* next;
    char *value;
} argv_t;

typedef struct Position{
  parse_policy_t policy;
  parse_policy_t prev_policy;
  argv_t **argv_list;
  env_t env;
  char *parsing_buffer;
  int argc;
  
  char *current_token;
  char current_sep;
} parse_state_t;

//parsing functions for each policy
void parseVarState(parse_state_t*);
void parseNormalState(parse_state_t*);
void parseStringState(parse_state_t*);

//"private" functions
var_t* parseVar(char *buff);
void parseSpecial(command_t *cmd, env_t env);
char* getFullPath(const char *file, var_t *path);
char* getCurrentWorkingDirectory();
int command2List(argv_t **head, char *cmdString, env_t env);
void expandEnv(command_t *cmd, env_t env);
bool checkHome(char *home);
bool checkPath(char *path);

//profile-string
var_t* parseVar(char *buff)
{
  if (buff == NULL)
    return NULL;
  stripWhitespaces(buff);
  char *comment = strchr(buff, '#');
  if (comment != NULL)
    (*comment) = '\0';
  //parse string
  if (strlen(buff) == 0)
    //empty string or comment, no variable parsed
    return NULL;
  //if a variable starter symbol is defined
  if (OPTN_VAR_STARTER != '\0')
  {
    //check var name syntax
    char *var_starter = strchr(buff, OPTN_VAR_STARTER);
    //note that if var_starter == NULL the error is catched anyway
    //because buffer can not be null at this point
    if (var_starter != buff)
    {
      //syntax of variable is incorrect
      return NULL;
    }
    else {
      //remove var starter and parse normally
      //strcpy manually because don't trust strcpy implementation
      //for copying on the same buffer
      char *dst = buff;
      var_starter++;
      while (*var_starter != '\0')
	*(dst++) = *(var_starter++);
      *dst = '\0';
    }
  }
  char *var_delim = strchr(buff, '=');
  if (var_delim == NULL) {
    return NULL;
  }
  var_t *var = malloc(sizeof(var_t));
  int length = (var_delim - buff);
  var->name = malloc(sizeof(char) * (length + 1));
  strncpy(var->name, buff, length);
  var->name[length] = '\0';
  stripWhitespaces(var->name);
  length = strlen(var_delim + 1);
  var->value = malloc(sizeof(char) * (length + 1));
  strncpy(var->value, var_delim + 1, length + 1);
  stripWhitespaces(var->value);
  return var;
}

//special cmd
void parseSpecial(command_t *cmd, env_t env)
{
  //if command is valid
  if (cmd->argc == 0) 
    return;
  if (strcmp(cmd->argv[0], CMD_CD) == 0)
  {
    //parse special meaning for no argument
    //parse special meaning for ~
    var_t *home = getEnvVar(env, "HOME");
    if (cmd->argc == 1)
    {
      //if cd without arguments was called
      cmd->argc = 2;
      //add home path as argument
      p_string_t tmp = malloc(sizeof(char*) * 3);
      tmp[0] = cmd->argv[0];
      tmp[1] = malloc(sizeof(char) * (strlen(home->value) + 1));
      tmp[2] = NULL;
      strcpy(tmp[1], home->value);
      //remove old argv because it was too short
      free(cmd->argv);
      cmd->argv = tmp;
    } 
    else if (cmd->argc == 2 && (strchr(cmd->argv[1], '~') == cmd->argv[1]))
    {
      //create new string for the full path
      //len = length of the two strings - num of char to strip + null char
      int len = strlen(home->value) + strlen(cmd->argv[1]) - 1 + 1;
      char *cd_path = malloc(sizeof(char) * len);
      cd_path[0] = '\0';
      //put home path at the beginning of the argument
      strcpy(cd_path, home->value);
      if (strlen(cmd->argv[1]) > 1)
      {
	//if there was something after ~ append it
	strcpy(cd_path + strlen(cd_path), cmd->argv[1] + 1);
      }
      //delete old string
      free(cmd->argv[1]);
      //put new cd path as agument
      cmd->argv[1] = cd_path;
    }
  }
}


//add token to argv list
void addToken(char *str, argv_t **head)
{
  if (strlen(str) == 0) 
    return;
  //temp str
  char *tmp = malloc(sizeof(char) * strlen(str));
  strcpy(tmp, str);
  unescape(tmp);
  argv_t *current;
  if (*(head) == NULL)
  {
    *(head) = malloc(sizeof(argv_t));
    current = *(head);
    current->next = NULL;
  }
  else
  {
    //append
    for(current = *(head); current->next != NULL; current = current->next);
    //insert-end
    current->next = malloc(sizeof(argv_t));
    current->next->next = NULL;
    current = current->next;
  }
  //copy value string to new malloc string buffer
  current->value = malloc(sizeof(char) * (strlen(str) + 1));
  strcpy(current->value, tmp);
  free(tmp);
}

//parse those which match ""
 void parseNormalState(parse_state_t* state)
{
  //add token as an argument and change handler
  state->argc++;
  addToken(state->current_token, state->argv_list);
  state->prev_policy = PARSE_NORMAL;
  switch (state->current_sep)
    {
  case '"':
    state->policy = PARSE_STRING;
    break;
  case '$':
    state->policy = PARSE_VAR;
    break;
  case ' ':
    //stay in the same state
    state->policy = PARSE_NORMAL;
    break;
  }
}

//parsing
void parseVarState(parse_state_t* state)
{
  var_t *var = getEnvVar(state->env, state->current_token);
  if (state->prev_policy == PARSE_STRING)
  {
    
    char* tmp_free_holder = state->parsing_buffer;
    int grow_length = strlen(var->value);
    if (state->current_sep == ' ')
      grow_length++;
    state->parsing_buffer = strgrow(state->parsing_buffer,
				    grow_length);
   if (tmp_free_holder != NULL) 
     free(tmp_free_holder);
   strcat(state->parsing_buffer, var->value);
   if (state->current_sep == ' ')
     strcat(state->parsing_buffer, " ");
   }
  else
  {
    //var->arg
    state->argc++;
    if (var != NULL) addToken(var->value, state->argv_list);
  }
  switch (state->current_sep)
    {
  case '"':
    if (state->prev_policy == PARSE_STRING)
    {
      state->policy = PARSE_NORMAL;
      addToken(state->parsing_buffer, state->argv_list);
      free(state->parsing_buffer);
      state->parsing_buffer = NULL;
    }
    else
    {
      state->policy = PARSE_STRING;
    }
    state->prev_policy = PARSE_VAR;
    break;
  case ' ':
    if (state->prev_policy == PARSE_STRING)
    {
      state->policy = PARSE_STRING;
    }
    else
    {
      state->policy = PARSE_NORMAL;
    }
    state->prev_policy = PARSE_VAR;
  }
}
//func for tokens
void parseStringState(parse_state_t* state)
{
  //append
  char* tmp_free_holder = state->parsing_buffer;
  int grow_length = strlen(state->current_token);
  if (state->current_sep == ' ')
    grow_length++;
  state->parsing_buffer = strgrow(state->parsing_buffer,grow_length);
  //free temp string
  if (tmp_free_holder != NULL) 
    free(tmp_free_holder);
  strcat(state->parsing_buffer, state->current_token);
  if (state->current_sep == ' ')
    strcat(state->parsing_buffer, " ");
  switch (state->current_sep)
    {
  case '"':
    //end of string, flush buffer
    state->argc++;
    state->prev_policy = PARSE_STRING;
    state->policy = PARSE_NORMAL;
    addToken(state->parsing_buffer, state->argv_list);
    //free temporary buffer
    free(state->parsing_buffer);
    state->parsing_buffer = NULL;
    break;
  case '$':
    //switch to parsing variable inside string
    state->prev_policy = PARSE_STRING;
    state->policy = PARSE_VAR;
    break;
  }
}

/*
 * put command arguments in a temporary list
 * @param {argv_t**} head head of the list
 * @param {char*} buffer command to be parsed
 * @returns {int} arguments count
 */
int command2List(argv_t **head, char *buff, env_t env)
{
  //init parsing state
  parse_state_t state;
  state.policy = PARSE_NORMAL;
  state.prev_policy = PARSE_NORMAL;
  state.argv_list = head;
  state.env = env;
  state.parsing_buffer = NULL;
  state.argc = 0;
  state.current_token = NULL;
  state.current_sep = '\0';
  //begin parsing
  //token string <malloc> by findFirstUnescapedChar
  //note that token refers to everything BEFORE the separator
  state.current_token = nextUnescapedTok(buff, " \"$",
					 &state.current_sep);
  while(state.current_token != NULL)
  {
    //handle parsing of current token
    switch (state.policy) {
    case PARSE_NORMAL:
      parseNormalState(&state);
      break;
    case PARSE_STRING:
      parseStringState(&state);
      break;
    case PARSE_VAR:
      parseVarState(&state);
      break;
    }
    buff += strlen(state.current_token); //skip token length
    if (state.current_sep != '\0') buff++; //skip separator char
    state.current_token = nextUnescapedTok(buff, " \"$",
					   &state.current_sep);
  }
  //check that parsing have finished correctly
  if (state.policy == PARSE_STRING)
  {
    consoleError("Syntax Error");
    return 0;
  }
  return state.argc;
}

/*
 * parse a command
 * @param {char*} buffer input string
 * @param {environment_t} env pointer to head of environment
 * variables list
 * @returns {command_t*} parsed command <malloc>
 */
command_t* parseCommand(char *buff, env_t env) {
  command_t *cmd = malloc(sizeof(command_t));
  //init command to avoid random behavior in case of error
  cmd->argc = 0;
  cmd->argv = NULL;
  cmd->envp = NULL;
  cmd->exitStatus = 0;
  //check if command inserted is a variable assignment
  var_t *var = parseVar(buff);
  if (var != NULL)
  {
    cmd->argc = 4; 
    //one argv slot is for NULL pointer signalisng 
    //the end of the argv array
    cmd->argv = malloc(cmd->argc * sizeof(char*));
    cmd->argv[0] = malloc(sizeof(char) * (strlen(CMD_ASSIGN) + 1));
    *(cmd->argv[0]) = '\0';
    strcpy(cmd->argv[0], CMD_ASSIGN);
    cmd->argv[1] = var->name;
    cmd->argv[2] = var->value;
    cmd->argv[3] = NULL;
    free(var);
    return cmd;
  }
  //if it is an actual command
  argv_t *list = NULL;
  cmd->argc = command2List(&list, buff, env);
  //build argv array
  //last argv location is set to NULL
  cmd->argv = malloc(sizeof(char*) * (cmd->argc + 1));
  argv_t *current = list;
  int index = 0;
  while (current != NULL)
  {
    cmd->argv[index] = current->value;
    argv_t *tmp = current->next;
    free(current);
    current = tmp;
    index++;
  }
  cmd->argv[index] = NULL;
  //build envp array
  int envLength = 0;
  profile_t *env_var = *(env);
  for (; env_var != NULL; env_var = env_var->next) envLength++;
  cmd->envp = malloc(sizeof(char*) * (envLength + 1));
  index = 0;
  env_var = *(env);
  while (env_var != NULL)
  {
    cmd->envp[index] = malloc(sizeof(char) * 
			      (strlen(env_var->var.name) + 
			       strlen(env_var->var.value) + 
			       2));
    cmd->envp[index][0] = '\0';
    strcat(cmd->envp[index], env_var->var.name);
    strcat(cmd->envp[index], "=");
    strcat(cmd->envp[index], env_var->var.value);
    env_var = env_var->next;
    index++;
  }
  cmd->envp[index] = NULL;
  //parsing specific to special commands
  parseSpecial(cmd, env);
  //expand $ variables
  //expandEnv(cmd, env);
  return cmd;
}

/*
 * prompt the user for a command
 * @param {environment_t} env evironment variables list
 * @param {char*} env list of environment variables
 */
void prompt(char *cmd)
{
  char *cwd = getCurrentWorkingDirectory();
  if (cwd != NULL) 
    printf("%s>", cwd);
  else 
    printf("unknown>");
  fgets(cmd, CMD_LENGTH, stdin);
  free(cwd);
}

/*
 * parse profile file
 * @param {environment_t} env environment variables list
 */
void parseProfile(env_t env)
{
  char *cwd = getCurrentWorkingDirectory();
  if (cwd == NULL) {
    printf("Error, could not get current working directory");
    exit(1);
  }
  char *profilePath;
  profilePath = malloc(sizeof(char) * 
		       (strlen(cwd) + 
			strlen(PROFILE) + 1));
  profilePath[0] = '\0';
  strcat(profilePath, cwd);
  strcat(profilePath, "/");
  strcat(profilePath, PROFILE);
  FILE *profileFile;
  //open profile file in current working dir
  profileFile = fopen(profilePath, "r");
  if (profileFile == NULL)
  {
    printf("Error, could not open profile file at %s\n", profilePath);
    free(profilePath);
    exit(1);
  }
  free(profilePath);
  free(cwd);
  //parse profile file content
  var_t *var = NULL;
  char buffer[1024];
  while (!feof(profileFile))
  {
    fgets(buffer, sizeof(buffer), profileFile);
    var = parseVar(buffer);
    if (var == NULL) continue;
    if (var->name != NULL && var->value != NULL)
    {
      updateEnvVar(env, *var);
      free(var);
    }
    else {
      //unused var
      free(var->name);
      free(var->value);
      free(var);
    }
  }
  //set cwd to home
  var_t *home = getEnvVar(env, "HOME");
  if (home != NULL)
  {
    chdir(home->value);
  }
  //else leave cwd as-is
}

/*
 * generate full path if file is found in path
 * @param {char*} file filename to look for
 * @param {var_t*} path path environment variable
 * @returns {char*} full path string <malloc>
 */
char* getFullPath(const char *file, var_t *path)
{
  char *tmp_path = malloc(sizeof(char) * (strlen(path->value) + 1));
  strcpy(tmp_path, path->value);
  char *full_path = NULL;
  char *tok = strtok(tmp_path, ":");
  while (tok != NULL)
  {
    DIR *dir = opendir(tok);
    if (dir == NULL) fatalError("PATH directory does not exist");
    struct dirent *dir_entity = readdir(dir);
    while (dir_entity != NULL)
    {
      if (strcmp(dir_entity->d_name, file) == 0)
      {
	//found file in this directory
	full_path = malloc(sizeof(char) * 
			   (strlen(file) + strlen(tok) + 2));
	full_path[0] = '\0';
	strcat(full_path, tok);
	strcat(full_path, "/");
	strcat(full_path, file);
	break;
      }
      dir_entity = readdir(dir);
    }
    tok = strtok(NULL, ":");
  }
  return full_path;
}

void execCommand(command_t *cmd, env_t env)
{
  if (cmd->argc <= 0)
  {
    //empty or unparsed command
    return;
  }
  //check for special commands
  if (strcmp(cmd->argv[0], CMD_ASSIGN) == 0)
  {
    //if assigning to HOME or PATH verify assigment first
    if (strcmp(cmd->argv[1], "HOME") == 0 && 
	! checkHome(cmd->argv[2])) {
      consoleError("invalid HOME");
      return;
    }
    if (strcmp(cmd->argv[1], "PATH") == 0 &&
	! checkPath(cmd->argv[2]))
    {
      consoleError("invalid PATH");
      return;
    }
    //variable assignment command
    var_t var;
    //copy vars from cmd so if cmd is free'd it doesn't matter
    var.name = malloc(sizeof(char) * (strlen(cmd->argv[1]) + 1));
    strcpy(var.name, cmd->argv[1]);
    var.value = malloc(sizeof(char) * (strlen(cmd->argv[2]) + 1));
    strcpy(var.value, cmd->argv[2]);
    updateEnvVar(env, var);
    return;
  }
  if (strcmp(cmd->argv[0], CMD_CD) == 0)
  {
    if(chdir(cmd->argv[1]) == -1) {
      //error
      consoleError("can not cd there!");
    }
    return;
  }
  if (strcmp(cmd->argv[0], CMD_EXIT) == 0)
  {
    if (cmd->argc == 2) {
      exit(atoi(cmd->argv[1]));
    }
    exit(0);
  }
  //normal command execution
  char *full_path = getFullPath(cmd->argv[0], getEnvVar(env,"PATH"));
  if (full_path == NULL)
  {
    consoleError("Unexisting command");
    return;
  }
  //fork
  pid_t pid = fork();
  if (pid < 0)
  {
    //error
    fatalError("Fork failed while executing command");
  }
  else if (pid == 0)
  {
    //child
    execve(full_path, cmd->argv, cmd->envp);
  }
  else
  {
    //parent
    waitpid(pid, &cmd->exitStatus, 0);
  }
  free(full_path);
}

//couple of helpers to deal with environment variables

/*
 * create environment variables list and init it
 * @returns {environment_t}
 */
env_t createEnv()
{
  env_t head = malloc(sizeof(profile_t*));
  *(head) = NULL;
  return head;
}

/*
 * check that PATH and HOME are set correctly
 * @param {environment_t} env environment var list
 * @returns {bool}
 */
bool checkShellEnv(env_t env)
{
  //particular env vars that are required
  var_t *home = getEnvVar(env, "HOME"); 
  var_t *path = getEnvVar(env, "PATH");
  //find PATH and HOME
  if (! checkHome(home->value)) return false;
  if (! checkPath(path->value)) return false;
  return true;
}

/*
 * check that HOME var is set to a valid value
 * @param {char*} home pointer to env var HOME value
 * @returns {boolean}
 */
bool checkHome(char *home)
{
  if (home != NULL && opendir(home) != NULL)
    return true;
  return false;
}

/*
 * check that PATH var is set to a valid value
 * @param {char*} path pointer to env var PATH value
 * @returns {boolean}
 */
bool checkPath(char *path)
{
  //strtok modify its input so make a copy
  char *tmp_path = malloc(sizeof(char) * (strlen(path) + 1));
  strcpy(tmp_path, path);
  char *tok = strtok(tmp_path, ":");
  bool ok = true;
  while (tok != NULL)
  {
    if (opendir(tok) == NULL)
    {
      ok = false;
      break;
    }
    tok = strtok(NULL, ":");
  }
  free(tmp_path);
  return ok;
}

/*
 * get environment variable from name
 * @param {environment_t} env environment var list
 * @param {const char*} name name of the var to look for
 * @returns {var_t*} pointer to env var
 */
var_t* getEnvVar(env_t env, const char *name)
{
  profile_t *current = *(env);
  for (; current != NULL; current = current->next)
  {
    if (strcmp(current->var.name, name) == 0)
    {
      return &(current->var);
    }
  }
  return NULL;
}

/*
 * update environment variable or add unexisting one
 * @param {environment_t} env list of environment variables
 * @param {var_t} var variable structure
 */
void updateEnvVar(env_t env, var_t var)
{
if (var.name != NULL && var.value != NULL)
{
  //search for already declared variable
  profile_t *found = *(env);
  for (; found != NULL; found = found->next)
  {
    if (strcmp(found->var.name, var.name) == 0) break;
  }
  if (found != NULL) {
    free(found->var.value);
    found->var.value = var.value;
  }
  else {
    //append var in the profileList
    profile_t *tmp = malloc(sizeof(profile_t));
    tmp->var.name = var.name;
    tmp->var.value = var.value;
    tmp->next = *(env);
    *(env) = tmp;
  }
 }
}

void expandEnv(command_t *cmd, env_t env)
{
  p_string_t current_arg = cmd->argv;
  char *token = NULL;
  var_t *var = NULL;
  char *tmp = NULL;
  int pre_token_length = 0;
  while (*(current_arg) != NULL)
  {
    token = findUnescapedChar(*(current_arg), '$');
    while (token != NULL)
    {
    //calc length
      pre_token_length = (token - *(current_arg));
      token++;
      if ((var = getEnvVar(env, token)) != NULL)
      {
	tmp = malloc(sizeof(char) * 
		     (pre_token_length + strlen(var->value) + 1));
	tmp[0] = '\0';
	strncpy(tmp, *(current_arg), pre_token_length);
	strcat(tmp, var->value);
	free(*(current_arg));
	*(current_arg) = tmp;
      }
      token = findUnescapedChar(token, '$');
    }
    current_arg++;
  }
}

//delte cmd
void deleteCommand(command_t *cmd) {
  p_string_t tmp;
  if (cmd->argv != NULL) {
    tmp = cmd->argv;
    while (*tmp != NULL) {
      free(*tmp);
      tmp = tmp + 1;
    }
    free(cmd->argv);
  }
  if (cmd->envp != NULL) { 
    tmp = cmd->envp;
    while (*tmp != NULL) {
      free(*tmp);
      tmp = tmp + 1;
    }
    free(cmd->envp);
  }
  free(cmd);
}

//clear env list
void deleteEnv(env_t env)
{
  profile_t *current = *(env);
  while (current != NULL)
  {
    profile_t *tmp = current->next;
    free(current->var.name);
    free(current->var.value);
    free(current);
    current = tmp;
  }
  free(env);
}

//get current dir
char* getCurrentWorkingDirectory()
{
  const int size_step = 128;
  int current_size = size_step;
  char *cwd = malloc(sizeof(char) * size_step);
  while (getcwd(cwd, current_size) == NULL)
  {
    free(cwd);
    if (errno == ERANGE)
    {
      current_size += size_step;
      cwd = malloc(sizeof(char) * current_size);
    }
    else
    {
      return NULL;
    }
  }
  return cwd;
}
