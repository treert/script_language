#include "mstate.h"
#include "mgc.h"
#include "mvm.h"
#include "mlex.h"
#include "mstring.h"
#include "mfunction.h"
#include "mtable.h"
#include "mtext_in_stream.h"
#include "mexception.h"
#include <cassert>

namespace oms
{
#define METATABLES "__metatables"
#define MODULES_TABLE "__modules"

    State::State()
    {
        string_pool_.reset(new StringPool);

        // Init GC
        gc_.reset(new GC([&](GCObject *obj, unsigned int type) {
            if (type == GCObjectType_String)
            {
                string_pool_->DeleteString(static_cast<String *>(obj));
            }
            delete obj;
        }));
        auto root = std::bind(&State::FullGCRoot, this, std::placeholders::_1);
        gc_->SetRootTraveller(root, root);

        // New global table
        global_.table_ = NewTable();
        global_.type_ = ValueT_Table;

        // New table for store metatables
        Value k;
        k.type_ = ValueT_String;
        k.str_ = GetString(METATABLES);
        Value v;
        v.type_ = ValueT_Table;
        v.table_ = NewTable();
        global_.table_->SetValue(k, v);

        // New table for store modules
        k.type_ = ValueT_String;
        k.str_ = GetString(MODULES_TABLE);
        v.type_ = ValueT_Table;
        v.table_ = NewTable();
        global_.table_->SetValue(k, v);

        // Init module manager
        module_manager_.reset(new ModuleManager(this, v.table_));
    }

    State::~State()
    {
        gc_->ResetDeleter();
    }

    bool State::IsModuleLoaded(const std::string &module_name) const
    {
        return module_manager_->IsLoaded(module_name);
    }

    void State::LoadModule(const std::string &module_name)
    {
        auto value = module_manager_->GetModuleClosure(module_name);
        if (value.IsNil())
            module_manager_->LoadModule(module_name);
        else
            *stack_.top_++ = value;
    }

    void State::DoModule(const std::string &module_name)
    {
        LoadModule(module_name);
        if (CallFunction(stack_.top_ - 1, 0))
        {
            VM vm(this);
            vm.Execute();
        }
    }

    void State::DoString(const std::string &str, const std::string &name)
    {
        module_manager_->LoadString(str, name);
        if (CallFunction(stack_.top_ - 1, 0))
        {
            VM vm(this);
            vm.Execute();
        }
    }

    bool State::CallFunction(Value *f, int arg_count)
    {
        assert(f->type_ == ValueT_Closure || f->type_ == ValueT_CFunction);

        if (f->type_ == ValueT_Closure)
        {
            // We need enter next ExecuteFrame
            CallClosure(f, arg_count);
            return true;
        }
        else
        {
            CallCFunction(f, arg_count);
            return false;
        }
    }

    String * State::GetString(const std::string &str)
    {
        auto s = string_pool_->GetString(str);
        if (!s)
        {
            s = gc_->NewString();
            s->SetValue(str);
            string_pool_->AddString(s);
        }
        return s;
    }

    String * State::GetString(const char *str, std::size_t len)
    {
        auto s = string_pool_->GetString(str, len);
        if (!s)
        {
            s = gc_->NewString();
            s->SetValue(str, len);
            string_pool_->AddString(s);
        }
        return s;
    }

    String * State::GetString(const char *str)
    {
        auto s = string_pool_->GetString(str);
        if (!s)
        {
            s = gc_->NewString();
            s->SetValue(str);
            string_pool_->AddString(s);
        }
        return s;
    }

    Function * State::NewFunction()
    {
        return gc_->NewFunction();
    }

    Closure * State::NewClosure()
    {
        return gc_->NewClosure();
    }

    Upvalue * State::NewUpvalue()
    {
        return gc_->NewUpvalue();
    }

    Table * State::NewTable()
    {
        return gc_->NewTable();
    }

    UserData * State::NewUserData()
    {
        return gc_->NewUserData();
    }

    CallInfo * State::GetCurrentCall()
    {
        if (calls_.empty())
            return nullptr;
        return &calls_.back();
    }

    Value * State::GetGlobal()
    {
        return &global_;
    }

