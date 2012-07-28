/*
 * File:  PPQueryRoot.cpp
 * Copyright (C) 2004 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */


#include "common/sedna.h"

#include "tr/executor/root/PPQueryRoot.h"
#include "tr/executor/base/dm_accessors.h"
#include "tr/executor/base/visitor/PPVisitor.h"
#include "tr/crmutils/serialization.h"
#include "tr/locks/locks.h"
#include "tr/tr_globals.h"
#include "tr/opt/algebra/IndependentPlan.h"
#include "tr/opt/algebra/PlanAlgorithms.h"
#include "tr/models/XmlConstructor.h"

#include "tr/opt/algorithms/ExecutionContext.h"

PPQueryRoot::PPQueryRoot(dynamic_context *_cxt_,
                         PPOpIn _child_) :       PPQueryEssence("PPQueryRoot"),
                                                 child(_child_),
                                                 data(_child_.ts),
                                                 cxt(_cxt_)
{
}

PPQueryRoot::~PPQueryRoot()
{
    delete child.op;
    child.op = NULL;
    delete cxt;
    cxt = NULL;
}

void PPQueryRoot::do_open()
{
    setDefaultSpace(local_space_base);
    local_lock_mrg->lock(lm_s);
    first = true;
    cxt->global_variables_open();
    child.op->open();
}

void PPQueryRoot::do_close()
{
    child.op->close();
    cxt->global_variables_close();
    popDefaultSpace();
}

void PPQueryRoot::do_accept(PPVisitor &v)
{
    v.visit (this);
    v.push  (this);
    child.op->accept(v);
    v.pop();
}

bool PPQueryRoot::next()
{
    if(executor_globals::profiler_mode)
    {
        U_ASSERT(info.profile.get() != NULL);
        info.profile->calls++;
        u_ftime(&current1);
    }
    if(1 == tr_globals::debug_mode)
    {
        U_ASSERT(info.profile.get() != NULL);
        if(!executor_globals::profiler_mode) info.profile->calls++;
        executor_globals::pp_stack.push_back(info);
    }

    bool result = PPQueryRoot::do_next();

    if(1 == tr_globals::debug_mode)
        executor_globals::pp_stack.pop_back();
    if(executor_globals::profiler_mode)
    {
        u_ftime(&current2);
        info.profile->time = current2 - current1;
    }

    return result;
}

static
std::string schemaPath(schema_node_cptr snode) {
    std::stringstream path;
    std::stack<schema_node_cptr> path_sn;

    while (snode.found()) {
        path_sn.push(snode);
        snode = snode->parent;
    };

    while (!path_sn.empty()) {
        path << path_sn.top()->get_qname().getColonizedName().c_str() << "/";
        path_sn.pop();
    }

    return path.str();
};

void sendTupleCellToClient(const tuple_cell & tc)
{
    GlobalSerializationOptions options;
    bool is_node = tc.is_node();
    
    tr_globals::create_serializer(tr_globals::client->get_result_type());
    tr_globals::serializer->prepare(tr_globals::client->get_se_ostream(), &options);
    tr_globals::client->begin_item(is_node, xs_untyped, element, "");
    tr_globals::serializer->serialize(tuple(tc));
    
};

