--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    keysym.h.lua
--  brief:   key symbol constants
--  author:  Fireball
--
--  Copyright (C) 2022.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- From SDL_scancode.h
SCANSYMS = {
    UNKNOWN = 0,
    --[[
     *  \name Usage page 0x07
     *
     *  These values are from usage page 0x07 (USB keyboard page).
     --]]
    SC_A = 4,
    SC_B = 5,
    SC_C = 6,
    SC_D = 7,
    SC_E = 8,
    SC_F = 9,
    SC_G = 10,
    SC_H = 11,
    SC_I = 12,
    SC_J = 13,
    SC_K = 14,
    SC_L = 15,
    SC_M = 16,
    SC_N = 17,
    SC_O = 18,
    SC_P = 19,
    SC_Q = 20,
    SC_R = 21,
    SC_S = 22,
    SC_T = 23,
    SC_U = 24,
    SC_V = 25,
    SC_W = 26,
    SC_X = 27,
    SC_Y = 28,
    SC_Z = 29,
    SC_1 = 30,
    SC_2 = 31,
    SC_3 = 32,
    SC_4 = 33,
    SC_5 = 34,
    SC_6 = 35,
    SC_7 = 36,
    SC_8 = 37,
    SC_9 = 38,
    SC_0 = 39,
    SC_RETURN = 40,
    SC_ESCAPE = 41,
    SC_BACKSPACE = 42,
    SC_TAB = 43,
    SC_SPACE = 44,
    SC_MINUS = 45,
    SC_EQUALS = 46,
    SC_LEFTBRACKET = 47,
    SC_RIGHTBRACKET = 48,
    SC_BACKSLASH = 49, --[[< Located at the lower left of the return
                                  *   key on ISO keyboards and at the right end
                                  *   of the QWERTY row on ANSI keyboards.
                                  *   Produces REVERSE SOLIDUS (backslash) and
                                  *   VERTICAL LINE in a US layout, REVERSE
                                  *   SOLIDUS and VERTICAL LINE in a UK Mac
                                  *   layout, NUMBER SIGN and TILDE in a UK
                                  *   Windows layout, DOLLAR SIGN and POUND SIGN
                                  *   in a Swiss German layout, NUMBER SIGN and
                                  *   APOSTROPHE in a German layout, GRAVE
                                  *   ACCENT and POUND SIGN in a French Mac
                                  *   layout, and ASTERISK and MICRO SIGN in a
                                  *   French Windows layout.
                                  --]]
    SC_NONUSHASH = 50, --[[< ISO USB keyboards actually use this code
                                  *   instead of 49 for the same key, but all
                                  *   OSes I've seen treat the two codes
                                  *   identically. So, as an implementor, unless
                                  *   your keyboard generates both of those
                                  *   codes and your OS treats them differently,
                                  *   you should generate BACKSLASH
                                  *   instead of this code. As a user, you
                                  *   should not rely on this code because SDL
                                  *   will never generate it with most (all?)
                                  *   keyboards.
                                  --]]
    SC_SEMICOLON = 51,
    SC_APOSTROPHE = 52,
    SC_GRAVE = 53, --[[< Located in the top left corner (on both ANSI
                              *   and ISO keyboards). Produces GRAVE ACCENT and
                              *   TILDE in a US Windows layout and in US and UK
                              *   Mac layouts on ANSI keyboards, GRAVE ACCENT
                              *   and NOT SIGN in a UK Windows layout, SECTION
                              *   SIGN and PLUS-MINUS SIGN in US and UK Mac
                              *   layouts on ISO keyboards, SECTION SIGN and
                              *   DEGREE SIGN in a Swiss German layout (Mac:
                              *   only on ISO keyboards), CIRCUMFLEX ACCENT and
                              *   DEGREE SIGN in a German layout (Mac: only on
                              *   ISO keyboards), SUPERSCRIPT TWO and TILDE in a
                              *   French Windows layout, COMMERCIAL AT and
                              *   NUMBER SIGN in a French Mac layout on ISO
                              *   keyboards, and LESS-THAN SIGN and GREATER-THAN
                              *   SIGN in a Swiss German, German, or French Mac
                              *   layout on ANSI keyboards.
                              --]]
    SC_COMMA = 54,
    SC_PERIOD = 55,
    SC_SLASH = 56,
    SC_CAPSLOCK = 57,
    SC_F1 = 58,
    SC_F2 = 59,
    SC_F3 = 60,
    SC_F4 = 61,
    SC_F5 = 62,
    SC_F6 = 63,
    SC_F7 = 64,
    SC_F8 = 65,
    SC_F9 = 66,
    SC_F10 = 67,
    SC_F11 = 68,
    SC_F12 = 69,
    SC_PRINTSCREEN = 70,
    SC_SCROLLLOCK = 71,
    SC_PAUSE = 72,
    SC_INSERT = 73, --[[< insert on PC, help on some Mac keyboards (but
                                   does send code 73, not 117) --]]
    SC_HOME = 74,
    SC_PAGEUP = 75,
    SC_DELETE = 76,
    SC_END = 77,
    SC_PAGEDOWN = 78,
    SC_RIGHT = 79,
    SC_LEFT = 80,
    SC_DOWN = 81,
    SC_UP = 82,
    SC_NUMLOCKCLEAR = 83, --[[< num lock on PC, clear on Mac keyboards
                                     --]]
    SC_KP_DIVIDE = 84,
    SC_KP_MULTIPLY = 85,
    SC_KP_MINUS = 86,
    SC_KP_PLUS = 87,
    SC_KP_ENTER = 88,
    SC_KP_1 = 89,
    SC_KP_2 = 90,
    SC_KP_3 = 91,
    SC_KP_4 = 92,
    SC_KP_5 = 93,
    SC_KP_6 = 94,
    SC_KP_7 = 95,
    SC_KP_8 = 96,
    SC_KP_9 = 97,
    SC_KP_0 = 98,
    SC_KP_PERIOD = 99,
    SC_NONUSBACKSLASH = 100, --[[< This is the additional key that ISO
                                        *   keyboards have over ANSI ones,
                                        *   located between left shift and Y.
                                        *   Produces GRAVE ACCENT and TILDE in a
                                        *   US or UK Mac layout, REVERSE SOLIDUS
                                        *   (backslash) and VERTICAL LINE in a
                                        *   US or UK Windows layout, and
                                        *   LESS-THAN SIGN and GREATER-THAN SIGN
                                        *   in a Swiss German, German, or French
                                        *   layout. --]]
    SC_APPLICATION = 101, --[[< windows contextual menu, compose --]]
    SC_POWER = 102, --[[< The USB document says this is a status flag,
                               *   not a physical key - but some Mac keyboards
                               *   do have a power key. --]]
    SC_KP_EQUALS = 103,
    SC_F13 = 104,
    SC_F14 = 105,
    SC_F15 = 106,
    SC_F16 = 107,
    SC_F17 = 108,
    SC_F18 = 109,
    SC_F19 = 110,
    SC_F20 = 111,
    SC_F21 = 112,
    SC_F22 = 113,
    SC_F23 = 114,
    SC_F24 = 115,
    SC_EXECUTE = 116,
    SC_HELP = 117,
    SC_MENU = 118,
    SC_SELECT = 119,
    SC_STOP = 120,
    SC_AGAIN = 121,   --[[< redo --]]
    SC_UNDO = 122,
    SC_CUT = 123,
    SC_COPY = 124,
    SC_PASTE = 125,
    SC_FIND = 126,
    SC_MUTE = 127,
    SC_VOLUMEUP = 128,
    SC_VOLUMEDOWN = 129,
-- not sure whether there's a reason to enable these 
--     LOCKINGCAPSLOCK = 130,  
--     LOCKINGNUMLOCK = 131, 
--     LOCKINGSCROLLLOCK = 132, 
    SC_KP_COMMA = 133,
    SC_KP_EQUALSAS400 = 134,
    SC_INTERNATIONAL1 = 135, --[[< used on Asian keyboards, see
                                            footnotes in USB doc --]]
    SC_INTERNATIONAL2 = 136,
    SC_INTERNATIONAL3 = 137, --[[< Yen --]]
    SC_INTERNATIONAL4 = 138,
    SC_INTERNATIONAL5 = 139,
    SC_INTERNATIONAL6 = 140,
    SC_INTERNATIONAL7 = 141,
    SC_INTERNATIONAL8 = 142,
    SC_INTERNATIONAL9 = 143,
    SC_LANG1 = 144, --[[< Hangul/English toggle --]]
    SC_LANG2 = 145, --[[< Hanja conversion --]]
    SC_LANG3 = 146, --[[< Katakana --]]
    SC_LANG4 = 147, --[[< Hiragana --]]
    SC_LANG5 = 148, --[[< Zenkaku/Hankaku --]]
    SC_LANG6 = 149, --[[< reserved --]]
    SC_LANG7 = 150, --[[< reserved --]]
    SC_LANG8 = 151, --[[< reserved --]]
    SC_LANG9 = 152, --[[< reserved --]]
    SC_ALTERASE = 153, --[[< Erase-Eaze --]]
    SC_SYSREQ = 154,
    SC_CANCEL = 155,
    SC_CLEAR = 156,
    SC_PRIOR = 157,
    SC_RETURN2 = 158,
    SC_SEPARATOR = 159,
    SC_OUT = 160,
    SC_OPER = 161,
    SC_CLEARAGAIN = 162,
    SC_CRSEL = 163,
    SC_EXSEL = 164,
    SC_KP_00 = 176,
    SC_KP_000 = 177,
    SC_THOUSANDSSEPARATOR = 178,
    SC_DECIMALSEPARATOR = 179,
    SC_CURRENCYUNIT = 180,
    SC_CURRENCYSUBUNIT = 181,
    SC_KP_LEFTPAREN = 182,
    SC_KP_RIGHTPAREN = 183,
    SC_KP_LEFTBRACE = 184,
    SC_KP_RIGHTBRACE = 185,
    SC_KP_TAB = 186,
    SC_KP_BACKSPACE = 187,
    SC_KP_A = 188,
    SC_KP_B = 189,
    SC_KP_C = 190,
    SC_KP_D = 191,
    SC_KP_E = 192,
    SC_KP_F = 193,
    SC_KP_XOR = 194,
    SC_KP_POWER = 195,
    SC_KP_PERCENT = 196,
    SC_KP_LESS = 197,
    SC_KP_GREATER = 198,
    SC_KP_AMPERSAND = 199,
    SC_KP_DBLAMPERSAND = 200,
    SC_KP_VERTICALBAR = 201,
    SC_KP_DBLVERTICALBAR = 202,
    SC_KP_COLON = 203,
    SC_KP_HASH = 204,
    SC_KP_SPACE = 205,
    SC_KP_AT = 206,
    SC_KP_EXCLAM = 207,
    SC_KP_MEMSTORE = 208,
    SC_KP_MEMRECALL = 209,
    SC_KP_MEMCLEAR = 210,
    SC_KP_MEMADD = 211,
    SC_KP_MEMSUBTRACT = 212,
    SC_KP_MEMMULTIPLY = 213,
    SC_KP_MEMDIVIDE = 214,
    SC_KP_PLUSMINUS = 215,
    SC_KP_CLEAR = 216,
    SC_KP_CLEARENTRY = 217,
    SC_KP_BINARY = 218,
    SC_KP_OCTAL = 219,
    SC_KP_DECIMAL = 220,
    SC_KP_HEXADECIMAL = 221,
    SC_LCTRL = 224,
    SC_LSHIFT = 225,
    SC_LALT = 226, --[[< alt, option --]]
    SC_LGUI = 227, --[[< windows, command (apple), meta --]]
    SC_RCTRL = 228,
    SC_RSHIFT = 229,
    SC_RALT = 230, --[[< alt gr, option --]]
    SC_RGUI = 231, --[[< windows, command (apple), meta --]]
    SC_MODE = 257,    --[[< I'm not sure if this is really not covered
                                 *   by any of the above, but since there's a
                                 *   special KMOD_MODE for it I'm adding it here
                                 --]]
    -- @} --]]-- Usage page 0x07 --
    --[[
     *  \name Usage page 0x0C
     *
     *  These values are mapped from usage page 0x0C (USB consumer page).
     --]]
    -- @{ --
    SC_AUDIONEXT = 258,
    SC_AUDIOPREV = 259,
    SC_AUDIOSTOP = 260,
    SC_AUDIOPLAY = 261,
    SC_AUDIOMUTE = 262,
    SC_MEDIASELECT = 263,
    SC_WWW = 264,
    SC_MAIL = 265,
    SC_CALCULATOR = 266,
    SC_COMPUTER = 267,
    SC_AC_SEARCH = 268,
    SC_AC_HOME = 269,
    SC_AC_BACK = 270,
    SC_AC_FORWARD = 271,
    SC_AC_STOP = 272,
    SC_AC_REFRESH = 273,
    SC_AC_BOOKMARKS = 274,
    -- @} ---- Usage page 0x0C --
    --[[
     *  \name Walther keys
     *
     *  These are values that Christian Walther added (for mac keyboard?).
     --]]
    -- @{ --
    SC_BRIGHTNESSDOWN = 275,
    SC_BRIGHTNESSUP = 276,
    SC_DISPLAYSWITCH = 277, --[[< display mirroring/dual display
                                           switch, video mode switch --]]
    SC_KBDILLUMTOGGLE = 278,
    SC_KBDILLUMDOWN = 279,
    SC_KBDILLUMUP = 280,
    SC_EJECT = 281,
    SC_SLEEP = 282,
    SC_APP1 = 283,
    SC_APP2 = 284,
    -- @} -- -- Walther keys --
    --[[
     *  \name Usage page 0x0C (additional media keys)
     *
     *  These values are mapped from usage page 0x0C (USB consumer page).
     --]]
    -- @{ --
    SC_AUDIOREWIND = 285,
    SC_AUDIOFASTFORWARD = 286,
    -- @} -- -- Usage page 0x0C (additional media keys) -- 
    -- Add any other keys here. -- 
    SDL_NUM_SCANCODES = 512 --< not a key, just marks the number of scancodes
}
               
