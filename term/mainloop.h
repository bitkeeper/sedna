/*
 * File:  mainloop.h
 * Copyright (C) 2004 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */
#ifndef MAINLOOP_H
#define MAINLOOP_H

#include <vector>
#include "common/sedna.h"
#include "libsedna.h"


int			MainLoop(FILE *source);

int         process_command(char* buffer);

int         process_query(char* buffer, bool is_query_from_file, char* tmp_file_name);

int         get_input_item(FILE* source, std::vector<char> & buffer);

#endif   /* MAINLOOP_H */
