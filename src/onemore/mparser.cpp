#include "mparser.h"
#include "mlex.h"
#include "mstate.h"
#include "mexception.h"
#include <assert.h>

namespace
{
    using namespace oms;

    enum PrefixExpType
    {
        PrefixExpType_Normal,
        PrefixExpType_Var,
    };

    bool exp_ret_any_results(SyntaxTree* exp)
    {
        auto* terminor = dynamic_cast<Terminator*>(exp);
        if (terminor)
        {
            return terminor->token_.token_ == Token_VarArg;
        }

        auto* normal_call = dynamic_cast<NormalFuncCall*>(exp);
        if (normal_call)
        {
            return true;
        }

        auto* member_call = dynamic_cast<MemberFuncCall*>(exp);
        if (member_call)
        {
            return true;
        }

        return false;
    }

    class ParserImpl
    {
    public:
        explicit ParserImpl(Lexer *lexer)
            : lexer_(lexer)
        {
        }

        std::unique_ptr<SyntaxTree> Parse()
        {
            return ParseChunk();
        }

        std::unique_ptr<SyntaxTree> ParseChunk()
        {
            std::unique_ptr<SyntaxTree> block = ParseBlock();
            if (NextToken().token_ != Token_EOF)
                throw ParseException("expect <eof>", current_);

            return std::unique_ptr<SyntaxTree>(new Chunk(std::move(block),
                                                         lexer_->GetLexModule()));
        }

        std::unique_ptr<SyntaxTree> ParseExp(std::unique_ptr<SyntaxTree> left = std::unique_ptr<SyntaxTree>(),
                                             TokenDetail op = TokenDetail(),
                                             int left_priority = 0)
        {
            std::unique_ptr<SyntaxTree> exp = ParseMainExp();
            while (true)
            {
                int right_priority = GetOpPriority(LookAhead());
                if (left_priority < right_priority ||
                    (left_priority == right_priority && IsRightAssociation(LookAhead())))
                {
                    exp = ParseExp(std::move(exp), NextToken(), right_priority);
                }
                else if (left_priority == right_priority)
                {
                    if (left_priority == 0)
                        return exp;
                    assert(left);
                    left = std::unique_ptr<BinaryExpression>(
                        new BinaryExpression(std::move(left), std::move(exp), op));
                    op = NextToken();
                    exp = ParseMainExp();
                }
                else
                {
                    if (left)
                        exp = std::unique_ptr<BinaryExpression>(
                            new BinaryExpression(std::move(left), std::move(exp), op));
                    return exp;
                }
            }
        }

        std::unique_ptr<SyntaxTree> ParseMainExp()
        {
            std::unique_ptr<SyntaxTree> exp;

            switch (LookAhead().token_)
            {
                case Token_Nil:
                case Token_False:
                case Token_True:
                case Token_Number:
                case Token_String:
                case Token_VarArg:
                    exp.reset(new Terminator(NextToken()));
                    break;

                case Token_Function:
                    exp = ParseFunctionDef();
                    break;

                case Token_Id:
                case '(':
                    exp = ParsePrefixExp();
                    break;

                case '{':
                    exp = ParseTableConstructor();
                    break;

                // unop exp priority is 90 less then ^
                case '-':
                case '#':
                case Token_Not:
                {
                    NextToken();
                    std::unique_ptr<UnaryExpression> unexp(new UnaryExpression);
                    unexp->op_token_ = current_;
                    unexp->exp_ = ParseExp(std::unique_ptr<SyntaxTree>(), TokenDetail(), 90);
                    exp = std::move(unexp);
                }
                    break;

                default:
                    throw ParseException("unexpect token for main exp.", look_ahead_);
            }

            return exp;
        }

        std::unique_ptr<SyntaxTree> ParseFunctionDef()
        {
            NextToken();
            assert(current_.token_ == Token_Function);
            return ParseFunctionBody();
        }

