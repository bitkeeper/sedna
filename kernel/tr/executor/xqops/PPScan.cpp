/*
 * File:  PPScan.cpp
 * Copyright (C) 2004 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#include "common/sedna.h"
#include "tr/executor/xqops/PPScan.h"
#include "tr/crmutils/node_utils.h"
#include "tr/executor/base/PPUtils.h"



PPScan::PPScan(dynamic_context *_cxt_, 
               schema_node *_scm_node_,
               counted_ptr<db_entity> _db_ent_) : PPIterator(_cxt_),
                                                  scm_node(_scm_node_),
                                                  db_ent(_db_ent_)
{
}

PPScan::~PPScan()
{
}

void PPScan::open ()
{
    // Put lock on documents and check security
    schema_node *root = get_schema_node(db_ent, "Unknown entity passed to PPScan");

    res = XNULL;
}


void PPScan::reopen()
{
    res = XNULL;
}


void PPScan::close ()
{
}

void PPScan::next(tuple &t)
{
    SET_XQUERY_LINE(__xquery_line);
    
    if (res == NULL)
    {
        res = scm_node->bblk;

        if (res == NULL)
        {
            t.set_eos();
            {UNDO_XQUERY_LINE; return;}
        }

        CHECKP(res);
        res = GETBLOCKFIRSTDESCRIPTORABSOLUTE(XADDR(res));

        if (res == NULL)
            t.set_eos();
        else
            t.copy(tuple_cell::node(res));

        {UNDO_XQUERY_LINE; return;}
    }

    res = getNextDescriptorOfSameSortXptr(res);
    if (res == NULL)
        t.set_eos();
    else
        t.copy(tuple_cell::node(res));

    UNDO_XQUERY_LINE;
}

PPIterator* PPScan::copy(dynamic_context *_cxt_)
{
    PPScan *res = se_new PPScan(_cxt_, scm_node, db_ent);
    res->set_xquery_line(__xquery_line);
    return res;
}

bool PPScan::result(PPIterator* cur, dynamic_context *cxt, void*& r)
{
    throw USER_EXCEPTION2(SE1002, "PPScan::result");
}

