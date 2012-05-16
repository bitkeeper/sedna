#ifndef JOINS_H
#define JOINS_H

#include "tr/opt/SequenceModel.h"
#include "tr/opt/algorithms/Comparison.h"
#include "tr/opt/algorithms/ValueFunction.h"

#include "tr/nid/nidstring.h"

class SortedSequence;
class ITupleSerializer;

namespace phop {

typedef std::vector<MappedTupleIn> TupleList;
typedef std::vector< std::pair<NidString, TupleList::size_type> > NIDMergeHeap;

class DocOrderMerge : public ITupleOperator {
private:
    bool initialized;
    TupleList tin;
    NIDMergeHeap mergeHeap;
protected:
    virtual void do_next();
public:
    OPINFO_DECL(0x301)
  
    DocOrderMerge(unsigned int _size, const TupleList & _tin);
    virtual void reset();
    virtual void setContext ( ExecutionContext* __context );
};

class TupleSort : public UnaryTupleOperator {
private:
    bool initialized;
protected:
    ICollationTupleSerializer * order;
    SortedSequence * _sorted_sequence;

    virtual void do_next();
public:
    OPINFO_DECL(0x302)

    TupleSort(unsigned int _size, MappedTupleIn _in, ICollationTupleSerializer * _order);
    virtual ~TupleSort();

    virtual void reset();
    virtual void setContext ( ExecutionContext* __context );
};

class TupleJoinFilter : public BinaryTupleOperator {
    enum step_t {
        step_both = 0,
        step_left = 1,
        step_right = 2,
    };
private:
    bool initialized;
    TupleCellComparison tcc;
    sequence * left_seq;
    sequence * right_seq;
    
    size_t left_seq_pos;
    size_t right_seq_pos;

    size_t initial_left_seq_pos;
    size_t initial_right_seq_pos;
    
    tuple initialLeft, initialRight;
    tuple leftValue, rightValue;
    
    step_t step_state;

    virtual void do_next();
public:
    OPINFO_DECL(0x304)

    TupleJoinFilter(unsigned int _size, const phop::MappedTupleIn& _left, const phop::MappedTupleIn& _right, const TupleCellComparison & tcc);
    virtual ~TupleJoinFilter();

    virtual void reset();
    virtual void setContext ( ExecutionContext* __context );
};

class TuplePredicateFilter : public UnaryTupleOperator {
private:
    ValueFunction vcc;
    virtual void do_next();
public:
    OPINFO_DECL(0x305)

    TuplePredicateFilter(const phop::MappedTupleIn& _in, const ValueFunction& _vcc);
    virtual void setContext ( ExecutionContext* __context );
};

}

/*
class GSMJMergeComparator {
public:
    bool stepLeft;
    bool stepRight;

    virtual bool compare(tuple left, tuple right) = 0;
};

AbstractSequence * createEqualValueJoin();
*/

#endif /* PLAN_EXECUTION_H */