        std::unique_ptr<SyntaxTree> ParseFunctionBody()
        {
            int line = LookAhead().line_;
            if (NextToken().token_ != '(')
                throw ParseException("unexpect token after 'function', expect '('", current_);

            std::unique_ptr<SyntaxTree> param_list;

            if (LookAhead().token_ != ')')
                param_list = ParseParamList();

            if (NextToken().token_ != ')')
                throw ParseException("unexpect token after param list, expect ')'", current_);

            std::unique_ptr<SyntaxTree> block = ParseBlock();

            if (NextToken().token_ != Token_End)
                throw ParseException("unexpect token after function body, expect 'end'", current_);

            return std::unique_ptr<SyntaxTree>(new FunctionBody(std::move(param_list),
                                                                std::move(block), line));
        }

        std::unique_ptr<SyntaxTree> ParseParamList()
        {
            bool vararg = false;
            std::unique_ptr<SyntaxTree> name_list;

            if (LookAhead().token_ == Token_Id)
            {
                std::unique_ptr<NameList> names(new NameList);
                names->names_.push_back(NextToken());

                while (LookAhead().token_ == ',')
                {
                    NextToken();        // skip ','
                    if (LookAhead().token_ == Token_Id)
                        names->names_.push_back(NextToken());
                    else if (LookAhead().token_ == Token_VarArg)
                    {
                        NextToken();    // skip Token_VarArg
                        vararg = true;
                        break;
                    }
                    else
                        throw ParseException("unexpect token in param list", look_ahead_);
                }

                name_list = std::move(names);
            }
            else if (LookAhead().token_ == Token_VarArg)
            {
                NextToken();            // skip Token_VarArg
                vararg = true;
            }
            else
                throw ParseException("unexpect token in param list", look_ahead_);

            return std::unique_ptr<SyntaxTree>(new ParamList(std::move(name_list), vararg));
        }

        std::unique_ptr<SyntaxTree> ParseBlock()
        {
            std::unique_ptr<Block> block(new Block);
            int end_state_cnt = 0;
            for (;;){
                std::unique_ptr<SyntaxTree> statement;
                auto current_token = LookAhead();
                switch (current_token.token_)
                {
                case ';':
                    NextToken(); continue;
                case Token_Do:
                    statement = ParseDoStatement(); break;
                case Token_While:
                    statement = ParseWhileStatement(); break;
                case Token_Repeat:
                    statement = ParseRepeatStatement(); break;
                case Token_If:
                    statement = ParseIfStatement(); break;
                case Token_For:
                    statement = ParseForStatement(); break;
                case Token_Function:
                    statement = ParseFunctionStatement(); break;
                case Token_Local:
                    statement = ParseLocalStatement(); break;
                case Token_Return:
                    statement = ParseReturnStatement();
                    ++end_state_cnt;
                    break;
                case Token_Break:
                    statement = ParseBreakStatement(); 
                    ++end_state_cnt;
                    break;
                default:
                    if (IsMainExp(LookAhead()))
                    {
                        statement = ParseOtherStatement(); break;
                    }
                }
                if (statement)
                {
                    if (end_state_cnt > 1)
                        throw ParseException("unexpect statement after return/break/continue statement", current_token);
                    block->statements_.push_back(std::move(statement));
                }
                else
                {
                    break;
                }
            }

            return std::move(block);
        }

        std::unique_ptr<SyntaxTree> ParseReturnStatement()
        {
            NextToken();                // skip 'return'
            assert(current_.token_ == Token_Return);

            std::unique_ptr<ReturnStatement> return_stmt(new ReturnStatement(current_.line_));
            if (IsMainExp(LookAhead()))
            {
                return_stmt->exp_list_ = ParseExpList();
            }
            return std::move(return_stmt);
        }

        std::unique_ptr<SyntaxTree> ParseBreakStatement()
        {
            assert(look_ahead_.token_ == Token_Break);
            return std::unique_ptr<SyntaxTree>(new BreakStatement(NextToken()));
        }

        std::unique_ptr<SyntaxTree> ParseDoStatement()
        {
            NextToken();                // skip 'do'
            assert(current_.token_ == Token_Do);

            std::unique_ptr<SyntaxTree> block = ParseBlock();
            if (NextToken().token_ != Token_End)
                throw ParseException("expect 'end' for do-statement", current_);

            return std::unique_ptr<SyntaxTree>(new DoStatement(std::move(block)));
        }

