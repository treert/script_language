#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "../mlex.h"
#include "../mparser.h"
#include "../mstate.h"
#include "../mstring.h"
#include "../mtext_in_stream.h"
#include "../mexception.h"
#include "../mvisitor.h"
#include <functional>
#include <type_traits>

class ParserWrapper
{
public:
    explicit ParserWrapper(const std::string &str = "")
        : iss_(str), state_(), name_("parser"),
          lexer_(&state_, &name_, std::bind(&io::text::InStringStream::GetChar, &iss_))
    {
    }

    void SetInput(const std::string &input)
    {
        iss_.SetInputString(input);
    }

    bool IsEOF()
    {
        oms::TokenDetail detail;
        return lexer_.GetToken(&detail) == oms::Token_EOF;
    }

    std::unique_ptr<oms::SyntaxTree> Parse()
    {
        return oms::Parse(&lexer_);
    }

    oms::State * GetState()
    {
        return &state_;
    }

private:
    io::text::InStringStream iss_;
    oms::State state_;
    oms::String name_;
    oms::Lexer lexer_;
};

#define MATCH_AST_TYPE(ast, not_match_stmt)                             \
    if (!the_ast_node_)                                                 \
    {                                                                   \
        auto f = [&]() not_match_stmt;                                  \
        SetResult(typename std::conditional<                            \
            std::is_same<std::remove_reference<decltype(*ast)>::type,   \
                         ASTType>::value,                               \
            std::true_type, std::false_type>::type(), ast, f);          \
    }

template<typename ASTType, typename FinderType>
class ASTFinder : public oms::Visitor
{
public:
    explicit ASTFinder(const FinderType &finder)
        : the_ast_node_(nullptr), finder_(finder) { }

    ASTType * GetResult() const
    {
        return the_ast_node_;
    }

    virtual void Visit(oms::Chunk *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->block_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::Block *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            for (auto &stmt : ast->statements_)
                stmt->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::ReturnStatement *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            if (ast->exp_list_)
                ast->exp_list_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::BreakStatement *ast, void *)
    {
        MATCH_AST_TYPE(ast, {})
    }

    virtual void Visit(oms::DoStatement *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->block_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::WhileStatement *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->exp_->Accept(this, nullptr);
            ast->block_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::RepeatStatement *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->block_->Accept(this, nullptr);
            ast->exp_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::IfStatement *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->exp_->Accept(this, nullptr);
            ast->true_branch_->Accept(this, nullptr);
            if (ast->false_branch_)
                ast->false_branch_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::ElseIfStatement *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->exp_->Accept(this, nullptr);
            ast->true_branch_->Accept(this, nullptr);
            if (ast->false_branch_)
                ast->false_branch_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::ElseStatement *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->block_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::NumericForStatement *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->exp1_->Accept(this, nullptr);
            ast->exp2_->Accept(this, nullptr);
            if (ast->exp3_)
                ast->exp3_->Accept(this, nullptr);
            ast->block_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::GenericForStatement *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->name_list_->Accept(this, nullptr);
            ast->exp_list_->Accept(this, nullptr);
            ast->block_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::FunctionStatement *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->func_name_->Accept(this, nullptr);
            ast->func_body_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::FunctionName *ast, void *)
    {
        MATCH_AST_TYPE(ast, {})
    }

    virtual void Visit(oms::LocalFunctionStatement *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->func_body_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::LocalNameListStatement *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->name_list_->Accept(this, nullptr);
            if (ast->exp_list_)
                ast->exp_list_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::AssignmentStatement *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->var_list_->Accept(this, nullptr);
            ast->exp_list_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::VarList *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            for (auto &var : ast->var_list_)
                var->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::Terminator *ast, void *)
    {
        MATCH_AST_TYPE(ast, {})
    }

    virtual void Visit(oms::BinaryExpression *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->left_->Accept(this, nullptr);
            ast->right_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::UnaryExpression *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->exp_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::FunctionBody *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            if (ast->param_list_)
                ast->param_list_->Accept(this, nullptr);
            ast->block_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::ParamList *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            if (ast->name_list_)
                ast->name_list_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::NameList *ast, void *)
    {
        MATCH_AST_TYPE(ast, {})
    }

    virtual void Visit(oms::TableDefine *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            for (auto &field : ast->fields_)
                field->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::TableIndexField *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->index_->Accept(this, nullptr);
            ast->value_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::TableNameField *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->value_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::TableArrayField *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->value_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::IndexAccessor *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->table_->Accept(this, nullptr);
            ast->index_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::MemberAccessor *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->table_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::NormalFuncCall *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->caller_->Accept(this, nullptr);
            ast->args_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::MemberFuncCall *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            ast->caller_->Accept(this, nullptr);
            ast->args_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::FuncCallArgs *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            if (ast->arg_)
                ast->arg_->Accept(this, nullptr);
        })
    }

    virtual void Visit(oms::ExpressionList *ast, void *)
    {
        MATCH_AST_TYPE(ast, {
            for (auto &exp : ast->exp_list_)
                exp->Accept(this, nullptr);
        })
    }

private:
    template<typename Type, typename Func>
    void SetResult(std::true_type, Type *t, const Func &op)
    {
        if (finder_(t))
            the_ast_node_ = t;
        else
            op();
    }

    template<typename Type, typename Func>
    void SetResult(std::false_type, Type *t, const Func &op)
    {
        op();
    }

    ASTType *the_ast_node_;
    FinderType finder_;
};

template<typename ASTType, typename FinderType>
ASTType * ASTFind(const std::unique_ptr<oms::SyntaxTree> &root,
                  const FinderType &finder)
{
    ASTFinder<ASTType, FinderType> ast_finder(finder);
    root->Accept(&ast_finder, nullptr);
    return ast_finder.GetResult();
}

struct FindName
{
    FindName(const std::string &name) : name_(name) { }

    bool operator () (const oms::Terminator *term) const
    {
        if (term->token_.token_ == oms::Token_Id)
            return term->token_.str_->GetStdString() == name_;
        else
            return false;
    }

    std::string name_;
};

struct AcceptAST
{
    bool operator () (const oms::SyntaxTree *) const
    { return true; }
};

#endif // TEST_COMMON_H
