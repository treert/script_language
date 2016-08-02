#include "mcode_generate.h"
#include "mstate.h"
#include "mstring.h"
#include "mfunction.h"
#include "mexception.h"
#include "mguard.h"
#include "mvisitor.h"
#include <memory>
#include <vector>
#include <stack>
#include <list>
#include <limits>
#include <utility>
#include <unordered_map>
#include <assert.h>

namespace oms
{
#define MAX_FUNCTION_REGISTER_COUNT 250
#define MAX_CLOSURE_UPVALUE_COUNT 250

#define CHECK_UPVALUE_MAX_COUNT(index, function)                        \
    if (index >= MAX_CLOSURE_UPVALUE_COUNT)                             \
    {                                                                   \
        throw CodeGenerateException(                                    \
            function->GetModule()->GetCStr(), function->GetLine(),      \
            "too many upvalues in function");                           \
    }

    struct LocalNameInfo
    {
        // Name register id
        int register_id_;
        // Name begin instruction
        int begin_pc_;

        explicit LocalNameInfo(int register_id = 0, int begin_pc = 0)
            : register_id_(register_id),
              begin_pc_(begin_pc) { }
    };

    // Loop AST info data in GenerateBlock
    struct LoopInfo
    {
        // Loop AST
        const SyntaxTree *loop_ast_;
        // Start instruction index
        int start_index_;

        LoopInfo() : loop_ast_(nullptr), start_index_(0) { }
    };

    // Lexical block struct for code generator
    struct GenerateBlock
    {
        GenerateBlock *parent_;
        // Current block register start id
        int register_start_id_;
        // Local names
        // Same names are the same instance String, so using String
        // pointer as key is fine
        std::unordered_map<String *, LocalNameInfo> names_;

        // Current loop ast info
        LoopInfo current_loop_;

        GenerateBlock() : parent_(nullptr), register_start_id_(0) { }
    };

    // Jump info for loop AST
    struct LoopJumpInfo
    {
        enum JumpType { JumpHead, JumpTail };
        // Owner loop AST
        const SyntaxTree *loop_ast_;
        // Jump to AST head or tail
        JumpType jump_type_;
        // Instruction need to be filled
        int instruction_index_;

        LoopJumpInfo(const SyntaxTree *loop_ast,
                     JumpType jump_type,
                     int instruction_index)
            : loop_ast_(loop_ast), jump_type_(jump_type),
              instruction_index_(instruction_index) { }
    };

    // Lexical function struct for code generator
    struct GenerateFunction
    {
        GenerateFunction *parent_;
        // Current block
        GenerateBlock *current_block_;
        // Current function for code generate
        Function *function_;
        // Index of current function in parent
        int func_index_;
        // Register id generator
        int register_id_;
        // Max register count used in current function
        int register_max_;
        // To be filled loop jump info
        std::list<LoopJumpInfo> loop_jumps_;

        GenerateFunction()
            : parent_(nullptr), current_block_(nullptr),
              function_(nullptr), func_index_(0),
              register_id_(0), register_max_(0) { }
    };

    class CodeGenerateVisitor : public Visitor
    {
    public:
        explicit CodeGenerateVisitor(State *state)
            : state_(state), current_function_(nullptr) { }

        ~CodeGenerateVisitor()
        {
            while (current_function_)
            {
                DeleteCurrentFunction();
            }
        }

        virtual void Visit(Chunk *, void *);
        virtual void Visit(Block *, void *);
        virtual void Visit(ReturnStatement *, void *);
        virtual void Visit(BreakStatement *, void *);
        virtual void Visit(DoStatement *, void *);
        virtual void Visit(WhileStatement *, void *);
        virtual void Visit(RepeatStatement *, void *);
        virtual void Visit(IfStatement *, void *);
        virtual void Visit(ElseIfStatement *, void *);
        virtual void Visit(ElseStatement *, void *);
        virtual void Visit(NumericForStatement *, void *);
        virtual void Visit(GenericForStatement *, void *);
        virtual void Visit(FunctionStatement *, void *);
        virtual void Visit(FunctionName *, void *);
        virtual void Visit(LocalFunctionStatement *, void *);
        virtual void Visit(LocalNameListStatement *, void *);
        virtual void Visit(AssignmentStatement *, void *);
        virtual void Visit(VarList *, void *);
        virtual void Visit(Terminator *, void *);
        virtual void Visit(BinaryExpression *, void *);
        virtual void Visit(UnaryExpression *, void *);
        virtual void Visit(FunctionBody *, void *);
        virtual void Visit(ParamList *, void *);
        virtual void Visit(NameList *, void *);
        virtual void Visit(TableDefine *, void *);
        virtual void Visit(TableIndexField *, void *);
        virtual void Visit(TableNameField *, void *);
        virtual void Visit(TableArrayField *, void *);
        virtual void Visit(IndexAccessor *, void *);
        virtual void Visit(MemberAccessor *, void *);
        virtual void Visit(NormalFuncCall *, void *);
        virtual void Visit(MemberFuncCall *, void *);
        virtual void Visit(FuncCallArgs *, void *);
        virtual void Visit(ExpressionList *, void *);

        // Prepare function data when enter each lexical function
        void EnterFunction()
        {
            auto function = new GenerateFunction;
            auto parent = current_function_;
            function->parent_ = parent;
            current_function_ = function;

            // New function is default on GCGen2, so barrier it
            current_function_->function_ = state_->NewFunction();
            CHECK_BARRIER(state_->GetGC(), current_function_->function_);

            if (parent)
            {
                auto index = parent->function_->AddChildFunction(function->function_);
                function->func_index_ = index;
                function->function_->SetSuperior(parent->function_);
                function->function_->SetModuleName(parent->function_->GetModule());
            }
        }

        // Clean up when leave lexical function
        void LeaveFunction()
        {
            DeleteCurrentFunction();
        }

        // Prepare some data when enter each lexical block
        void EnterBlock()
        {
            auto block = new GenerateBlock;
            block->parent_ = current_function_->current_block_;
            block->register_start_id_ = current_function_->register_id_;
            current_function_->current_block_ = block;
        }