        std::unique_ptr<SyntaxTree> ParseWhileStatement()
        {
            NextToken();                // skip 'while'
            assert(current_.token_ == Token_While);

            int first_line = current_.line_;

            std::unique_ptr<SyntaxTree> exp = ParseExp();

            if (NextToken().token_ != Token_Do)
                throw ParseException("expect 'do' for while-statement", current_);

            std::unique_ptr<SyntaxTree> block = ParseBlock();

            if (NextToken().token_ != Token_End)
                throw ParseException("expect 'end' for while-statement", current_);

            int last_line = current_.line_;

            return std::unique_ptr<SyntaxTree>(new WhileStatement(std::move(exp), std::move(block),
                                                                  first_line, last_line));
        }

        std::unique_ptr<SyntaxTree> ParseRepeatStatement()
        {
            NextToken();                // skip 'repeat'
            assert(current_.token_ == Token_Repeat);

            std::unique_ptr<SyntaxTree> block = ParseBlock();

            if (NextToken().token_ != Token_Until)
                throw ParseException("expect 'until' for repeat-statement", current_);

            int line = current_.line_;
            std::unique_ptr<SyntaxTree> exp = ParseExp();

            return std::unique_ptr<SyntaxTree>(new RepeatStatement(std::move(block),
                                                                   std::move(exp),
                                                                   line));
        }

        std::unique_ptr<SyntaxTree> ParseIfStatement()
        {
            NextToken();                // skip 'if'
            assert(current_.token_ == Token_If);
            int line = current_.line_;

            std::unique_ptr<SyntaxTree> exp = ParseExp();

            if (NextToken().token_ != Token_Then)
                throw ParseException("expect 'then' for if", current_);

            std::unique_ptr<SyntaxTree> true_branch = ParseBlock();
            int block_end_line = LookAhead().line_;
            std::unique_ptr<SyntaxTree> false_branch = ParseFalseBranchStatement();

            return std::unique_ptr<SyntaxTree>(new IfStatement(std::move(exp),
                                                               std::move(true_branch),
                                                               std::move(false_branch),
                                                               line, block_end_line));
        }

        std::unique_ptr<SyntaxTree> ParseElseIfStatement()
        {
            NextToken();                // skip 'elseif'
            assert(current_.token_ == Token_Elseif);
            int line = current_.line_;

            std::unique_ptr<SyntaxTree> exp = ParseExp();

            if (NextToken().token_ != Token_Then)
                throw ParseException("expect 'then' for elseif", current_);

            std::unique_ptr<SyntaxTree> true_branch = ParseBlock();
            int block_end_line = LookAhead().line_;
            std::unique_ptr<SyntaxTree> false_branch = ParseFalseBranchStatement();

            return std::unique_ptr<SyntaxTree>(new ElseIfStatement(std::move(exp),
                                                                   std::move(true_branch),
                                                                   std::move(false_branch),
                                                                   line, block_end_line));
        }

        std::unique_ptr<SyntaxTree> ParseFalseBranchStatement()
        {
            if (LookAhead().token_ == Token_Elseif)
                return ParseElseIfStatement();
            else if (LookAhead().token_ == Token_Else)
                return ParseElseStatement();
            else if (LookAhead().token_ == Token_End)
                NextToken();            // skip 'end'
            else
                throw ParseException("expect 'end' for if", look_ahead_);

            return std::unique_ptr<SyntaxTree>();
        }

        std::unique_ptr<SyntaxTree> ParseElseStatement()
        {
            NextToken();                // skip 'else'
            assert(current_.token_ == Token_Else);

            std::unique_ptr<SyntaxTree> block = ParseBlock();
            if (NextToken().token_ != Token_End)
                throw ParseException("expect 'end' for else", current_);

            return std::unique_ptr<SyntaxTree>(new ElseStatement(std::move(block)));
        }

