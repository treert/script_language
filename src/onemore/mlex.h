#pragma once
#include <functional>
#include <cstdint>
#include <string>

#include "mcommon.h"

namespace oms{
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
        double number_;
        std::string str_;
        int line_;
        int column_;
        int32_t token_;
        TokenDetail() :
            line_(0),
            column_(0),
            token_(Token_EOF)
        {}
    };

    std::string GetTokenStr(const TokenDetail &tokenDetail);

    class Lexer
    {
    public:
        typedef std::function<int32_t()> CharInStream;

        Lexer(CharInStream in):
            in_stream_(in),
            line_(0),
            column_(1),
            current_(EOF)
        {}

        DISABLE_DEFALT_COPY_AND_ASSIGN(Lexer);

        void SetInStream(CharInStream in){
            in_stream_ = in;
            line_ = 0;
            column_ = 1;
            current_ = EOF;
        }

        // 获取下一个token
        int32_t GetToken(TokenDetail *detail);

    private:
        int32_t _NextChar()
        {
            auto c = in_stream_();
            if (EOF != c) ++column_;
            return c;
        }

        void _NewLine();

        void _Comment();
        void _MultiLineComment();
        void _SingleLineComment();

        int32_t _Number(TokenDetail *detail);
        int32_t _NumberX(TokenDetail *detail, bool integer_part,
            const std::function<bool(int)> &is_number_char,
            const std::function<bool(int)> &is_exponent);
        int32_t _NumberXFractional(TokenDetail *detail,
            bool integer_part, bool point,
            const std::function<bool(int)> &is_number_char,
            const std::function<bool(int)> &is_exponent);

        int32_t _XEqual(TokenDetail *detail, int equal_token);

        int32_t _MultiLineString(TokenDetail *detail);
        int32_t _SingleLineString(TokenDetail *detail);
        void _StringChar();

        int32_t _Id(TokenDetail *detail);


        CharInStream in_stream_;
        int32_t current_;

        int32_t line_;
        int32_t column_;

        std::string token_buffer_;
    };
} // oms


