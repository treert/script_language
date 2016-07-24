#include "mlex.h"
#include "mstate.h"
#include "mexception.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <algorithm>

namespace
{
    const char *keyword[] = {
        "and", "break", "do", "else", "elseif", "end",
        "false", "for", "function", "if", "in",
        "local", "nil", "not", "or", "repeat",
        "return", "then", "true", "until", "while"
    };

    bool IsKeyWord(const std::string& name, int *token)
    {
        assert(token);
        auto result = std::equal_range(keyword, keyword + sizeof(keyword) / sizeof(keyword[0]),
                                       name.c_str(), [](const char *left, const char *right) {
                                           return strcmp(left, right) < 0;
                                       });
        if (result.first == result.second)
            return false;

        *token = result.first - keyword + oms::Token_And;
        return true;
    }

    bool IsIdLeftLetter(int c)
    {
        return strchr("_",c) || isalnum(c);
    }

    bool IsIdFirstLetter(int c)
    {
        return strchr("$_", c) || isalpha(c);
    }

    bool IsWordLetter(int c)
    {
        return strchr("_", c) || isalnum(c);
    }

} // namespace

namespace oms
{
#define RETURN_NORMAL_TOKEN_DETAIL(detail, token)               \
    do {                                                        \
        detail->token_ = token;                                 \
        detail->line_ = line_;                                  \
        detail->column_ = column_;                              \
        detail->module_ = module_;                              \
        return token;                                           \
    } while (0)

#define RETURN_NUMBER_TOKEN_DETAIL(detail, number)              \
    do {                                                        \
        detail->number_ = number;                               \
        RETURN_NORMAL_TOKEN_DETAIL(detail, Token_Number);       \
    } while (0)

#define RETURN_TOKEN_DETAIL(detail, string, token)              \
    do {                                                        \
        detail->str_ = state_->GetString(string);               \
        RETURN_NORMAL_TOKEN_DETAIL(detail, token);              \
    } while (0)

#define SET_EOF_TOKEN_DETAIL(detail)                            \
    do {                                                        \
        detail->str_ = nullptr;                                 \
        detail->token_ = Token_EOF;                             \
        detail->line_ = line_;                                  \
        detail->column_ = column_;                              \
        detail->module_ = module_;                              \
    } while (0)

    Lexer::Lexer(State *state, String *module, CharInStream in)
        : state_(state),
          module_(module),
          in_stream_(in),
          current_(EOF),
          line_(1),
          column_(0)
    {
        Next();
    }

    int Lexer::GetToken(TokenDetail *detail)
    {
        assert(detail);
        SET_EOF_TOKEN_DETAIL(detail);
        while (current_ != EOF)
        {
            switch (current_) {
            case '\r': case '\n':
                LexNewLine();
                break;
            case '-':
                {
                    Next();
                    if (current_ == '-')
                        LexComment();
                    else
                    {
                        RETURN_NORMAL_TOKEN_DETAIL(detail, '-');
                    }
                }
                break;
            case '.':
                {
                    Next();
                    if (current_ == '.')
                    {
                        Next();
                        if (current_ == '.')
                        {
                            Next();
                            RETURN_NORMAL_TOKEN_DETAIL(detail, Token_VarArg);
                        }
                        else
                        {
                            RETURN_NORMAL_TOKEN_DETAIL(detail, Token_Concat);
                        }
                    }
                    else if (isdigit(current_))
                    {
                        token_buffer_.assign(1,'.');
                        return LexNumber(detail);
                    }
                    else
                    {
                        RETURN_NORMAL_TOKEN_DETAIL(detail, '.');
                    }
                }
                break;
            case '~':
                {
                    Next();
                    if (current_ != '=')
                        throw LexException(module_->GetCStr(),
                                line_, column_, "expect '=' after '~'");
                    Next();
                    RETURN_NORMAL_TOKEN_DETAIL(detail, Token_NotEqual);
                }
                break;
            case '=':
                return LexXEqual(detail, Token_Equal);
            case '>':
                return LexXEqual(detail, Token_GreaterEqual);
            case '<':
                return LexXEqual(detail, Token_LessEqual);
            case '[':
                {
                    Next();
                    if (current_ == '[' || current_ == '=')
                        return LexMultiLineString(detail);
                    else
                        RETURN_NORMAL_TOKEN_DETAIL(detail, '[');
                }
                break;
            case '\'': case '"':
                return LexSingleLineString(detail);
            default:
                if (isspace(current_)) {
                    Next();
                    break;
                }
                else if (isdigit(current_))
                {
                    token_buffer_.clear();
                    return LexNumber(detail);
                }
                else if (IsIdFirstLetter(current_))
                {
                    return LexId(detail);
                }
                else
                {
                    int token = current_;
                    Next();
                    RETURN_NORMAL_TOKEN_DETAIL(detail, token);
                }
            }
        }

        return Token_EOF;
    }


    void Lexer::Next()
    {
        current_ = in_stream_();
        assert(current_ != 0);
        if (current_ != EOF) ++column_;
    }

    void Lexer::LexNewLine()
    {
        Next();
        if ((current_ == '\r' || current_ == '\n') && current_ != current_)
        {
            Next();
        }
        ++line_;
        column_ = 0;
    }

    void Lexer::LexComment()
    {
        Next();
        if (current_ == '[')
        {
            // named comment
            Next();
            token_buffer_.clear();
            while (IsWordLetter(current_))
            {
                token_buffer_.push_back(current_);
                Next();
            }
            if (current_ == '[')
            {
                LexNamedComment();
            }
            else
            {
                throw LexException(module_->GetCStr(), line_, column_,
                        "named comment expect [ after name '",token_buffer_,"'");
            }
        }
        else
            LexSingleLineComment();
    }

