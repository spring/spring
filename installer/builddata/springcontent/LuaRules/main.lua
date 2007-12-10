

AllowUnsafeChanges("USE AT YOUR OWN PERIL")
--
--  Allows the use of the following call-outs when
--  not in the GameFrame() or GotChatMsg() call-ins:
--
--    CreateUnit()
--    DestroyUnit()
--    TransferUnit()
--    CreateFeature()
--    DestroyFeature()
--    TransferFeature()
--    GiveOrderToUnit()
--    GiveOrderToUnitMap()
--    GiveOrderToUnitArray()
--    GiveOrderArrayToUnitMap()
--    GiveOrderArrayToUnitArray()
--
--  *** The string argument must be an exact match ***


do  --  wrap print() in a closure
  local origPrint = print
  print = function(...)
    if (arg[1]) then
      arg[1] = Script.GetName() .. ': ' .. tostring(arg[1])
    end
    origPrint(unpack(arg))
  end
end


function tprint(t)
  for k,v in pairs(t) do
    Spring.Echo(k, tostring(v))
  end
end


if (not Spring.IsDevLuaEnabled()) then
  VFS.Include(Script.GetName() .. "/gadgets.lua", nil, VFS.ZIP_ONLY)
  Spring.Echo("LUARULES-MAIN  (GADGETS)")
else
  VFS.Include(Script.GetName() .. "/gadgets.lua", nil, VFS.RAW_ONLY)
  Spring.Echo("LUARULES-MAIN  (GADGETS)  --  DEVMODE")
end
