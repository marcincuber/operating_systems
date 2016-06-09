#include "shell.h"


int main (int argc, char *argv[], char *envp[])
{
  env_t env = createEnv();
  parseProfile(env);
  OPTN_VAR_STARTER = '$';
  
    //check that PATH and HOME exists and are valid
  if (!checkShellEnv(env)) 
    fatalError("Environment's PATH and HOME are incorrect");
  //main command listening loop
  command_t *cmd;
  char buff[CMD_LENGTH];
    
  while (true)
  {
    prompt(buff);
    cmd = parseCommand(buff, env);
    execCommand(cmd, env);
    deleteCommand(cmd);
  }

  deleteEnv(env);
  exit(0);
}
