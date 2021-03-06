/*
 * File:  XQueryLexer.h
 * Copyright (C) 2009 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#ifndef _XQUERY_LEXER_H_
#define _XQUERY_LEXER_H_


// scan function definition for flex
#define YY_DECL                                              \
  sedna::XQueryParser::token_type                         \
  sedna::XQueryLexer::yylex(                              \
    sedna::XQueryParser::semantic_type* yylval,           \
    sedna::XQueryParser::location_type* yylloc)

#include "tr/xqp/XQueryParser.h"

#ifndef __FLEX_LEXER_H

#undef yyFlexLexer
#define yyFlexLexer sednaFlexLexer
#include <FlexLexer.h>

#endif /* #ifndef __FLEX_LEXER_H */

namespace sedna
{
    // Main class that drives scanning process
   class XQueryLexer : public sednaFlexLexer
   {
        private:
            XQueryDriver &driver;

            // lookahead definitions
            bool have_la;
            XQueryParser::token_type prev_tok, la_tok;
            XQueryParser::semantic_type la_lval;
            XQueryParser::location_type la_lloc;

        public:
            XQueryLexer(XQueryDriver &drv, std::istream *in) : sednaFlexLexer(in), driver(drv), have_la(false), prev_tok(XQueryParser::token::END)
            {}

            ~XQueryLexer() {}

            // Scanning function that is called by parser; we need it to be aware of possible lookahead tokens
            XQueryParser::token_type nextToken(XQueryParser::semantic_type* yylval, XQueryParser::location_type* yylloc);

            // lookahead function that looks for i-th token ahead; use with care since we can change lexing states via parser
            XQueryParser::token_type LookAhead(int i);

            // we need this to tell lexer to step out of constructor state when it has seen '<' (parser calls this)
            void xqDiscardConstructor();

        protected:
            // Main scanning function generated by flex in XQueryLexer.cpp
            XQueryParser::token_type yylex(XQueryParser::semantic_type *yylval, XQueryParser::location_type* yylloc);

            // function that creates numeric literal
            //
            // Parameters:
            //      yylloc -- location
            //      text -- StringLiteral string
            //      len -- its length
            //
            // Returns:
            //      string to replace StringLiteral
            //
            std::string *xqMakeNumLiteral(XQueryParser::location_type *loc, const char *text, int len);

            // Decodes predefined entity to a usual string
            //
            // Parameters:
            //      yylloc -- location
            //      text -- StringLiteral string
            //      len -- its length
            //
            // Returns:
            //      string to replace StringLiteral
            //
            std::string *xqDecodePredRef(XQueryParser::location_type *loc, const char *text, int len);

            // Decodes char reference entity to a usual string
            // Checks if char reference is valid
            // This function doesn't check validity of reference and assumes that it is in format &#xxx; with &#; as mininmum
            //
            // Parameters:
            //      yylloc -- location
            //      text -- StringLiteral string
            //      len -- its length
            //
            // Returns:
            //      string to replace StringLiteral
            //
            std::string *xqDecodeCharRef(XQueryParser::location_type *loc, const char *text, int len);

            // Function that creates string literal
            // Modifies StringLiteral
            //      -- escapes " with \"
            //      -- replaces charrefs ant predef entities
            //      -- replaces '' xor "" with ' xor "
            //
            // Parameters:
            //      yylloc -- location
            //      text -- StringLiteral string
            //      len -- its length
            //
            // Returns:
            //      string to replace StringLiteral
            //
            std::string *xqMakeStrLiteral(XQueryParser::location_type *loc, const char *text, int len);

            // this function checks that QName consists of valid characters as defined by XQuery spec
            //
            // Parameters:
            //      yylloc -- location
            //      text -- StringLiteral string
            //      len -- its length
            //
            // Returns:
            //      string to replace StringLiteral
            //
            std::string *xqMakeQName(XQueryParser::location_type *loc, const char *text, int len);

            // checks content for valid XML 1.1 characters
            //
            // Parameters:
            //      yylloc -- location
            //      text -- StringLiteral string
            //      len -- its length
            //      in_attr -- means that we are parsing attribute content
            //
            // Returns:
            //      string to replace StringLiteral
            //
            std::string *xqMakeContent(XQueryParser::location_type *loc, const char *text, int len, bool in_attr);

            // check if pi-target is valid; it must consist of valid Name chars
            //
            // Parameters:
            //      yylloc -- location
            //      text -- StringLiteral string
            //      len -- its length
            //
            // Returns:
            //      string to replace StringLiteral
            //
            std::string *xqMakePITarget(XQueryParser::location_type *loc, const char *text, int len);

            // Checks if XQuery comment consist of valid characters; XQuery spec explicitly says that it must be sequence of Chars
            //
            // Parameters:
            //      yylloc -- location
            //      text -- StringLiteral string
            //      len -- its length
            //
            // Returns:
            //      true - ok; false - invalid character;
            //
            bool xqCheckComment(XQueryParser::location_type *loc, const char *text, int len);
    };
}

#endif
