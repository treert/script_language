
print "begin test MyClass"
--aaa = hello.MyClass:new_local()
aaa = hello.MyClass:new()
aaa:DoSomething()
print "end test MyClass"


--test.hello()

--hello.show()

while true do 
	io.write("dofile('test.lua'): Y/N")
	local line = io.read()
	if line == 'Y' then
		dofile('test.lua')
	else
		break
	end
end

io.write("end")