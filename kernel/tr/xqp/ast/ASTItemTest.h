/*
 * File:  ASTItemTest.h
 * Copyright (C) 2009 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#ifndef _AST_ITEM_TEST_H_
#define _AST_ITEM_TEST_H_

#include "ASTNode.h"
#include "AST.h"

class ASTItemTest : public ASTNode
{
public:
    ASTItemTest(ASTLocation &loc) : ASTNode(loc) {}

    ~ASTItemTest() {}

    void accept(ASTVisitor &v);
    ASTNode *dup();
};

#endif