#include "mlex.h"
#include<cassert>
#include<algorithm>
#include<string.h>

namespace{
    const char *keyword[] = {
        "and", "break", "do", "else", "elseif", "end",
        "false", "for", "function", "if", "in",
        "local", "nil", "not", "or", "repeat",
        "return", "then", "true", "until", "while"
    };
    const int32_t kKeyWordNum = sizeof(keyword) / sizeof(keyword[0]);

    bool IsKeyWord(const std::string &name, int32_t *token)
    {
        assert(token);
        auto result = std::equal_range(
            keyword,
            keyword+kKeyWordNum,
            name.c_str(),
            [](const char *left, const char *right){
                return strcmp(left, right) < 0;
            }
        );
        if (result.first == result.second) 
            return false;
        
        *token = result.first - keyword + static_cast<int32_t>(oms::Token_And);
        return true;
    }

    inline bool IsHexChar(int32_t c)
    {
        return (c >= '0' && c <= '9')
            || (c >= 'a' && c <= 'f')
            || (c >= 'A' && c <= 'F');
    }
} // namespace


namespace oms{
    const char *token_str[] = {
        "and", "break", "do", "else", "elseif", "end",
        "false", "for", "function", "if", "in",
        "local", "nil", "not", "or", "repeat",
        "return", "then", "true", "until", "while",
        "<id>", "<string>", "<number>",
        "==", "~=", "<=", ">=",
        "..", "...", "<EOF>"
    };

    std::string GetTokenStr(const TokenDetail &tokenDetail)
    {
        int32_t token = tokenDetail.token;
        switch (token)
        {
        case Token_Number:
            return std::to_string(tokenDetail.number);
            break;
        case Token_Id:case Token_String:
            return tokenDetail.str;
            break;
        default:
            if (token >= Token_And && token <= Token_EOF)
            {
                return token_str[token - Token_And];
            }
            else
            {
                std::string str;
                str.push_back(token);
                return str;
            }
            break;
        }
    }

#define RETURN_NORMAL_TOKEN_DETAIL(detail, token)               \
    do {                                                        \
        detail->m_token = token;                                 \
        detail->m_line = _line;                                  \
        detail->m_column = _column;                              \
        return token;                                           \
    } while (0)


#define RETURN_NUMBER_TOKEN_DETAIL(detail, number)              \
    do {                                                        \
    detail->m_number = number;                                  \
    RETURN_NORMAL_TOKEN_DETAIL(detail, Token_Number);           \
    } while (0)

#define RETURN_TOKEN_DETAIL(detail, string, token)              \
    do {                                                        \
    detail->m_str = state_->GetString(string);                    \
    RETURN_NORMAL_TOKEN_DETAIL(detail, token);                  \
    } while (0)

#define SET_EOF_TOKEN_DETAIL(detail)                            \
    do {                                                        \
        detail->m_str = nullptr;                                 \
        detail->m_token = Token_EOF;                             \
        detail->m_line = _line;                                  \
        detail->m_column = _column;                              \
    } while (0)

    int32_t Lexer::GetToken(TokenDetail *detail)
    {
        assert(detail);
        SET_EOF_TOKEN_DETAIL(detail);
        if (EOF == _current)
        {
            _current = _NextChar();
        }

        for (;;)
        {
            switch (_current)
            {
            case ' ':case '\t':case '\v':case '\f':
                _current = _NextChar();
                break;
            case '\n':case '\r':
                _NewLine();
                break;
            case '-':
                {
                    int next = _NextChar();
                    if ('-' == next)
                    {
                        _Comment();
                    }
                    else
                    {
                        _current = next;
                        RETURN_NORMAL_TOKEN_DETAIL(detail, '-');
                    }
                }
                break;
            case '[':
                {
                    _current = _NextChar();
                    if ('[' == _current || '=' == _current)
                    {
                        return _MultiLineString(detail);
                    }
                    else
                    {
                        RETURN_NORMAL_TOKEN_DETAIL(detail, '[');
                    }
                }
                break;
            case '=':
                break;
            case '<':
                break;
            case '>':
                break;
            case '~':
                break;
            case '"':
                break;
            case '\'':
                break;
            case '.':
                break;
            case EOF:
                break;
            default:
                break;
            }
        }
    }
} // namespace oms