        // Clean up when leave lexical block
        void LeaveBlock()
        {
            auto block = current_function_->current_block_;

            // Add all variables in block to the function local variable list
            auto function = current_function_->function_;
            auto end_pc = function->OpCodeSize();
            for (auto it = block->names_.begin(); it != block->names_.end(); ++it)
            {
                function->AddLocalVar(it->first, it->second.register_id_,
                                      it->second.begin_pc_, end_pc);
            }

            // add one instruction to close upvalue
            auto instruction = Instruction::ACode(OpType_CloseUpvalue, block->register_start_id_);
            function->AddInstruction(instruction, 0);

            current_function_->current_block_ = block->parent_;
            current_function_->register_id_ = block->register_start_id_;
            delete block;
        }

        // Prepare data for loop AST
        void EnterLoop(const SyntaxTree *loop_ast)
        {
            // Start instruction index of loop
            int start_index = GetCurrentFunction()->OpCodeSize();
            auto block = current_function_->current_block_;
            block->current_loop_.loop_ast_ = loop_ast;
            block->current_loop_.start_index_ = start_index;
        }

        // Complete loop AST in current block
        void LeaveLoop()
        {
            auto function = GetCurrentFunction();
            // Instruction index after loop
            int end_index = function->OpCodeSize();

            auto &loop = current_function_->current_block_->current_loop_;
            if (loop.loop_ast_)
            {
                auto &loop_jumps = current_function_->loop_jumps_;
                auto it = loop_jumps.begin();
                while (it != loop_jumps.end())
                {
                    if (it->loop_ast_ == loop.loop_ast_)
                    {
                        // Calculate diff between current index with index of destination
                        int diff = 0;
                        if (it->jump_type_ == LoopJumpInfo::JumpHead)
                            diff = loop.start_index_ - it->instruction_index_;
                        else if (it->jump_type_ == LoopJumpInfo::JumpTail)
                            diff = end_index - it->instruction_index_;

                        // Get instruction and refill its jump diff
                        auto i = function->GetMutableInstruction(it->instruction_index_);
                        i->RefillsBx(diff);

                        // Remove it from loop_jumps when it refilled
                        loop_jumps.erase(it++);
                    }
                    else
                        ++it;
                }
            }
        }

        // Add one LoopJumpInfo, the instruction will be refilled
        // when the loop AST complete
        void AddLoopJumpInfo(const SyntaxTree *loop_ast, int instruction_index,
                             LoopJumpInfo::JumpType jump_type)
        {
            current_function_->loop_jumps_.push_back(LoopJumpInfo(loop_ast, jump_type,
                                                                  instruction_index));
        }

        // Insert name into current local scope, replace its info when existed
        void InsertName(String *name, int register_id)
        {
            assert(current_function_ && current_function_->current_block_);

            auto function = current_function_->function_;
            auto block = current_function_->current_block_;
            auto begin_pc = function->OpCodeSize();

            auto it = block->names_.find(name);
            if (it != block->names_.end())
            {
                // Add the same name variable to the function local variable list
                auto end_pc = function->OpCodeSize();
                function->AddLocalVar(name, it->second.register_id_,
                                      it->second.begin_pc_, end_pc);

                // New variable replace the old one
                it->second = LocalNameInfo(register_id, begin_pc);
            }
            else
            {
                // Variable not existed, then insert into
                LocalNameInfo local(register_id, begin_pc);
                block->names_.insert(std::make_pair(name, local));
            }
        }

        // Search name in current lexical function
        const LocalNameInfo * SearchLocalName(String *name) const
        {
            return SearchFunctionLocalName(current_function_, name);
        }

        // Search name in lexical function
        const LocalNameInfo * SearchFunctionLocalName(GenerateFunction *function,
                                                      String *name) const
        {
            auto block = function->current_block_;
            while (block)
            {
                auto it = block->names_.find(name);
                if (it != block->names_.end())
                    return &it->second;
                else
                    block = block->parent_;
            }

            return nullptr;
        }

        // Prepare upvalue info when the name upvalue info not existed, and
        // return upvalue index, otherwise just return upvalue index
        // the name must reference a upvalue, otherwise will assert fail
        int PrepareUpvalue(String *name) const
        {
            // If the upvalue info existed, then return the index of the upvalue
            auto function = GetCurrentFunction();
            auto index = function->SearchUpvalue(name);
            if (index >= 0)
                return index;

            // Search start from parent
            std::stack<GenerateFunction *> parents;
            parents.push(current_function_->parent_);

            int register_index = -1;
            bool parent_local = false;
            while (!parents.empty())
            {
                auto current = parents.top();
                assert(current);
                if (register_index >= 0)
                {
                    // Find it, add it as upvalue to function,
                    // and continue backtrack
                    auto index = current->function_->AddUpvalue(name, parent_local,
                                                                register_index);
                    CHECK_UPVALUE_MAX_COUNT(index, current->function_);
                    register_index = index;
                    parent_local = false;
                    parents.pop();
                }
                else
                {
                    // Find name from local names
                    auto name_info = SearchFunctionLocalName(current, name);
                    if (name_info)
                    {
                        // Find it, get its register_id and start backtrack
                        register_index = name_info->register_id_;
                        parent_local = true;
                        parents.pop();
                    }
                    else
                    {
                        // Find it from current function upvalue list
                        auto index = current->function_->SearchUpvalue(name);
                        if (index >= 0)
                        {
                            // Find it, the name upvalue has been inserted,
                            // then get the upvalue index, and start backtrack
                            register_index = index;
                            parent_local = false;
                            parents.pop();
                        }
                        else
                        {
                            // Not find it, continue to search its parent
                            parents.push(current->parent_);
                        }
                    }
                }
            }

            // Add it as upvalue to current function
            assert(register_index >= 0);
            index = function->AddUpvalue(name, parent_local, register_index);
            CHECK_UPVALUE_MAX_COUNT(index, function);
            return index;
        }

        // Get current function data
        Function * GetCurrentFunction() const
        {
            return current_function_->function_;
        }

        // Generate one register id from current function
        int GenerateRegisterId()
        {
            int id = current_function_->register_id_++;
            if (IsRegisterCountOverflow())
            {
                throw CodeGenerateException(
                    GetCurrentFunction()->GetModule()->GetCStr(),
                    GetCurrentFunction()->GetLine(),
                    "too many local variables in function");
            }
            return id;
        }

        // Get next register id, do not change register generator
        int GetNextRegisterId() const
        {
            return current_function_->register_id_;
        }

