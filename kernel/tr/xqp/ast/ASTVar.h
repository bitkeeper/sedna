/*
 * File:  ASTVar.h
 * Copyright (C) 2009 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#ifndef _AST_VAR_H_
#define _AST_VAR_H_

#include "ASTNode.h"
#include "AST.h"

#include <string>

class ASTVar : public ASTNode
{
public:
    std::string *pref, *local; // pragma name

public:
    ASTVar(ASTLocation &loc, std::string *var_name) : ASTNode(loc)
    {
        ASTParseQName(var_name, &pref, &local);

        delete var_name;
    }

    ASTVar(ASTLocation &loc, std::string *var_pref, std::string *var_local) : ASTNode(loc), pref(var_pref), local(var_local) {}

    ~ASTVar();

    void accept(ASTVisitor &v);
    ASTNode *dup();
};

#endif
