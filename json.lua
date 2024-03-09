local cjson = require "cjson"
local json = require "oldjson"
local obj

obj = cjson.decode([[{"products":[{"handle":"3d-3d-25th-anniversary-edition","title":"3D 3D 25th Anniversary Edition.","metafields":[{"namespace":"Dianna","value":2,"key":"fancy","value_type":"integer"}]}]}]])
local str = io.open("/home/adam/Work/search-core/t/products250.json"):read("*all")
print(cjson.time())
obj = cjson.decode(str)
print(cjson.time())
-- obj = json.decode(str)
print(cjson.time())
print(#obj.products)


obj = cjson.decode("{\"a\":[],\"c\":\"d\"}")
assert(#obj == 2)
assert(obj.c == "d")

obj = cjson.decode("{\"a\":{\"d\":2},   \"c\":\"d\"}")
assert(#obj == 2)
assert(obj.c == "d")


obj = cjson.decode("{\"a\":2,   \"c\":\"d\"}")
assert(#obj == 2)
assert(obj.a == 2)
assert(obj.c == "d")

obj = cjson.decode("\"abcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjk\"")
assert(type(obj) == "string")
assert(obj == "abcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjkabcdefhjkdfhgdjlksghkjlhgksdfjlhgdjlkfhgdfjk")

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