bool PPQueryRoot::do_next()
{
    static int hui = 0;

    if (first) {
        first = false;
        hui = 0;

        XmlConstructor xmlConstructor(VirtualRootConstructor(0));
        data.cells[0] = optimizedPlan->toXML(xmlConstructor).getLastChild();
        sendTupleCellToClient(data.cells[0]);

        hui = 1;

    //    data.cells[0].set_eos();

        return true;
    } else {
        if (hui == 1) {
            if (optimizedPlan != NULL) {
                optimizedPlan = rqp::PlanRewriter::rewrite(optimizedPlan);
            }

            XmlConstructor xmlConstructor(VirtualRootConstructor(0));
            xmlConstructor.openElement(SE_EL_NAME("plan"));
            optimizedPlan->toXML(xmlConstructor);
            optimizer->dgm()->toXML(xmlConstructor);
            xmlConstructor.closeElement();

            data.cells[0] = xmlConstructor.getLastChild();
            sendTupleCellToClient(data.cells[0]);

            hui = 2;
            return true;
        } else if (hui == 2 || hui == 3) {
            if (hui == 2) {
                optimizer->pexecutor()->execute(optimizedPlan);
                hui = 3;
            }

            data.cells[0] = optimizer->pexecutor()->rootStack->next();
            if (!data.cells[0].is_eos()) {
/*
                if (data.cells[0].is_node()) {
                    data.cells[0] =
                      tuple_cell::atomic_deep(xs_string, schemaPath(getSchemaNode(data.cells[0].get_node())).c_str());
                };
*/
                sendTupleCellToClient(data.cells[0]);
                return true;
            }

            first = true;
            return false;
        };
    };

    if (first) {
        tr_globals::create_serializer(tr_globals::client->get_result_type());
    }

    child.op->next(data);

    if (data.is_eos()) return false;

    if(data.size() != 1)
        throw XQUERY_EXCEPTION2(SE1003, "Result of the query contains nested sequences");

    tuple_cell tc = data.cells[0];

    t_item nt = element;   /* nt will be ignored if tuple cell contains atomic value */
    xmlscm_type st = 0;    /* by default there is no type (anyType) */
    bool is_node;
    str_counted_ptr uri;   /* for document and attribute nodes */

    if((is_node = tc.is_node())) {
        /* Determine node type */
        xptr node = tc.get_node();
        CHECKP(node);
        nt = getNodeType(node);
        st = getScmType(node);

        if (nt == attribute)  {
            /* Retrieve attribute namespace to return it to the client */
            tuple_cell tc = se_node_namespace_uri(node);
            if ( !tc.is_eos() ) {
               uri = tuple_cell::make_sure_light_atomic(tc).get_str_ptr();
            } else {
              // BUG (or maybe not?)
            }
        }
        else if(nt == document) {
            /* Retrieve document URI to return it to the client */
            tuple_cell tc = dm_document_uri(node);
            if ( !tc.is_eos() ) {
               tc = tuple_cell::make_sure_light_atomic(tc);
               uri = tc.get_str_ptr();
            }
        }
    } else {
        /* Determine atomic type */
        st = tc.get_atomic_type();
    }

    tr_globals::client->begin_item(!is_node, st, nt, uri.get());

    GlobalSerializationOptions * options = cxt->get_static_context()->get_serialization_options();

    if (tr_globals::client->supports_serialization()) {
        options->indent = false;
        options->separateTuples = false;
    } else if (!first && options->indent) {
        (* tr_globals::client->get_se_ostream()) << "\n"; // separate tuples!
        // This is needed for backward compatibility with old ( < 4 ) versions of protocol
    }

    tr_globals::serializer->prepare(tr_globals::client->get_se_ostream(), options);

    XmlConstructor xmlConstructor(VirtualRootConstructor(0));
    data.cells[0] = optimizedPlan->toXML(xmlConstructor).getLastChild();
    tr_globals::serializer->serialize(data);

    first = false;

    return true;
}

void PPQueryRoot::do_execute()
{
    while (do_next())
        ;
}

/*
 * PPSubQuery
 */

PPSubQuery::PPSubQuery(dynamic_context *_cxt_,
                       PPOpIn _child_) :       PPQueryEssence("PPSubQuery"),
                                               child(_child_),
                                               data(_child_.ts),
                                               cxt(_cxt_)
{
}

PPSubQuery::~PPSubQuery()
{
    delete child.op;
    child.op = NULL;

    delete cxt;
    cxt = NULL;
}

void PPSubQuery::do_open()
{
    /*
     * we should proobably save the existent lock mode here since subquery
     * could be run in the middle of a query
     */
    lmode = local_lock_mrg->get_cur_lock_mode();
    local_lock_mrg->lock(lm_s);
    cxt->global_variables_open();
    child.op->open();
}

void PPSubQuery::do_close()
{
    child.op->close();
    cxt->global_variables_close();
    local_lock_mrg->lock(lmode);
}

void PPSubQuery::do_accept(PPVisitor &v)
{
    v.visit (this);
    v.push  (this);
    child.op->accept(v);
    v.pop();
}

bool PPSubQuery::next(tuple &t)
{
    if (executor_globals::profiler_mode)
    {
        U_ASSERT(info.profile.get() != NULL);
        info.profile->calls++;
        u_ftime(&current1);
    }

    if (1 == tr_globals::debug_mode)
    {
        U_ASSERT(info.profile.get() != NULL);
        if (!executor_globals::profiler_mode) info.profile->calls++;
        executor_globals::pp_stack.push_back(info);
    }

    bool result = PPSubQuery::do_next();

    if (1 == tr_globals::debug_mode)
        executor_globals::pp_stack.pop_back();

    if (executor_globals::profiler_mode)
    {
        u_ftime(&current2);
        info.profile->time = current2 - current1;
    }

    t = data;

    return result;
}

bool PPSubQuery::do_next()
{
    child.op->next(data);

    if (data.is_eos())
        return false;

    if (data.size() != 1)
        throw XQUERY_EXCEPTION2(SE1003, "Result of a subquery contains nested sequences");

    return true;
}

void PPSubQuery::do_execute()
{
    U_ASSERT(false);
}
