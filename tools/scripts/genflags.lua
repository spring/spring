#!/usr/bin/env lua
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    genflags.lua
--  brief:   flag atlas texture generator
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

require('gd') -- needs the gd lua image library to make the texture atlas

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


local countries = {
  ["ad"] = "ANDORRA",
  ["ae"] = "UNITED ARAB EMIRATES",
  ["af"] = "AFGHANISTAN",
  ["ag"] = "ANTIGUA AND BARBUDA",
  ["ai"] = "ANGUILLA",
  ["al"] = "ALBANIA",
  ["am"] = "ARMENIA",
  ["an"] = "NETHERLANDS ANTILLES",
  ["ao"] = "ANGOLA",
  ["aq"] = "ANTARCTICA",
  ["ar"] = "ARGENTINA",
  ["as"] = "AMERICAN SAMOA",
  ["at"] = "AUSTRIA",
  ["au"] = "AUSTRALIA",
  ["aw"] = "ARUBA",
  ["az"] = "AZERBAIJAN",
  ["ba"] = "BOSNIA AND HERZEGOWINA",
  ["bb"] = "BARBADOS",
  ["bd"] = "BANGLADESH",
  ["be"] = "BELGIUM",
  ["bf"] = "BURKINA FASO",
  ["bg"] = "BULGARIA",
  ["bh"] = "BAHRAIN",
  ["bi"] = "BURUNDI",
  ["bj"] = "BENIN",
  ["bm"] = "BERMUDA",
  ["bn"] = "BRUNEI DARUSSALAM",
  ["bo"] = "BOLIVIA",
  ["br"] = "BRAZIL",
  ["bs"] = "BAHAMAS",
  ["bt"] = "BHUTAN",
  ["bv"] = "BOUVET ISLAND",
  ["bw"] = "BOTSWANA",
  ["by"] = "BELARUS",
  ["bz"] = "BELIZE",
  ["ca"] = "CANADA",
  ["cc"] = "COCOS (KEELING) ISLANDS",
  ["cd"] = "CONGO, Democratic Republic of (was Zaire)",
  ["cf"] = "CENTRAL AFRICAN REPUBLIC",
  ["cg"] = "CONGO, Peoples Republic of",
  ["ch"] = "SWITZERLAND",
  ["ci"] = "COTE D'IVOIRE",
  ["ck"] = "COOK ISLANDS",
  ["cl"] = "CHILE",
  ["cm"] = "CAMEROON",
  ["cn"] = "CHINA",
  ["co"] = "COLOMBIA",
  ["cr"] = "COSTA RICA",
  ["cu"] = "CUBA",
  ["cv"] = "CAPE VERDE",
  ["cx"] = "CHRISTMAS ISLAND",
  ["cy"] = "CYPRUS",
  ["cz"] = "CZECH REPUBLIC",
  ["de"] = "GERMANY",
  ["dj"] = "DJIBOUTI",
  ["dk"] = "DENMARK",
  ["dm"] = "DOMINICA",
  ["do"] = "DOMINICAN REPUBLIC",
  ["dz"] = "ALGERIA",
  ["ec"] = "ECUADOR",
  ["ee"] = "ESTONIA",
  ["eg"] = "EGYPT",
  ["eh"] = "WESTERN SAHARA",
  ["er"] = "ERITREA",
  ["es"] = "SPAIN",
  ["et"] = "ETHIOPIA",
  ["fi"] = "FINLAND",
  ["fj"] = "FIJI",
  ["fk"] = "FALKLAND ISLANDS (MALVINAS)",
  ["fm"] = "MICRONESIA, FEDERATED STATES OF",
  ["fo"] = "FAROE ISLANDS",
  ["fr"] = "FRANCE",
--  ["fx"] = "FRANCE, METROPOLITAN",
  ["ga"] = "GABON",
  ["gb"] = "UNITED KINGDOM",
  ["gd"] = "GRENADA",
  ["ge"] = "GEORGIA",
  ["gf"] = "FRENCH GUIANA",
  ["gh"] = "GHANA",
  ["gi"] = "GIBRALTAR",
  ["gl"] = "GREENLAND",
  ["gm"] = "GAMBIA",
  ["gn"] = "GUINEA",
  ["gp"] = "GUADELOUPE",
  ["gq"] = "EQUATORIAL GUINEA",
  ["gr"] = "GREECE",
  ["gs"] = "SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS",
  ["gt"] = "GUATEMALA",
  ["gu"] = "GUAM",
  ["gw"] = "GUINEA-BISSAU",
  ["gy"] = "GUYANA",
  ["hk"] = "HONG KONG",
  ["hm"] = "HEARD AND MC DONALD ISLANDS",
  ["hn"] = "HONDURAS",
  ["hr"] = "CROATIA (local name: Hrvatska)",
  ["ht"] = "HAITI",
  ["hu"] = "HUNGARY",
  ["id"] = "INDONESIA",
  ["ie"] = "IRELAND",
  ["il"] = "ISRAEL",
  ["in"] = "INDIA",
  ["io"] = "BRITISH INDIAN OCEAN TERRITORY",
  ["iq"] = "IRAQ",
  ["ir"] = "IRAN (ISLAMIC REPUBLIC OF)",
  ["is"] = "ICELAND",
  ["it"] = "ITALY",
  ["jm"] = "JAMAICA",
  ["jo"] = "JORDAN",
  ["jp"] = "JAPAN",
  ["ke"] = "KENYA",
  ["kg"] = "KYRGYZSTAN",
  ["kh"] = "CAMBODIA",
  ["ki"] = "KIRIBATI",
  ["km"] = "COMOROS",
  ["kn"] = "SAINT KITTS AND NEVIS",
  ["kp"] = "KOREA, DEMOCRATIC PEOPLE'S REPUBLIC OF",
  ["kr"] = "KOREA, REPUBLIC OF",
  ["kw"] = "KUWAIT",
  ["ky"] = "CAYMAN ISLANDS",
  ["kz"] = "KAZAKHSTAN",
  ["la"] = "LAO PEOPLE'S DEMOCRATIC REPUBLIC",
  ["lb"] = "LEBANON",
  ["lc"] = "SAINT LUCIA",
  ["li"] = "LIECHTENSTEIN",
  ["lk"] = "SRI LANKA",
  ["lr"] = "LIBERIA",
  ["ls"] = "LESOTHO",
  ["lt"] = "LITHUANIA",
  ["lu"] = "LUXEMBOURG",
  ["lv"] = "LATVIA",
  ["ly"] = "LIBYAN ARAB JAMAHIRIYA",
  ["ma"] = "MOROCCO",
  ["mc"] = "MONACO",
  ["md"] = "MOLDOVA, REPUBLIC OF",
  ["mg"] = "MADAGASCAR",
  ["mh"] = "MARSHALL ISLANDS",
  ["mk"] = "MACEDONIA, THE FORMER YUGOSLAV REPUBLIC OF",
  ["ml"] = "MALI",
  ["mm"] = "MYANMAR",
  ["mn"] = "MONGOLIA",
  ["mo"] = "MACAU",
  ["mp"] = "NORTHERN MARIANA ISLANDS",
  ["mq"] = "MARTINIQUE",
  ["mr"] = "MAURITANIA",
  ["ms"] = "MONTSERRAT",
  ["mt"] = "MALTA",
  ["mu"] = "MAURITIUS",
  ["mv"] = "MALDIVES",
  ["mw"] = "MALAWI",
  ["mx"] = "MEXICO",
  ["my"] = "MALAYSIA",
  ["mz"] = "MOZAMBIQUE",
  ["na"] = "NAMIBIA",
  ["nc"] = "NEW CALEDONIA",
  ["ne"] = "NIGER",
  ["nf"] = "NORFOLK ISLAND",
  ["ng"] = "NIGERIA",
  ["ni"] = "NICARAGUA",
  ["nl"] = "NETHERLANDS",
  ["no"] = "NORWAY",
  ["np"] = "NEPAL",
  ["nr"] = "NAURU",
  ["nu"] = "NIUE",
  ["nz"] = "NEW ZEALAND",
  ["om"] = "OMAN",
  ["pa"] = "PANAMA",
  ["pe"] = "PERU",
  ["pf"] = "FRENCH POLYNESIA",
  ["pg"] = "PAPUA NEW GUINEA",
  ["ph"] = "PHILIPPINES",
  ["pk"] = "PAKISTAN",
  ["pl"] = "POLAND",
  ["pm"] = "ST. PIERRE AND MIQUELON",
  ["pn"] = "PITCAIRN",
  ["pr"] = "PUERTO RICO",
  ["ps"] = "PALESTINIAN TERRITORY, Occupied",
  ["pt"] = "PORTUGAL",
  ["pw"] = "PALAU",
  ["py"] = "PARAGUAY",
  ["qa"] = "QATAR",
  ["re"] = "REUNION",
  ["ro"] = "ROMANIA",
  ["ru"] = "RUSSIAN FEDERATION",
  ["rw"] = "RWANDA",
  ["sa"] = "SAUDI ARABIA",
  ["sb"] = "SOLOMON ISLANDS",
  ["sc"] = "SEYCHELLES",
  ["sd"] = "SUDAN",
  ["se"] = "SWEDEN",
  ["sg"] = "SINGAPORE",
  ["sh"] = "ST. HELENA",
  ["si"] = "SLOVENIA",
  ["sj"] = "SVALBARD AND JAN MAYEN ISLANDS",
  ["sk"] = "SLOVAKIA (Slovak Republic)",
  ["sl"] = "SIERRA LEONE",
  ["sm"] = "SAN MARINO",
  ["sn"] = "SENEGAL",
  ["so"] = "SOMALIA",
  ["sr"] = "SURINAME",
  ["st"] = "SAO TOME AND PRINCIPE",
  ["sv"] = "EL SALVADOR",
  ["sy"] = "SYRIAN ARAB REPUBLIC",
  ["sz"] = "SWAZILAND",
  ["tc"] = "TURKS AND CAICOS ISLANDS",
  ["td"] = "CHAD",
  ["tf"] = "FRENCH SOUTHERN TERRITORIES",
  ["tg"] = "TOGO",
  ["th"] = "THAILAND",
  ["tj"] = "TAJIKISTAN",
  ["tk"] = "TOKELAU",
  ["tl"] = "EAST TIMOR",
  ["tm"] = "TURKMENISTAN",
  ["tn"] = "TUNISIA",
  ["to"] = "TONGA",
  ["tr"] = "TURKEY",
  ["tt"] = "TRINIDAD AND TOBAGO",
  ["tv"] = "TUVALU",
  ["tw"] = "TAIWAN",
  ["tz"] = "TANZANIA, UNITED REPUBLIC OF",
  ["ua"] = "UKRAINE",
  ["ug"] = "UGANDA",
  ["um"] = "UNITED STATES MINOR OUTLYING ISLANDS",
  ["us"] = "UNITED STATES",
  ["uy"] = "URUGUAY",
  ["uz"] = "UZBEKISTAN",
  ["va"] = "VATICAN CITY STATE (HOLY SEE)",
  ["vc"] = "SAINT VINCENT AND THE GRENADINES",
  ["ve"] = "VENEZUELA",
  ["vg"] = "VIRGIN ISLANDS (BRITISH)",
  ["vi"] = "VIRGIN ISLANDS (U.S.)",
  ["vn"] = "VIET NAM",
  ["vu"] = "VANUATU",
  ["wf"] = "WALLIS AND FUTUNA ISLANDS",
  ["ws"] = "SAMOA",
  ["xx"] = "Unknown country",
  ["ye"] = "YEMEN",
  ["yt"] = "MAYOTTE",
  ["yu"] = "YUGOSLAVIA",
  ["za"] = "SOUTH AFRICA",
  ["zm"] = "ZAMBIA",
  ["zw"] = "ZIMBABWE",
}


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local imgs = {}

