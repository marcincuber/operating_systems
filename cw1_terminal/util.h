#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//erros
void fatalError(const char *message);
void consoleError(const char *message);

//delete white spaces
void stripWhitespaces(char * str);
//find first (\)
char* findUnescapedChar(char* str, const char c);
//search next unescaped
char* nextUnescapedTok(char *str, const char *delim, char *match);

//unescape string
void unescape(char* str);
//string length count
char* strgrow(char* str, int increment);