    void Lexer::LexNamedComment()
    {
        while (true)
        {
            if (current_ == ']')
            {
                Next();
                int i = 0;
                int len = token_buffer_.size();
                while (current_ == token_buffer_[i])
                {
                    ++i;
                    Next();
                }
                if (i == len && current_ == ']')
                {
                    Next();
                    break;
                }
            }
            else if (current_ == EOF)
            {
                // uncompleted multi-line comment
                //throw LexException(module_->GetCStr(), line_, column_,
                //        "expect complete multi-line comment before <eof>.");
                break;
            }
            else if (current_ == '\r' || current_ == '\n')
            {
                LexNewLine();
            }
            else
            {
                Next();
            }
        }
    }

    void Lexer::LexSingleLineComment()
    {
        while (current_ != '\r' && current_ != '\n' && current_ != EOF)
        {
            Next();
        }
    }

    int Lexer::LexNumber(TokenDetail *detail)
    {
        assert(isdigit(current_));
        do{
            token_buffer_.push_back(current_);
            Next();
        } while (isdigit(current_) || '.' == current_);
        if (strchr("Ee", current_))
        {
            token_buffer_.push_back(current_);
            Next();
            if (strchr("+-", current_))
            {
                token_buffer_.push_back(current_);
                Next();
            }
        }
        while (isalnum(current_))
        {
            token_buffer_.push_back(current_);
            Next();
        }

        double number;
        char *end_ptr;
        if ('X' == token_buffer_[1] || 'x' == token_buffer_[1])
        {
            number = static_cast<double>(strtoul(token_buffer_.c_str(), &end_ptr, 16));
        }
        else
        {
            number = strtod(token_buffer_.c_str(), &end_ptr);
        }

        if (*end_ptr != '\0')
        {
            throw LexException(module_->GetCStr(), line_, column_,
                "bad number '", token_buffer_, "'");
        }
        RETURN_NUMBER_TOKEN_DETAIL(detail, number);
    }

    int Lexer::LexXEqual(TokenDetail *detail, int equal_token)
    {
        int single_token = current_;
        Next();
        if (current_ == '=')
        {
            Next();
            RETURN_NORMAL_TOKEN_DETAIL(detail, equal_token);
        }
        else
        {
            RETURN_NORMAL_TOKEN_DETAIL(detail, single_token);
        }
    }

    int Lexer::LexMultiLineString(TokenDetail *detail)
    {
        int equals = 0;
        while (current_ == '=')
        {
            ++equals;
            Next();
        }

        if (current_ != '[')
            throw LexException(module_->GetCStr(), line_, column_,
                    "incomplete multi-line string at '", token_buffer_, "'");

        Next();
        token_buffer_.clear();

        if (current_ == '\r' || current_ == '\n')
        {
            LexNewLine();// ignore first '\n'
        }

        while (current_ != EOF)
        {
            if (current_ == ']')
            {
                Next();
                int i = 0;
                for (; i < equals; ++i, Next())
                {
                    if (current_ != '=')
                        break;
                }

                if (i == equals && current_ == ']')
                {
                    Next();
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
                token_buffer_.push_back('\n');
                LexNewLine();
            }
            else
            {
                token_buffer_.push_back(current_);
                Next();
            }
        }

        throw LexException(module_->GetCStr(), line_, column_,
                "incomplete multi-line string at <eof>");
    }

    int Lexer::LexSingleLineString(TokenDetail *detail)
    {
        int quote = current_;
        Next();
        token_buffer_.clear();

        while (current_ != quote)
        {
            if (current_ == EOF)
                throw LexException(module_->GetCStr(), line_, column_,
                        "incomplete string at <eof>");

            if (current_ == '\r' || current_ == '\n')
                throw LexException(module_->GetCStr(), line_, column_,
                        "incomplete string at this line");

            LexStringChar();
        }

        Next();
        RETURN_TOKEN_DETAIL(detail, token_buffer_, Token_String);
    }

    void Lexer::LexStringChar()
    {
        if (current_ == '\\')
        {
            Next();
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
                Next();
                char hex[3] = { 0 };
                int i = 0;
                for (; i < 2 && isxdigit(current_); ++i, Next())
                    hex[i] = current_;
                if (i == 0)
                    throw LexException(module_->GetCStr(), line_, column_,
                            "unexpect character after '\\x'");
                token_buffer_.push_back(static_cast<char>(strtoul(hex, nullptr, 16)));
                return ;
            }
            else if (isdigit(current_))
            {
                char oct[4] = { 0 };
                for (int i = 0; i < 3 && isdigit(current_); ++i, Next())
                    oct[i] = current_;
                token_buffer_.push_back(static_cast<char>(strtoul(oct, 0, 8)));
                return ;
            }
            else
                throw LexException(module_->GetCStr(), line_, column_,
                        "unexpect character after '\\'");
        }
        else
        {
            token_buffer_.push_back(current_);
        }

        Next();
    }

    int Lexer::LexId(TokenDetail *detail)
    {
        assert(IsIdFirstLetter(current_));
        token_buffer_.clear();
        do{
            token_buffer_.push_back(current_);
            Next();
        } while (IsIdLeftLetter(current_));

        int token = 0;
        if (!IsKeyWord(token_buffer_, &token))
            token = Token_Id;
        RETURN_TOKEN_DETAIL(detail, token_buffer_, token);
    }
} // namespace oms
