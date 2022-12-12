-- SPDX-License-Identifier: GPL-2.0-or-later

function widget:GetInfo()
    return {
        name = "Draw Demo",
        desc = "Draw stuff using Spring.Draw",
        author = "CommanderSpice",
        date = "2022-09-12",
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
/draw           Reload widget.
/drawtexts      Toggle demo description texts.
/drawscreen     Toggle drawing demos to screen.
/drawworld      Toggle drawing demos to world.
/drawminimap    Toggle drawing demos to minimap.
/drawshaders    Toggle shader reloading on every frame. Note that this does not
                pick up changes of a file in VFS.
/drawdemo n     Select demo n. If a demo is selected, it is drawn at the mouse
                cursor. Selecting demo 0 removes demo selection.
/drawnext       Select next demo.
/drawprevious   Select previous demo.
/drawcount n    Set draw repetition counter. Sizes and positions are randomized
                when n>1. This can be used for profiling.
]]--

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
local vsx, vsy
local fontsize = 14
local font

local texture = "bitmaps/demotexture.png"
local world = false

local demosScreen = true
local demosWorld = true
local demosMinimap = true
local texts = false
local reloaShaders = false
local selectedDemo = 0
local count = 1

--------------------------------------------------------------------------------
-- Demos
--------------------------------------------------------------------------------
local demo = {}
local function addDemo(title, func)
    demo[#demo+1] = {title=title, func=func}
end

addDemo("Lines, width=1.0", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py,
            px, h, py+height,
            px+width, h, py,
            px+width, h, py+height,
        }
    else
        vertices = {
            px, py,
            px, py-height,
            px+width, py,
            px+width, py-height,
        }
    end

    Spring.Draw.Lines(
        vertices,
        {
            width=1.0,
            color={1,1,0,1},
        })
end)

addDemo("Lines, width=3.0, colors", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py,
            px, h, py+height/2+5,
            px+width, h, py+height/2-5,
            px+width, h, py+height,
            px+width-25, h, py+height,
        }
    else
        vertices = {
            px, py,
            px, py-height/2-5,
            px+width, py-height/2+5,
            px+width, py-height,
            px+width-25, py-height,
        }
    end

    Spring.Draw.Lines(
        vertices,
        {
            width=3.0,
            colors={
                {1,0,0,1},
                {0,1,0,1},
                {0,0,1,1},
                {1,1,1,1},
                {1,1,1,1},
            },
            loop=true,
        })
end)

addDemo("Lines, width=25.0, loop, textured, animated", function (px, py, width, height)
    local lwidth = 25.0
    width = width - lwidth
    height = height - lwidth

    local vertices
    if world then
        px = px + lwidth/2
        py = py + lwidth/2
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py,
            px, h, py+height,
            px+width, h, py,
            px+width, h, py+height,
        }
    else
        px = px + lwidth/2
        py = py - lwidth/2
        vertices = {
            px, py,
            px, py-height,
            px+width, py,
            px+width, py-height,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Lines(
        vertices,
        {
            width=lwidth,
            loop=true,
            animationspeed=1,
        })
    gl.Texture(false)
end)

addDemo("Lines, width=35.0, loop, textured, animated, texcoords", function (px, py, width, height)
    local lwidth = 35.0
    width = width - lwidth
    height = height - lwidth

    local vertices
    if world then
        px = px + lwidth/2
        py = py + lwidth/2
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py,
            px+width/2, h, py+height/3,
            px+width, h, py,
        }
    else
        px = px + lwidth/2
        py = py - lwidth/2
        vertices = {
            px, py,
            px+width/2, py-height/3,
            px+width, py,
        }
    end

    gl.Texture("bitmaps/arrow.png")
    Spring.Draw.Lines(
        vertices,
        {
            width=lwidth,
            loop=true,
            texcoords={
                0, 2,
                -1, 0,
            },
            animationspeed=1,
        })
    gl.Texture(false)
end)

addDemo("Triangle", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width/2, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py-height,
            px+width/2, py,
        }
    end

    Spring.Draw.Triangle(
        vertices,
        {
            color={0,0,0,0.5},
        })
end)

