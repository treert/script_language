--[[
function tostring(a)
    if type(a) == "string" then
        return '"'..a..'"'
    end
    if type(a) == "number" then
        return a..""
    end
    
    return type(a)
end

function print_tab(deep)
    deep = deep or 0
    for i=1,deep,1 do
        puts("  ")
    end
end

function _dump(a,deep)
    if type(a) == "table" then
        print("{")
        for k,v in pairs(a) do
            print_tab(deep+1)
            puts("["..tostring(k).."] = ")
            _dump(v,deep+1)
            print(",")
        end
        print_tab(deep)
        puts("}")
        return
    end

    puts(tostring(a))
end

function dump(a)
    _dump(a,0)
    print("")
end

]]
---            common----



do
    local a = {}
    local b = {1,2}
    for _,i in pairs(b) do
    -- for i = 1,2 do
        a[i] = function() i = i+1; print(i); end
    end
    local b = 1
    a[1]()
    a[1]()
    a[2]()
end

do return end

local function test_repeat_local()
    local a = 1
    local a = 2
    print(a.."")
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