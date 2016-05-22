#include "mLibTable.h"
#include "mState.h"
#include "mTable.h"
#include <sstream>

namespace lib {
namespace table {

    // If the index value is number, then get the number value,
    // else report type error.
    template<typename NumType>
    bool GetNumber(oms::StackAPI &api, int index, NumType &num)
    {
        if (!api.IsNumber(index))
        {
            api.ArgTypeError(index, oms::ValueT_Number);
            return false;
        }

        num = static_cast<NumType>(api.GetNumber(index));
        return true;
    }

    int Concat(oms::State *state)
    {
        oms::StackAPI api(state);
        if (!api.CheckArgs(1, oms::ValueT_Table))
            return 0;

        auto table = api.GetTable(0);
        const char *sep = "";
        std::size_t i = 1;
        std::size_t j = table->ArraySize();

        auto params = api.GetStackSize();
        if (params > 1)
        {
            // If the value of index 1 is string, then get the string as sep
            if (api.IsString(1))
            {
                sep = api.GetString(1)->GetCStr();

                // Try to get the range of table
                if (params > 2 && !GetNumber(api, 2, i))
                    return 0;

                if (params > 3 && !GetNumber(api, 3, j))
                    return 0;
            }
            else
            {
                // Try to get the range of table
                if (!GetNumber(api, 1, i))
                    return 0;

                if (params > 2 && !GetNumber(api, 2, j))
                    return 0;
            }
        }

        oms::Value key;
        key.type_ = oms::ValueT_Number;

        // Concat values(number or string) of the range [i, j]
        std::ostringstream oss;
        for (; i <= j; ++i)
        {
            key.num_ = i;
            auto value = table->GetValue(key);

            if (value.type_ == oms::ValueT_Number)
                oss << value.num_;
            else if (value.type_ == oms::ValueT_String)
                oss << value.str_->GetCStr();

            if (i != j) oss << sep;
        }

        api.PushString(oss.str());
        return 1;
    }

    int Insert(oms::State *state)
    {
        oms::StackAPI api(state);
        if (!api.CheckArgs(2, oms::ValueT_Table))
            return 0;

        auto params = api.GetStackSize();
        auto table = api.GetTable(0);
        auto index = table->ArraySize() + 1;
        int value = 1;

        if (params > 2)
        {
            if (!GetNumber(api, 1, index))
                return 0;

            value = 2;
        }

        api.PushBool(table->InsertArrayValue(index, *api.GetValue(value)));
        return 1;
    }

    int Pack(oms::State *state)
    {
        oms::StackAPI api(state);

        auto table = state->NewTable();
        auto params = api.GetStackSize();
        for (int i = 0; i < params; ++i)
            table->SetArrayValue(i + 1, *api.GetValue(i));

        api.PushTable(table);
        return 1;
    }

    int Remove(oms::State *state)
    {
        oms::StackAPI api(state);
        if (!api.CheckArgs(1, oms::ValueT_Table, oms::ValueT_Number))
            return 0;

        auto table = api.GetTable(0);
        auto index = table->ArraySize();

        // There is no elements in array of table.
        if (index == 0)
        {
            api.PushBool(false);
            return 1;
        }

        auto params = api.GetStackSize();
        if (params > 1)
            index = static_cast<decltype(index)>(api.GetNumber(1));

        api.PushBool(table->EraseArrayValue(index));
        return 1;
    }

    int Unpack(oms::State *state)
    {
        oms::StackAPI api(state);
        if (!api.CheckArgs(1, oms::ValueT_Table,
                           oms::ValueT_Number, oms::ValueT_Number))
            return 0;

        auto params = api.GetStackSize();
        auto table = api.GetTable(0);

        int begin = 1;
        int end = table->ArraySize();
        if (params > 1)
            begin = static_cast<int>(api.GetNumber(1));
        if (params > 2)
            end = static_cast<int>(api.GetNumber(2));

        int count = 0;
        oms::Value key;
        key.type_ = oms::ValueT_Number;
        for (int i = begin; i <= end; ++i)
        {
            key.num_ = i;
            api.PushValue(table->GetValue(key));
            ++count;
        }

        return count;
    }

    void RegisterLibTable(oms::State *state)
    {
        oms::Library lib(state);
        oms::TableMemberReg table[] = {
            { "concat", Concat },
            { "insert", Insert },
            { "pack", Pack },
            { "remove", Remove },
            { "unpack", Unpack }
        };

        lib.RegisterTableFunction("table", table);
    }

} // namespace table
} // namespace lib