        // Reset register id generator, then next GenerateRegisterId
        // use the new id generator to generate register id
        void ResetRegisterIdGenerator(int generator)
        {
            current_function_->register_id_ = generator;
        }

        void AssertRegisterIdValid(int register_id)
        {
            if (register_id > MAX_FUNCTION_REGISTER_COUNT)
            {
                throw CodeGenerateException(
                    GetCurrentFunction()->GetModule()->GetCStr(),
                    GetCurrentFunction()->GetLine(),
                    "too many local variables in function");
            }
        }

        // Is register count overflow
        bool IsRegisterCountOverflow()
        {
            if (current_function_->register_id_ > current_function_->register_max_)
                current_function_->register_max_ = current_function_->register_id_;
            return current_function_->register_max_ > MAX_FUNCTION_REGISTER_COUNT;
        }

    private:
        State *state_;

        void DeleteCurrentFunction()
        {
            auto function = current_function_;

            // Delete all blocks in function
            while (function->current_block_)
            {
                auto block = function->current_block_;
                function->current_block_ = block->parent_;
                delete block;
            }

            current_function_ = function->parent_;
            delete function;
        }

        void FillRemainRegisterNil(int register_id, int end_register, int line)
        {
            // Fill nil into all remain registers
            // when end_register != EXP_VALUE_COUNT_ANY
            auto function = GetCurrentFunction();
            if (end_register != EXP_VALUE_COUNT_ANY)
            {
                while (register_id < end_register)
                {
                    auto instruction = Instruction::ACode(OpType_LoadNil, register_id++);
                    function->AddInstruction(instruction, line);
                }
            }
        }

        template<typename StatementType>
        void IfStatementGenerateCode(StatementType *if_stmt);

        template<typename TableFieldType>
        void SetTableFieldValue(TableFieldType *field,
                                int table_register,
                                int key_register,
                                int line);

        template<typename TableAccessorType, typename LoadKey>
        void AccessTableField(TableAccessorType *accessor,
                              void *data, int line,
                              const LoadKey &load_key);

        template<typename FuncCallType, typename CallerArgAdjuster>
        void FunctionCall(FuncCallType *func_call,
                          const CallerArgAdjuster &adjust_caller_arg);

        // Current code generating function
        GenerateFunction *current_function_;
    };

#define CODE_GENERATE_GUARD(enter, leave)                               \
    Guard g([this]() { this->enter(); }, [this]() { this->leave(); })

#define REGISTER_GENERATOR_GUARD()                                      \
    auto r = GetNextRegisterId();                                       \
    Guard g([]() { }, [=]() { this->ResetRegisterIdGenerator(r); })

#define LOOP_GUARD(loop_ast)                                            \
    Guard l([=]() { this->EnterLoop(loop_ast); },                       \
            [=]() { this->LeaveLoop(); })

    // For Var
    // Var need register to get value
    struct VarValueData
    {
        int value_register_id_;

        VarValueData(int register_id)
            :value_register_id_(register_id)
        {}
    };

    // For table field
    struct TableFieldData
    {
        // Table register
        int table_register_;

        // Array part index, start from 1
        unsigned int array_index_;

        explicit TableFieldData(int table_register)
            : table_register_(table_register),
              array_index_(1) { }
    };

    // For FunctionName AST
    struct FunctionNameData
    {
        int func_register_;

        explicit FunctionNameData(int func_register)
            : func_register_(func_register) { }
    };

    template<typename StatementType>
    void CodeGenerateVisitor::IfStatementGenerateCode(StatementType *if_stmt)
    {
        auto function = GetCurrentFunction();
        int jmp_end_index = 0;
        {
            if_stmt->exp_->Accept(this, nullptr);
            auto register_id = GetNextRegisterId();
            auto instruction = Instruction::AsBxCode(OpType_JmpFalse, register_id, 0);
            int jmp_index = function->AddInstruction(instruction, if_stmt->line_);

            {
                // True branch block generate code
                CODE_GENERATE_GUARD(EnterBlock, LeaveBlock);
                if_stmt->true_branch_->Accept(this, nullptr);
            }

            // Jmp to the end of if-elseif-else statement after excute block
            instruction = Instruction::AsBxCode(OpType_Jmp, 0, 0);
            jmp_end_index = function->AddInstruction(instruction, if_stmt->block_end_line_);

            // Refill OpType_JmpFalse instruction
            int index = function->OpCodeSize();
            function->GetMutableInstruction(jmp_index)->RefillsBx(index - jmp_index);
        }

        if (if_stmt->false_branch_)
            if_stmt->false_branch_->Accept(this, nullptr);

        // Refill OpType_Jmp instruction
        int end_index = function->OpCodeSize();
        function->GetMutableInstruction(jmp_end_index)->RefillsBx(end_index - jmp_end_index);
    }

    template<typename TableFieldType>
    void CodeGenerateVisitor::SetTableFieldValue(TableFieldType *field,
                                                 int table_register,
                                                 int key_register,
                                                 int line)
    {
        REGISTER_GENERATOR_GUARD();
        // Load value
        field->value_->Accept(this, nullptr);
        auto value_register = GenerateRegisterId();

        // Set table field
        auto instruction = Instruction::ABCCode(OpType_SetTable, table_register,
                                                key_register, value_register);
        GetCurrentFunction()->AddInstruction(instruction, line);
    }

    template<typename TableAccessorType, typename LoadKey>
    void CodeGenerateVisitor::AccessTableField(TableAccessorType *accessor,
                                               void *data, int line,
                                               const LoadKey &load_key)
    {
        REGISTER_GENERATOR_GUARD();
        auto start_register = GetNextRegisterId();
        // Load table
        accessor->table_->Accept(this, nullptr);
        auto table_register = GenerateRegisterId();
        // Load key
        load_key();
        auto key_register = GenerateRegisterId();

        if (accessor->semantic_ == SemanticOp_Read)
        {
            // Get table value by key
            auto instruction = Instruction::ABCCode(OpType_GetTable, table_register,
                key_register, start_register);
        }
        else
        {
            assert(accessor->semantic_ == SemanticOp_Write);
            assert(data != nullptr);
            auto var_value_data = static_cast<VarValueData*>(data);
            auto value_register = var_value_data->value_register_id_;

            // Set table value by key
            auto instruction = Instruction::ABCCode(OpType_SetTable, table_register,
                key_register, value_register);
        }
    }