local caseFunc = string.lower
if (io.open('CA.png', 'r')) then
  caseFunc = string.upper
end

--------------------------------------------------------------------------------

local sorted = {}
for abbr, name in pairs(countries) do
  local pngFile = caseFunc(abbr) .. '.png'
  local img = gd.createFromPng(pngFile)
  if (img == nil) then
    error('missing flag image file: ' .. pngFile)
  else
    imgs[abbr] = {
      img = img,
      sx = img:sizeX(),
      sy = img:sizeY(),
    }
    table.insert(sorted, {
      abbr = abbr,
      img  = img,
      sx   = img:sizeX(),
      sy   = img:sizeY()
    })
  end
end
table.sort(sorted, function(a, b) return (a.abbr < b.abbr) end)

--------------------------------------------------------------------------------

local sx = sorted[1].sx
local sy = sorted[1].sy
if (not sx or not sy) then
  error('This just will not do ...')
end

for i, tbl in ipairs(sorted) do
  if ((tbl.sx ~= sx) or (tbl.sy ~= sy)) then
    error(string.format('Mismatched flag sizes: %ix%i vs. %ix%i',
                        sx, sy, tbl.sx, tbl.sy))
  end
end

--------------------------------------------------------------------------------

local function MorePOT(num)
  -- the next power-of-two
  local x = 1
  while (true) do
    if (num <= x) then
      return x
    end
    x = x * 2
  end