addDemo("Triangle, colors", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width/2, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py-height,
            px+width/2, py,
        }
    end

    Spring.Draw.Triangle(
        vertices,
        {
            colors={
                {1,0,0,1},
                {0,1,0,1},
                {0,0,1,1},
            },
        })
end)

addDemo("Triangle, textured", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width/2, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py-height,
            px+width/2, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Triangle(
        vertices,
        {
            texcoords={
                0, 1,
                1, 1,
                0.5, 0,
            },
        })
    gl.Texture(false)
end)

addDemo("Rectangle", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width, h, py,
            px, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    Spring.Draw.Rectangle(
        vertices,
        {
            color={0,0,0,0.5},
        })
end)

addDemo("Rectangle, colors", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width, h, py,
            px, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    Spring.Draw.Rectangle(
        vertices,
        {
            colors={
                {1,1,0,1},
                {1,0,0,1},
                {0,1,1,1},
                {0,0,1,1},
            },
        })
end)

addDemo("Rectangle, textured", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width, h, py,
            px, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(vertices)
    gl.Texture(false)
end)

addDemo("Rectangle, texcoords=\"xflip\"", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width, h, py,
            px, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            texcoords="xflip",
        })
    gl.Texture(false)
end)

addDemo("Rectangle, texcoords=\"yflip\"", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width, h, py,
            px, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            texcoords="yflip",
        })
    gl.Texture(false)
end)

addDemo("Rectangle, texcoords=\"duflip\"", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width, h, py,
            px, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            texcoords="duflip",
        })
    gl.Texture(false)
end)

addDemo("Rectangle, texcoords=\"ddflip\"", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width, h, py,
            px, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            texcoords="ddflip",
        })
    gl.Texture(false)
end)

addDemo("Rectangle, texcoords=90", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width, h, py,
            px, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            texcoords=90,
        })
    gl.Texture(false)
end)

addDemo("Rectangle, texcoords=180", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width, h, py,
            px, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            texcoords="180",
        })
    gl.Texture(false)
end)

addDemo("Rectangle, texcoords=270", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width, h, py,
            px, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            texcoords=270,
        })
    gl.Texture(false)
end)

addDemo("Rectangle, color, textured", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width, h, py,
            px, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            color={0.5,1,0.5,1},
        })
    gl.Texture(false)
end)

addDemo("Rectangle, colors, textured", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width, h, py,
            px, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            colors={
                {1,1,0,1},
                {1,0,0,1},
                {0,1,1,1},
                {0,0,1,1},
            },
        })
    gl.Texture(false)
end)

addDemo("Rectangle, color, textured, transparent", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width, h, py,
            px, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            color={0.5,1,1,0.2},
        })
    gl.Texture(false)
end)

addDemo("Rectangle, texcoords", function (px, py, width, height)
    local vertices
    if world then
        local h = Spring.GetGroundHeight(px+width/2, py+height/2)
        vertices = {
            px, h, py+height,
            px+width, h, py+height,
            px+width, h, py,
            px, h, py,
        }
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            texcoords={
                0, 0,
                2, 0,
                2, 2,
                0, 2,
            },
        })
    gl.Texture(false)
end)

addDemo("Rectangle, border=1, bordercolor", function (px, py, width, height)
    local vertices
    if world then
        return
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    Spring.Draw.Rectangle(
        vertices,
        {
            color={0,0,0,0.5},
            border=1.0,
            bordercolor={1,1,0,1},
        })
end)

addDemo("Rectangle, textured, border=5, bordercolors", function (px, py, width, height)
    local vertices
    if world then
        return
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            border=5,
            bordercolors={
                {1,0,0,1},
                {1,1,0,1},
                {0,1,1,1},
                {0,0,1,1},
            },
        })
    gl.Texture(false)
end)

addDemo("Rectangle, bevel=5", function (px, py, width, height)
    local vertices
    if world then
        return
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    Spring.Draw.Rectangle(
        vertices,
        {
            color={0,0,0,0.5},
            bevel=5,
        })
end)

addDemo("Rectangle, bevel=15, colors", function (px, py, width, height)
    local vertices
    if world then
        return
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    Spring.Draw.Rectangle(
        vertices,
        {
            colors={
                {1,1,0,1},
                {1,0,0,1},
                {0,1,1,1},
                {0,0,1,1},
            },
            bevel=15,
        })
end)