    Table * State::GetMetatable(const char *metatable_name)
    {
        Value k;
        k.type_ = ValueT_String;
        k.str_ = GetString(metatable_name);

        auto metatables = GetMetatables();
        auto metatable = metatables->GetValue(k);

        // Create table when metatable not existed
        if (metatable.type_ == ValueT_Nil)
        {
            metatable.type_ = ValueT_Table;
            metatable.table_ = NewTable();
            metatables->SetValue(k, metatable);
        }

        assert(metatable.type_ == ValueT_Table);
        return metatable.table_;
    }

    void State::EraseMetatable(const char *metatable_name)
    {
        Value k;
        k.type_ = ValueT_String;
        k.str_ = GetString(metatable_name);

        Value nil;
        auto metatables = GetMetatables();
        metatables->SetValue(k, nil);
    }

    void State::FullGCRoot(GCObjectVisitor *v)
    {
        // Visit global table
        global_.Accept(v);

        // Visit stack values
        for (const auto &value : stack_.stack_)
        {
            value.Accept(v);
        }

        // Visit upvalue values
        for (const auto &value : stack_.upvalue_list_)
        {
            value->Accept(v);
        }

        // Visit call info
        for (const auto &call : calls_)
        {
            call.register_->Accept(v);
            if (call.func_)
            {
                call.func_->Accept(v);
            }
        }
    }

    Table * State::GetMetatables()
    {
        Value k;
        k.type_ = ValueT_String;
        k.str_ = GetString(METATABLES);

        auto v = global_.table_->GetValue(k);
        assert(v.type_ == ValueT_Table);
        return v.table_;
    }

    void State::CallClosure(Value *f, int arg_count)
    {
        CallInfo callee;
        Function *callee_proto = f->closure_->GetPrototype();

        callee.func_ = f;
        callee.instruction_ = callee_proto->GetOpCodes();
        callee.end_ = callee.instruction_ + callee_proto->OpCodeSize();

        int fixed_args = callee_proto->FixedArgCount();
        Value *arg = f + 1;
        if (callee_proto->HasVararg())
        {
            // move args for var_arg
            Value *old_arg = arg;
            arg += arg_count;
            for (int i = 0; i < arg_count && i < fixed_args; ++i)
                *(arg + i) = *old_arg++;
        }
        callee.register_ = arg;
        // Fill nil for rest fixed_arg
        for (int i = arg_count; i < fixed_args; ++i)
        {
            (callee.register_ + i)->SetNil();
        }

        calls_.push_back(callee);
    }

    void State::CallCFunction(Value *f, int arg_count)
    {
        // Push the c function CallInfo
        CallInfo callee;
        callee.register_ = f + 1;
        callee.func_ = f;
        calls_.push_back(callee);

        // Call c function, use stack top tell arg_count
        stack_.top_ = f + 1 + arg_count;
        CFunctionType cfunc = f->cfunc_;
        ClearCFunctionError();
        int res_count = cfunc(this);
        CheckCFunctionError();

        // Copy c function result to caller stack
        Value *src = stack_.top_ - res_count;
        Value *dst = f;
        for (int i = 0; i < res_count; ++i)
        {
            *dst++ = *src++;
        }

        // Set registers which after dst to nil
        // and set new stack top pointer
        dst->SetNil();
        stack_.SetNewTop(dst);

        // Pop the c function CallInfo
        calls_.pop_back();
    }

    void State::CheckCFunctionError()
    {
        auto error = GetCFunctionErrorData();
        if (error->type_ == CFuntionErrorType_NoError)
            return ;

        CallCFuncException exp;
        if (error->type_ == CFuntionErrorType_ArgCount)
        {
            exp = CallCFuncException("expect ",
                    error->expect_arg_count_, " arguments");
        }
        else if (error->type_ == CFuntionErrorType_ArgType)
        {
            auto &call = calls_.back();
            auto arg = call.register_ + error->arg_index_;
            exp = CallCFuncException("argument #", error->arg_index_ + 1,
                    " is a ", arg->TypeName(), " value, expect a ",
                    Value::TypeName(error->expect_type_), " value");
        }

        // Pop the c function CallInfo
        calls_.pop_back();
        throw exp;
    }
} // namespace oms