        std::unique_ptr<SyntaxTree> ParseFunctionStatement()
        {
            NextToken();                // skip 'function'
            assert(current_.token_ == Token_Function);

            std::unique_ptr<SyntaxTree> func_name = ParseFunctionName();
            std::unique_ptr<SyntaxTree> func_body = ParseFunctionBody();
            return std::unique_ptr<SyntaxTree>(new FunctionStatement(std::move(func_name),
                                                                     std::move(func_body)));
        }

        std::unique_ptr<SyntaxTree> ParseFunctionName()
        {
            if (NextToken().token_ != Token_Id)
                throw ParseException("unexpect token after 'function'", current_);

            std::unique_ptr<FunctionName> func_name(new FunctionName);
            func_name->names_.push_back(current_);

            while (LookAhead().token_ == '.')
            {
                NextToken();            // skip '.'
                if (NextToken().token_ != Token_Id)
                    throw ParseException("unexpect token in function name after '.'", current_);
                func_name->names_.push_back(current_);
            }

            if (LookAhead().token_ == ':')
            {
                NextToken();            // skip ':'
                if (NextToken().token_ != Token_Id)
                    throw ParseException("unexpect token in function name after ':'", current_);
                func_name->member_name_ = current_;
            }

            return std::move(func_name);
        }

        std::unique_ptr<SyntaxTree> ParseForStatement()
        {
            NextToken();                // skip 'for'
            assert(current_.token_ == Token_For);

            if (LookAhead().token_ != Token_Id)
                throw ParseException("expect 'id' after 'for'", look_ahead_);

            if (LookAhead2().token_ == '=')
                return ParseNumericForStatement();
            else
                return ParseGenericForStatement();
        }

        std::unique_ptr<SyntaxTree> ParseNumericForStatement()
        {
            TokenDetail name = NextToken();
            assert(current_.token_ == Token_Id);

            NextToken();                // skip '='
            assert(current_.token_ == '=');

            std::unique_ptr<SyntaxTree> exp1 = ParseExp();
            if (NextToken().token_ != ',')
                throw ParseException("expect ',' in numeric-for", current_);

            std::unique_ptr<SyntaxTree> exp2 = ParseExp();
            std::unique_ptr<SyntaxTree> exp3;

            if (LookAhead().token_ == ',')
            {
                NextToken();            // skip ','
                exp3 = ParseExp();
            }

            if (NextToken().token_ != Token_Do)
                throw ParseException("expect 'do' to start numeric-for body", current_);
            std::unique_ptr<SyntaxTree> block = ParseBlock();

            if (NextToken().token_ != Token_End)
                throw ParseException("expect 'end' to complete numeric-for", current_);
            return std::unique_ptr<SyntaxTree>(new NumericForStatement(name,
                                                                       std::move(exp1), std::move(exp2),
                                                                       std::move(exp3), std::move(block)));
        }

        std::unique_ptr<SyntaxTree> ParseGenericForStatement()
        {
            int line = LookAhead().line_;
            std::unique_ptr<SyntaxTree> name_list = ParseNameList();

            if (NextToken().token_ != Token_In)
                throw ParseException("expect 'in' in generic-for", current_);

            std::unique_ptr<SyntaxTree> exp_list = ParseExpList();

            if (NextToken().token_ != Token_Do)
                throw ParseException("expect 'do' to start generic-for body", current_);

            std::unique_ptr<SyntaxTree> block = ParseBlock();

            if (NextToken().token_ != Token_End)
                throw ParseException("expect 'end' to complete generic-for", current_);

            return std::unique_ptr<SyntaxTree>(new GenericForStatement(std::move(name_list),
                                                                       std::move(exp_list),
                                                                       std::move(block), line));
        }

        std::unique_ptr<SyntaxTree> ParseLocalStatement()
        {
            NextToken();                // skip 'local'
            assert(current_.token_ == Token_Local);

            if (LookAhead().token_ == Token_Function)
                return ParseLocalFunction();
            else if (LookAhead().token_ == Token_Id)
                return ParseLocalNameList();
            else
                throw ParseException("unexpect token after 'local'", look_ahead_);

            assert(!"unreachable");
            return std::unique_ptr<SyntaxTree>();
        }

