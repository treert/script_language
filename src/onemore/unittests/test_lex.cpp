
#include "unit_test.h"
#include "../mlex.h"
#include "../mexception.h"

namespace
{

    class LexerGetChar
    {
    public:
        LexerGetChar(const std::string &str):_iss(str),_pos(0){}

        int32_t operator() (){
            if (_pos < _iss.size())
            {
                return static_cast<int32_t>(_iss[_pos++]);
            }
            else
            {
                return EOF;
            }
        }
    private:
        std::string _iss;
        std::size_t _pos;
    };

    class LexerWrapper
    {
    public:
        explicit LexerWrapper(const std::string &str):
            _lexer(LexerGetChar(str))
        {

        }

        int32_t GetToken()
        {
            oms::TokenDetail token;
            return _lexer.GetToken(&token);
        }

    private:
        oms::Lexer _lexer;
    };
} // namespace

TEST_CASE(lex1)
{
    LexerWrapper lexer("\r\n\t\v\f ");
    EXPECT_TRUE(lexer.GetToken() == oms::Token_EOF);
}

TEST_CASE(lex2)
{
    LexerWrapper lexer("-- this is comment\n"
        "--[[this is long\n comment]]"
        "--[[this is long\n comment too--]]"
        "--[[incomplete comment]");

    EXPECT_EXCEPTION(oms::Exception, {
        lexer.GetToken();
    });
}

TEST_CASE(lex3)
{
    LexerWrapper lexer("[==[long\nlong\nstring]==]'string'\"string\""
        "[=[incomplete string]=");
    for (int i = 0; i < 3; ++i)
        EXPECT_TRUE(lexer.GetToken() == oms::Token_String);

    EXPECT_EXCEPTION(oms::Exception, {
        lexer.GetToken();
    });
}

TEST_CASE(lex4)
{
    LexerWrapper lexer("3 3.0 3.1416 314.16e-2 0.31416E1 0xff 0x0.1E 0xA23p-4 0X1.921FB54442D18P+1"
        " 0x");
    for (int i = 0; i < 9; ++i)
        EXPECT_TRUE(lexer.GetToken() == oms::Token_Number);

    EXPECT_EXCEPTION(oms::Exception, {
        lexer.GetToken();
    });
}

TEST_CASE(lex5)
{
    LexerWrapper lexer("+ - * / % ^ # == ~= <= >= < > = ( ) { } [ ] ; : , . .. ...");
    EXPECT_TRUE(lexer.GetToken() == '+');
    EXPECT_TRUE(lexer.GetToken() == '-');
    EXPECT_TRUE(lexer.GetToken() == '*');
    EXPECT_TRUE(lexer.GetToken() == '/');
    EXPECT_TRUE(lexer.GetToken() == '%');
    EXPECT_TRUE(lexer.GetToken() == '^');
    EXPECT_TRUE(lexer.GetToken() == '#');
    EXPECT_TRUE(lexer.GetToken() == oms::Token_Equal);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_NotEqual);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_LessEqual);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_GreaterEqual);
    EXPECT_TRUE(lexer.GetToken() == '<');
    EXPECT_TRUE(lexer.GetToken() == '>');
    EXPECT_TRUE(lexer.GetToken() == '=');
    EXPECT_TRUE(lexer.GetToken() == '(');
    EXPECT_TRUE(lexer.GetToken() == ')');
    EXPECT_TRUE(lexer.GetToken() == '{');
    EXPECT_TRUE(lexer.GetToken() == '}');
    EXPECT_TRUE(lexer.GetToken() == '[');
    EXPECT_TRUE(lexer.GetToken() == ']');
    EXPECT_TRUE(lexer.GetToken() == ';');
    EXPECT_TRUE(lexer.GetToken() == ':');
    EXPECT_TRUE(lexer.GetToken() == ',');
    EXPECT_TRUE(lexer.GetToken() == '.');
    EXPECT_TRUE(lexer.GetToken() == oms::Token_Concat);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_VarArg);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_EOF);
}

TEST_CASE(lex6)
{
    LexerWrapper lexer("and do else elseif end false for function if in local "
        "nil not or repeat return then true until while");
    EXPECT_TRUE(lexer.GetToken() == oms::Token_And);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_Do);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_Else);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_Elseif);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_End);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_False);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_For);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_Function);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_If);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_In);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_Local);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_Nil);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_Not);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_Or);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_Repeat);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_Return);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_Then);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_True);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_Until);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_While);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_EOF);
}

TEST_CASE(lex7)
{
    LexerWrapper lexer("_ __ ___ _1 _a _a1 a1 a_ a_1 name");
    for (int i = 0; i < 10; ++i)
        EXPECT_TRUE(lexer.GetToken() == oms::Token_Id);
    EXPECT_TRUE(lexer.GetToken() == oms::Token_EOF);
}