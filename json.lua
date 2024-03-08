local cjson = require "cjson"
local obj

obj = cjson.decode("\"\\u0010\"")
assert(#obj == 1)
assert(obj:byte(1) == 16)

obj = cjson.decode("{\"a\":\"b\",   \"c\":\"d\"}")
assert(#obj == 2)
assert(obj.a == "b")
assert(obj.c == "d")

obj = cjson.decode("{\"a\":\"b\"}")
assert(#obj == 1)
assert(obj.a == "b")

obj = cjson.decode("[1,2,3,4]")
assert(#obj == 4)
assert(obj[2] == 2)
assert(obj[4] == 4)
assert(obj[10] == nil)

obj = cjson.decode("\"a\"")
assert(#obj == 1)
assert(type(obj) == "string")

