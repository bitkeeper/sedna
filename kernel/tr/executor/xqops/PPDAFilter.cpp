/*
 * File:  PPDAFilter.cpp
 * Copyright (C) 2004 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#include "common/sedna.h"

#include "tr/executor/xqops/PPDAFilter.h"
#include "tr/executor/base/merge.h"
#include "tr/executor/base/visitor/PPVisitor.h"


PPDAFilter::PPDAFilter(dynamic_context *_cxt_,
                       operation_info _info_,
                       PPOpIn _child1_,
                       PPOpIn _child2_) : PPIterator(_cxt_, _info_),
                                          child1(_child1_),
                                          child2(_child2_)
{
}

PPDAFilter::~PPDAFilter()
{
    delete child1.op;
    child1.op = NULL;
    delete child2.op;
    child2.op = NULL;
}

void PPDAFilter::do_open ()
{
    child1.op->open();
    child2.op->open();

    tug_first = true;
    tug_second = true;
}

void PPDAFilter::do_reopen()
{
    child1.op->reopen();
    child2.op->reopen();

    tug_first = true;
    tug_second = true;
}

void PPDAFilter::do_close()
{
    child1.op->close();
    child2.op->close();
}

void PPDAFilter::do_next (tuple &t)
{
        
    while (true)
    {
        if (tug_first)
        {
            child1.op->next(t);
            if (t.is_eos())
            {
                tug_first = true;
                tug_second = true;

                child2.op->reopen();

                return;
            }
            else
            {
                if (!child1.get(t).is_node()) throw XQUERY_EXCEPTION2(XPTY0004, "Argument of ancestor-descendant-filter is not a node");
                xptr1 = child1.get(t).get_node();
            }

            tug_first = false;
        }

        if (tug_second)
        {
            child2.op->next(t);
            if (t.is_eos())
            {
                tug_first = true;
                tug_second = true;

                child1.op->reopen();

                return;
            }
            else
            {
                if (!child2.get(t).is_node()) throw XQUERY_EXCEPTION2(XPTY0004, "Argument of ancestor-descendant-filter is not a node");
                xptr2 = child2.get(t).get_node();
            }

            tug_second = false;
        }


        switch (nid_cmp_effective(xptr1, xptr2))
        {
            case -2: /// (1) ancestor (2)
            {
                tug_second = true;
                t.copy(tuple_cell::node(xptr2));
                return;
            }
            case -1: /// (1) < (2)
            {
                tug_first = true;
                break;
            }
            case  0: /// (1) == (2)
            {
                tug_first = true;
                tug_second = true;
                t.copy(tuple_cell::node(xptr2));
                return;
            }
            case  1: /// (1) > (2)
            {
                tug_second = true;
                break;
            }
            case  2: /// (1) descendant (2)
            {
                tug_second = true;
                break;
            }
            default: throw USER_EXCEPTION2(SE1003, "Impossible case in PPDAFilter::next");
        }
    }
}

PPIterator* PPDAFilter::do_copy(dynamic_context *_cxt_)
{
    PPDAFilter *res = se_new PPDAFilter(_cxt_, info, child1, child2);
    res->child1.op = child1.op->copy(_cxt_);
    res->child2.op = child2.op->copy(_cxt_);
    return res;
}

void PPDAFilter::do_accept(PPVisitor &v)
{
    v.push  (this);
    v.visit (this);
    child1.op->accept(v);
    child2.op->accept(v);
    v.pop();
}
