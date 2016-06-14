
local function test_repeat_local()
    local a = 1
    local a = 2
    print(a.."")
end



while true do
    puts("exit > ")
    local line = getline()

    if line == "exit" then
        break
    end
    
    test_repeat_local()()
end
