// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/06.

#include "polarphp/parser/internal/YYLexerExtras.h"
#include "polarphp/parser/internal/YYLexerDefs.h"
#include "polarphp/parser/Token.h"
#include "polarphp/parser/Lexer.h"

namespace polar::parser::internal {


int token_lex_wrapper(ParserSemantic *value, YYLocation *loc, Lexer *lexer)
{
   Token token;
   lexer->setSemanticValueContainer(value);
   lexer->lex(token);
   // setup values that parser need
   return token.getKind();
}

} // polar::parser::internal