    template<typename FuncCallType, typename CallerArgAdjuster>
    void CodeGenerateVisitor::FunctionCall(FuncCallType *func_call,
                                           const CallerArgAdjuster &adjust_caller_arg)
    {
        REGISTER_GENERATOR_GUARD();

        // Generate code to get caller
        func_call->caller_->Accept(this, nullptr);
        auto caller_register = GenerateRegisterId();

        // Adjust caller, and also adjust args,
        // return how many args adjusted
        int adjust_args = adjust_caller_arg(caller_register);

        // handle args
        func_call->args_->Accept(this, nullptr);
        int args_count = 0;
        bool args_any = false;
        {
            auto* args = static_cast<FuncCallArgs*>(func_call->args_.get());
            if (args->type_ == FuncCallArgs::ExpList)
            {
                auto* exp_list = static_cast<ExpressionList*>(args->arg_.get());
                if (exp_list)
                {
                    args_count = exp_list->exp_list_.size();
                    args_any = exp_list->exp_any_;
                }
                else
                {
                    args_count = 0;
                    args_any = false;
                }
            }
            else
            {
                assert(args->type_ == FuncCallArgs::Table);
                args_count = 1;
                args_any = false;
            }
            args_count += adjust_args;
        }

        // Generate call instruction
        auto function = GetCurrentFunction();
        auto instruction = Instruction::ABCCode(OpType_Call, caller_register, args_count, args_any);
        function->AddInstruction(instruction, func_call->line_);
    }

    void CodeGenerateVisitor::Visit(Chunk *chunk, void *data)
    {
        CODE_GENERATE_GUARD(EnterFunction, LeaveFunction);
        {
            // Generate function code
            auto function = GetCurrentFunction();
            function->SetModuleName(chunk->module_);
            function->SetLine(1);

            CODE_GENERATE_GUARD(EnterBlock, LeaveBlock);
            chunk->block_->Accept(this, nullptr);

            // New one closure
            auto closure = state_->NewClosure();
            closure->SetPrototype(function);

            // Put closure on stack
            auto top = state_->stack_.top_++;
            top->closure_ = closure;
            top->type_ = ValueT_Closure;
        }
    }

    void CodeGenerateVisitor::Visit(Block *block, void *data)
    {
        for (auto &stmt : block->statements_)
            stmt->Accept(this, nullptr);
    }

    void CodeGenerateVisitor::Visit(ReturnStatement *ret_stmt, void *data)
    {
        auto function = GetCurrentFunction();
        Instruction instruction;
        int line = ret_stmt->line_;
        int register_id = GetNextRegisterId();
        bool exp_any_ = false;
        int exp_count = 0;
        if (ret_stmt->exp_list_)
        {
            auto* exp_list = static_cast<ExpressionList*>(ret_stmt->exp_list_.get());
            exp_any_ = exp_list->exp_any_;
            exp_count = exp_list->exp_list_.size();

            exp_list->Accept(this, nullptr);
        }

        instruction = Instruction::ABCCode(OpType_Ret, register_id, exp_count, exp_any_);
        function->AddInstruction(instruction, line);
    }

    void CodeGenerateVisitor::Visit(BreakStatement *break_stmt, void *data)
    {
        assert(break_stmt->loop_);
        auto function = GetCurrentFunction();
        auto instruction = Instruction::AsBxCode(OpType_Jmp, 0, 0);
        int index = function->AddInstruction(instruction, break_stmt->break_.line_);
        AddLoopJumpInfo(break_stmt->loop_, index, LoopJumpInfo::JumpTail);
    }

    void CodeGenerateVisitor::Visit(DoStatement *do_stmt, void *data)
    {
        CODE_GENERATE_GUARD(EnterBlock, LeaveBlock);
        do_stmt->block_->Accept(this, nullptr);
    }

    void CodeGenerateVisitor::Visit(WhileStatement *while_stmt, void *data)
    {
        CODE_GENERATE_GUARD(EnterBlock, LeaveBlock);
        LOOP_GUARD(while_stmt);

        while_stmt->exp_->Accept(this, nullptr);
        auto register_id = GetNextRegisterId();

        // Jump to loop tail when expression is false
        auto function = GetCurrentFunction();
        auto instruction = Instruction::AsBxCode(OpType_JmpFalse, register_id, 0);
        int index = function->AddInstruction(instruction, while_stmt->first_line_);
        AddLoopJumpInfo(while_stmt, index, LoopJumpInfo::JumpTail);

        while_stmt->block_->Accept(this, nullptr);

        // Jump to loop head
        instruction = Instruction::AsBxCode(OpType_Jmp, 0, 0);
        index = function->AddInstruction(instruction, while_stmt->last_line_);
        AddLoopJumpInfo(while_stmt, index, LoopJumpInfo::JumpHead);
    }

    void CodeGenerateVisitor::Visit(RepeatStatement *repeat_stmt, void *data)
    {
        CODE_GENERATE_GUARD(EnterBlock, LeaveBlock);
        LOOP_GUARD(repeat_stmt);

        repeat_stmt->block_->Accept(this, nullptr);

        repeat_stmt->exp_->Accept(this, nullptr);
        auto register_id = GetNextRegisterId();

        // Jump to head when exp value is true
        auto function = GetCurrentFunction();
        auto instruction = Instruction::AsBxCode(OpType_JmpFalse, register_id, 0);
        int index = function->AddInstruction(instruction, repeat_stmt->line_);
        AddLoopJumpInfo(repeat_stmt, index, LoopJumpInfo::JumpHead);
    }

    void CodeGenerateVisitor::Visit(IfStatement *if_stmt, void *data)
    {
        IfStatementGenerateCode(if_stmt);
    }

    void CodeGenerateVisitor::Visit(ElseIfStatement *elseif_stmt, void *data)
    {
        IfStatementGenerateCode(elseif_stmt);
    }

    void CodeGenerateVisitor::Visit(ElseStatement *else_stmt, void *data)
    {
        else_stmt->block_->Accept(this, nullptr);
    }

