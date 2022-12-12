-- SPDX-License-Identifier: GPL-2.0-or-later

function widget:GetInfo()
    return {
        name = "Font Demo",
        desc = "Test fonts",
        author = "CommanderSpice",
        date = "2022-11-26",
        license = "GPL-2.0+",
        layer = 1,
        enabled = true,
        handler = true,
    }
end

--------------------------------------------------------------------------------
-- Commands
--------------------------------------------------------------------------------
--[[
/fonts          Reload widget.
/fontscreen     Toggle drawing demos to screen.
/fontworld      Toggle drawing demos to world.
/fontminimap    Toggle drawing demos to minimap.
/fontdemo n     Select demo n. If a demo is selected, it is drawn at the mouse
                cursor. Selecting demo 0 removes demo selection.
/fontnext       Select next demo.
/fontprevious   Select previous demo.
/fontcount n    Set draw repetition counter. Positions are randomized when n>1.
                This can be used for profiling.
]]--

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
local vsx, vsy

local fontFile = "LuaUI/Fonts/FreeSansBold.otf"
local demoText = "The quick brown flea jumps over the lazy bulldog."
local world = false

local demosScreen = true
local demosWorld = true
local demosMinimap = true
local selectedDemo = 0
local count = 1

--------------------------------------------------------------------------------
-- Demos
--------------------------------------------------------------------------------
local demo = {}
local function addDemo(title, fontParams, func)
    demo[#demo+1] = {title=title, fontParams=fontParams, func=func}
end

--[[
Font parameters:
    gl.LoadFont(fontFile, size:14, outlinewidth:2, outlineweight:15.0f, relativeSize:false)

Font printing:
    font:Print(text, xpos, ypos, size, options)

Alignment options:
    c: FONT_CENTER
    r: FONT_RIGHT
    a: FONT_ASCENDER
    t: FONT_TOP
    v: FONT_VCENTER
    x: FONT_BASELINE
    b: FONT_BOTTOM
    d: FONT_DESCENDER

Appearance options:
    s: FONT_SHADOW
    o/O: FONT_OUTLINE

Rendering options:
    n: FONT_NEAREST
    B: FONT_BUFFERED

Relative options:
    N: FONT_NORM
    S: FONT_SCALE
]]--

addDemo("0.5x", {fontFile, 14, 2, 15.0}, function (font, text, px, py)
    if world then
        local h = Spring.GetGroundHeight(px, py)
        font:WorldBegin()
        font:WorldPrint(text, px, h, py, 0.5, "ctOS")
        font:WorldEnd()
    else
        font:Print(text, px, py, 0.5, "ctOS")
    end
    return 0.5 * font.size
end)

addDemo("1x", {fontFile, 14, 2, 15.0}, function (font, text, px, py)
    if world then
        local h = Spring.GetGroundHeight(px, py)
        font:WorldBegin()
        font:WorldPrint(text, px, h, py, 1, "ctOS")
        font:WorldEnd()
    else
        font:Print(text, px, py, 1, "ctOS")
    end
    return font.size
end)

addDemo("2x", {fontFile, 14, 2, 15.0}, function (font, text, px, py)
    if world then
        local h = Spring.GetGroundHeight(px, py)
        font:WorldBegin()
        font:WorldPrint(text, px, h, py, 2, "ctOS")
        font:WorldEnd()
    else
        font:Print(text, px, py, 2, "ctOS")
    end
    return 2 * font.size
end)

addDemo("3x", {fontFile, 14, 2, 15.0}, function (font, text, px, py)
    if world then
        local h = Spring.GetGroundHeight(px, py)
        font:WorldBegin()
        font:WorldPrint(text, px, h, py, 3, "ctOS")
        font:WorldEnd()
    else
        font:Print(text, px, py, 3, "ctOS")
    end
    return 3 * font.size
end)

addDemo("1x rel", {fontFile, 14, 2, 15.0, true}, function (font, text, px, py)
    font:SetOutlineColor(0, 0, 0, 1)
    font:SetTextColor(1, 0, 0, 1)
    if world then
        local h = Spring.GetGroundHeight(px, py)
        font:WorldBegin()
        font:WorldPrint("(" .. font.size .. ") " .. text, px, h, py, 1, "ctOS")
        font:WorldEnd()
    else
        font:Print("(" .. font.size .. ") " .. text, px, py, 1, "ctOS")
    end
    return font.size
end)

addDemo("2x rel", {fontFile, 14, 2, 15.0, true}, function (font, text, px, py)
    font:SetOutlineColor(0, 0, 0, 1)
    font:SetTextColor(1, 0, 0, 1)
    if world then
        local h = Spring.GetGroundHeight(px, py)
        font:WorldBegin()
        font:WorldPrint("(" .. font.size .. ") " .. text, px, h, py, 2, "ctOS")
        font:WorldEnd()
    else
        font:Print("(" .. font.size .. ") " .. text, px, py, 2, "ctOS")
    end
    return font.size
end)

addDemo("huge", {fontFile, 56, 2, 15.0}, function (font, text, px, py)
    font:SetOutlineColor(0, 0, 0, 1)
    font:SetTextColor(1, 0, 0, 1)
    if world then
        local h = Spring.GetGroundHeight(px, py)
        font:WorldBegin()
        font:WorldPrint("(" .. font.size .. ") " .. text, px, h, py, 1, "ctOS")
        font:WorldEnd()
    else
        font:Print("(" .. font.size .. ") " .. text, px, py, 1, "ctOS")
    end
    return font.size
end)

local function drawDemosScreen(gap)
    world = false

    local px, py = vsx / 2, vsy - gap

    for i = 1, #demo, 1 do
        if i == selectedDemo or selectedDemo == 0 then
            if count > 1 then
                px = math.random(0, vsx)
                py = math.random(0, vsy)

            elseif i == selectedDemo then
                local mouseX, mouseY = Spring.GetMouseState()
                px = mouseX
                py = mouseY
            end

            -- FIXME: Font rendering is slow.
            local height = demo[i].func(demo[i].font, demo[i].title .. ": " .. demoText, px, py)
            py = py - height - gap
        end
    end
end

local function drawDemosWorld(gap)
    world = true

    local px, pz = 100 + gap, 0 + gap

    for i = 1, #demo, 1 do
        if i == selectedDemo or selectedDemo == 0 then
            if count > 1 then
                px = math.random(0, 2000)
                pz = math.random(0, 2000)

            elseif i == selectedDemo then
                local mouseX, mouseY = Spring.GetMouseState()
                local _, pos = Spring.TraceScreenRay(mouseX, mouseY, true)
                if pos then
                    px = pos[1]
                    pz = pos[3]
                end
            end

            ---- FIXME: Font rendering in minimap uses world projection matrices.
            ---- FIXME: Font rendering is slow.
            demo[i].func(demo[i].font, demo[i].title .. ": " .. demoText, px, pz)
            pz = pz + gap
        end
    end
end

--------------------------------------------------------------------------------
-- Engine Calls
--------------------------------------------------------------------------------
function widget:Initialize()
    Spring.Echo("Font Demo - Initialize")
    widget:ViewResize(Spring.GetViewGeometry())
    for i = 1, #demo, 1 do
        demo[i].font = gl.LoadFont(unpack(demo[i].fontParams))
    end
end

function widget:Shutdown()
    Spring.Echo("Font Demo - Shutdown")
    for i = 1, #demo, 1 do
        gl.DeleteFont(demo.font)
    end
end

function widget:ViewResize(viewSizeX, viewSizeY)
    Spring.Echo("Resized to " .. viewSizeX .. "x" .. viewSizeY)
    vsx, vsy = viewSizeX, viewSizeY
end

function widget:DrawWorldPreUnit()
    if demosWorld then
        math.randomseed(0)
        for i = 1, count, 1 do
            drawDemosWorld(10)
        end
    end
end

function widget:DrawWorld()
end

function widget:DrawScreenEffects()
end

function widget:DrawScreen()
    if demosScreen then
        math.randomseed(0)
        for i = 1, count, 1 do
            drawDemosScreen(5)
        end
    end
end

function widget:DrawScreenPost()
end

function widget:DrawInMiniMapBackground(sx, sy)
end

function widget:DrawInMiniMap(sx, sy)
    if demosMinimap then
        math.randomseed(0)
        for i = 1, count, 1 do
            drawDemosWorld(10)
        end
    end
end

function widget:TextCommand(command)
    command = command:lower()

    if command == "fonts" then
        Spring.Echo("Reloading Font Demo")
        widgetHandler:DisableWidget("Font Demo")
        widgetHandler:EnableWidget("Font Demo")

    elseif command == "fontscreen" then
        demosScreen = not demosScreen
        Spring.Echo("screen = " .. tostring(demosScreen))

    elseif command == "fontworld" then
        demosWorld = not demosWorld
        Spring.Echo("world = " .. tostring(demosWorld))

    elseif command == "fontminimap" then
        demosMinimap = not demosMinimap
        Spring.Echo("minimap = " .. tostring(demosMinimap))

    elseif command == "fontdemo" then
        Spring.Echo("selectedDemo = " .. selectedDemo)

    elseif command == "fontnext" then
        selectedDemo = (selectedDemo + 1) % (#demo+1)
        if selectedDemo ~= 0 then
            Spring.Echo(demo[selectedDemo].title)
        end

    elseif command == "fontprevious" then
        selectedDemo = (selectedDemo - 1) % (#demo+1)
        if selectedDemo ~= 0 then
            Spring.Echo(demo[selectedDemo].title)
        end

    elseif command == "fontcount" then
        Spring.Echo("fontCount = " .. count)
    else
        local drawDemo = command:match("fontdemo (%d+)")
        if drawDemo ~= nil then
            selectedDemo = tonumber(drawDemo)
            if demo[selectedDemo] == nil then
                selectedDemo = 0
            end
            if selectedDemo ~= 0 then
                Spring.Echo(demo[selectedDemo].title)
            end
            return
        end

        local fontCount = command:match("fontcount (%d+)")
        if fontCount ~= nil then
            count = tonumber(fontCount)
            Spring.Echo("fontCount = " .. count)
            return
        end
    end
end