addDemo("Rectangle, bevel=15, border=1, colors", function (px, py, width, height)
    local vertices
    if world then
        return
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    Spring.Draw.Rectangle(
        vertices,
        {
            colors={
                {1,1,0,1},
                {1,0,0,1},
                {0,1,1,1},
                {0,0,1,1},
            },
            bevel=15,
            border=1,
        })
end)

addDemo("Rectangle, bevel=15, textured", function (px, py, width, height)
    local vertices
    if world then
        return
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            bevel=15,
        })
    gl.Texture(false)
end)

addDemo("Rectangle, bevel=15, border=1, bordercolor, textured", function (px, py, width, height)
    local vertices
    if world then
        return
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            bevel=15,
            border=1,
            bordercolor={0,1,0,1},
        })
    gl.Texture(false)
end)

addDemo("Rectangle, bevel=15, border=5, border=1, textured", function (px, py, width, height)
    local vertices
    if world then
        return
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            bevel=height/3.5,
            border=5,
            bordercolor={0,1,0,1},
            bordercolors={
                {1,0,0,1},
                {1,0,0,1},
                {1,1,0,1},
                {1,1,0,1},
            },
        })
    gl.Texture(false)
end)

addDemo("Rectangle, radius=16, border, bordercolor", function (px, py, width, height)
    local vertices
    if world then
        return
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            color={1,1,1,1},
            radius=16,
            border=1,
            bordercolor={0,1,0,1},
        })
    gl.Texture(false)
end)

addDemo("Rectangle, radius=32, border, bordercolors", function (px, py, width, height)
    local vertices
    if world then
        return
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    gl.Texture(texture)
    Spring.Draw.Rectangle(
        vertices,
        {
            radius=32,
            border=5,
            bordercolors={
                {1,0,0,1},
                {1,1,0,1},
                {0,1,0,1},
                {0,1,1,1},
            },
        })
    gl.Texture(false)
end)

addDemo("Rectangle, radius=32, border, bordercolor", function (px, py, width, height)
    local vertices
    if world then
        return
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    Spring.Draw.Rectangle(
        vertices,
        {
            color={0,0,0,0.5},
            radius=32,
            border=1,
            bordercolor={0,1,0,1},
        })
end)

addDemo("Rectangle, radius=32", function (px, py, width, height)
    local vertices
    if world then
        return
    else
        vertices = {
            px, py-height,
            px+width, py,
        }
    end

    Spring.Draw.Rectangle(
        vertices,
        {
            color={0,0,0,0.5},
            radius=32,
        })
end)

addDemo("Rectangle, fractional coords, radius=8", function (px, py, width, height)
    local vertices
    if world then
        return
    else
        vertices = {
            px+0.5, py-height+0.5,
            px+width-0.5, py-0.5,
        }
    end

    Spring.Draw.Rectangle(
        vertices,
        {
            color={0,0,0,0.8},
            radius=8,
        })
end)

local function drawDemosScreen(size, gap)
    world = false

    gl.Texture(false)

    local px, py = 0 + gap, vsy - gap
    local firstDemo = true

    for i = 1, #demo, 1 do
        if px + size + gap > vsx then
            if firstDemo then
                -- Failed to fit the first demo.
                return
            end
            px = gap
            py = py - size - gap
        end

        if i == selectedDemo or selectedDemo == 0 then
--            Spring.Echo("Draw Demo - Drawing demo #" .. i .. ": " .. demo[i].title)

            if count > 1 then
                size = math.random(50, 200)
                px = math.random(0, vsx - size)
                py = math.random(size, vsy)

            elseif i == selectedDemo then
                local mouseX, mouseY = Spring.GetMouseState()
                px = mouseX - size/2
                py = mouseY + size/2
            end

            demo[i].func(px, py, size, size)

            if texts then
                -- FIXME: Font rendering is slow.
                font:Print(font:WrapText(demo[i].title, size), px+size/2, py-size+7, fontsize, "cbo")
            end

            px = px + size + gap
        end

        firstDemo = false
    end
end

local function drawDemosWorld(size, gap)
    world = true

    gl.Texture(false)

    local width = 1000

    local px, pz = 0 + gap, 0 + gap
    local firstDemo = true

    for i = 1, #demo, 1 do
        if px + size + gap > width then
            if firstDemo then
                -- Failed to fit the first demo.
                return
            end
            px = gap
            pz = pz + size + gap
        end

        if i == selectedDemo or selectedDemo == 0 then
