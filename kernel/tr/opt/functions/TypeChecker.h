#ifndef _TYPE_CHECKER_H_
#define _TYPE_CHECKER_H_

#include "tr/executor/fo/casting_operations.h"
#include "tr/executor/base/PPUtils.h"

struct error_info_t {
    int code;
    const char * description;
};

template <typename SequenceType, typename TypeChecker>
struct TypedSequenceIterator
{
    SequenceType iterator;
    TypeChecker checker;
    int min, max;
    error_info_t & error_info;
    tuple_cell value;

    TypedSequenceIterator(
        const SequenceType & _iterator,
        const TypeChecker & _checker,
        int _min, int _max,
        const error_info_t & _error_info)

      : iterator(_iterator),
        checker(_checker),
        min(_min), max(-_max),
        error_info(_error_info) {}

    inline
    tuple_cell next()
    {
        value = iterator.next();

        if (value.is_eos()) {
            if (min > 0) {
                throw USER_EXCEPTION2(error_info.code, error_info.description);
            };

            return value;
        }

        if (!checker.check(x)) {
            throw USER_EXCEPTION2(error_info.code, error_info.description);
        };

        min--;
        max++;

        if (max == 0) {
            if (!iterator.next().is_eos()) {
                throw USER_EXCEPTION2(error_info.code, error_info.description);
            }
        };

        return value;
    };
};

struct AnyTypeChecker
{
    AnyTypeChecker() {};
    inline bool check(tuple_cell & tc) { return true; };
};

struct AtomicTypeChecker
{
    xmlscm_type type;
    AtomicTypeChecker(xmlscm_type _type) : type(_type) {};

    inline
    bool check(tuple_cell & tc)
    {
        return tc.is_atomic_type(tc);
    };
};

struct LightAtomicTypeChecker
{
    xmlscm_type type;
    AtomicTypeChecker(xmlscm_type _type) : type(_type) {};

    inline
    bool check(tuple_cell & tc)
    {
        if (tc.is_atomic_type(type)) {
            tc = tuple_cell::make_sure_light_atomic(tc);
            return true;
        }

        return false;
    };
};

struct NodeChecker
{
    int type_mask;
    NodeChecker(int _type_mask) : type_mask(_type_mask) {};

    inline
    bool check(tuple_cell & tc)
    {
        // TODO : add node type check
        return tc.is_node(tc);
    };
};

struct AtomicTypeCaster
{
    xmlscm_type type;
    AtomicTypeCaster(xmlscm_type _type) : type(_type) {};

    inline
    bool check(tuple_cell & tc)
    {
        if (!tc.is_atomic_type(tc)) {
            tc = cast(atomize(tc), type);
        };

        tc = tuple_cell::make_sure_light_atomic(tc);

        return tc.is_atomic_type(type);
    };
};

#endif /* _TYPE_CHECKER_H_ */
