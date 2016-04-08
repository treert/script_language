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
        
        *token = result.first - keyword + static_cast<int32_t>(oms::Token::And);
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
    int32_t Lexer::GetToken(TokenDetail *detail)
    {
        assert(detail);

        for (;;)
        {
            switch (_current)
            {
            case ' ':case '\t':case '\v':case '\f':
                _current = _NextChar();
                break;
            default:
                break;
            }
        }
    }
} // namespace oms