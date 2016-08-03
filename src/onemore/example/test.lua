
function f1()
    local x2
    local function f2()
        print(x2)
    end
    --f2()
    return f2
end

--f1()
f1()()

--[asdfasd[
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

---            common----


function ret(...)
    local a = 1
    local b = 2
    return a,...
end

function get_table(...)
    local a,b = ret(...)
    return {1,2,a,b}
end


dump({
    .3,0x70,3.3e-100,get_table("a","b"),4,6
    })

    dump(2+2*2*2 and 3)
    dump((2+2*2*2) and 3)

a  = {}
for i = 1,3 do
    a[i] = function() print(i);i = i+1; end  
end
a[1]()
a[1]()
a[1]()
a[2]()

b = {1,2,3,4}

b[1],b[2],b[3] =b[3],b[2],b[1]

dump(b)



--false and print(1)
--true and print(2)

while true do
    puts("exit > ")
    local line = getline()

    if line == "exit" then
        break
    end
end