--//=============================================================================
--// GlassSkin

local skin = {
  info = {
    name    = "DarkGlass",
    version = "0.11",
    author  = "jK",

    depend = {
      "Glass",
    },
  }
}

--//=============================================================================
--//

skin.general = {
  textColor = {1,1,1,1},
}

skin.window = {
  TileImage = ":cl:glass_.png",
  tiles = {22, 24, 22, 23}, --// tile widths: left,top,right,bottom
  padding = {14, 23, 14, 14},
  hitpadding = {10, 4, 10, 10},

  captionColor = {1, 1, 1, 0.55},

  boxes = {
    resize = {-25, -25, -14, -14},
    drag = {0, 0, "100%", 24},
  },

  NCHitTest = NCHitTestWithPadding,
  NCMouseDown = WindowNCMouseDown,
  NCMouseDownPostChildren = WindowNCMouseDownPostChildren,

  DrawControl = DrawWindow,
  DrawDragGrip = DrawDragGrip,
  DrawResizeGrip = DrawResizeGrip,
}

--//=============================================================================
--//

return skin