    void CodeGenerateVisitor::Visit(NumericForStatement *num_for, void *data)
    {
        CODE_GENERATE_GUARD(EnterBlock, LeaveBlock);
        auto function = GetCurrentFunction();
        auto line = num_for->name_.line_;

        // Init name, limit, step
        num_for->exp1_->Accept(this, nullptr);
        auto var_register = GenerateRegisterId();

        num_for->exp2_->Accept(this, nullptr);
        auto limit_register = GenerateRegisterId();

        if (num_for->exp3_)
        {
            num_for->exp3_->Accept(this, nullptr);
        }
        else
        {
            // Default step is 1
            AssertRegisterIdValid(GetNextRegisterId());
            auto instruction = Instruction::ABxCode(OpType_LoadInt, GetNextRegisterId(), 1);
            function->AddInstruction(instruction, line);
        }
        auto step_register = GenerateRegisterId();

        // Init 'for' var, limit, step value
        auto instruction = Instruction::ABCCode(OpType_ForInit, var_register,
                                                limit_register, step_register);
        function->AddInstruction(instruction, line);

        LOOP_GUARD(num_for);
        {
            CODE_GENERATE_GUARD(EnterBlock, LeaveBlock);

            // Check 'for', continue loop or not
            instruction = Instruction::ABCCode(OpType_ForStep, var_register,
                limit_register, step_register);
            function->AddInstruction(instruction, line);

            // Break loop, prepare to jump to the end of the loop
            instruction = Instruction::AsBxCode(OpType_Jmp, 0, 0);
            int index = function->AddInstruction(instruction, line);
            AddLoopJumpInfo(num_for, index, LoopJumpInfo::JumpTail);

            auto name_register = GenerateRegisterId();
            InsertName(num_for->name_.str_, name_register);

            // Prepare name value
            instruction = Instruction::ABCode(OpType_Move, name_register, var_register);
            function->AddInstruction(instruction, line);

            num_for->block_->Accept(this, nullptr);

            // var = var + step
            instruction = Instruction::ABCCode(OpType_Add, var_register,
                var_register, step_register);
            function->AddInstruction(instruction, line);
        }
        // Jump to the begin of the loop
        instruction = Instruction::AsBxCode(OpType_Jmp, 0, 0);
        int index = function->AddInstruction(instruction, line);
        AddLoopJumpInfo(num_for, index, LoopJumpInfo::JumpHead);
    }

    void CodeGenerateVisitor::Visit(GenericForStatement *gen_for, void *data)
    {
        CODE_GENERATE_GUARD(EnterBlock, LeaveBlock);
        auto function = GetCurrentFunction();
        auto line = gen_for->line_;
        Instruction instruction;

        // Init generic for statement data
        auto* exp_list = static_cast<ExpressionList*>(gen_for->exp_list_.get());
        exp_list->Accept(this, nullptr);
        if (exp_list->exp_any_)
        {
            instruction = Instruction::ACode(OpType_SetTop, GetNextRegisterId() + 3);
            function->AddInstruction(instruction, line);
        }
        auto func_register = GenerateRegisterId();
        auto state_register = GenerateRegisterId();
        auto var_register = GenerateRegisterId();

        LOOP_GUARD(gen_for);
        {
            CODE_GENERATE_GUARD(EnterBlock, LeaveBlock);
            
            // Alloca registers for names
            auto name_start = GetNextRegisterId();
            gen_for->name_list_->Accept(this, nullptr);
            auto name_end = GetNextRegisterId();
            assert(name_start < name_end);

            // Alloca temp registers for call iterate function
            AssertRegisterIdValid(name_start + 3);
            auto temp_func = name_start;
            auto temp_state = name_start + 1;
            auto temp_var = name_start + 2;

            // Call iterate function
            auto move = [=](int dst, int src) {
                auto instruction = Instruction::ABCode(OpType_Move, dst, src);
                function->AddInstruction(instruction, line);
            };
            move(temp_func, func_register);
            move(temp_state, state_register);
            move(temp_var, var_register);

            instruction = Instruction::ABCCode(OpType_Call, temp_func, 2, 0);
            function->AddInstruction(instruction, line);

            instruction = Instruction::ACode(OpType_SetTop, name_end);
            function->AddInstruction(instruction, line);

            // Break the loop when the first name value is nil
            instruction = Instruction::AsBxCode(OpType_JmpNil, name_start, 0);
            int index = function->AddInstruction(instruction, line);
            AddLoopJumpInfo(gen_for, index, LoopJumpInfo::JumpTail);

            // Copy first name value to var_register
            move(var_register, name_start);

            gen_for->block_->Accept(this, nullptr);
        }
        
        // Jump to loop start
        instruction = Instruction::AsBxCode(OpType_Jmp, 0, 0);
        int index = function->AddInstruction(instruction, line);
        AddLoopJumpInfo(gen_for, index, LoopJumpInfo::JumpHead);
    }

    void CodeGenerateVisitor::Visit(FunctionStatement *func_stmt, void *data)
    {
        REGISTER_GENERATOR_GUARD();
        func_stmt->func_body_->Accept(this, nullptr);
        auto func_register = GenerateRegisterId();

        FunctionNameData name_data{ func_register };
        func_stmt->func_name_->Accept(this, &name_data);
    }

