#pragma once
#include<functional>
#include<cstdint>

#include "mcommon.h"

namespace oms{
    class String;
    enum class Token:int32_t
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
            char *str;
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

        Lexer(CharInStream in)
           :_inStream(in),
           _line(0),
           _column(1)
        {}

        DISABLE_DEFALT_COPY_AND_ASSIGN(Lexer);

        int32_t GetToken(TokenDetail *detail);

    private:
        int32_t _NextChar()
        {
            auto c = _inStream();
            if (EOF != c) ++_column;
            return c;
        }

        void _NewLine();
        void _Comment();
        void _MultiLineComment();
        void _SingleLineComment();

        int32_t _Number(TokenDetail *detail);


        CharInStream _inStream;
        int32_t _current;

        int32_t _line;
        int32_t _column;



    };
} // oms


