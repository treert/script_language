

---[asdfasd[
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





function get_table(...)

    return {1,2,...}
end


dump({
    .3,0x70,3.3e-100,get_table("a","b")[3],4,6
    })

    dump(2+2*2*2 and 3)
    dump((2+2*2*2) and 3)


--dump(get_table("a","b"))

while true do
    puts("exit > ")
    local line = getline()

    if line == "exit" then
        break
    end
end