    void CodeGenerateVisitor::Visit(FunctionName *func_name, void *data)
    {
        assert(!func_name->names_.empty());
        auto func_register = static_cast<FunctionNameData *>(data)->func_register_;
        auto function = GetCurrentFunction();

        bool has_member = func_name->names_.size() > 1 ||
            func_name->member_name_.token_ == Token_Id;

        auto first_name = func_name->names_[0].str_;
        auto first_line = func_name->names_[0].line_;

        if (!has_member)
        {
            assert(func_name->names_.size() == 1);
            Instruction instruction;
            if (func_name->scoping_ == LexicalScoping_Global)
            {
                // Define a global function
                auto index = function->AddConstString(first_name);
                instruction = Instruction::ABxCode(OpType_SetGlobal, func_register, index);
            }
            else if (func_name->scoping_ == LexicalScoping_Upvalue)
            {
                // Change a upvalue to a function
                auto index = PrepareUpvalue(first_name);
                instruction = Instruction::ABCode(OpType_SetUpvalue, func_register, index);
            }
            else if (func_name->scoping_ == LexicalScoping_Local)
            {
                // Change a local variable to a function
                auto local_name = SearchLocalName(first_name);
                instruction = Instruction::ABCode(OpType_Move, local_name->register_id_,
                                                  func_register);
            }
            function->AddInstruction(instruction, first_line);
        }
        else
        {
            Instruction instruction;
            auto table_register = GenerateRegisterId();
            if (func_name->scoping_ == LexicalScoping_Global)
            {
                // Load global variable to table register
                auto index = function->AddConstString(first_name);
                instruction = Instruction::ABxCode(OpType_GetGlobal,
                                                   table_register, index);
            }
            else if (func_name->scoping_ == LexicalScoping_Upvalue)
            {
                // Load upvalue to table register
                auto index = PrepareUpvalue(first_name);
                instruction = Instruction::ABCode(OpType_GetUpvalue, table_register, index);
            }
            else if (func_name->scoping_ == LexicalScoping_Local)
            {
                // Load local variable to table register
                auto local_name = SearchLocalName(first_name);
                instruction = Instruction::ABCode(OpType_Move, table_register,
                                                  local_name->register_id_);
            }
            function->AddInstruction(instruction, first_line);

            bool member = func_name->member_name_.token_ == Token_Id;
            auto size = func_name->names_.size();
            auto count = member ? size : size - 1;
            auto key_register = GenerateRegisterId();

            auto load_key = [=](String *name, int line) {
                auto index = function->AddConstString(name);
                auto instruction = Instruction::ABxCode(OpType_LoadConst, key_register, index);
                function->AddInstruction(instruction, line);
            };

            for (std::size_t i = 1; i < count; ++i)
            {
                // Get value from table by key
                auto name = func_name->names_[i].str_;
                auto line = func_name->names_[i].line_;
                load_key(name, line);
                instruction = Instruction::ABCCode(OpType_GetTable, table_register,
                                                   key_register, table_register);
                function->AddInstruction(instruction, line);
            }

            // Set function as value of table by key 'token'
            const auto &token = member ? func_name->member_name_ : func_name->names_.back();
            load_key(token.str_, token.line_);
            instruction = Instruction::ABCCode(OpType_SetTable, table_register,
                                               key_register, func_register);
            function->AddInstruction(instruction, token.line_);
        }
    }

    void CodeGenerateVisitor::Visit(LocalFunctionStatement *l_func_stmt, void *data)
    {
        InsertName(l_func_stmt->name_.str_, GetNextRegisterId());
        l_func_stmt->func_body_->Accept(this, nullptr);
    }

    void CodeGenerateVisitor::Visit(LocalNameListStatement *l_namelist_stmt, void *data)
    {
        auto function = GetCurrentFunction();
        Instruction instruction;
        auto line = l_namelist_stmt->line_;
        auto* name_list = dynamic_cast<NameList*>(l_namelist_stmt->name_list_.get());
        // Generate code for expression list first, then expression list can get
        // variables which has the same name with variables defined in NameList
        // e.g.
        //     local i = 1
        //     local i = i -- i value is 1
        if (l_namelist_stmt->exp_list_)
        {
            auto* exp_list = static_cast<ExpressionList*>(l_namelist_stmt->exp_list_.get());
            try
            {
                int start_register = GetNextRegisterId();
                exp_list->Accept(this, nullptr);
                if (exp_list->exp_any_)
                {
                    instruction = Instruction::ACode(OpType_SetTop,
                        start_register + name_list->names_.size());
                    function->AddInstruction(instruction, line);
                }
                else
                {
                    FillRemainRegisterNil(start_register + exp_list->exp_list_.size(),
                        start_register + name_list->names_.size(), line);
                }
            }
            catch (const CodeGenerateException &)
            {
                throw CodeGenerateException(
                    GetCurrentFunction()->GetModule()->GetCStr(),
                    l_namelist_stmt->line_,
                    "expression of local name list is too complex");
            }
        }
        name_list->Accept(this, nullptr);
    }

    void CodeGenerateVisitor::Visit(AssignmentStatement *assign_stmt, void *data)
    {
        auto function = GetCurrentFunction();
        auto line = assign_stmt->line_;
        Instruction instruction;
        auto start_register = GetNextRegisterId();

        auto* var_list = static_cast<VarList*>(assign_stmt->var_list_.get());
        auto* exp_list = static_cast<ExpressionList*>(assign_stmt->exp_list_.get());

        auto end_register = start_register + var_list->var_list_.size();
        try
        {
            exp_list->Accept(this, nullptr);
            if (exp_list->exp_any_)
            {
                instruction = Instruction::ACode(OpType_SetTop, end_register);
                function->AddInstruction(instruction, line);
            }
            else
            {
                FillRemainRegisterNil(start_register + exp_list->exp_list_.size(),
                    end_register, line);
            }

            // Assign results to var list
            assign_stmt->var_list_->Accept(this, nullptr);
        }
        catch (const CodeGenerateException &)
        {
            // Var or Exp list consume some registers, and register count overflow,
            // catch it, throw new exception to report assignment statement
            // is too complex
            throw CodeGenerateException(
                GetCurrentFunction()->GetModule()->GetCStr(),
                assign_stmt->line_,
                "assignment statement is too complex");
        }
    }

    void CodeGenerateVisitor::Visit(VarList *var_list, void *data)
    {
        REGISTER_GENERATOR_GUARD();
        int var_count = var_list->var_list_.size();
        auto start_register = GetNextRegisterId();
        auto end_register = start_register + var_count;
        AssertRegisterIdValid(start_register);
        ResetRegisterIdGenerator(end_register);

        // Assign results to each variable
        for (int i = 0; i < var_count; ++i, ++start_register)
        {
            VarValueData var_value_data{start_register};
            var_list->var_list_[i]->Accept(this, &var_value_data);
        }
    }

