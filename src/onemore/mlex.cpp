#include "mlex.h"
#include<cassert>
#include<algorithm>
#include<string.h>

#include "mexception.h"

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
        int32_t token = tokenDetail.token_;
        switch (token)
        {
        case Token_Number:
            return std::to_string(tokenDetail.number_);
            break;
        case Token_Id:case Token_String:
            return tokenDetail.str_;
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
        detail->token_ = token;                                 \
        detail->line_ = line_;                                  \
        detail->column_ = column_;                              \
        return token;                                           \
    } while (0)


#define RETURN_NUMBER_TOKEN_DETAIL(detail, number)              \
    do {                                                        \
    detail->number_ = number;                                  \
    RETURN_NORMAL_TOKEN_DETAIL(detail, Token_Number);           \
    } while (0)

#define RETURN_TOKEN_DETAIL(detail, string, token)              \
    do {                                                        \
    detail->str_ = string;                    \
    RETURN_NORMAL_TOKEN_DETAIL(detail, token);                  \
    } while (0)

#define SET_EOF_TOKEN_DETAIL(detail)                            \
    do {                                                        \
        detail->token_ = Token_EOF;                             \
        detail->line_ = line_;                                  \
        detail->column_ = column_;                              \
    } while (0)

    int32_t Lexer::GetToken(TokenDetail *detail)
    {
        assert(detail);
        SET_EOF_TOKEN_DETAIL(detail);
        if (EOF == current_)
        {
            current_ = _NextChar();
        }

        while (EOF != current_)
        {
            switch (current_)
            {
            case ' ':case '\t':case '\v':case '\f':
                current_ = _NextChar();
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
                        current_ = next;
                        RETURN_NORMAL_TOKEN_DETAIL(detail, '-');
                    }
                }
                break;
            case '[':
                {
                    current_ = _NextChar();
                    if ('[' == current_ || '=' == current_)
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
            case '>':
                return _XEqual(detail, Token_GreaterEqual);
                break;
            case '<':
                return _XEqual(detail, Token_LessEqual);
                break;
            case '~':
                {
                    int32_t next = _NextChar();
                    if ('=' != next)
                    {
                        throw Exception("expect '=' after '~'");
                    }
                    current_ = _NextChar();
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
                            current_ = _NextChar();
                            RETURN_NORMAL_TOKEN_DETAIL(detail, Token_VarArg);
                        }
                        else
                        {
                            current_ = pre_next;
                            RETURN_NORMAL_TOKEN_DETAIL(detail, Token_Concat);
                        }
                    }
                    else if (isdigit(next))
                    {
                        token_buffer_.clear();
                        token_buffer_.push_back(current_);
                        current_ = next;
                        return _NumberXFractional(
                            detail, false, true,
                            [](int c){ return isdigit(c) != 0; },
                            [](int c){ return c == 'e' || c == 'E'; }
                        );
                    }
                    else
                    {
                        current_ = next;
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
                    int token = current_;
                    current_ = _NextChar();
                    RETURN_NORMAL_TOKEN_DETAIL(detail, token);
                }
                break;
            default:
                return _Id(detail);
                break;
            }
        }
        return Token_EOF;
    }

    void Lexer::_NewLine()
    {
        int next = _NextChar();
        if (('\n' == next || '\r' == next) && next != current_)
            current_ = _NextChar();
        else
            current_ = next;
        ++line_;
        column_ = 0;
    }

    void Lexer::_Comment()
    {
        current_ = _NextChar();
        if ('[' == current_)
        {
            current_ = _NextChar();
            if ('[' == current_)
                _MultiLineComment();
            else
                _SingleLineComment();
        }
        else
            _SingleLineComment();
    }

    void Lexer::_SingleLineComment()
    {
        while (current_ != '\r' && current_ != '\n' && current_ != EOF)
            current_ = _NextChar();
    }

    void Lexer::_MultiLineComment()
    {
        bool is_comment_end = false;
        while (!is_comment_end)
        {
            if (']' == current_)
            {
                current_ = _NextChar();
                if (']' == current_)
                {
                    is_comment_end = true;
                    current_ = _NextChar();
                }
            }
            else if (EOF == current_)
            {
                throw Exception("expect complete multi - line comment before <eof>.");
            }
            else if ('\r' == current_ || '\n' == current_)
            {
                _NewLine();
            }
            else
            {
                current_ = _NextChar();
            }
        }
    }

    int32_t Lexer::_Number(TokenDetail *detail)
    {
        bool integer_part = false;
        token_buffer_.clear();
        if ('0' == current_)
        {
            int32_t next = _NextChar();
            if ('x' == next || 'X' == next)
            {
                token_buffer_.push_back(current_);
                token_buffer_.push_back(next);
                current_ = _NextChar();
                return _NumberX(
                    detail, false, IsHexChar,
                    [](int c) { return c == 'p' || c == 'P'; }
                );
            }
            else
            {
                token_buffer_.push_back(current_);
                current_ = next;
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
        while (is_number_char(current_))
        {
            token_buffer_.push_back(current_);
            current_ = _NextChar();
            integer_part = true;
        }
        bool point = false;
        if ('.' == current_)
        {
            token_buffer_.push_back(current_);
            current_ = _NextChar();
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
        while (is_number_char(current_))
        {
            token_buffer_.push_back(current_);
            current_ = _NextChar();
            fractional_part = true;
        }

        if (point && !integer_part && !fractional_part)
            throw Exception("unexpect '.'");
        else if (!point && !integer_part && !fractional_part)
            throw Exception("unexpect incomplete number ");

        if (is_exponent(current_))
        {
            token_buffer_.push_back(current_);
            current_ = _NextChar();
            if (current_ == '-' || current_ == '+')
            {
                token_buffer_.push_back(current_);
                current_ = _NextChar();
            }

            if (!isdigit(current_))
                throw Exception("expect exponent after ");

            while (isdigit(current_))
            {
                token_buffer_.push_back(current_);
                current_ = _NextChar();
            }
        }

        double number = strtod(token_buffer_.c_str(), nullptr);
        RETURN_NUMBER_TOKEN_DETAIL(detail, number);
    }

    int Lexer::_XEqual(TokenDetail *detail, int equal_token)
    {
        int token = current_;

        int next = _NextChar();
        if (next == '=')
        {
            current_ = _NextChar();
            RETURN_NORMAL_TOKEN_DETAIL(detail, equal_token);
        }
        else
        {
            current_ = next;
            RETURN_NORMAL_TOKEN_DETAIL(detail, token);
        }
    }

    int Lexer::_MultiLineString(TokenDetail *detail)
    {
        int equals = 0;
        while (current_ == '=')
        {
            ++equals;
            current_ = _NextChar();
        }

        if (current_ != '[')
            throw Exception("incomplete multi-line string at");

        current_ = _NextChar();
        token_buffer_.clear();

        if (current_ == '\r' || current_ == '\n')
        {
            _NewLine();
            if (equals == 0)    // "[[]]" keeps first '\n'
                token_buffer_.push_back('\n');
        }

        while (current_ != EOF)
        {
            if (current_ == ']')
            {
                current_ = _NextChar();
                int i = 0;
                for (; i < equals; ++i, current_ = _NextChar())
                {
                    if (current_ != '=')
                        break;
                }

                if (i == equals && current_ == ']')
                {
                    current_ = _NextChar();
                    RETURN_TOKEN_DETAIL(detail, token_buffer_, Token_String);
                }
                else
                {
                    token_buffer_.push_back(']');
                    token_buffer_.append(i, '=');
                }
            }
            else if (current_ == '\r' || current_ == '\n')
            {
                _NewLine();
                token_buffer_.push_back('\n');
            }
            else
            {
                token_buffer_.push_back(current_);
                current_ = _NextChar();
            }
        }

        throw Exception("incomplete multi-line string at <eof>");
    }

    int Lexer::_SingleLineString(TokenDetail *detail)
    {
        int quote = current_;
        current_ = _NextChar();
        token_buffer_.clear();

        while (current_ != quote)
        {
            if (current_ == EOF)
                throw Exception("incomplete string at <eof>");

            if (current_ == '\r' || current_ == '\n')
                throw Exception("incomplete string at this line");

            _StringChar();
        }

        current_ = _NextChar();
        RETURN_TOKEN_DETAIL(detail, token_buffer_, Token_String);
    }

    void Lexer::_StringChar()
    {
        if (current_ == '\\')
        {
            current_ = _NextChar();
            if (current_ == 'a')
                token_buffer_.push_back('\a');
            else if (current_ == 'b')
                token_buffer_.push_back('\b');
            else if (current_ == 'f')
                token_buffer_.push_back('\f');
            else if (current_ == 'n')
                token_buffer_.push_back('\n');
            else if (current_ == 'r')
                token_buffer_.push_back('\r');
            else if (current_ == 't')
                token_buffer_.push_back('\t');
            else if (current_ == 'v')
                token_buffer_.push_back('\v');
            else if (current_ == '\\')
                token_buffer_.push_back('\\');
            else if (current_ == '"')
                token_buffer_.push_back('"');
            else if (current_ == '\'')
                token_buffer_.push_back('\'');
            else if (current_ == 'x')
            {
                current_ = _NextChar();
                char hex[3] = { 0 };
                int i = 0;
                for (; i < 2 && IsHexChar(current_); ++i, current_ = _NextChar())
                    hex[i] = current_;
                if (i == 0)
                    throw Exception("unexpect character after '\\x'");
                token_buffer_.push_back(static_cast<char>(strtoul(hex, 0, 16)));
                return;
            }
            else if (isdigit(current_))
            {
                char oct[4] = { 0 };
                for (int i = 0; i < 3 && isdigit(current_); ++i, current_ = _NextChar())
                    oct[i] = current_;
                token_buffer_.push_back(static_cast<char>(strtoul(oct, 0, 8)));
                return;
            }
            else
                throw Exception("unexpect character after '\\'");
        }
        else
        {
            token_buffer_.push_back(current_);
        }

        current_ = _NextChar();
    }

    int Lexer::_Id(TokenDetail *detail)
    {
        if (!isalpha(current_) && current_ != '_')
            throw Exception("unexpect character");

        token_buffer_.clear();
        token_buffer_.push_back(current_);
        current_ = _NextChar();

        while (isalnum(current_) || current_ == '_')
        {
            token_buffer_.push_back(current_);
            current_ = _NextChar();
        }

        int token = 0;
        if (!IsKeyWord(token_buffer_, &token))
            token = Token_Id;
        RETURN_TOKEN_DETAIL(detail, token_buffer_, token);
    }
} // namespace oms