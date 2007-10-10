/*
 * File:  xs_binary.h
 * Copyright (C) 2006 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#ifndef _XS_BIMARY_H
#define _XS_BINARY_H

#include "common/sedna.h"
#include "tr/executor/base/PPBase.h"

/////////////////////////////////////////////////////////////////////
/// This file defines interface fot casting to or from binary types.
/// For binary types we can perform only the following operations:
/////////////////////////////////////////////////////////////////////

tuple_cell cast_string_type_to_xs_base64Binary(const tuple_cell &c, int __xquery_line = 0);
tuple_cell cast_string_type_to_xs_hexBinary(const tuple_cell &c, int __xquery_line = 0);

tuple_cell cast_base64Binary_to_hexBinary(const tuple_cell &c);
tuple_cell cast_hexBinary_to_base64Binary(const tuple_cell &c);

#endif
