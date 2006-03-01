/*
 * File:  exceptions.cpp
 * Copyright (C) 2004 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#include <stdio.h>
#include "exceptions.h"
#include "uprocess.h"


void sedna_soft_fault(const SednaException &e)
{
    fprintf(stderr, "Soft fault with the following error\n%s", e.getMsg().c_str());
    fflush(stdout);
    uExitProcess(1);
}

void sedna_soft_fault()
{
    fprintf(stderr, "Soft fault with unknown error\n");
    fflush(stdout);
    uExitProcess(1);
}

