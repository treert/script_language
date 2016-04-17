#include "LibBase.h"
#include "Table.h"
#include "Upvalue.h"
#include "State.h"
#include "String.h"
#include <string>
#include <iostream>
#include <assert.h>
#include <stdio.h>

namespace lib {
namespace base {

    int Print(luna::State *state)
    {
        luna::StackAPI api(state);
        int params = api.GetStackSize();

        for (int i = 0; i < params; ++i)
        {
            luna::ValueT type = api.GetValueType(i);

            switch (type) {
                case luna::ValueT_Nil:
                    printf("nil");
                    break;
                case luna::ValueT_Bool:
                    printf("%s", api.GetBool(i) ? "true" : "false");
                    break;
                case luna::ValueT_Number:
                    printf("%.14g", api.GetNumber(i));
                    break;
                case luna::ValueT_String:
                    printf("%s", api.GetCString(i));
                    break;
                case luna::ValueT_Closure:
                    printf("function:\t%p", api.GetClosure(i));
                    break;
                case luna::ValueT_Table:
                    printf("table:\t%p", api.GetTable(i));
                    break;
                case luna::ValueT_UserData:
                    printf("userdata:\t%p", api.GetUserData(i));
                    break;
                case luna::ValueT_CFunction:
                    printf("function:\t%p", api.GetCFunction(i));
                    break;
                default:
                    break;
            }

            if (i != params - 1)
                printf("\t");
        }

        printf("\n");
        return 0;
    }

    int Puts(luna::State *state)
    {
        luna::StackAPI api(state);
        if (!api.CheckArgs(1, luna::ValueT_String))
            return 0;

        printf("%s", api.GetCString(0));
        return 0;
    }

    int Type(luna::State *state)
    {
        luna::StackAPI api(state);
        if (!api.CheckArgs(1))
            return 0;

        const luna::Value *v = api.GetValue(0);
        luna::ValueT type = v->type_ == luna::ValueT_Upvalue ?
            v->upvalue_->GetValue()->type_ : v->type_;

        switch (type) {
            case luna::ValueT_Nil:
                api.PushString("nil");
                break;
            case luna::ValueT_Bool:
                api.PushString("boolean");
                break;
            case luna::ValueT_Number:
                api.PushString("number");
                break;
            case luna::ValueT_String:
                api.PushString("string");
                break;
            case luna::ValueT_Table:
                api.PushString("table");
                break;
            case luna::ValueT_UserData:
                api.PushString("userdata");
                break;
            case luna::ValueT_Closure:
            case luna::ValueT_CFunction:
                api.PushString("function");
                break;
            default:
                assert(0);
                return 0;
        }
        return 1;
    }

    int DoIPairs(luna::State *state)
    {
        luna::StackAPI api(state);
        if (!api.CheckArgs(2, luna::ValueT_Table, luna::ValueT_Number))
            return 0;

        luna::Table *t = api.GetTable(0);
        double num = api.GetNumber(1) + 1;

        luna::Value k;
        k.type_ = luna::ValueT_Number;
        k.num_ = num;
        luna::Value v = t->GetValue(k);

        if (v.type_ == luna::ValueT_Nil)
            return 0;

        api.PushValue(k);
        api.PushValue(v);
        return 2;
    }

    int IPairs(luna::State *state)
    {
        luna::StackAPI api(state);
        if (!api.CheckArgs(1, luna::ValueT_Table))
            return 0;

        luna::Table *t = api.GetTable(0);
        api.PushCFunction(DoIPairs);
        api.PushTable(t);
        api.PushNumber(0);
        return 3;
    }

    int DoPairs(luna::State *state)
    {
        luna::StackAPI api(state);
        if (!api.CheckArgs(2, luna::ValueT_Table))
            return 0;

        luna::Table *t = api.GetTable(0);
        luna::Value *last_key = api.GetValue(1);

        luna::Value key;
        luna::Value value;
        if (last_key->type_ == luna::ValueT_Nil)
            t->FirstKeyValue(key, value);
        else
            t->NextKeyValue(*last_key, key, value);

        api.PushValue(key);
        api.PushValue(value);
        return 2;
    }

    int Pairs(luna::State *state)
    {
        luna::StackAPI api(state);
        if (!api.CheckArgs(1, luna::ValueT_Table))
            return 0;

        luna::Table *t = api.GetTable(0);
        api.PushCFunction(DoPairs);
        api.PushTable(t);
        api.PushNil();
        return 3;
    }

    int GetLine(luna::State *state)
    {
        luna::StackAPI api(state);
        std::string line;
        std::getline(std::cin, line);
        api.PushString(line.c_str());
        return 1;
    }

    int Require(luna::State *state)
    {
        luna::StackAPI api(state);
        if (!api.CheckArgs(1, luna::ValueT_String))
            return 0;

        auto module = api.GetString(0)->GetStdString();
        if (!state->IsModuleLoaded(module))
            state->DoModule(module);

        return 0;
    }

    void RegisterLibBase(luna::State *state)
    {
        luna::Library lib(state);
        lib.RegisterFunc("print", Print);
        lib.RegisterFunc("puts", Puts);
        lib.RegisterFunc("ipairs", IPairs);
        lib.RegisterFunc("pairs", Pairs);
        lib.RegisterFunc("type", Type);
        lib.RegisterFunc("getline", GetLine);
        lib.RegisterFunc("require", Require);
    }

} // namespace base
} // namespace lib
