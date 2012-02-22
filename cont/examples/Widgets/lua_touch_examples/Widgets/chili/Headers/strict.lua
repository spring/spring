--
-- strict.lua
-- checks uses of undeclared global variables
-- All global variables must be 'declared' through a regular assignment
-- (even assigning nil will do) in a main chunk before being used
-- anywhere or assigned to inside a function.
--

-- backup table works like a normal __index table
local function MakeStrict(env, backup)
  local mt = getmetatable(env)
  if mt == nil then
    mt = {}
    setmetatable(env, mt)
  end
  
  mt.__index = function (t, n)
    if (backup[n]==nil) then
      error("variable '"..n.."' is not declared (strict mode)", 2)
    end
    return backup[n]
  end
end

return MakeStrict