--            Spring.Echo("Draw Demo - Drawing demo #" .. i .. ": " .. demo[i].title)

            if count > 1 then
                px = math.random(0, 2000)
                pz = math.random(0, 2000)
                size = math.random(50, 200)

            elseif i == selectedDemo then
                local mouseX, mouseY = Spring.GetMouseState()
                local _, pos = Spring.TraceScreenRay(mouseX, mouseY, true)
                if pos then
                    px = pos[1] - size/2
                    pz = pos[3] - size/2
                end
            end

            demo[i].func(px, pz, size, size)

            if texts then
                local h = Spring.GetGroundHeight(px+size/2, pz+size/2)
                -- FIXME: This does not work correctly when drawing in minimap.
                font:WorldBegin()
                -- FIXME: Font rendering is slow.
                font:WorldPrint(font:WrapText(demo[i].title, size), px+size/2, h, pz+size-7, fontsize, "cbo")
                font:WorldEnd()
            end

            px = px + size + gap
        end

        firstDemo = false
    end
end

--------------------------------------------------------------------------------
-- Engine Calls
--------------------------------------------------------------------------------
function widget:Initialize()
    Spring.Echo("Draw Demo - Initialize")
    widget:ViewResize(Spring.GetViewGeometry())
    font = gl.LoadFont("LuaUI/Fonts/FreeSansBold.otf", fontsize, 4, 2.0)
    font:SetAutoOutlineColor(true)
end

function widget:Shutdown()
    Spring.Echo("Draw Demo - Shutdown")
    if font then
        gl.DeleteFont(font)
    end
end

function widget:ViewResize(viewSizeX, viewSizeY)
    Spring.Echo("Resized to " .. viewSizeX .. "x" .. viewSizeY)
    vsx, vsy = viewSizeX, viewSizeY
end

function widget:Update()
    if reloaShaders then
        Spring.Draw.ReloadShaders()
    end
end

function widget:DrawWorldPreUnit()
    if demosWorld then
        math.randomseed(0)
        for i = 1, count, 1 do
            drawDemosWorld(120, 10)
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
            drawDemosScreen(120, 10)
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
            drawDemosWorld(120, 10)
        end
    end
end

function widget:TextCommand(command)
    command = command:lower()

    if command == "draw" then
        Spring.Echo("Reloading Draw Demo")
        widgetHandler:DisableWidget("Draw Demo")
        Spring.Draw.ReloadShaders()
        widgetHandler:EnableWidget("Draw Demo")
    elseif command == "drawtexts" then
        texts = not texts
        Spring.Echo("texts = " .. tostring(texts))

    elseif command == "drawscreen" then
        demosScreen = not demosScreen
        Spring.Echo("screen = " .. tostring(demosScreen))

    elseif command == "drawworld" then
        demosWorld = not demosWorld
        Spring.Echo("world = " .. tostring(demosWorld))

    elseif command == "drawminimap" then
        demosMinimap = not demosMinimap
        Spring.Echo("minimap = " .. tostring(demosMinimap))

    elseif command == "drawshaders" then
        reloaShaders = not reloaShaders
        Spring.Echo("shaders = " .. tostring(reloaShaders))

    elseif command == "drawdemo" then
        Spring.Echo("selectedDemo = " .. selectedDemo)

    elseif command == "drawnext" then
        selectedDemo = (selectedDemo + 1) % (#demo+1)
        if selectedDemo ~= 0 then
            Spring.Echo(demo[selectedDemo].title)
        end

    elseif command == "drawprevious" then
        selectedDemo = (selectedDemo - 1) % (#demo+1)
        if selectedDemo ~= 0 then
            Spring.Echo(demo[selectedDemo].title)
        end

    elseif command == "drawcount" then
        Spring.Echo("drawCount = " .. count)
    else
        local drawDemo = command:match("drawdemo (%d+)")
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

        local drawCount = command:match("drawcount (%d+)")
        if drawCount ~= nil then
            count = tonumber(drawCount)
            Spring.Echo("drawCount = " .. count)
            return
        end
    end
end
