#ifndef VALUE_H
#define VALUE_H

#include "mgc.h"
#include <functional>

namespace oms
{
#define EXP_VALUE_COUNT_ANY -1

    class String;
    class Closure;
    class Upvalue;
    class Table;
    class UserData;
    class State;

    typedef int (*CFunctionType)(State *);

    enum ValueT
    {
        ValueT_Nil,
        ValueT_Bool,
        ValueT_Number,
        ValueT_Obj,
        ValueT_String,
        ValueT_Closure,
        ValueT_Table,
        ValueT_UserData,
        ValueT_CFunction,
    };

    // Value type of oms
    struct Value
    {
        union
        {
            GCObject *obj_;
            String *str_;
            Closure *closure_;
            Upvalue *upvalue_;
            Table *table_;
            UserData *user_data_;
            CFunctionType cfunc_;
            double num_;
            bool bvalue_;
        };

        ValueT type_;

        Value() : obj_(nullptr), type_(ValueT_Nil) { }
        explicit Value(bool bvalue) : bvalue_(bvalue), type_(ValueT_Bool) { }
        explicit Value(double num) : num_(num), type_(ValueT_Number) { }
        explicit Value(String *str) : str_(str), type_(ValueT_String) { }
        explicit Value(Closure *closure) : closure_(closure), type_(ValueT_Closure) { }
        explicit Value(Table *table) : table_(table), type_(ValueT_Table) { }
        explicit Value(UserData *user_data) : user_data_(user_data), type_(ValueT_UserData) { }
        explicit Value(CFunctionType cfunc) : cfunc_(cfunc), type_(ValueT_CFunction) { }

        void SetNil()
        { obj_ = nullptr; type_ = ValueT_Nil; }

        void SetBool(bool bvalue)
        { bvalue_ = bvalue; type_ = ValueT_Bool; }

        bool IsNil() const
        { return type_ == ValueT_Nil; }

        bool IsFalse() const
        { return type_ == ValueT_Nil || (type_ == ValueT_Bool && !bvalue_); }

        void Accept(GCObjectVisitor *v) const;
        const char * TypeName() const;

        static const char * TypeName(ValueT type);
    };

    inline bool operator == (const Value &left, const Value &right)
    {
        if (left.type_ != right.type_)
            return false;

        switch (left.type_)
        {
            case ValueT_Nil: return true;
            case ValueT_Bool: return left.bvalue_ == right.bvalue_;
            case ValueT_Number: return left.num_ == right.num_;
            case ValueT_Obj: return left.obj_ == right.obj_;
            case ValueT_String: return left.str_ == right.str_;
            case ValueT_Closure: return left.closure_ == right.closure_;
            case ValueT_Table: return left.table_ == right.table_;
            case ValueT_UserData: return left.user_data_ == right.user_data_;
            case ValueT_CFunction: return left.cfunc_ == right.cfunc_;
            default: return false;
        }
    }

    inline bool operator != (const Value &left, const Value &right)
    {
        return !(left == right);
    }
} // namespace oms

namespace std
{
    template<>
    struct hash<oms::Value>
    {
        size_t operator () (const oms::Value &t) const
        {
            switch (t.type_)
            {
                case oms::ValueT_Nil:
                    return hash<int>()(0);
                case oms::ValueT_Bool:
                    return hash<bool>()(t.bvalue_);
                case oms::ValueT_Number:
                    return hash<double>()(t.num_);
                case oms::ValueT_String:
                    return hash<void *>()(t.str_);
                case oms::ValueT_Closure:
                    return hash<void *>()(t.closure_);
                case oms::ValueT_Table:
                    return hash<void *>()(t.table_);
                case oms::ValueT_UserData:
                    return hash<void *>()(t.user_data_);
                case oms::ValueT_CFunction:
                    return hash<void *>()(reinterpret_cast<void *>(t.cfunc_));
                default:
                    return hash<void *>()(t.obj_);
            }
        }
    };
} // namespace std

#endif // VALUE_H
