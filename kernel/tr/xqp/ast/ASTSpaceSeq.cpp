/*
 * File:  ASTSpaceSeq.cpp
 * Copyright (C) 2009 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#include "tr/xqp/serial/deser.h"

#include "tr/xqp/visitor/ASTVisitor.h"
#include "ASTSpaceSeq.h"

ASTSpaceSeq::~ASTSpaceSeq()
{
    delete expr;
}

void ASTSpaceSeq::accept(ASTVisitor &v)
{
    v.addToPath(this);
    v.visit(*this);
    v.removeFromPath(this);
}

ASTNode *ASTSpaceSeq::dup()
{
    return new ASTSpaceSeq(cd, expr->dup());
}

ASTNode *ASTSpaceSeq::createNode(scheme_list &sl)
{
    ASTNodeCommonData cd;
    ASTNode *expr = NULL;
    ASTSpaceSeq *res;

    U_ASSERT(sl[1].type == SCM_LIST && sl[2].type == SCM_LIST && sl[3].type == SCM_BOOL);

    cd = dsGetASTCommonFromSList(*sl[1].internal.list);
    expr = dsGetASTFromSchemeList(*sl[2].internal.list);

    res = new ASTSpaceSeq(cd, expr);

    res->atomize = sl[3].internal.b;

    return res;
}

void ASTSpaceSeq::modifyChild(const ASTNode *oldc, ASTNode *newc)
{
    if (expr == oldc)
    {
        expr = newc;
        return;
    }
}
