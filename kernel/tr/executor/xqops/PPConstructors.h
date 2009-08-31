/*
 * File:  PPConstructors.h
 * Copyright (C) 2004 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#ifndef _PPCONSTRUCTORS_H
#define _PPCONSTRUCTORS_H

#include "common/sedna.h"

#include "tr/executor/base/PPBase.h"
#include "tr/structures/schema.h"
#include "tr/strings/strings.h"
#include "tr/structures/indirection.h"
#include "tr/mo/micro.h"

///////////////////////////////////////////////////////////////////////////////
/// PPConstructor
///////////////////////////////////////////////////////////////////////////////
class PPConstructor : public PPIterator
{
protected:
    bool first_time;
    bool eos_reached;
	static bool firstCons;
	static schema_node_cptr root_schema;
	static xptr virt_root;
	static xptr last_elem;
	static xptr cont_parind;
	static xptr cont_leftind;
	static int conscnt;
	bool deep_copy;

private:
    virtual void do_open ();
    
public:
	static bool checkInitial();
    PPConstructor(dynamic_context *_cxt_,
                  operation_info _info_,
                  bool _deep_copy) : PPIterator(_cxt_, _info_),
                                     deep_copy(_deep_copy) {};
    friend xptr copy_to_temp(xptr node);
    friend void clear_virtual_root();
};

void clear_virtual_root();

///////////////////////////////////////////////////////////////////////////////
/// PPElementConstructor
///////////////////////////////////////////////////////////////////////////////
class PPElementConstructor : public PPConstructor
{
protected:
    PPOpIn qname;
    PPOpIn content;
    char* el_name;
    bool ns_inside;

private:
    virtual void do_open   ();
    virtual void do_reopen ();
    virtual void do_close  ();
    virtual void do_next   (tuple &t) ; 

    virtual PPIterator* do_copy(dynamic_context *_cxt_);

public:    

    PPElementConstructor(dynamic_context *_cxt_,
                         operation_info _info_,    
                         PPOpIn _qname_, 
                         PPOpIn _content_,
                         bool _deep_copy,
                         bool _ns_inside);

    PPElementConstructor(dynamic_context *_cxt_,
                         operation_info _info_,
                         const char* name,
                         PPOpIn _content_,
                         bool _deep_copy,
                         bool _ns_inside);

    virtual ~PPElementConstructor();
};

///////////////////////////////////////////////////////////////////////////////
/// PPAttributeConstructor
///////////////////////////////////////////////////////////////////////////////
class PPAttributeConstructor : public PPConstructor
{
protected:
    PPOpIn qname;
	PPOpIn content;
	char* at_name;
	char* at_value;


private:
    virtual void do_open   ();
    virtual void do_reopen ();
    virtual void do_close  ();
    virtual void do_next   (tuple &t) ; 

    virtual PPIterator* do_copy(dynamic_context *_cxt_);

public:    
    PPAttributeConstructor(dynamic_context *_cxt_,
                           operation_info _info_,
                           PPOpIn _qname_,
                           PPOpIn _content_,
                           bool _deep_copy);

	PPAttributeConstructor(dynamic_context *_cxt_, 
                           operation_info _info_,
                           const char* name,
                           PPOpIn _content_,
                           bool _deep_copy);

	PPAttributeConstructor(dynamic_context *_cxt_,
                           operation_info _info_,
                           PPOpIn _qname_,
                           const char* value,
                           bool _deep_copy);

	PPAttributeConstructor(dynamic_context *_cxt_,
                           operation_info _info_,
                           const char* name,
                           const char* value,
                           bool _deep_copy);

    virtual ~PPAttributeConstructor();
};

///////////////////////////////////////////////////////////////////////////////
/// PPNamespaceConstructor
///////////////////////////////////////////////////////////////////////////////
class PPNamespaceConstructor : public PPConstructor
{
protected:
    PPOpIn content;
	char* at_name;
	char* at_value;
	
private:
    virtual void do_open   ();
    virtual void do_reopen ();
    virtual void do_close  ();
    virtual void do_next   (tuple &t) ; 

    virtual PPIterator* do_copy(dynamic_context *_cxt_);

public:    
    PPNamespaceConstructor(dynamic_context *_cxt_,
                           operation_info _info_,
                           const char* name,
                           PPOpIn _content_);
                           
	PPNamespaceConstructor(dynamic_context *_cxt_,
                           operation_info _info_,
                           const char* name,
                           const char* value);

    virtual ~PPNamespaceConstructor();
};

///////////////////////////////////////////////////////////////////////////////
/// PPCommentConstructor
///////////////////////////////////////////////////////////////////////////////
class PPCommentConstructor : public PPConstructor
{
protected:
    PPOpIn content;
	char* at_value;
	StrMatcher strm;

private:
    virtual void do_open   ();
    virtual void do_reopen ();
    virtual void do_close  ();
    virtual void do_next   (tuple &t) ; 

    virtual PPIterator* do_copy(dynamic_context *_cxt_);

public:    
    PPCommentConstructor(dynamic_context *_cxt_,
                         operation_info _info_,
                         PPOpIn _content_,
                         bool _deep_copy);

	PPCommentConstructor(dynamic_context *_cxt_,
                         operation_info _info_,
                         const char* value,
                         bool _deep_copy);

    virtual ~PPCommentConstructor();
};

///////////////////////////////////////////////////////////////////////////////
/// PPTextConstructor
///////////////////////////////////////////////////////////////////////////////
class PPTextConstructor : public PPConstructor
{
protected:
    PPOpIn content;
	char* at_value;

private:
    virtual void do_open   ();
    virtual void do_reopen ();
    virtual void do_close  ();
    virtual void do_next   (tuple &t) ; 

    virtual PPIterator* do_copy(dynamic_context *_cxt_);

public:    
    PPTextConstructor(dynamic_context *_cxt_,
                      operation_info _info_,
                      PPOpIn _content_,
                      bool _deep_copy);

	PPTextConstructor(dynamic_context *_cxt_,
                      operation_info _info_,
                      const char* value,
                      bool _deep_copy);

    virtual ~PPTextConstructor();
};

///////////////////////////////////////////////////////////////////////////////
/// PPDocumentConstructor
///////////////////////////////////////////////////////////////////////////////
class PPDocumentConstructor : public PPConstructor
{
protected:
    PPOpIn content;

private:
    virtual void do_open   ();
    virtual void do_reopen ();
    virtual void do_close  ();
    virtual void do_next   (tuple &t) ; 

    virtual PPIterator* do_copy(dynamic_context *_cxt_);

public:    
    PPDocumentConstructor(dynamic_context *_cxt_,
                          operation_info _info_,
                          PPOpIn _content_);

    virtual ~PPDocumentConstructor();
};


///////////////////////////////////////////////////////////////////////////////
/// PPPIConstructor
///////////////////////////////////////////////////////////////////////////////
class PPPIConstructor : public PPConstructor
{
protected:
    PPOpIn qname;
	PPOpIn content;
	char* at_name;
	char* at_value;
	StrMatcher strm;
    
private:
    virtual void do_open   ();
    virtual void do_reopen ();
    virtual void do_close  ();
    virtual void do_next   (tuple &t) ; 

    virtual PPIterator* do_copy(dynamic_context *_cxt_);

public:    
    PPPIConstructor(dynamic_context *_cxt_,
                    operation_info _info_,
                    PPOpIn _qname_,
                    PPOpIn _content_,
                    bool _deep_copy);

	PPPIConstructor(dynamic_context *_cxt_,
                    operation_info _info_,
                    const char* name,
                    PPOpIn _content_,
                    bool _deep_copy);

	PPPIConstructor(dynamic_context *_cxt_,
                    operation_info _info_,
                    PPOpIn _qname_,
                    const char* value,
                    bool _deep_copy);

	PPPIConstructor(dynamic_context *_cxt_,
                    operation_info _info_,
                    const char* name,
                    const char* value,
                    bool _deep_copy);

    virtual ~PPPIConstructor();
};


#endif
