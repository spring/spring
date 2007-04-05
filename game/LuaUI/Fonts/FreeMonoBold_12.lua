
local fontSpecs = {
  srcFile  = [[Fonts/FreeMonoBold.ttf]],
  family   = [[FreeMono]],
  style    = [[Bold]],
  fontHeight = 12,
  yStep    = 13,
  height   = 12,
  xTexSize = 512,
  yTexSize = 128,
}

local glyphs = {}

glyphs[32] = { --' '--
  num = 32,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    2, oyp =    1,
  txn =    1, tyn =    1, txp =    4, typ =    4,
}
glyphs[33] = { --'!'--
  num = 33,
  adv = 7,
  oxn =    1, oyn =   -2, oxp =    6, oyp =    9,
  txn =   14, tyn =    1, txp =   19, typ =   12,
}
glyphs[34] = { --'"'--
  num = 34,
  adv = 7,
  oxn =    0, oyn =    2, oxp =    7, oyp =    9,
  txn =   27, tyn =    1, txp =   34, typ =    8,
}
glyphs[35] = { --'#'--
  num = 35,
  adv = 7,
  oxn =   -1, oyn =   -3, oxp =    8, oyp =   10,
  txn =   40, tyn =    1, txp =   49, typ =   14,
}
glyphs[36] = { --'$'--
  num = 36,
  adv = 7,
  oxn =    0, oyn =   -3, oxp =    8, oyp =   10,
  txn =   53, tyn =    1, txp =   61, typ =   14,
}
glyphs[37] = { --'%'--
  num = 37,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =   66, tyn =    1, txp =   75, typ =   12,
}
glyphs[38] = { --'&'--
  num = 38,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    8,
  txn =   79, tyn =    1, txp =   88, typ =   11,
}
glyphs[39] = { --'''--
  num = 39,
  adv = 7,
  oxn =    1, oyn =    2, oxp =    6, oyp =    9,
  txn =   92, tyn =    1, txp =   97, typ =    8,
}
glyphs[40] = { --'('--
  num = 40,
  adv = 7,
  oxn =    2, oyn =   -3, oxp =    7, oyp =    9,
  txn =  105, tyn =    1, txp =  110, typ =   13,
}
glyphs[41] = { --')'--
  num = 41,
  adv = 7,
  oxn =    0, oyn =   -3, oxp =    6, oyp =    9,
  txn =  118, tyn =    1, txp =  124, typ =   13,
}
glyphs[42] = { --'*'--
  num = 42,
  adv = 7,
  oxn =    0, oyn =    1, oxp =    8, oyp =    9,
  txn =  131, tyn =    1, txp =  139, typ =    9,
}
glyphs[43] = { --'+'--
  num = 43,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  144, tyn =    1, txp =  153, typ =   10,
}
glyphs[44] = { --','--
  num = 44,
  adv = 7,
  oxn =    0, oyn =   -3, oxp =    6, oyp =    3,
  txn =  157, tyn =    1, txp =  163, typ =    7,
}
glyphs[45] = { --'-'--
  num = 45,
  adv = 7,
  oxn =   -1, oyn =    1, oxp =    8, oyp =    5,
  txn =  170, tyn =    1, txp =  179, typ =    5,
}
glyphs[46] = { --'.'--
  num = 46,
  adv = 7,
  oxn =    1, oyn =   -2, oxp =    6, oyp =    3,
  txn =  183, tyn =    1, txp =  188, typ =    6,
}
glyphs[47] = { --'/'--
  num = 47,
  adv = 7,
  oxn =    0, oyn =   -3, oxp =    8, oyp =   10,
  txn =  196, tyn =    1, txp =  204, typ =   14,
}
glyphs[48] = { --'0'--
  num = 48,
  adv = 7,
  oxn =    0, oyn =   -2, oxp =    8, oyp =    9,
  txn =  209, tyn =    1, txp =  217, typ =   12,
}
glyphs[49] = { --'1'--
  num = 49,
  adv = 7,
  oxn =    0, oyn =   -1, oxp =    8, oyp =    9,
  txn =  222, tyn =    1, txp =  230, typ =   11,
}
glyphs[50] = { --'2'--
  num = 50,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    9,
  txn =  235, tyn =    1, txp =  244, typ =   11,
}
glyphs[51] = { --'3'--
  num = 51,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =  248, tyn =    1, txp =  257, typ =   12,
}
glyphs[52] = { --'4'--
  num = 52,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    9,
  txn =  261, tyn =    1, txp =  270, typ =   11,
}
glyphs[53] = { --'5'--
  num = 53,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =  274, tyn =    1, txp =  283, typ =   12,
}
glyphs[54] = { --'6'--
  num = 54,
  adv = 7,
  oxn =    0, oyn =   -2, oxp =    8, oyp =    9,
  txn =  287, tyn =    1, txp =  295, typ =   12,
}
glyphs[55] = { --'7'--
  num = 55,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =  300, tyn =    1, txp =  309, typ =   12,
}
glyphs[56] = { --'8'--
  num = 56,
  adv = 7,
  oxn =    0, oyn =   -2, oxp =    8, oyp =    9,
  txn =  313, tyn =    1, txp =  321, typ =   12,
}
glyphs[57] = { --'9'--
  num = 57,
  adv = 7,
  oxn =    0, oyn =   -2, oxp =    8, oyp =    9,
  txn =  326, tyn =    1, txp =  334, typ =   12,
}
glyphs[58] = { --':'--
  num = 58,
  adv = 7,
  oxn =    1, oyn =   -2, oxp =    6, oyp =    7,
  txn =  339, tyn =    1, txp =  344, typ =   10,
}
glyphs[59] = { --';'--
  num = 59,
  adv = 7,
  oxn =    0, oyn =   -3, oxp =    6, oyp =    7,
  txn =  352, tyn =    1, txp =  358, typ =   11,
}
glyphs[60] = { --'<'--
  num = 60,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  365, tyn =    1, txp =  374, typ =   10,
}
glyphs[61] = { --'='--
  num = 61,
  adv = 7,
  oxn =   -1, oyn =    0, oxp =    8, oyp =    7,
  txn =  378, tyn =    1, txp =  387, typ =    8,
}
glyphs[62] = { --'>'--
  num = 62,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    7,
  txn =  391, tyn =    1, txp =  400, typ =    9,
}
glyphs[63] = { --'?'--
  num = 63,
  adv = 7,
  oxn =    0, oyn =   -2, oxp =    8, oyp =    9,
  txn =  404, tyn =    1, txp =  412, typ =   12,
}
glyphs[64] = { --'@'--
  num = 64,
  adv = 7,
  oxn =   -1, oyn =   -3, oxp =    8, oyp =    9,
  txn =  417, tyn =    1, txp =  426, typ =   13,
}
glyphs[65] = { --'A'--
  num = 65,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =    8,
  txn =  430, tyn =    1, txp =  441, typ =   10,
}
glyphs[66] = { --'B'--
  num = 66,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  443, tyn =    1, txp =  452, typ =   10,
}
glyphs[67] = { --'C'--
  num = 67,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =  456, tyn =    1, txp =  465, typ =   12,
}
glyphs[68] = { --'D'--
  num = 68,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  469, tyn =    1, txp =  478, typ =   10,
}
glyphs[69] = { --'E'--
  num = 69,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  482, tyn =    1, txp =  491, typ =   10,
}
glyphs[70] = { --'F'--
  num = 70,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  495, tyn =    1, txp =  504, typ =   10,
}
glyphs[71] = { --'G'--
  num = 71,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    9, oyp =    9,
  txn =    1, tyn =   17, txp =   11, typ =   28,
}
glyphs[72] = { --'H'--
  num = 72,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =   14, tyn =   17, txp =   23, typ =   26,
}
glyphs[73] = { --'I'--
  num = 73,
  adv = 7,
  oxn =    0, oyn =   -1, oxp =    8, oyp =    8,
  txn =   27, tyn =   17, txp =   35, typ =   26,
}
glyphs[74] = { --'J'--
  num = 74,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    9, oyp =    8,
  txn =   40, tyn =   17, txp =   50, typ =   27,
}
glyphs[75] = { --'K'--
  num = 75,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    9, oyp =    8,
  txn =   53, tyn =   17, txp =   63, typ =   26,
}
glyphs[76] = { --'L'--
  num = 76,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =   66, tyn =   17, txp =   75, typ =   26,
}
glyphs[77] = { --'M'--
  num = 77,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =    9,
  txn =   79, tyn =   17, txp =   90, typ =   27,
}
glyphs[78] = { --'N'--
  num = 78,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =    8,
  txn =   92, tyn =   17, txp =  103, typ =   26,
}
glyphs[79] = { --'O'--
  num = 79,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =  105, tyn =   17, txp =  114, typ =   28,
}
glyphs[80] = { --'P'--
  num = 80,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  118, tyn =   17, txp =  127, typ =   26,
}
glyphs[81] = { --'Q'--
  num = 81,
  adv = 7,
  oxn =   -1, oyn =   -3, oxp =    8, oyp =    9,
  txn =  131, tyn =   17, txp =  140, typ =   29,
}
glyphs[82] = { --'R'--
  num = 82,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    9, oyp =    8,
  txn =  144, tyn =   17, txp =  154, typ =   26,
}
glyphs[83] = { --'S'--
  num = 83,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =  157, tyn =   17, txp =  166, typ =   28,
}
glyphs[84] = { --'T'--
  num = 84,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  170, tyn =   17, txp =  179, typ =   26,
}
glyphs[85] = { --'U'--
  num = 85,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    9, oyp =    8,
  txn =  183, tyn =   17, txp =  193, typ =   27,
}
glyphs[86] = { --'V'--
  num = 86,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =    8,
  txn =  196, tyn =   17, txp =  207, typ =   26,
}
glyphs[87] = { --'W'--
  num = 87,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =    8,
  txn =  209, tyn =   17, txp =  220, typ =   26,
}
glyphs[88] = { --'X'--
  num = 88,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    9, oyp =    8,
  txn =  222, tyn =   17, txp =  232, typ =   26,
}
glyphs[89] = { --'Y'--
  num = 89,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  235, tyn =   17, txp =  244, typ =   26,
}
glyphs[90] = { --'Z'--
  num = 90,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  248, tyn =   17, txp =  257, typ =   26,
}
glyphs[91] = { --'['--
  num = 91,
  adv = 7,
  oxn =    2, oyn =   -3, oxp =    7, oyp =    9,
  txn =  261, tyn =   17, txp =  266, typ =   29,
}
glyphs[92] = { --'\'--
  num = 92,
  adv = 7,
  oxn =    0, oyn =   -3, oxp =    8, oyp =   10,
  txn =  274, tyn =   17, txp =  282, typ =   30,
}
glyphs[93] = { --']'--
  num = 93,
  adv = 7,
  oxn =    0, oyn =   -3, oxp =    6, oyp =    9,
  txn =  287, tyn =   17, txp =  293, typ =   29,
}
glyphs[94] = { --'^'--
  num = 94,
  adv = 7,
  oxn =    0, oyn =    2, oxp =    8, oyp =    9,
  txn =  300, tyn =   17, txp =  308, typ =   24,
}
glyphs[95] = { --'_'--
  num = 95,
  adv = 7,
  oxn =   -2, oyn =   -3, oxp =    9, oyp =    1,
  txn =  313, tyn =   17, txp =  324, typ =   21,
}
glyphs[96] = { --'`'--
  num = 96,
  adv = 7,
  oxn =    0, oyn =    4, oxp =    6, oyp =   10,
  txn =  326, tyn =   17, txp =  332, typ =   23,
}
glyphs[97] = { --'a'--
  num = 97,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    7,
  txn =  339, tyn =   17, txp =  348, typ =   26,
}
glyphs[98] = { --'b'--
  num = 98,
  adv = 7,
  oxn =   -2, oyn =   -2, oxp =    8, oyp =    9,
  txn =  352, tyn =   17, txp =  362, typ =   28,
}
glyphs[99] = { --'c'--
  num = 99,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    7,
  txn =  365, tyn =   17, txp =  374, typ =   26,
}
glyphs[100] = { --'d'--
  num = 100,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    9, oyp =    9,
  txn =  378, tyn =   17, txp =  388, typ =   28,
}
glyphs[101] = { --'e'--
  num = 101,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    7,
  txn =  391, tyn =   17, txp =  400, typ =   26,
}
glyphs[102] = { --'f'--
  num = 102,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    9,
  txn =  404, tyn =   17, txp =  413, typ =   27,
}
glyphs[103] = { --'g'--
  num = 103,
  adv = 7,
  oxn =   -1, oyn =   -4, oxp =    9, oyp =    7,
  txn =  417, tyn =   17, txp =  427, typ =   28,
}
glyphs[104] = { --'h'--
  num = 104,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    9,
  txn =  430, tyn =   17, txp =  439, typ =   27,
}
glyphs[105] = { --'i'--
  num = 105,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    9,
  txn =  443, tyn =   17, txp =  452, typ =   27,
}
glyphs[106] = { --'j'--
  num = 106,
  adv = 7,
  oxn =    0, oyn =   -4, oxp =    7, oyp =    9,
  txn =  456, tyn =   17, txp =  463, typ =   30,
}
glyphs[107] = { --'k'--
  num = 107,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    9,
  txn =  469, tyn =   17, txp =  478, typ =   27,
}
glyphs[108] = { --'l'--
  num = 108,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    9,
  txn =  482, tyn =   17, txp =  491, typ =   27,
}
glyphs[109] = { --'m'--
  num = 109,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =    7,
  txn =  495, tyn =   17, txp =  506, typ =   25,
}
glyphs[110] = { --'n'--
  num = 110,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    7,
  txn =    1, tyn =   33, txp =   10, typ =   41,
}
glyphs[111] = { --'o'--
  num = 111,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    7,
  txn =   14, tyn =   33, txp =   23, typ =   42,
}
glyphs[112] = { --'p'--
  num = 112,
  adv = 7,
  oxn =   -2, oyn =   -4, oxp =    8, oyp =    7,
  txn =   27, tyn =   33, txp =   37, typ =   44,
}
glyphs[113] = { --'q'--
  num = 113,
  adv = 7,
  oxn =   -1, oyn =   -4, oxp =    9, oyp =    7,
  txn =   40, tyn =   33, txp =   50, typ =   44,
}
glyphs[114] = { --'r'--
  num = 114,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    7,
  txn =   53, tyn =   33, txp =   62, typ =   41,
}
glyphs[115] = { --'s'--
  num = 115,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    7,
  txn =   66, tyn =   33, txp =   75, typ =   42,
}
glyphs[116] = { --'t'--
  num = 116,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =   79, tyn =   33, txp =   88, typ =   44,
}
glyphs[117] = { --'u'--
  num = 117,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    7,
  txn =   92, tyn =   33, txp =  101, typ =   42,
}
glyphs[118] = { --'v'--
  num = 118,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    9, oyp =    7,
  txn =  105, tyn =   33, txp =  115, typ =   41,
}
glyphs[119] = { --'w'--
  num = 119,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    9, oyp =    7,
  txn =  118, tyn =   33, txp =  128, typ =   41,
}
glyphs[120] = { --'x'--
  num = 120,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    7,
  txn =  131, tyn =   33, txp =  140, typ =   41,
}
glyphs[121] = { --'y'--
  num = 121,
  adv = 7,
  oxn =   -1, oyn =   -4, oxp =    8, oyp =    7,
  txn =  144, tyn =   33, txp =  153, typ =   44,
}
glyphs[122] = { --'z'--
  num = 122,
  adv = 7,
  oxn =    0, oyn =   -1, oxp =    8, oyp =    7,
  txn =  157, tyn =   33, txp =  165, typ =   41,
}
glyphs[123] = { --'{'--
  num = 123,
  adv = 7,
  oxn =    1, oyn =   -3, oxp =    7, oyp =    9,
  txn =  170, tyn =   33, txp =  176, typ =   45,
}
glyphs[124] = { --'|'--
  num = 124,
  adv = 7,
  oxn =    2, oyn =   -3, oxp =    6, oyp =    9,
  txn =  183, tyn =   33, txp =  187, typ =   45,
}
glyphs[125] = { --'}'--
  num = 125,
  adv = 7,
  oxn =    1, oyn =   -3, oxp =    7, oyp =    9,
  txn =  196, tyn =   33, txp =  202, typ =   45,
}
glyphs[126] = { --'~'--
  num = 126,
  adv = 7,
  oxn =   -1, oyn =    1, oxp =    8, oyp =    6,
  txn =  209, tyn =   33, txp =  218, typ =   38,
}
glyphs[127] = { --''--
  num = 127,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  222, tyn =   33, txp =  229, typ =   44,
}
glyphs[128] = { --'Ä'--
  num = 128,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  235, tyn =   33, txp =  242, typ =   44,
}
glyphs[129] = { --'Å'--
  num = 129,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  248, tyn =   33, txp =  255, typ =   44,
}
glyphs[130] = { --'Ç'--
  num = 130,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  261, tyn =   33, txp =  268, typ =   44,
}
glyphs[131] = { --'É'--
  num = 131,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  274, tyn =   33, txp =  281, typ =   44,
}
glyphs[132] = { --'Ñ'--
  num = 132,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  287, tyn =   33, txp =  294, typ =   44,
}
glyphs[133] = { --'Ö'--
  num = 133,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  300, tyn =   33, txp =  307, typ =   44,
}
glyphs[134] = { --'Ü'--
  num = 134,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  313, tyn =   33, txp =  320, typ =   44,
}
glyphs[135] = { --'á'--
  num = 135,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  326, tyn =   33, txp =  333, typ =   44,
}
glyphs[136] = { --'à'--
  num = 136,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  339, tyn =   33, txp =  346, typ =   44,
}
glyphs[137] = { --'â'--
  num = 137,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  352, tyn =   33, txp =  359, typ =   44,
}
glyphs[138] = { --'ä'--
  num = 138,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  365, tyn =   33, txp =  372, typ =   44,
}
glyphs[139] = { --'ã'--
  num = 139,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  378, tyn =   33, txp =  385, typ =   44,
}
glyphs[140] = { --'å'--
  num = 140,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  391, tyn =   33, txp =  398, typ =   44,
}
glyphs[141] = { --'ç'--
  num = 141,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  404, tyn =   33, txp =  411, typ =   44,
}
glyphs[142] = { --'é'--
  num = 142,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  417, tyn =   33, txp =  424, typ =   44,
}
glyphs[143] = { --'è'--
  num = 143,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  430, tyn =   33, txp =  437, typ =   44,
}
glyphs[144] = { --'ê'--
  num = 144,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  443, tyn =   33, txp =  450, typ =   44,
}
glyphs[145] = { --'ë'--
  num = 145,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  456, tyn =   33, txp =  463, typ =   44,
}
glyphs[146] = { --'í'--
  num = 146,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  469, tyn =   33, txp =  476, typ =   44,
}
glyphs[147] = { --'ì'--
  num = 147,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  482, tyn =   33, txp =  489, typ =   44,
}
glyphs[148] = { --'î'--
  num = 148,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  495, tyn =   33, txp =  502, typ =   44,
}
glyphs[149] = { --'ï'--
  num = 149,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =    1, tyn =   49, txp =    8, typ =   60,
}
glyphs[150] = { --'ñ'--
  num = 150,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =   14, tyn =   49, txp =   21, typ =   60,
}
glyphs[151] = { --'ó'--
  num = 151,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =   27, tyn =   49, txp =   34, typ =   60,
}
glyphs[152] = { --'ò'--
  num = 152,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =   40, tyn =   49, txp =   47, typ =   60,
}
glyphs[153] = { --'ô'--
  num = 153,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =   53, tyn =   49, txp =   60, typ =   60,
}
glyphs[154] = { --'ö'--
  num = 154,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =   66, tyn =   49, txp =   73, typ =   60,
}
glyphs[155] = { --'õ'--
  num = 155,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =   79, tyn =   49, txp =   86, typ =   60,
}
glyphs[156] = { --'ú'--
  num = 156,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =   92, tyn =   49, txp =   99, typ =   60,
}
glyphs[157] = { --'ù'--
  num = 157,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  105, tyn =   49, txp =  112, typ =   60,
}
glyphs[158] = { --'û'--
  num = 158,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  118, tyn =   49, txp =  125, typ =   60,
}
glyphs[159] = { --'ü'--
  num = 159,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    6, oyp =   10,
  txn =  131, tyn =   49, txp =  138, typ =   60,
}
glyphs[160] = { --'†'--
  num = 160,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    2, oyp =    1,
  txn =  144, tyn =   49, txp =  147, typ =   52,
}
glyphs[161] = { --'°'--
  num = 161,
  adv = 7,
  oxn =    1, oyn =   -4, oxp =    6, oyp =    7,
  txn =  157, tyn =   49, txp =  162, typ =   60,
}
glyphs[162] = { --'¢'--
  num = 162,
  adv = 7,
  oxn =    0, oyn =   -2, oxp =    7, oyp =    9,
  txn =  170, tyn =   49, txp =  177, typ =   60,
}
glyphs[163] = { --'£'--
  num = 163,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    9,
  txn =  183, tyn =   49, txp =  192, typ =   59,
}
glyphs[164] = { --'§'--
  num = 164,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  196, tyn =   49, txp =  205, typ =   58,
}
glyphs[165] = { --'•'--
  num = 165,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  209, tyn =   49, txp =  218, typ =   58,
}
glyphs[166] = { --'¶'--
  num = 166,
  adv = 7,
  oxn =    2, oyn =   -3, oxp =    6, oyp =    9,
  txn =  222, tyn =   49, txp =  226, typ =   61,
}
glyphs[167] = { --'ß'--
  num = 167,
  adv = 7,
  oxn =   -1, oyn =   -4, oxp =    8, oyp =    8,
  txn =  235, tyn =   49, txp =  244, typ =   61,
}
glyphs[168] = { --'®'--
  num = 168,
  adv = 7,
  oxn =    0, oyn =    5, oxp =    7, oyp =    9,
  txn =  248, tyn =   49, txp =  255, typ =   53,
}
glyphs[169] = { --'©'--
  num = 169,
  adv = 7,
  oxn =   -2, oyn =   -2, oxp =    9, oyp =    9,
  txn =  261, tyn =   49, txp =  272, typ =   60,
}
glyphs[170] = { --'™'--
  num = 170,
  adv = 7,
  oxn =    0, oyn =    1, oxp =    7, oyp =    9,
  txn =  274, tyn =   49, txp =  281, typ =   57,
}
glyphs[171] = { --'´'--
  num = 171,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    6,
  txn =  287, tyn =   49, txp =  296, typ =   56,
}
glyphs[172] = { --'¨'--
  num = 172,
  adv = 7,
  oxn =   -1, oyn =    0, oxp =    7, oyp =    7,
  txn =  300, tyn =   49, txp =  308, typ =   56,
}
glyphs[173] = { --'≠'--
  num = 173,
  adv = 7,
  oxn =   -1, oyn =    1, oxp =    8, oyp =    5,
  txn =  313, tyn =   49, txp =  322, typ =   53,
}
glyphs[174] = { --'Æ'--
  num = 174,
  adv = 7,
  oxn =   -2, oyn =   -2, oxp =    9, oyp =    9,
  txn =  326, tyn =   49, txp =  337, typ =   60,
}
glyphs[175] = { --'Ø'--
  num = 175,
  adv = 7,
  oxn =    0, oyn =    5, oxp =    7, oyp =    9,
  txn =  339, tyn =   49, txp =  346, typ =   53,
}
glyphs[176] = { --'∞'--
  num = 176,
  adv = 7,
  oxn =    0, oyn =    1, oxp =    7, oyp =    9,
  txn =  352, tyn =   49, txp =  359, typ =   57,
}
glyphs[177] = { --'±'--
  num = 177,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    9,
  txn =  365, tyn =   49, txp =  374, typ =   59,
}
glyphs[178] = { --'≤'--
  num = 178,
  adv = 7,
  oxn =    0, oyn =    1, oxp =    7, oyp =    9,
  txn =  378, tyn =   49, txp =  385, typ =   57,
}
glyphs[179] = { --'≥'--
  num = 179,
  adv = 7,
  oxn =    0, oyn =    1, oxp =    7, oyp =    9,
  txn =  391, tyn =   49, txp =  398, typ =   57,
}
glyphs[180] = { --'¥'--
  num = 180,
  adv = 7,
  oxn =    2, oyn =    4, oxp =    7, oyp =   10,
  txn =  404, tyn =   49, txp =  409, typ =   55,
}
glyphs[181] = { --'µ'--
  num = 181,
  adv = 7,
  oxn =   -1, oyn =   -3, oxp =    8, oyp =    7,
  txn =  417, tyn =   49, txp =  426, typ =   59,
}
glyphs[182] = { --'∂'--
  num = 182,
  adv = 7,
  oxn =   -1, oyn =   -4, oxp =    8, oyp =    8,
  txn =  430, tyn =   49, txp =  439, typ =   61,
}
glyphs[183] = { --'∑'--
  num = 183,
  adv = 7,
  oxn =    1, oyn =    1, oxp =    6, oyp =    6,
  txn =  443, tyn =   49, txp =  448, typ =   54,
}
glyphs[184] = { --'∏'--
  num = 184,
  adv = 7,
  oxn =    1, oyn =   -4, oxp =    6, oyp =    1,
  txn =  456, tyn =   49, txp =  461, typ =   54,
}
glyphs[185] = { --'π'--
  num = 185,
  adv = 7,
  oxn =    0, oyn =    1, oxp =    7, oyp =    9,
  txn =  469, tyn =   49, txp =  476, typ =   57,
}
glyphs[186] = { --'∫'--
  num = 186,
  adv = 7,
  oxn =    0, oyn =    1, oxp =    7, oyp =    9,
  txn =  482, tyn =   49, txp =  489, typ =   57,
}
glyphs[187] = { --'ª'--
  num = 187,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    6,
  txn =  495, tyn =   49, txp =  504, typ =   56,
}
glyphs[188] = { --'º'--
  num = 188,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =    9,
  txn =    1, tyn =   65, txp =   12, typ =   75,
}
glyphs[189] = { --'Ω'--
  num = 189,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =    9,
  txn =   14, tyn =   65, txp =   25, typ =   75,
}
glyphs[190] = { --'æ'--
  num = 190,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =    9,
  txn =   27, tyn =   65, txp =   38, typ =   75,
}
glyphs[191] = { --'ø'--
  num = 191,
  adv = 7,
  oxn =   -1, oyn =   -5, oxp =    7, oyp =    6,
  txn =   40, tyn =   65, txp =   48, typ =   76,
}
glyphs[192] = { --'¿'--
  num = 192,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =   12,
  txn =   53, tyn =   65, txp =   64, typ =   78,
}
glyphs[193] = { --'¡'--
  num = 193,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =   12,
  txn =   66, tyn =   65, txp =   77, typ =   78,
}
glyphs[194] = { --'¬'--
  num = 194,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =   12,
  txn =   79, tyn =   65, txp =   90, typ =   78,
}
glyphs[195] = { --'√'--
  num = 195,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =   11,
  txn =   92, tyn =   65, txp =  103, typ =   77,
}
glyphs[196] = { --'ƒ'--
  num = 196,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =   11,
  txn =  105, tyn =   65, txp =  116, typ =   77,
}
glyphs[197] = { --'≈'--
  num = 197,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =   12,
  txn =  118, tyn =   65, txp =  129, typ =   78,
}
glyphs[198] = { --'∆'--
  num = 198,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =    8,
  txn =  131, tyn =   65, txp =  142, typ =   74,
}
glyphs[199] = { --'«'--
  num = 199,
  adv = 7,
  oxn =   -1, oyn =   -4, oxp =    8, oyp =    9,
  txn =  144, tyn =   65, txp =  153, typ =   78,
}
glyphs[200] = { --'»'--
  num = 200,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =   12,
  txn =  157, tyn =   65, txp =  166, typ =   78,
}
glyphs[201] = { --'…'--
  num = 201,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =   12,
  txn =  170, tyn =   65, txp =  179, typ =   78,
}
glyphs[202] = { --' '--
  num = 202,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =   12,
  txn =  183, tyn =   65, txp =  192, typ =   78,
}
glyphs[203] = { --'À'--
  num = 203,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =   11,
  txn =  196, tyn =   65, txp =  205, typ =   77,
}
glyphs[204] = { --'Ã'--
  num = 204,
  adv = 7,
  oxn =    0, oyn =   -1, oxp =    8, oyp =   12,
  txn =  209, tyn =   65, txp =  217, typ =   78,
}
glyphs[205] = { --'Õ'--
  num = 205,
  adv = 7,
  oxn =    0, oyn =   -1, oxp =    8, oyp =   12,
  txn =  222, tyn =   65, txp =  230, typ =   78,
}
glyphs[206] = { --'Œ'--
  num = 206,
  adv = 7,
  oxn =    0, oyn =   -1, oxp =    8, oyp =   12,
  txn =  235, tyn =   65, txp =  243, typ =   78,
}
glyphs[207] = { --'œ'--
  num = 207,
  adv = 7,
  oxn =    0, oyn =   -1, oxp =    8, oyp =   11,
  txn =  248, tyn =   65, txp =  256, typ =   77,
}
glyphs[208] = { --'–'--
  num = 208,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  261, tyn =   65, txp =  270, typ =   74,
}
glyphs[209] = { --'—'--
  num = 209,
  adv = 7,
  oxn =   -2, oyn =   -1, oxp =    9, oyp =   11,
  txn =  274, tyn =   65, txp =  285, typ =   77,
}
glyphs[210] = { --'“'--
  num = 210,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   12,
  txn =  287, tyn =   65, txp =  296, typ =   79,
}
glyphs[211] = { --'”'--
  num = 211,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   12,
  txn =  300, tyn =   65, txp =  309, typ =   79,
}
glyphs[212] = { --'‘'--
  num = 212,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   12,
  txn =  313, tyn =   65, txp =  322, typ =   79,
}
glyphs[213] = { --'’'--
  num = 213,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   11,
  txn =  326, tyn =   65, txp =  335, typ =   78,
}
glyphs[214] = { --'÷'--
  num = 214,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   11,
  txn =  339, tyn =   65, txp =  348, typ =   78,
}
glyphs[215] = { --'◊'--
  num = 215,
  adv = 7,
  oxn =    0, oyn =   -1, oxp =    7, oyp =    7,
  txn =  352, tyn =   65, txp =  359, typ =   73,
}
glyphs[216] = { --'ÿ'--
  num = 216,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    9, oyp =    9,
  txn =  365, tyn =   65, txp =  375, typ =   76,
}
glyphs[217] = { --'Ÿ'--
  num = 217,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    9, oyp =   12,
  txn =  378, tyn =   65, txp =  388, typ =   79,
}
glyphs[218] = { --'⁄'--
  num = 218,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    9, oyp =   12,
  txn =  391, tyn =   65, txp =  401, typ =   79,
}
glyphs[219] = { --'€'--
  num = 219,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    9, oyp =   12,
  txn =  404, tyn =   65, txp =  414, typ =   79,
}
glyphs[220] = { --'‹'--
  num = 220,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    9, oyp =   11,
  txn =  417, tyn =   65, txp =  427, typ =   78,
}
glyphs[221] = { --'›'--
  num = 221,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =   12,
  txn =  430, tyn =   65, txp =  439, typ =   78,
}
glyphs[222] = { --'ﬁ'--
  num = 222,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  443, tyn =   65, txp =  452, typ =   74,
}
glyphs[223] = { --'ﬂ'--
  num = 223,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =  456, tyn =   65, txp =  465, typ =   76,
}
glyphs[224] = { --'‡'--
  num = 224,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   10,
  txn =  469, tyn =   65, txp =  478, typ =   77,
}
glyphs[225] = { --'·'--
  num = 225,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   10,
  txn =  482, tyn =   65, txp =  491, typ =   77,
}
glyphs[226] = { --'‚'--
  num = 226,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   10,
  txn =  495, tyn =   65, txp =  504, typ =   77,
}
glyphs[227] = { --'„'--
  num = 227,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =    1, tyn =   81, txp =   10, typ =   92,
}
glyphs[228] = { --'‰'--
  num = 228,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =   14, tyn =   81, txp =   23, typ =   92,
}
glyphs[229] = { --'Â'--
  num = 229,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   10,
  txn =   27, tyn =   81, txp =   36, typ =   93,
}
glyphs[230] = { --'Ê'--
  num = 230,
  adv = 7,
  oxn =   -2, oyn =   -2, oxp =    9, oyp =    7,
  txn =   40, tyn =   81, txp =   51, typ =   90,
}
glyphs[231] = { --'Á'--
  num = 231,
  adv = 7,
  oxn =   -1, oyn =   -4, oxp =    8, oyp =    7,
  txn =   53, tyn =   81, txp =   62, typ =   92,
}
glyphs[232] = { --'Ë'--
  num = 232,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   10,
  txn =   66, tyn =   81, txp =   75, typ =   93,
}
glyphs[233] = { --'È'--
  num = 233,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   10,
  txn =   79, tyn =   81, txp =   88, typ =   93,
}
glyphs[234] = { --'Í'--
  num = 234,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   10,
  txn =   92, tyn =   81, txp =  101, typ =   93,
}
glyphs[235] = { --'Î'--
  num = 235,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =  105, tyn =   81, txp =  114, typ =   92,
}
glyphs[236] = { --'Ï'--
  num = 236,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =   10,
  txn =  118, tyn =   81, txp =  127, typ =   92,
}
glyphs[237] = { --'Ì'--
  num = 237,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =   10,
  txn =  131, tyn =   81, txp =  140, typ =   92,
}
glyphs[238] = { --'Ó'--
  num = 238,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =   10,
  txn =  144, tyn =   81, txp =  153, typ =   92,
}
glyphs[239] = { --'Ô'--
  num = 239,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    9,
  txn =  157, tyn =   81, txp =  166, typ =   91,
}
glyphs[240] = { --''--
  num = 240,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =  170, tyn =   81, txp =  179, typ =   92,
}
glyphs[241] = { --'Ò'--
  num = 241,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    9,
  txn =  183, tyn =   81, txp =  192, typ =   91,
}
glyphs[242] = { --'Ú'--
  num = 242,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   10,
  txn =  196, tyn =   81, txp =  205, typ =   93,
}
glyphs[243] = { --'Û'--
  num = 243,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   10,
  txn =  209, tyn =   81, txp =  218, typ =   93,
}
glyphs[244] = { --'Ù'--
  num = 244,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   10,
  txn =  222, tyn =   81, txp =  231, typ =   93,
}
glyphs[245] = { --'ı'--
  num = 245,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =  235, tyn =   81, txp =  244, typ =   92,
}
glyphs[246] = { --'ˆ'--
  num = 246,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =  248, tyn =   81, txp =  257, typ =   92,
}
glyphs[247] = { --'˜'--
  num = 247,
  adv = 7,
  oxn =   -1, oyn =   -1, oxp =    8, oyp =    8,
  txn =  261, tyn =   81, txp =  270, typ =   90,
}
glyphs[248] = { --'¯'--
  num = 248,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    7,
  txn =  274, tyn =   81, txp =  283, typ =   90,
}
glyphs[249] = { --'˘'--
  num = 249,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   10,
  txn =  287, tyn =   81, txp =  296, typ =   93,
}
glyphs[250] = { --'˙'--
  num = 250,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   10,
  txn =  300, tyn =   81, txp =  309, typ =   93,
}
glyphs[251] = { --'˚'--
  num = 251,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =   10,
  txn =  313, tyn =   81, txp =  322, typ =   93,
}
glyphs[252] = { --'¸'--
  num = 252,
  adv = 7,
  oxn =   -1, oyn =   -2, oxp =    8, oyp =    9,
  txn =  326, tyn =   81, txp =  335, typ =   92,
}
glyphs[253] = { --'˝'--
  num = 253,
  adv = 7,
  oxn =   -1, oyn =   -4, oxp =    8, oyp =   10,
  txn =  339, tyn =   81, txp =  348, typ =   95,
}
glyphs[254] = { --'˛'--
  num = 254,
  adv = 7,
  oxn =   -2, oyn =   -4, oxp =    8, oyp =    9,
  txn =  352, tyn =   81, txp =  362, typ =   94,
}
glyphs[255] = { --'ˇ'--
  num = 255,
  adv = 7,
  oxn =   -1, oyn =   -4, oxp =    8, oyp =    9,
  txn =  365, tyn =   81, txp =  374, typ =   94,
}

fontSpecs.glyphs = glyphs

return fontSpecs

