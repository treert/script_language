#pragma once
#include<functional>
#include<cstdint>

#include "mcommon.h"

namespace oms{
    class String;
    enum class Token
    {
        And = 256, Break, Do, Else, Elseif, End,
        False, For, Function, If, In,
        Local, Nil, Not, Or, Repeat,
        Return, Then, True, Until, While,
        Id, String, Number,
        Equal, NotEqual, LessEqual, GreaterEqual,
        Concat, VarArg, Eof,
    };

    struct TokenDetail
    {
        union
        {
            double number;
            String *str;
        };
        int line;
        int column;
        Token token;
        TokenDetail() :
            str(nullptr),
            line(0),
            column(0),
            token(Token::Eof)
        {}
    };

    class Lexer
    {
    public:
        typedef std::function<int32_t()> CharInStream;

        Lexer(CharInStream in);

        DISABLE_DEFALT_COPY_AND_ASSIGN(Lexer);

    };
} // oms


