#include "mlib_base.h"
#include "mtable.h"
#include "mupvalue.h"
#include "mstate.h"
#include "mstring.h"
#include <string>
#include <iostream>
#include <assert.h>
#include <stdio.h>

namespace lib {
namespace base {

    int Print(oms::State *state)
    {
        oms::StackAPI api(state);
        int params = api.GetStackSize();

        for (int i = 0; i < params; ++i)
        {
            oms::ValueT type = api.GetValueType(i);

            switch (type) {
                case oms::ValueT_Nil:
                    printf("nil");
                    break;
                case oms::ValueT_Bool:
                    printf("%s", api.GetBool(i) ? "true" : "false");
                    break;
                case oms::ValueT_Number:
                    printf("%.14g", api.GetNumber(i));
                    break;
                case oms::ValueT_String:
                    printf("%s", api.GetCString(i));
                    break;
                case oms::ValueT_Closure:
                    printf("function:\t%p", api.GetClosure(i));
                    break;
                case oms::ValueT_Table:
                    printf("table:\t%p", api.GetTable(i));
                    break;
                case oms::ValueT_UserData:
                    printf("userdata:\t%p", api.GetUserData(i));
                    break;
                case oms::ValueT_CFunction:
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

    int Puts(oms::State *state)
    {
        oms::StackAPI api(state);
        if (!api.CheckArgs(1, oms::ValueT_String))
            return 0;

        printf("%s", api.GetCString(0));
        return 0;
    }

    int Type(oms::State *state)
    {
        oms::StackAPI api(state);
        if (!api.CheckArgs(1))
            return 0;

        const oms::Value *v = api.GetValue(0);

        switch (v->type_) {
            case oms::ValueT_Nil:
                api.PushString("nil");
                break;
            case oms::ValueT_Bool:
                api.PushString("boolean");
                break;
            case oms::ValueT_Number:
                api.PushString("number");
                break;
            case oms::ValueT_String:
                api.PushString("string");
                break;
            case oms::ValueT_Table:
                api.PushString("table");
                break;
            case oms::ValueT_UserData:
                api.PushString("userdata");
                break;
            case oms::ValueT_Closure:
            case oms::ValueT_CFunction:
                api.PushString("function");
                break;
            default:
                assert(0);
                return 0;
        }
        return 1;
    }

    int DoIPairs(oms::State *state)
    {
        oms::StackAPI api(state);
        if (!api.CheckArgs(2, oms::ValueT_Table, oms::ValueT_Number))
            return 0;

        oms::Table *t = api.GetTable(0);
        double num = api.GetNumber(1) + 1;

        oms::Value k;
        k.type_ = oms::ValueT_Number;
        k.num_ = num;
        oms::Value v = t->GetValue(k);

        if (v.type_ == oms::ValueT_Nil)
            return 0;

        api.PushValue(k);
        api.PushValue(v);
        return 2;
    }

    int IPairs(oms::State *state)
    {
        oms::StackAPI api(state);
        if (!api.CheckArgs(1, oms::ValueT_Table))
            return 0;

        oms::Table *t = api.GetTable(0);
        api.PushCFunction(DoIPairs);
        api.PushTable(t);
        api.PushNumber(0);
        return 3;
    }

    int DoPairs(oms::State *state)
    {
        oms::StackAPI api(state);
        if (!api.CheckArgs(2, oms::ValueT_Table))
            return 0;

        oms::Table *t = api.GetTable(0);
        oms::Value *last_key = api.GetValue(1);

        oms::Value key;
        oms::Value value;
        if (last_key->type_ == oms::ValueT_Nil)
            t->FirstKeyValue(key, value);
        else
            t->NextKeyValue(*last_key, key, value);

        api.PushValue(key);
        api.PushValue(value);
        return 2;
    }

    int Pairs(oms::State *state)
    {
        oms::StackAPI api(state);
        if (!api.CheckArgs(1, oms::ValueT_Table))
            return 0;

        oms::Table *t = api.GetTable(0);
        api.PushCFunction(DoPairs);
        api.PushTable(t);
        api.PushNil();
        return 3;
    }

    int GetLine(oms::State *state)
    {
        oms::StackAPI api(state);
        std::string line;
        std::getline(std::cin, line);
        api.PushString(line.c_str());
        return 1;
    }

    int Require(oms::State *state)
    {
        oms::StackAPI api(state);
        if (!api.CheckArgs(1, oms::ValueT_String))
            return 0;

        auto module = api.GetString(0)->GetStdString();
        if (!state->IsModuleLoaded(module))
            state->DoModule(module);

        return 0;
    }

    void RegisterLibBase(oms::State *state)
    {
        oms::Library lib(state);
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
