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
        int32_t token = tokenDetail.m_token;
        switch (token)
        {
        case Token_Number:
            return std::to_string(tokenDetail.m_number);
            break;
        case Token_Id:case Token_String:
            return tokenDetail.m_str;
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
    detail->m_str = "todo";                    \
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
                return _XEqual(detail,Token_Equal);
                break;
            case '<':
                return _XEqual(detail, Token_GreaterEqual);
                break;
            case '>':
                return _XEqual(detail, Token_LessEqual);
                break;
            case '~':
                {
                    int32_t next = _NextChar();
                    if ('=' != next)
                    {
                        throw "todo";
                    }
                    _current = _NextChar();
                    RETURN_NORMAL_TOKEN_DETAIL(detail, Token_NotEqual);
                }
                break;
            case '"':case '\'':
                return _SingleLineString(detail);
                break;
            case '.':
                {
                    int32_t next = _NextChar();
                    if ('.' == next)
                    {
                        int32_t pre_next = _NextChar();
                        if ('.' == pre_next)
                        {
                            _current = _NextChar();
                            RETURN_NORMAL_TOKEN_DETAIL(detail, Token_VarArg);
                        }
                        else
                        {
                            _current = pre_next;
                            RETURN_NORMAL_TOKEN_DETAIL(detail, Token_Concat);
                        }
                    }
                    else if (isdigit(next))
                    {
                        _tokenBuffer.clear();
                        _tokenBuffer.push_back(_current);
                        _current = next;
                        return _NumberXFractional(
                            detail, false, true,
                            [](int c){ return isdigit(c) != 0; },
                            [](int c){ return c == 'e' || c == 'E'; }
                        );
                    }
                    else
                    {
                        _current = next;
                        RETURN_NORMAL_TOKEN_DETAIL(detail, '.');
                    }
                }
                break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                return _Number(detail);
            case '+': case '*': case '/': case '%': case '^':
            case '#': case '(': case ')': case '{': case '}':
            case ']': case ';': case ':': case ',':
                {
                    int token = _current;
                    _current = _NextChar();
                    RETURN_NORMAL_TOKEN_DETAIL(detail, token);
                }
                break;
            default:
                return _Id(detail);
                break;
            }
            return Token_EOF;
        }
    }

    void Lexer::_NewLine()
    {
        int next = _NextChar();
        if (('\n' == next || '\r' == next) && next != _current)
            _current = _NextChar();
        else
            _current = next;
        ++_line;
        _column = 0;
    }

    void Lexer::_Comment()
    {
        _current = _NextChar();
        if ('[' == _current)
        {
            _current = _NextChar();
            if ('[' == _current)
                _MultiLineComment();
            else
                _SingleLineComment();
        }
        else
            _SingleLineComment();
    }

    void Lexer::_MultiLineComment()
    {
        bool is_comment_end = false;
        while (!is_comment_end)
        {
            if (']' == _current)
            {
                _current = _NextChar();
                if (']' == _current)
                {
                    is_comment_end = true;
                    _current = _NextChar();
                }
            }
            else if (EOF == _current)
            {
                throw "todo";
            }
            else if ('\r' == _current || '\n' == _current)
            {
                _NewLine();
            }
            else
            {
                _current = _NextChar();
            }
        }
    }

    int32_t Lexer::_Number(TokenDetail *detail)
    {
        bool integer_part = false;
        _tokenBuffer.clear();
        if ('0' == _current)
        {
            int32_t next = _NextChar();
            if ('x' == next || 'X' == next)
            {
                _tokenBuffer.push_back(_current);
                _tokenBuffer.push_back(next);
                _current = _NextChar();
                return _NumberX(
                    detail, false, IsHexChar,
                    [](int c) { return c == 'p' || c == 'P'; }
                );
            }
            else
            {
                _tokenBuffer.push_back(_current);
                _current = next;
                integer_part = true;
            }
        }
        return _NumberX(
            detail, integer_part,
            [](int c){return isdigit(c) != 0; },
            [](int c){return 'e' == c || 'E' == c; }
            );
    }

    int32_t Lexer::_NumberX(
        TokenDetail *detail, bool integer_part,
        const std::function<bool(int)> &is_number_char,
        const std::function<bool(int)> &is_exponent
        )
    {
        while (is_number_char(_current))
        {
            _tokenBuffer.push_back(_current);
            _current = _NextChar();
            integer_part = true;
        }
        bool point = false;
        if ('.' == _current)
        {
            _tokenBuffer.push_back(_current);
            _current = _NextChar();
            point = true;
        }

        return _NumberXFractional(
            detail,integer_part,point,
            is_number_char,is_exponent
            );
    }

    int Lexer::_NumberXFractional(TokenDetail *detail,
        bool integer_part, bool point,
        const std::function<bool(int)> &is_number_char,
        const std::function<bool(int)> &is_exponent)
    {
        bool fractional_part = false;
        while (is_number_char(_current))
        {
            _tokenBuffer.push_back(_current);
            _current = _NextChar();
            fractional_part = true;
        }

        if (point && !integer_part && !fractional_part)
            throw "todo";
        else if (!point && !integer_part && !fractional_part)
            throw "todo";

        if (is_exponent(_current))
        {
            _tokenBuffer.push_back(_current);
            _current = _NextChar();
            if (_current == '-' || _current == '+')
            {
                _tokenBuffer.push_back(_current);
                _current = _NextChar();
            }

            if (!isdigit(_current))
                throw "todo";

            while (isdigit(_current))
            {
                _tokenBuffer.push_back(_current);
                _current = _NextChar();
            }
        }

        double number = strtod(_tokenBuffer.c_str(), nullptr);
        RETURN_NUMBER_TOKEN_DETAIL(detail, number);
    }

    int Lexer::_XEqual(TokenDetail *detail, int equal_token)
    {
        int token = _current;

        int next = _NextChar();
        if (next == '=')
        {
            _current = _NextChar();
            RETURN_NORMAL_TOKEN_DETAIL(detail, equal_token);
        }
        else
        {
            _current = next;
            RETURN_NORMAL_TOKEN_DETAIL(detail, token);
        }
    }

    int Lexer::_MultiLineString(TokenDetail *detail)
    {
        int equals = 0;
        while (_current == '=')
        {
            ++equals;
            _current = _NextChar();
        }

        if (_current != '[')
            throw "todo";

        _current = _NextChar();
        _tokenBuffer.clear();

        if (_current == '\r' || _current == '\n')
        {
            _NewLine();
            if (equals == 0)    // "[[]]" keeps first '\n'
                _tokenBuffer.push_back('\n');
        }

        while (_current != EOF)
        {
            if (_current == ']')
            {
                _current = _NextChar();
                int i = 0;
                for (; i < equals; ++i, _current = _NextChar())
                {
                    if (_current != '=')
                        break;
                }

                if (i == equals && _current == ']')
                {
                    _current = _NextChar();
                    RETURN_TOKEN_DETAIL(detail, _tokenBuffer, Token_String);
                }
                else
                {
                    _tokenBuffer.push_back(']');
                    _tokenBuffer.append(i, '=');
                }
            }
            else if (_current == '\r' || _current == '\n')
            {
                _NewLine();
                _tokenBuffer.push_back('\n');
            }
            else
            {
                _tokenBuffer.push_back(_current);
                _current = _NextChar();
            }
        }

        throw "todo";
    }

    int Lexer::_SingleLineString(TokenDetail *detail)
    {
        int quote = _current;
        _current = _NextChar();
        _tokenBuffer.clear();

        while (_current != quote)
        {
            if (_current == EOF)
                throw "incomplete string at <eof>";

            if (_current == '\r' || _current == '\n')
                throw "incomplete string at this line";

            _StringChar();
        }

        _current = _NextChar();
        RETURN_TOKEN_DETAIL(detail, _tokenBuffer, Token_String);
    }

    void Lexer::_StringChar()
    {
        if (_current == '\\')
        {
            _current = _NextChar();
            if (_current == 'a')
                _tokenBuffer.push_back('\a');
            else if (_current == 'b')
                _tokenBuffer.push_back('\b');
            else if (_current == 'f')
                _tokenBuffer.push_back('\f');
            else if (_current == 'n')
                _tokenBuffer.push_back('\n');
            else if (_current == 'r')
                _tokenBuffer.push_back('\r');
            else if (_current == 't')
                _tokenBuffer.push_back('\t');
            else if (_current == 'v')
                _tokenBuffer.push_back('\v');
            else if (_current == '\\')
                _tokenBuffer.push_back('\\');
            else if (_current == '"')
                _tokenBuffer.push_back('"');
            else if (_current == '\'')
                _tokenBuffer.push_back('\'');
            else if (_current == 'x')
            {
                _current = _NextChar();
                char hex[3] = { 0 };
                int i = 0;
                for (; i < 2 && IsHexChar(_current); ++i, _current = _NextChar())
                    hex[i] = _current;
                if (i == 0)
                    throw "unexpect character after '\\x'";
                _tokenBuffer.push_back(static_cast<char>(strtoul(hex, 0, 16)));
                return;
            }
            else if (isdigit(_current))
            {
                char oct[4] = { 0 };
                for (int i = 0; i < 3 && isdigit(_current); ++i, _current = _NextChar())
                    oct[i] = _current;
                _tokenBuffer.push_back(static_cast<char>(strtoul(oct, 0, 8)));
                return;
            }
            else
                throw "unexpect character after '\\'";
        }
        else
        {
            _tokenBuffer.push_back(_current);
        }

        _current = _NextChar();
    }

    int Lexer::_Id(TokenDetail *detail)
    {
        if (!isalpha(_current) && _current != '_')
            throw "unexpect character";

        _tokenBuffer.clear();
        _tokenBuffer.push_back(_current);
        _current = _NextChar();

        while (isalnum(_current) || _current == '_')
        {
            _tokenBuffer.push_back(_current);
            _current = _NextChar();
        }

        int token = 0;
        if (!IsKeyWord(_tokenBuffer, &token))
            token = Token_Id;
        RETURN_TOKEN_DETAIL(detail, _tokenBuffer, token);
    }
} // namespace oms