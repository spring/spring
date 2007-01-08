--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    opengl.h.lua
--  brief:   OpenGL constants
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

-- glBegin() types
GL_POINTS         = 0
GL_LINES          = 1
GL_LINE_LOOP      = 2
GL_LINE_STRIP     = 3
GL_TRIANGLES      = 4
GL_TRIANGLE_STRIP = 5
GL_TRIANGLE_FAN   = 6
GL_QUADS          = 7
GL_QUAD_STRIP     = 8
GL_POLYGON        = 9

-- BlendingFactorDest
GL_ZERO                =   0
GL_ONE                 =   1
GL_SRC_COLOR           = 768 -- 0x0300
GL_ONE_MINUS_SRC_COLOR = 769 -- 0x0301
GL_SRC_ALPHA           = 770 -- 0x0302
GL_ONE_MINUS_SRC_ALPHA = 771 -- 0x0303
GL_DST_ALPHA           = 772 -- 0x0304
GL_ONE_MINUS_DST_ALPHA = 773 -- 0x0305

-- BlendingFactorSrc
-- GL_ZERO
-- GL_ONE
GL_DST_COLOR           = 774 -- 0x0306
GL_ONE_MINUS_DST_COLOR = 775 -- 0x0307
GL_SRC_ALPHA_SATURATE  = 776 -- 0x0308

-- AlphaFunction and  DepthFunction
GL_NEVER    = 512 -- 0x0200
GL_LESS     = 513 -- 0x0201
GL_EQUAL    = 514 -- 0x0202
GL_LEQUAL   = 515 -- 0x0203
GL_GREATER  = 516 -- 0x0204
GL_NOTEQUAL = 517 -- 0x0205
GL_GEQUAL   = 518 -- 0x0206
GL_ALWAYS   = 519 -- 0x0207


-- LogicOp
GL_CLEAR          = 5376 -- 0x1500
GL_AND            = 5377 -- 0x1501
GL_AND_REVERSE    = 5378 -- 0x1502
GL_COPY           = 5379 -- 0x1503
GL_AND_INVERTED   = 5380 -- 0x1504
GL_NOOP           = 5381 -- 0x1505
GL_XOR            = 5382 -- 0x1506
GL_OR             = 5383 -- 0x1507
GL_NOR            = 5384 -- 0x1508
GL_EQUIV          = 5385 -- 0x1509
GL_INVERT         = 5386 -- 0x150A
GL_OR_REVERSE     = 5387 -- 0x150B
GL_COPY_INVERTED  = 5388 -- 0x150C
GL_OR_INVERTED    = 5389 -- 0x150D
GL_NAND           = 5390 -- 0x150E
GL_SET            = 5391 -- 0x150F

-- Culling
GL_BACK           = 1029 -- 0x0405
GL_FRONT          = 1028 -- 0x0404
GL_FRONT_AND_BACK = 1032 -- 0x0408

-- PolygonMode
GL_POINT          = 6912 -- 0x1B00
GL_LINE           = 6913 -- 0x1B01
GL_FILL           = 6914 -- 0x1B02

-- Clear Bits
GL_DEPTH_BUFFER_BIT   =   256 -- 0x00000100
GL_ACCUM_BUFFER_BIT   =   512 -- 0x00000200
GL_STENCIL_BUFFER_BIT =  1024 -- 0x00000400
GL_COLOR_BUFFER_BIT   = 16384 -- 0x00004000

-- ShadeModel
GL_FLAT   = 7424 -- 0x1D00
GL_SMOOTH = 7425 -- 0x1D01

-- MatrixMode
GL_MODELVIEW  = 5888 -- 0x1700
GL_PROJECTION = 5889 -- 0x1701
GL_TEXTURE    = 5890 -- 0x1702