    void CodeGenerateVisitor::Visit(Terminator *term, void *data)
    {
        REGISTER_GENERATOR_GUARD();
        auto function = GetCurrentFunction();

        // Generate code for SemanticOp_Write
        if (term->semantic_ == SemanticOp_Write)
        {
            assert(term->token_.token_ == Token_Id);
            assert(data != nullptr);
            auto var_value_data = static_cast<VarValueData*>(data);
            auto register_id = var_value_data->value_register_id_;
            if (term->scoping_ == LexicalScoping_Global)
            {
                auto index = function->AddConstString(term->token_.str_);
                auto instruction = Instruction::ABxCode(OpType_SetGlobal, register_id, index);
                function->AddInstruction(instruction, term->token_.line_);
            }
            else if (term->scoping_ == LexicalScoping_Local)
            {
                auto local = SearchLocalName(term->token_.str_);
                assert(local);
                auto instruction = Instruction::ABCode(OpType_Move, local->register_id_, register_id);
                function->AddInstruction(instruction, term->token_.line_);
            }
            else if (term->scoping_ == LexicalScoping_Upvalue)
            {
                auto index = PrepareUpvalue(term->token_.str_);
                auto instruction = Instruction::ABCode(OpType_SetUpvalue, register_id, index);
                function->AddInstruction(instruction, term->token_.line_);
            }
            return ;
        }

        // Generate code for SemanticOp_Read
        auto register_id = GenerateRegisterId();
        Instruction instruction;
        if (term->token_.token_ == Token_Number || term->token_.token_ == Token_String)
        {
            // Load const to register
            auto index = 0;
            if (term->token_.token_ == Token_Number)
                index = function->AddConstNumber(term->token_.number_);
            else
                index = function->AddConstString(term->token_.str_);
            instruction = Instruction::ABxCode(OpType_LoadConst, register_id, index);
        }
        else if (term->token_.token_ == Token_Id)
        {
            if (term->scoping_ == LexicalScoping_Global)
            {
                // Get value from global table by key index
                auto index = function->AddConstString(term->token_.str_);
                instruction = Instruction::ABxCode(OpType_GetGlobal, register_id, index);
            }
            else if (term->scoping_ == LexicalScoping_Local)
            {
                // Load local variable value to dst register
                auto local = SearchLocalName(term->token_.str_);
                assert(local);
                instruction = Instruction::ABCode(OpType_Move, register_id, local->register_id_);
            }
            else if (term->scoping_ == LexicalScoping_Upvalue)
            {
                // Get upvalue index
                auto index = PrepareUpvalue(term->token_.str_);
                instruction = Instruction::ABCode(OpType_GetUpvalue, register_id, index);
            }
        }
        else if (term->token_.token_ == Token_True || term->token_.token_ == Token_False)
        {
            auto bvalue = term->token_.token_ == Token_True ? 1 : 0;
            instruction = Instruction::ABCode(OpType_LoadBool, register_id, bvalue);
        }
        else if (term->token_.token_ == Token_Nil)
        {
            instruction = Instruction::ACode(OpType_LoadNil, register_id);
        }
        else if (term->token_.token_ == Token_VarArg)
        {
            instruction = Instruction::ACode(OpType_VarArg, register_id);
        }

        function->AddInstruction(instruction, term->token_.line_);
    }

    void CodeGenerateVisitor::Visit(BinaryExpression *bin_exp, void *data)
    {
        REGISTER_GENERATOR_GUARD();

        auto function = GetCurrentFunction();
        auto line = bin_exp->op_token_.line_;
        auto token = bin_exp->op_token_.token_;
        if (token == Token_And || token == Token_Or)
        {
            // Calculate left expression
            bin_exp->left_->Accept(this, nullptr);

            // Do not calculate right expression when the result of left expression
            // satisfy semantics of operator
            auto op_type = token == Token_And ? OpType_JmpFalse : OpType_JmpTrue;
            auto instruction = Instruction::AsBxCode(op_type, GetNextRegisterId(), 0);
            int index = function->AddInstruction(instruction, line);

            // Calculate right expression
            bin_exp->right_->Accept(this, nullptr);

            int dst_index = function->OpCodeSize();
            function->GetMutableInstruction(index)->RefillsBx(dst_index - index);

            return;
        }

        int left_register = 0;
        // Generate code to calculate left expression
        {
            bin_exp->left_->Accept(this, nullptr);
            left_register = GenerateRegisterId();
        }

        int right_register = 0;
        // Generate code to calculate right expression
        {
            bin_exp->right_->Accept(this, nullptr);
            right_register = GenerateRegisterId();
        }

        // Choose OpType by operator
        OpType op_type;
        switch (token) {
            case '+': op_type = OpType_Add; break;
            case '-': op_type = OpType_Sub; break;
            case '*': op_type = OpType_Mul; break;
            case '/': op_type = OpType_Div; break;
            case '^': op_type = OpType_Pow; break;
            case '%': op_type = OpType_Mod; break;
            case '<': op_type = OpType_Less; break;
            case '>': op_type = OpType_Greater; break;
            case Token_Concat: op_type = OpType_Concat; break;
            case Token_Equal: op_type = OpType_Equal; break;
            case Token_NotEqual: op_type = OpType_UnEqual; break;
            case Token_LessEqual: op_type = OpType_LessEqual; break;
            case Token_GreaterEqual: op_type = OpType_GreaterEqual; break;
            default: assert(0); break;
        }

        // Generate instruction to calculate
        auto instruction = Instruction::ABCCode(op_type, left_register,
                                                left_register, right_register);
        function->AddInstruction(instruction, line);
    }

    void CodeGenerateVisitor::Visit(UnaryExpression *unexp, void *data)
    {
        REGISTER_GENERATOR_GUARD();

        unexp->exp_->Accept(this, nullptr);
        auto register_id = GenerateRegisterId();

        // Choose OpType by operator
        OpType op_type;
        switch (unexp->op_token_.token_)
        {
            case '-': op_type = OpType_Neg; break;
            case '#': op_type = OpType_Len; break;
            case Token_Not: op_type = OpType_Not; break;
            default: assert(0); break;
        }

        // Generate instruction
        auto function = GetCurrentFunction();
        auto instruction = Instruction::ACode(op_type, register_id);
        function->AddInstruction(instruction, unexp->op_token_.line_);
    }

    void CodeGenerateVisitor::Visit(FunctionBody *func_body, void *data)
    {
        REGISTER_GENERATOR_GUARD();
        int child_index = 0;
        {
            CODE_GENERATE_GUARD(EnterFunction, LeaveFunction);
            auto function = GetCurrentFunction();
            function->SetLine(func_body->line_);
            child_index = current_function_->func_index_;

            {
                CODE_GENERATE_GUARD(EnterBlock, LeaveBlock);
                // Child function generate code
                if (func_body->has_self_)
                {
                    auto register_id = GenerateRegisterId();
                    auto self = state_->GetString("self");
                    InsertName(self, register_id);

                    auto function = GetCurrentFunction();
                    function->AddFixedArgCount(1);
                }

                if (func_body->param_list_)
                    func_body->param_list_->Accept(this, nullptr);
                func_body->block_->Accept(this, nullptr);
            }
        }

        // Generate closure
        auto function = GetCurrentFunction();
        auto i = Instruction::ABxCode(OpType_Closure,
                                        GenerateRegisterId(),
                                        child_index);
        function->AddInstruction(i, func_body->line_);
    }

