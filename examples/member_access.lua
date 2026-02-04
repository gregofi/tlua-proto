
function new_person(name: string, age: number) -> { name: string, age: number }
    return { name = name, age = age }
end

local person = new_person("Alice", 30)
print(person.name) -- Output: Alice
print(person.age)  -- Output: 30