        std::unique_ptr<SyntaxTree> ParseLocalFunction()
        {
            NextToken();                // skip 'function'
            assert(current_.token_ == Token_Function);

            if (NextToken().token_ != Token_Id)
                throw ParseException("expect 'id' after 'local function'", current_);

            TokenDetail name = current_;
            std::unique_ptr<SyntaxTree> body = ParseFunctionBody();

            return std::unique_ptr<SyntaxTree>(new LocalFunctionStatement(name, std::move(body)));
        }

        std::unique_ptr<SyntaxTree> ParseLocalNameList()
        {
            int start_line = LookAhead().line_;
            std::unique_ptr<SyntaxTree> name_list = ParseNameList();
            std::unique_ptr<SyntaxTree> exp_list;

            if (LookAhead().token_ == '=')
            {
                NextToken();            // skip '='
                exp_list = ParseExpList();
            }

            return std::unique_ptr<SyntaxTree>(new LocalNameListStatement(std::move(name_list),
                std::move(exp_list),
                start_line));
        }

        std::unique_ptr<SyntaxTree> ParseNameList()
        {
            if (NextToken().token_ != Token_Id)
                throw ParseException("expect 'id'", current_);

            std::unique_ptr<NameList> name_list(new NameList);

            name_list->names_.push_back(current_);
            while (LookAhead().token_ == ',')
            {
                NextToken();            // skip ','
                if (NextToken().token_ != Token_Id)
                    throw ParseException("expect 'id' after ','", current_);
                name_list->names_.push_back(current_);
            }

            return std::move(name_list);
        }

        std::unique_ptr<SyntaxTree> ParseOtherStatement()
        {
            assert(IsMainExp(LookAhead()));
            std::unique_ptr<SyntaxTree> exp;
            if (LookAhead().token_ == Token_Id)
            {
                PrefixExpType type;
                int start_line = LookAhead().line_;
                exp = ParsePrefixExp(&type);
                if (type == PrefixExpType_Var &&
                    (LookAhead().token_ == '=' || LookAhead().token_ == ','))
                {
                    std::unique_ptr<VarList> var_list(new VarList);
                    var_list->var_list_.push_back(std::move(exp));
                    while (LookAhead().token_ != '=')
                    {
                        if (LookAhead().token_ != ',')
                            throw ParseException("expect ',' to split var", look_ahead_);
                        NextToken();        // skip ','
                        exp = ParsePrefixExp(&type);
                        if (type != PrefixExpType_Var)
                            throw ParseException("expect var here", current_);
                        var_list->var_list_.push_back(std::move(exp));
                    }
                    NextToken();            // skip '='
                    std::unique_ptr<SyntaxTree> exp_list = ParseExpList();
                    return std::unique_ptr<SyntaxTree>(new AssignmentStatement(std::move(var_list),
                        std::move(exp_list),
                        start_line));
                }
                else
                {
                    int next_priority = GetOpPriority(LookAhead());
                    while (next_priority > 0)
                    {
                        exp = ParseExp(std::move(exp), NextToken(), next_priority);
                        next_priority = GetOpPriority(LookAhead());
                    }
                }
            }
            else
            {
                exp = ParseExp();
            }
            return exp;
        }

        std::unique_ptr<SyntaxTree> ParsePrefixExp(PrefixExpType *type = nullptr)
        {
            NextToken();
            assert(current_.token_ == Token_Id || current_.token_ == '(');

            std::unique_ptr<SyntaxTree> exp;
            PrefixExpType the_type = PrefixExpType_Var;
            PrefixExpType last_type = PrefixExpType_Var;

            if (current_.token_ == '(')
            {
                exp = ParseExp();
                if (NextToken().token_ != ')')
                    throw ParseException("expect ')'", current_);
                the_type = PrefixExpType_Normal;
            }
            else
            {
                exp.reset(new Terminator(current_));
            }

            // parse prefix exp tail
            for (;;)
            {
                if (LookAhead().token_ == '[' || LookAhead().token_ == '.')
                {
                    exp = ParseVar(std::move(exp));
                    last_type = PrefixExpType_Var;
                }
                else if (LookAhead().token_ == ':' || LookAhead().token_ == '(' ||
                    LookAhead().token_ == '{')
                {
                    exp = ParseFunctionCall(std::move(exp));
                    last_type = PrefixExpType_Normal;
                }
                else
                {
                    break;
                }
            }

            if (type){
                *type = (the_type ==  PrefixExpType_Var) ? last_type : PrefixExpType_Normal;
            }
            return exp;
        }

