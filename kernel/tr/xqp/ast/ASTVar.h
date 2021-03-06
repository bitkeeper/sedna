/*
 * File:  ASTVar.h
 * Copyright (C) 2009 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#ifndef _AST_VAR_H_
#define _AST_VAR_H_

#include "ASTNode.h"
class ASTVisitor;

#include <string>

class ASTVar : public ASTNode
{
public:
    std::string *pref, *local; // variable name

    std::string *uri; // resolved prefix; added by semantic analyzer

public:
    ASTVar(const ASTNodeCommonData &loc, std::string *var_name) : ASTNode(loc)
    {
        ASTParseQName(var_name, &pref, &local);

        delete var_name;

        uri = NULL;
    }

    ASTVar(const ASTNodeCommonData &loc, std::string *var_pref, std::string *var_local) :
            ASTNode(loc), pref(var_pref), local(var_local)
    {
        uri = NULL;
    }

    ~ASTVar();

    std::string getStandardName() const
    {
        if (*uri == "")
            return *local;
        else
            return "{" + *pref + "}" + *local;
    }

    void accept(ASTVisitor &v);
    ASTNode *dup();
    void modifyChild(const ASTNode *oldc, ASTNode *newc);

    static ASTNode *createNode(scheme_list &sl);
};

#endif
