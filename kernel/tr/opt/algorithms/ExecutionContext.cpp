#include "ExecutionContext.h"

#include "tr/opt/algorithms/VariableMap.h"

#include "tr/opt/algebra/IndependentPlan.h"
#include "tr/opt/algebra/GraphOperations.h"
#include "tr/opt/algebra/MapOperations.h"
#include "tr/opt/algebra/FunctionOperations.h"
#include "tr/opt/algebra/ElementaryOperations.h"
#include "tr/opt/graphs/DataGraphCollection.h"
#include "tr/opt/SequenceModel.h"
#include "tr/models/SCElementProducer.h"

#include <stack>
#include <queue>

using namespace phop;
using namespace opt;
using namespace rqp;
using namespace executor;

class ConstructorImplementation : public IExecuteProc
{
public:
    DynamicContext * context;
    rqp::Construct * op;

    explicit ConstructorImplementation(DynamicContext * _context, Construct * _op) : context(_context), op(_op) {};
    virtual void execute(ExecutionStack* executor);
};

class GroupNextImplementation : public IExecuteProc
{
public:
    DynamicContext * context;
    VariableProducer * producer;
    rqp::RPBase * nextOp;
    uint64_t tupleMask;
    
    explicit GroupNextImplementation(DynamicContext * _context, VariableProducer * _producer, RPBase * _nextOp, uint64_t _tupleMask)
      : context(_context), producer(_producer), nextOp(_nextOp), tupleMask(_tupleMask) {};

    virtual void execute(ExecutionStack* executor);
};

class GraphNext : public IExecuteProc
{
public:
    GraphExecutionBlock * geb;

    explicit GraphNext(GraphExecutionBlock * _geb) : geb(_geb) {};
    virtual void execute(ExecutionStack* executor);
};

void ConstructorImplementation::execute(ExecutionStack* result)
{
    xsd::QName nameValue;
    VariableModel * varmodel = context->variables;

    if (op->getType() == element || op->getType() == attribute || op->getType() == pr_ins)
    {
        ExecutionStack nameEvaluation;
        optimizer->executor()->push(&nameEvaluation, context, op->getName());
        tuple_cell qname_tc = nameEvaluation.next();

        if (qname_tc.get_atomic_type() != xs_QName)
        {
            U_ASSERT(false);
            // TODO: USER_Exception
        };

        nameValue = qname_tc.get_xs_qname();

        if (!nameEvaluation.next().is_eos())
        {
            U_ASSERT(false);
            // TODO: USER_Exception
        };
    }

    if (op->getType() == element) {
        IElementProducer * parent = context->constructorContext();
        VarCacheInfo * varInfo = varmodel->getProducer(op->contentId);
        context->setConstructorContext(parent->addElement(nameValue));
        VariableProducer * producer;

        if (varInfo == NULL) {
            producer = new VariableProducer(op->contentId);
            varmodel->bind(producer);
        } else {
            producer = varInfo->producer;
            producer->resetResult();
        };

        optimizer->executor()->push(producer->valueSequence, context, op->getList());
        VarIterator contentIterator = varmodel->getIterator(op->contentId);

        while (!contentIterator.next().is_eos()) {
            context->constructorContext()->addValue(contentIterator.get(), false);
        };

        result->push(Result(parent->close()));
        context->setConstructorContext(parent);
    } else {
        U_ASSERT(false);
    };
}

uint64_t getRestrictMask(const TupleScheme & tscheme)
{
    uint64_t result;

    for (TupleScheme::const_iterator it = tscheme.begin(); it != tscheme.end(); ++it)
    {
        result |= (1ULL << *it);
    };

    return result;
};

void GroupNextImplementation::execute(ExecutionStack* result)
{
    uint64_t saveRestrickMask = producer->restrictMask;

    if (saveRestrickMask == 0) {
        saveRestrickMask = tupleMask;
    };

    producer->restrictMask = 0;
    producer->next();
    producer->restrictMask = saveRestrickMask;

    result->push(Result(new GroupNextImplementation(*this)));
    optimizer->executor()->push(result, context, nextOp);
};

void GraphNext::execute(ExecutionStack* result)
{
    geb->top()->next();
    result->push(Result(geb->top()->get().cells[geb->outputTupleId]));
    result->push(Result(new GraphNext(*this)));
};


#define CASE_TYPE(TYPE, V, I) \
   case TYPE::opid : if (TYPE * V = static_cast<TYPE *>(I))

void PlanExecutor::push(ExecutionStack* executionStack, DynamicContext* context, RPBase* op)
{
    switch (op->info()->opType) {
      CASE_TYPE(Construct, vop, op)
        {
            executionStack->push(Result(new ConstructorImplementation(context, vop)));
        }
        break;
      CASE_TYPE(MapGraph, vop, op)
        {
            // TODO: optimize graph execution. There is no need in most cases to rebuild the graph
            GraphExecutionBlock * geb = gc.compile(vop->graph());
            geb->context = context;

            VariableProducer * producer =
              context->variables->bindGraph(geb, &(vop->graph()));

            executionStack->push(Result(
              new GroupNextImplementation(
                context, producer, vop->getList(), getRestrictMask(vop->tupleMask))));
        }
        break;
      CASE_TYPE(Const, vop, op)
        {
            MemoryTupleSequencePtr values = vop->getSequence();

            for (MemoryTupleSequence::const_iterator it = values->begin(); it != values->end(); ++it) {
                executionStack->push(Result(*it));
            };
        }
        break;
      CASE_TYPE(DataGraphOperation, vop, op)
        {
            GraphExecutionBlock * geb = gc.compile(vop->graph());
            geb->context = context;
            geb->outputTupleId = geb->resultMap[vop->out->varTupleId];
            executionStack->push(Result(new GraphNext(geb)));
        }
        break;
/*
      case SequenceConcat::opid :
      case MapConcat::opid :
        {
          NestedOperation * nop = static_cast<NestedOperation *>(op);
        }
        break;
*/
      default:
        {
            U_ASSERT(false);
        } break;
    }
}

void DynamicContext::createVirtualRoot()
{
    _constructorContext = SCElementProducer::getVirtualRoot(XNULL);
}

PlanExecutor::PlanExecutor()
{
    executionStack = new ExecutionStack();
    baseContext.setConstructorContext(NULL);
    baseContext.variables = new VariableModel;
}

PlanExecutor::~PlanExecutor()
{
    delete executionStack;
    delete baseContext.variables;
//    delete baseContext.constructorContext;
}

void PlanExecutor::execute(RPBase* op)
{
    push(executionStack, &baseContext, op);
}
