#pragma once
#include<functional>
#include<cstdint>

#include "mcommon.h"

namespace oms{
    class String;
    enum Token:int32_t
    {
        Token_And = 256, Token_Break, Token_Do, Token_Else, Token_Elseif, Token_End,
        Token_False, Token_For, Token_Function, Token_If, Token_In,
        Token_Local, Token_Nil, Token_Not, Token_Or, Token_Repeat,
        Token_Return, Token_Then, Token_True, Token_Until, Token_While,
        Token_Id, Token_String, Token_Number,
        Token_Equal, Token_NotEqual, Token_LessEqual, Token_GreaterEqual,
        Token_Concat, Token_VarArg, Token_EOF,
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
        int32_t token;
        TokenDetail() :
            str(nullptr),
            line(0),
            column(0),
            token(Token_EOF)
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

        // 获取下一个token
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


