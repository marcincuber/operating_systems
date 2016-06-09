#include "util.h"

void fatalError(const char *message)
{
    printf("Error-fatal: %s\n", message);
    exit(1);
}

void consoleError(const char *message)
{
    printf("Error-console: %s\n", message);
}

//delete white spaces
void stripWhitespaces(char *str)
{
  char *start = str;
  while (*start == ' ' || *start == '\t') start++;
  char *end = str + (strlen(str) - 1);
  while (*end == ' ' || *end == '\t' || *end == '\n') end--;
  *(++end) = '\0';
  strcpy(str, start);
}

/*
 * find first unescaped (\) char in the string, the char to look
 * for are passed as a string, this function leaves the given
 * string intact but returns the matched token or \0
 * this is a custom implementation of strtok
 * @param {char*} str str to be evaluated 
 * @param {const char*} delim chars to look for
 * @param {char*} match matching delimiter, \0 in case of no match 
 * or end of string
 * @returns {char*} pointer to matching token <malloc>
 */
char* nextUnescapedTok(char *str, const char *delim, char *match)
{
  if (*str == '\0')
  {
    *match = '\0';
    return NULL;
  }
  char *end = str + (strlen(str));
  char *tmp = NULL;
  const char *curr_delim = delim;
  while (*(curr_delim) != '\0')
  {
    tmp = findUnescapedChar(str, *(curr_delim));
    if (tmp != NULL && end > tmp) end = tmp;
    curr_delim++;
  }
  tmp = malloc(sizeof(char) * (end - str + 1));
  strncpy(tmp, str, (end - str));
  tmp[(end - str)] = '\0';
  *match = *end;
  return tmp;
}
char* findUnescapedChar(char* str, const char c)
{
  bool escaped = false;
  while (*(str) != '\0')
  {
    if (escaped) escaped = false;
    else {
      if (*str == '\\') escaped = true;
      else if (*str == c) return str;
    }
    str++;
  }
  return NULL;
}

//unescaped string
void unescape(char* str)
{
  char *dst = str;
  char *curr = str;
  while (*(curr) != '\0')
  {
    if (*curr != '\\') *(dst++) = *curr;
    curr++;
  }
  *dst = '\0';
} 
//string incrementation
char* strgrow(char* str, int increment)
{
  char *tmp = NULL;
  if (str == NULL)
  {
    tmp = malloc(sizeof(char) * (increment + 1));
    tmp[0] = '\0';
  }
  else
  {
    tmp = malloc(sizeof(char) * 
		       (strlen(str) + increment + 1));
    strcpy(tmp, str);
  }
  return tmp;
}