        std::unique_ptr<SyntaxTree> ParseVar(std::unique_ptr<SyntaxTree> table)
        {
            NextToken();
            assert(current_.token_ == '[' || current_.token_ == '.');

            if (current_.token_ == '[')
            {
                int line = LookAhead().line_;
                std::unique_ptr<SyntaxTree> exp = ParseExp();
                if (NextToken().token_ != ']')
                    throw ParseException("expect ']'", current_);
                return std::unique_ptr<SyntaxTree>(new IndexAccessor(std::move(table),
                                                                     std::move(exp),
                                                                     line));
            }
            else
            {
                if (NextToken().token_ != Token_Id)
                    throw ParseException("expect 'id' after '.'", current_);
                return std::unique_ptr<SyntaxTree>(new MemberAccessor(std::move(table), current_));
            }
        }

        std::unique_ptr<SyntaxTree> ParseFunctionCall(std::unique_ptr<SyntaxTree> caller)
        {
            if (LookAhead().token_ == ':')
            {
                NextToken();
                if (NextToken().token_ != Token_Id)
                    throw ParseException("expect 'id' after ':'", current_);

                TokenDetail member = current_;
                int line = LookAhead().line_;
                std::unique_ptr<SyntaxTree> args = ParseArgs();
                return std::unique_ptr<SyntaxTree>(new MemberFuncCall(std::move(caller), member,
                                                                      std::move(args), line));
            }
            else
            {
                int line = LookAhead().line_;
                std::unique_ptr<SyntaxTree> args = ParseArgs();
                return std::unique_ptr<SyntaxTree>(new NormalFuncCall(std::move(caller),
                                                                      std::move(args), line));
            }
        }

        std::unique_ptr<SyntaxTree> ParseArgs()
        {
            std::unique_ptr<SyntaxTree> arg;
            FuncCallArgs::ArgType type;

            if (LookAhead().token_ == '{')
            {
                type = FuncCallArgs::Table;
                arg = ParseTableConstructor();
            }
            else if (LookAhead().token_ == '(')
            {
                type = FuncCallArgs::ExpList;
                NextToken();            // skip '('
                if (LookAhead().token_ != ')')
                    arg = ParseExpList();

                if (NextToken().token_ != ')')
                    throw ParseException("expect ')' to end function call args", current_);
            }
            else
            {
                throw ParseException("expect '(' or '{' to start function call args", look_ahead_);
            }

            return std::unique_ptr<SyntaxTree>(new FuncCallArgs(std::move(arg), type));
        }

        std::unique_ptr<SyntaxTree> ParseExpList()
        {
            std::unique_ptr<ExpressionList> exp_list(new ExpressionList(LookAhead().line_));

            bool anymore = true;
            while (anymore)
            {
                exp_list->exp_list_.push_back(ParseExp());

                if (LookAhead().token_ == ',')
                    NextToken();
                else
                    anymore = false;
            }

            exp_list->exp_any_ = exp_ret_any_results(exp_list->exp_list_.back().get());

            return std::move(exp_list);
        }

        std::unique_ptr<SyntaxTree> ParseTableConstructor()
        {
            NextToken();
            assert(current_.token_ == '{');

            std::unique_ptr<TableDefine> table(new TableDefine(current_.line_));

            while (LookAhead().token_ != '}')
            {
                if (LookAhead().token_ == '[')
                    table->fields_.push_back(ParseTableIndexField());
                else if (LookAhead().token_ == Token_Id && LookAhead2().token_ == '=')
                    table->fields_.push_back(ParseTableNameField());
                else
                    table->fields_.push_back(ParseTableArrayField());

                if (LookAhead().token_ != '}')
                {
                    NextToken();
                    if (current_.token_ != ',' && current_.token_ != ';')
                        throw ParseException("expect ',' or ';' to split table fields", current_);
                }
            }

            if (NextToken().token_ != '}')
                throw ParseException("expect '}' for table", current_);
            return std::move(table);
        }