    void CodeGenerateVisitor::Visit(ParamList *param_list, void *data)
    {
        auto function = GetCurrentFunction();

        if (param_list->vararg_) function->SetHasVararg();

        if (param_list->name_list_)
        {
            auto* name_list = dynamic_cast<NameList*>(param_list->name_list_.get());
            function->AddFixedArgCount(name_list->names_.size());
            name_list->Accept(this, nullptr);
        }
    }

    void CodeGenerateVisitor::Visit(NameList *name_list, void *data)
    {
        auto size = name_list->names_.size();
        for (std::size_t i = 0; i < size; ++i)
        {
            auto register_id = GenerateRegisterId();
            InsertName(name_list->names_[i].str_, register_id);
        }
    }

    void CodeGenerateVisitor::Visit(TableDefine *table, void *data)
    {
        REGISTER_GENERATOR_GUARD();
        // New table
        auto function = GetCurrentFunction();
        auto table_register = GenerateRegisterId();
        auto instruction = Instruction::ACode(OpType_NewTable, table_register);
        function->AddInstruction(instruction, table->line_);

        if (!table->fields_.empty())
        {
            // Init table value
            TableFieldData field_data{ table_register };
            for (auto &field : table->fields_)
            {
                REGISTER_GENERATOR_GUARD();
                field->Accept(this, &field_data);
            }
        }
    }

    void CodeGenerateVisitor::Visit(TableIndexField *field, void *data)
    {
        REGISTER_GENERATOR_GUARD();
        auto field_data = static_cast<TableFieldData *>(data);
        auto table_register = field_data->table_register_;

        // Load key
        field->index_->Accept(this, nullptr);
        auto key_register = GenerateRegisterId();

        SetTableFieldValue(field, table_register, key_register, field->line_);
    }

    void CodeGenerateVisitor::Visit(TableNameField *field, void *data)
    {
        REGISTER_GENERATOR_GUARD();
        auto field_data = static_cast<TableFieldData *>(data);
        auto table_register = field_data->table_register_;

        // Load key
        auto function = GetCurrentFunction();
        auto key_index = function->AddConstString(field->name_.str_);
        auto key_register = GenerateRegisterId();
        auto instruction = Instruction::ABxCode(OpType_LoadConst, key_register, key_index);
        function->AddInstruction(instruction, field->name_.line_);

        SetTableFieldValue(field, table_register, key_register, field->name_.line_);
    }

    void CodeGenerateVisitor::Visit(TableArrayField *field, void *data)
    {
        REGISTER_GENERATOR_GUARD();
        auto field_data = static_cast<TableFieldData *>(data);
        auto table_register = field_data->table_register_;

        // Load key
        auto function = GetCurrentFunction();
        auto key_register = GenerateRegisterId();
        auto instruction = Instruction::ABxCode(OpType_LoadInt, key_register, field_data->array_index_++);
        function->AddInstruction(instruction, field->line_);

        SetTableFieldValue(field, table_register, key_register, field->line_);
    }

    void CodeGenerateVisitor::Visit(IndexAccessor *accessor, void *data)
    {
        AccessTableField(accessor, data, accessor->line_, [=]() {
            accessor->index_->Accept(this, nullptr);
        });
    }

    void CodeGenerateVisitor::Visit(MemberAccessor *accessor, void *data)
    {
        AccessTableField(accessor, data, accessor->member_.line_, [=]() {
            auto key_register = GetNextRegisterId();
            AssertRegisterIdValid(key_register);
            auto function = GetCurrentFunction();
            auto key_index = function->
                AddConstString(accessor->member_.str_);
            auto instruction = Instruction::
                ABxCode(OpType_LoadConst, key_register, key_index);
            function->AddInstruction(instruction,
                accessor->member_.line_);
        });
    }

    void CodeGenerateVisitor::Visit(NormalFuncCall *func_call, void *data)
    {
        FunctionCall(func_call, [](int) { return 0; });
    }

    void CodeGenerateVisitor::Visit(MemberFuncCall *func_call, void *data)
    {
        FunctionCall(func_call, [=](int caller_register) {
            auto function = GetCurrentFunction();
            // Copy table to arg_register as first argument
            auto arg_register = GenerateRegisterId();
            auto instruction = Instruction::ABCode(OpType_Move, arg_register, caller_register);
            function->AddInstruction(instruction, func_call->member_.line_);

            {
                REGISTER_GENERATOR_GUARD();
                // Get key
                auto index = function->AddConstString(func_call->member_.str_);
                auto key_register = GenerateRegisterId();
                instruction = Instruction::ABxCode(OpType_LoadConst, key_register, index);
                function->AddInstruction(instruction, func_call->member_.line_);

                // Get caller function from table
                instruction = Instruction::ABCCode(OpType_GetTable, caller_register,
                                                   key_register, caller_register);
                function->AddInstruction(instruction, func_call->member_.line_);
            }

            return 1;
        });
    }

    void CodeGenerateVisitor::Visit(FuncCallArgs *arg, void *data)
    {
        if (arg->type_ == FuncCallArgs::ExpList)
        {
            if (arg->arg_)
            {
                arg->arg_->Accept(this, nullptr);
            }
        }
        else
        {
            // arg->type_ is FuncCallArgs::Table or FuncCallArgs::String
            arg->arg_->Accept(this, nullptr);
        }
    }

    void CodeGenerateVisitor::Visit(ExpressionList *exp_list, void *data)
    {
        REGISTER_GENERATOR_GUARD();
        // Each expression consume one register
        int count = exp_list->exp_list_.size();
        for (int i = 0; i < count; ++i)
        {
            GenerateRegisterId();
            REGISTER_GENERATOR_GUARD();
            exp_list->exp_list_[i]->Accept(this, nullptr);
        }
    }

    void CodeGenerate(SyntaxTree *root, State *state)
    {
        assert(root && state);
        CodeGenerateVisitor code_generator(state);
        root->Accept(&code_generator, nullptr);
    }
} // namespace oms
