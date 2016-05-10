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
        double m_number;
        std::string m_str;
        int m_line;
        int m_column;
        int32_t m_token;
        TokenDetail() :
            m_line(0),
            m_column(0),
            m_token(Token_EOF)
        {}
    };

    std::string GetTokenStr(const TokenDetail &tokenDetail);

    class Lexer
    {
    public:
        typedef std::function<int32_t()> CharInStream;

        Lexer(CharInStream in):
            _inStream(in),
            _line(0),
            _column(1),
            _current(EOF)
        {}

        DISABLE_DEFALT_COPY_AND_ASSIGN(Lexer);

        void SetInStream(CharInStream in){
            _inStream = in;
            _line = 0;
            _column = 1;
            _current = EOF;
        }

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


        CharInStream _inStream;
        int32_t _current;

        int32_t _line;
        int32_t _column;

        std::string _tokenBuffer;
    };
} // oms