        std::unique_ptr<SyntaxTree> ParseTableIndexField()
        {
            NextToken();
            assert(current_.token_ == '[');

            int line = LookAhead().line_;
            std::unique_ptr<SyntaxTree> index = ParseExp();

            if (NextToken().token_ != ']')
                throw ParseException("expect ']'", current_);

            if (NextToken().token_ != '=')
                throw ParseException("expect '='", current_);

            std::unique_ptr<SyntaxTree> value = ParseExp();

            return std::unique_ptr<SyntaxTree>(new TableIndexField(std::move(index),
                                                                   std::move(value),
                                                                   line));
        }

        std::unique_ptr<SyntaxTree> ParseTableNameField()
        {
            TokenDetail name = NextToken();

            NextToken();
            assert(current_.token_ == '=');

            std::unique_ptr<SyntaxTree> value = ParseExp();

            return std::unique_ptr<SyntaxTree>(new TableNameField(name, std::move(value)));
        }

        std::unique_ptr<SyntaxTree> ParseTableArrayField()
        {
            int line = LookAhead().line_;
            std::unique_ptr<SyntaxTree> value = ParseExp();
            return std::unique_ptr<SyntaxTree>(new TableArrayField(std::move(value), line));
        }

    private:
        TokenDetail& NextToken()
        {
            if (look_ahead_.token_ != Token_EOF)
            {
                current_ = look_ahead_;
                look_ahead_ = look_ahead2_;
                if (look_ahead2_.token_ != Token_EOF)
                    look_ahead2_.token_ = Token_EOF;
            }
            else
            {
                lexer_->GetToken(&current_);
            }

            return current_;
        }

        TokenDetail& LookAhead()
        {
            if (look_ahead_.token_ == Token_EOF)
                lexer_->GetToken(&look_ahead_);
            return look_ahead_;
        }

        TokenDetail& LookAhead2()
        {
            LookAhead();
            if (look_ahead2_.token_ == Token_EOF)
                lexer_->GetToken(&look_ahead2_);
            return look_ahead2_;
        }

        bool IsMainExp(const TokenDetail &t) const
        {
            int token = t.token_;
            return
                IsUnop(t) ||
                token == Token_Nil ||
                token == Token_False ||
                token == Token_True ||
                token == Token_Number ||
                token == Token_String ||
                token == Token_VarArg ||
                token == Token_Function ||
                token == Token_Id ||
                token == '(' ||
                token == '{';
        }

        bool IsUnop(const TokenDetail &t) const
        {
            int token = t.token_;
            return
                token == Token_Not ||
                token == '#' ||
                token == '-';
        }

        bool IsRightAssociation(const TokenDetail &t) const
        {
            return t.token_ == '^';
        }

        // just for bin op
        int GetOpPriority(const TokenDetail &t) const
        {
            switch (t.token_)
            {
                case '^':               return 100;
                case '*':
                case '/':
                case '%':               return 80;
                case '+':
                case '-':               return 70;
                case Token_Concat:      return 60;
                case '>':
                case '<':
                case Token_GreaterEqual:
                case Token_LessEqual:
                case Token_NotEqual:
                case Token_Equal:       return 50;
                case Token_And:         return 40;
                case Token_Or:          return 30;
                default:                return 0;
            }
        }

        Lexer *lexer_;
        TokenDetail current_;
        TokenDetail look_ahead_;
        TokenDetail look_ahead2_;
    };
} // namespace

namespace oms
{
    std::unique_ptr<SyntaxTree> Parse(Lexer *lexer)
    {
        ParserImpl impl(lexer);
        return impl.Parse();
    }
} // namespace oms
