#include "mUnitTest.h"
#include "../mTable.h"
#include "../mString.h"

TEST_CASE(table1)
{
    oms::Table t;

    for (int i = 0; i < 3; ++i)
    {
        oms::Value value;
        value.num_ = i + 1;
        value.type_ = oms::ValueT_Number;
        EXPECT_TRUE(t.SetArrayValue(i + 1, value));
    }

    oms::Value key;
    oms::Value value;
    EXPECT_TRUE(t.FirstKeyValue(key, value));
    EXPECT_TRUE(key.type_ == oms::ValueT_Number);
    EXPECT_TRUE(key.num_ == static_cast<double>(1));
    EXPECT_TRUE(value.type_ == oms::ValueT_Number);
    EXPECT_TRUE(value.num_ == static_cast<double>(1));

    for (int i = 1; i < 3; ++i)
    {
        oms::Value next_key;
        oms::Value next_value;
        EXPECT_TRUE(t.NextKeyValue(key, next_key, next_value));
        EXPECT_TRUE(next_key.type_ == oms::ValueT_Number);
        EXPECT_TRUE(next_key.num_ == static_cast<double>(i + 1));
        EXPECT_TRUE(next_value.type_ == oms::ValueT_Number);
        EXPECT_TRUE(next_value.num_ == static_cast<double>(i + 1));
        key = next_key;
    }

    EXPECT_TRUE(!t.NextKeyValue(key, key, value));

    value = t.GetValue(key);
    EXPECT_TRUE(value.type_ == oms::ValueT_Number);
    EXPECT_TRUE(value.num_ == static_cast<double>(3));
}

TEST_CASE(table2)
{
    oms::Table t;
    oms::String key_str("key");
    oms::String value_str("value");

    oms::Value key;
    oms::Value value;

    key.type_ = oms::ValueT_Obj;
    key.obj_ = &key_str;
    value.type_ = oms::ValueT_Obj;
    value.obj_ = &value_str;

    t.SetValue(key, value);
    value = t.GetValue(key);

    EXPECT_TRUE(value.type_ == oms::ValueT_Obj);
    EXPECT_TRUE(value.obj_ == &value_str);

    oms::Value key_not_existed;
    key_not_existed.type_ = oms::ValueT_Obj;
    key_not_existed.obj_ = &value_str;

    value = t.GetValue(key_not_existed);
    EXPECT_TRUE(value.type_ == oms::ValueT_Nil);

    EXPECT_TRUE(t.FirstKeyValue(key, value));
    EXPECT_TRUE(key.obj_ == &key_str);
    EXPECT_TRUE(value.obj_ == &value_str);

    EXPECT_TRUE(!t.NextKeyValue(key, key, value));
}

TEST_CASE(table3)
{
    oms::Table t;
    oms::Value key;
    oms::Value value;

    key.type_ = oms::ValueT_Bool;
    key.bvalue_ = true;

    value.type_ = oms::ValueT_Bool;
    value.bvalue_ = false;

    t.SetValue(key, value);
    value = t.GetValue(key);
    EXPECT_TRUE(value.type_ == oms::ValueT_Bool);
    EXPECT_TRUE(value.bvalue_ == false);

    t.SetValue(value, key);
    key = t.GetValue(value);
    EXPECT_TRUE(key.type_ == oms::ValueT_Bool);
    EXPECT_TRUE(key.bvalue_ == true);

    oms::Value nil;
    value = t.GetValue(nil);
    EXPECT_TRUE(value.type_ == oms::ValueT_Nil);
}

TEST_CASE(table4)
{
    oms::Table t;
    oms::Value key;
    oms::Value value;

    value.type_ = oms::ValueT_Number;

    for (int i = 0; i < 3; ++i)
    {
        value.num_ = i + 1;
        t.InsertArrayValue(i + 1, value);
    }

    value.num_ = 0;
    t.InsertArrayValue(1, value);

    EXPECT_TRUE(t.ArraySize() == 4);

    key.type_ = oms::ValueT_Number;
    for (int i = 0; i < 4; ++i)
    {
        key.num_ = i + 1;
        value = t.GetValue(key);
        EXPECT_TRUE(value.type_ == oms::ValueT_Number);
        EXPECT_TRUE(value.num_ == i);
    }
}

TEST_CASE(table5)
{
    oms::Table t;
    oms::Value key;
    oms::Value value;

    value.type_ = oms::ValueT_Number;

    for (int i = 0; i < 4; ++i)
    {
        value.num_ = i + 1;
        t.InsertArrayValue(i + 1, value);
    }

    EXPECT_TRUE(t.EraseArrayValue(1));
    EXPECT_TRUE(t.EraseArrayValue(1));

    EXPECT_TRUE(t.ArraySize() == 2);

    key.type_ = oms::ValueT_Number;

    key.num_ = 1;
    value = t.GetValue(key);
    EXPECT_TRUE(value.type_ == oms::ValueT_Number);
    EXPECT_TRUE(value.num_ == 3);

    key.num_ = 2;
    value = t.GetValue(key);
    EXPECT_TRUE(value.type_ == oms::ValueT_Number);
    EXPECT_TRUE(value.num_ == 4);
}
