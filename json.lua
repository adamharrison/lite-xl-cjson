local cjson = require "cjson"

local obj = cjson.decode("{\"a\":\"b\"}")
assert(#obj == 1)
assert(obj.a == "b")

local array = cjson.decode("[1,2,3,4]")
assert(#array == 4)
assert(array[2] == 2)
assert(array[4] == 4)
assert(array[10] == nil)

