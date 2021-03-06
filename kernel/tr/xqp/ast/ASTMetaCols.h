/*
 * File:  ASTMetaCols.h
 * Copyright (C) 2009 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#ifndef _AST_META_COLS_H_
#define _AST_META_COLS_H_

#include "ASTNode.h"
class ASTVisitor;

class ASTMetaCols : public ASTNode
{
public:
    bool need_stats;

public:
    ASTMetaCols(const ASTNodeCommonData &loc, bool stats) : ASTNode(loc), need_stats(stats) {}

    ~ASTMetaCols() {}

    void accept(ASTVisitor &v);
    ASTNode *dup();
    void modifyChild(const ASTNode *oldc, ASTNode *newc);

    static ASTNode *createNode(scheme_list &sl);
};

#endif
