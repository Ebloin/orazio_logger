#pragma once
#include "orazio_shell_globals.h"

#ifdef __cplusplus
extern "C" {
#endif

  typedef int (*CmdFn)(char* response, char** args);
  typedef struct {
    const char* name;
    int n_args;
    CmdFn cmd_fn;
    const char* help;
  } Command;

  extern Command commands[];

  extern const int num_commands;

  Command* findCommand(const char* name);

  int executeCommand(char* response, const char* line_);

#ifdef __cplusplus
}
#endif
