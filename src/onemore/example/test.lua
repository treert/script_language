


f1 = function (a,b)
    print(type(a).."x"..type(b))
end

f1({2})


local function test_repeat_local()
    local a = 1
    local a = 2
    print(a.."")
end

function print_tab(deep)
    deep = deep or 0
    for i=1,deep,1 do
        puts("  ")
    end
end

function tostring(a)
    if type(a) == "string" then
        return '"'..a..'"'
    end
    if type(a) == "number" then
        return a..""
    end
    
    return type(a)
end

function dump(a,deep)
    deep = deep or 0
    if type(a) == "table" then
        print("{")
        for k,v in pairs(a) do
            print_tab(deep+1)
            puts("["..tostring(k).."] = ")
            dump(v,deep+1)
            print(",")
        end
        print_tab(deep)
        puts("}")
        return
    end

    puts(tostring(a))
end

function get_table(...)

    return {1,2,...}
end


dump({
    1,2,3,get_table("a","b"),4,6
    })

    --[[
--dump(get_table("a","b"))

while true do
    puts("exit > ")
    local line = getline()

    if line == "exit" then
        break
    end
end


--]]