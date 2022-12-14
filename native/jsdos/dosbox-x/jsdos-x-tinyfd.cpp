//
// Created by Alexander Guryanov on 14/12/22.
//
#include "logging.h"
#include <cstdlib>

extern "C" char * tinyfd_inputBox(
    char const * aTitle , /* NULL or "" */
    char const * aMessage , /* NULL or "" (\n and \t have no effect) */
    char const * aDefaultInput ) /* "" , if NULL it's a passwordBox */
{
  char* empty = (char*) malloc(1);
  empty[0] = 0;
  return empty;
}

extern "C" int tinyfd_messageBox(
    char const * aTitle, /* NULL or "" */
    char const * aMessage, /* NULL or ""  may contain \n and \t */
    char const * aDialogType, /* "ok" "okcancel" "yesno" "yesnocancel" */
    char const * aIconType, /* "info" "warning" "error" "question" */
    int aDefaultButton) /* 0 for cancel/no , 1 for ok/yes , 2 for no in yesnocancel */
{
  LOG_MSG("%s: %s\n",
          aTitle ? aTitle : "tinyfd: no title",
          aMessage ? aMessage : "tinyfd: no message");
  return 0;
}

extern "C" char * tinyfd_saveFileDialog(
    char const * aTitle , /* NULL or "" */
    char const * aDefaultPathAndFile , /* NULL or "" */
    int aNumOfFilterPatterns , /* 0 */
    char const * const * aFilterPatterns , /* NULL or {"*.jpg","*.png"} */
    char const * aSingleFilterDescription ) /* NULL or "image files" */
{
  LOG_MSG("%s: %s\n",
          "tinyfd", 
          "saveFileDialog is not supported");
  return nullptr;
}