end

local imgX = MorePOT(math.sqrt(sx * sy * #sorted))
local gx = math.floor(imgX / sx)
local gy = math.floor(#sorted / gx) + 1
local imgY = MorePOT(gy * sy)

local out = gd.createTrueColor(imgX, imgY)

local alpha = out:colorExactAlpha(255, 255, 255, 0) -- not working
out:filledRectangle(0, 0, imgX - 1, imgY - 1, alpha)

--------------------------------------------------------------------------------

print()
print('local flags = {')
print('  xsize = ' .. sx .. ',')
print('  ysize = ' .. sy .. ',')
print('  specs = {')
for i, tbl in ipairs(sorted) do
  local cellX = ((i - 1) % gx)
  local cellY = math.floor((i - 1) / gx)
  local offx = sx * cellX
  local offy = sy * cellY
  gd.copy(out, tbl.img, offx, imgY - offy - sy, 0, 0, sx, sy)
  print(string.format('    [%q] = { %3i, %3i, %3i, %3i, name = %q },',
        tbl.abbr, 
        offx,
        offy,
        (offx + sx),
        (offy + sy),
        countries[tbl.abbr]))
end
        print('  }')
print('}')

print()
print('for abbr, tbl in pairs(flags.specs) do')
print('  tbl[1] = tbl[1] / ' .. imgX)
print('  tbl[2] = 1 - (tbl[2] / ' .. imgY .. ')')
print('  tbl[3] = tbl[3] / ' .. imgX)
print('  tbl[4] = 1 - (tbl[4] / ' .. imgY .. ')')
print('end')
print()

print('return flags')

out:png('flags.png')


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
