module("learn",package.seeall)

----------------- module --------------

-- 单行注释
--[[
    多行注释，-和[之前不能有空格
]]

----------------- 注释 ------------------

print("hello".." ".."world")
print(1 + 2)
print(true and false)
print(nil)
print(math.sqrt(2))

arr = {1,2,3,"go"}
print(table.concat( arr, " "))

text1 = "最普通的字符串"
text2 = '和"一样'
text3 = [[
长字符串定义，忽略任何转义，最前面的换行，会被忽略
]]
text4 = [===[
长字符串定义，忽略任何转义，加=号为了能在字符串里写 a[b[1]]，=数量不限
]===]
---------------------------- table·数组 --------------------
-- 初始化
t = {1,2} -- 索引从数字1开始，相当于下面的例子
t = {}; 
t[1] = 1;
t[2] = 2;

-- 插入删除
t = {1,2,3}
table.insert(t,4) -- 在结尾追加4，变成{1,2,3,4}
table.remove(t)   -- 移除结尾元素，变成{1,2,3}
table.remove(t,2) -- 移除第2个元素，变成{1,3}，会移动后边的元素

-- 获取长度
t = {1,2,3}
print(#t) -- 3

-- 遍历
t = {1,2,3}
for i = 1,#t do
    print(i .. " : " .. t[i]) -- 1:1 2:2 3:3
end
-- 这是第二种for循环，"for in"
for i,v in ipairs(t) do
    print(i .. " : " .. v) -- 和上面一样的功能
end

-- 排序
t = {1,3,2}
table.sort(t) -- 1,2,3
table.sort(t,function(a,b) return a > b end) -- 3,2,1


---------- table·hash表 ------------------
-- 初始化
t = {["x"] = 10, ["y"] = 20}
t = {x = 10, y = 20} -- 字符常量索引，和上面一样
-- 多维数组
t = {
    hp = 10,
    pos = {100,200},
}

-- 删除
t = {x = 1, y = 2}
t["x"] = nil -- 删除key "x"

-- 遍历
t = {x = 1, y = 2}
for k,v in pairs(t) do
    print(k .. ":".. v) -- x:1  y:2
end

---------- table 混合使用 -----------------
t = {
    1,
    ["x"] = 2,
    y = 3,
    [4] = 4,
}

-- #运算获取数组长度有坑
t = {1,2}
t[3] = 1 
print(#t) -- 数组是{1,2,3} 结果是 3
t[2] = nil 
print(#t) -- 数组是{1,nil,3} 结果是 1


------------------------- data ----------------------------

function Sum(x,y)
    local sum = 0
    for i = x,y do
        sum = sum + i
    end
    return sum
end

print(Sum(1,4))

local _Sum = Sum
print(_Sum(-4,-1))

--------------------------- function --------------------------

function FibonacciFactory()
    local a,b = 0,1
    return function ()
        a,b = b,a+b
        return a
    end
end

local fibonacci_generator = FibonacciFactory()
for i = 1,6 do
    print(fibonacci_generator())
end
-- 输出 1 1 2 3 5 8

function get_closure()
	-- 这个局部变量被下面的函数保存了
	local x = 1
	local add = function() x = x + 1 end
	local get = function() return x end
	-- add和get使用的是同一个x
	return get,add
end
local get,add = get_closure()
print(get()) -- 1
add()
print(get()) -- 2

arr = {}
for i = 1,3 do
    arr[i] = function ()
        i = i+1
        print(i)
    end
end

arr[1]()
arr[2]()
arr[3]()

-------------------------- closure ---------------------------
t = {}
print(t.x) -- nil
mt = {
    -- 当table不存在某个key值时，调用这个函数
    __index = function(_,key)
        return key .. " not find"
    end,
}
setmetatable(t,mt)
print(t.x) -- x not find

setmetatable(t,{
    -- 当t中不存在某个key值，尝试从这个表中获取
    __index = {x = "x save in metatable"}
})
print(t.x) -- x save in metatable

-------------------------- metatable -------------------------

function New(name)
    local obj = {}
    local _name = name or "default name"

    obj.PrintName = function ()
        print(_name)
    end
    obj.SetName = function (name)
        _name = name
    end
    return obj
end

local obj = New()
obj.PrintName() -- default name
obj.SetName("new name")
obj.PrintName() -- new name

--------------------------- table class --------------------------
-- 单继承框架
function class(class_name,base_class)
    local cls = {__name = class_name}
    cls.__index = cls

    function cls:new(...)
        local obj = setmetatable({}, cls)
        cls.ctor(obj,...)
        return obj
    end

    if base_class then
        cls.super = base_class
        setmetatable(cls,{__index = base_class})
    end
    return cls
end

-- 使用
BaseClass = class("base")
function BaseClass:ctor()
end
function BaseClass:Say()
    print(self.__name .. " say")
end

DerivedClass = class("derived",BaseClass)

local base = BaseClass.new()
local derived = DerivedClass.new()
base:Say() -- base say
derived:Say() -- derived say

-------------------------- metatable class ---------------------------

local work = coroutine.create(function (cmd)
    local cnt = 1
    while cmd ~= "stop" do
        local out = "#"..cnt.." cmd: "..tostring(cmd)
        cmd = coroutine.yield(out)
    end
    return "#"..cnt.." cmd: "..tostring(cmd)
end)

print(coroutine.resume(work,"work1")) -- true	#1 cmd: work1
print(coroutine.resume(work,"work2")) -- true	#1 cmd: work2
print(coroutine.resume(work,"work3")) -- true	#1 cmd: work3
print(coroutine.resume(work,"stop")) -- true	#1 cmd: stop


-- 生产者，消费者
-- yield：中断执行，resume恢复执行


producer = coroutine.create(function()
    for x = 1,2 do
        print("生产 --> 第" .. x .. "个产品")
        coroutine.yield(x)
    end
    print("生产者结束生产")
end)

function consumer()
    while true do
        local _,x = coroutine.resume(producer)
        if x then
            print("消费 <-- 第" .. x .. "个产品")
        else
            print("消费者离场")
            break
        end
    end
end

consumer()

------------------------- coroutine ----------------------------

print(-1)
_G["print"](-1)

local print = print
print(-1)


------------------ local ---------------------------

print("you" and "me") -- me
print("you" or "me")  -- you

--------------------- and or -------------------------
arr = {1,2,3,4,5,6,7,8,9,10}

arr = {}
for i = 1,10 do
    arr[i] = i
end
-------------------- table ctor --------------------

arr = {1,2,3,4,5}

str = ""
for _,val in ipairs(arr) do
    str = str.." "..val
end
print(str)

str = table.concat( arr, " ")
print(str)


-------------------- string ---------------------