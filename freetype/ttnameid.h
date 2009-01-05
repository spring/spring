/***************************************************************************/
/*                                                                         */
/*  ttnameid.h                                                             */
/*                                                                         */
/*    TrueType name ID definitions (specification only).                   */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __TTNAMEID_H__
#define __TTNAMEID_H__


#ifdef __cplusplus
  extern "C" {
#endif


  /*************************************************************************/
  /*                                                                       */
  /* Possible values for the `platform' identifier code in the name        */
  /* records of the TTF `name' table.                                      */
  /*                                                                       */
#define TT_PLATFORM_APPLE_UNICODE  0
#define TT_PLATFORM_MACINTOSH      1
#define TT_PLATFORM_ISO            2 /* deprecated */
#define TT_PLATFORM_MICROSOFT      3

  /* artificial values defined ad-hoc by FreeType */
#define TT_PLATFORM_ADOBE          7


  /*************************************************************************/
  /*                                                                       */
  /* Possible values of the platform specific encoding identifier field in */
  /* the name records of the TTF `name' table if the `platform' identifier */
  /* code is TT_PLATFORM_APPLE_UNICODE.                                    */
  /*                                                                       */
#define TT_APPLE_ID_DEFAULT      0
#define TT_APPLE_ID_UNICODE_1_1  1 /* specify Hangul at U+34xx */
#define TT_APPLE_ID_ISO_10646    2 /* deprecated */
#define TT_APPLE_ID_UNICODE_2_0  3 /* or later */


  /*************************************************************************/
  /*                                                                       */
  /* Possible values of the platform specific encoding identifier field in */
  /* the name records of the TTF `name' table if the `platform' identifier */
  /* code is TT_PLATFORM_MACINTOSH.                                        */
  /*                                                                       */
#define TT_MAC_ID_ROMAN                 0
#define TT_MAC_ID_JAPANESE              1
#define TT_MAC_ID_TRADITIONAL_CHINESE   2
#define TT_MAC_ID_KOREAN                3
#define TT_MAC_ID_ARABIC                4
#define TT_MAC_ID_HEBREW                5
#define TT_MAC_ID_GREEK                 6
#define TT_MAC_ID_RUSSIAN               7
#define TT_MAC_ID_RSYMBOL               8
#define TT_MAC_ID_DEVANAGARI            9
#define TT_MAC_ID_GURMUKHI             10
#define TT_MAC_ID_GUJARATI             11
#define TT_MAC_ID_ORIYA                12
#define TT_MAC_ID_BENGALI              13
#define TT_MAC_ID_TAMIL                14
#define TT_MAC_ID_TELUGU               15
#define TT_MAC_ID_KANNADA              16
#define TT_MAC_ID_MALAYALAM            17
#define TT_MAC_ID_SINHALESE            18
#define TT_MAC_ID_BURMESE              19
#define TT_MAC_ID_KHMER                20
#define TT_MAC_ID_THAI                 21
#define TT_MAC_ID_LAOTIAN              22
#define TT_MAC_ID_GEORGIAN             23
#define TT_MAC_ID_ARMENIAN             24
#define TT_MAC_ID_MALDIVIAN            25
#define TT_MAC_ID_SIMPLIFIED_CHINESE   25
#define TT_MAC_ID_TIBETAN              26
#define TT_MAC_ID_MONGOLIAN            27
#define TT_MAC_ID_GEEZ                 28
#define TT_MAC_ID_SLAVIC               29
#define TT_MAC_ID_VIETNAMESE           30
#define TT_MAC_ID_SINDHI               31
#define TT_MAC_ID_UNINTERP             32


  /*************************************************************************/
  /*                                                                       */
  /* Possible values of the platform specific encoding identifier field in */
  /* the name records of the TTF `name' table if the `platform' identifier */
  /* code is TT_PLATFORM_ISO.                                              */
  /*                                                                       */
  /* This use is now deprecated.                                           */
  /*                                                                       */
#define TT_ISO_ID_7BIT_ASCII  0
#define TT_ISO_ID_10646       1
#define TT_ISO_ID_8859_1      2


  /*************************************************************************/
  /*                                                                       */
  /* possible values of the platform specific encoding identifier field in */
  /* the name records of the TTF `name' table if the `platform' identifier */
  /* code is TT_PLATFORM_MICROSOFT.                                        */
  /*                                                                       */
#define TT_MS_ID_SYMBOL_CS   0
#define TT_MS_ID_UNICODE_CS  1
#define TT_MS_ID_SJIS        2
#define TT_MS_ID_GB2312      3
#define TT_MS_ID_BIG_5       4
#define TT_MS_ID_WANSUNG     5
#define TT_MS_ID_JOHAB       6


  /*************************************************************************/
  /*                                                                       */
  /* possible values of the platform specific encoding identifier field in */
  /* the name records of the TTF `name' table if the `platform' identifier */
  /* code is TT_PLATFORM_ADOBE.                                            */
  /*                                                                       */
  /* These are artificial values defined ad-hoc by FreeType.               */
  /*                                                                       */
#define TT_ADOBE_ID_STANDARD  0
#define TT_ADOBE_ID_EXPERT    1
#define TT_ADOBE_ID_CUSTOM    2


  /*************************************************************************/
  /*                                                                       */
  /* Possible values of the language identifier field in the name records  */
  /* of the TTF `name' table if the `platform' identifier code is          */
  /* TT_PLATFORM_MACINTOSH.                                                */
  /*                                                                       */
  /* The canonical source for the Apple assigned Language ID's is at       */
  /*                                                                       */
  /*   http://fonts.apple.com/TTRefMan/RM06/Chap6name.html                 */
  /*                                                                       */
#define TT_MAC_LANGID_ENGLISH                       0
#define TT_MAC_LANGID_FRENCH                        1
#define TT_MAC_LANGID_GERMAN                        2
#define TT_MAC_LANGID_ITALIAN                       3
#define TT_MAC_LANGID_DUTCH                         4
#define TT_MAC_LANGID_SWEDISH                       5
#define TT_MAC_LANGID_SPANISH                       6
#define TT_MAC_LANGID_DANISH                        7
#define TT_MAC_LANGID_PORTUGUESE                    8
#define TT_MAC_LANGID_NORWEGIAN                     9
#define TT_MAC_LANGID_HEBREW                       10
#define TT_MAC_LANGID_JAPANESE                     11
#define TT_MAC_LANGID_ARABIC                       12
#define TT_MAC_LANGID_FINNISH                      13
#define TT_MAC_LANGID_GREEK                        14
#define TT_MAC_LANGID_ICELANDIC                    15
#define TT_MAC_LANGID_MALTESE                      16
#define TT_MAC_LANGID_TURKISH                      17
#define TT_MAC_LANGID_CROATIAN                     18
#define TT_MAC_LANGID_CHINESE_TRADITIONAL          19
#define TT_MAC_LANGID_URDU                         20
#define TT_MAC_LANGID_HINDI                        21
#define TT_MAC_LANGID_THAI                         22
#define TT_MAC_LANGID_KOREAN                       23
#define TT_MAC_LANGID_LITHUANIAN                   24
#define TT_MAC_LANGID_POLISH                       25
#define TT_MAC_LANGID_HUNGARIAN                    26
#define TT_MAC_LANGID_ESTONIAN                     27
#define TT_MAC_LANGID_LETTISH                      28
#define TT_MAC_LANGID_SAAMISK                      29
#define TT_MAC_LANGID_FAEROESE                     30
#define TT_MAC_LANGID_FARSI                        31
#define TT_MAC_LANGID_RUSSIAN                      32
#define TT_MAC_LANGID_CHINESE_SIMPLIFIED           33
#define TT_MAC_LANGID_FLEMISH                      34
#define TT_MAC_LANGID_IRISH                        35
#define TT_MAC_LANGID_ALBANIAN                     36
#define TT_MAC_LANGID_ROMANIAN                     37
#define TT_MAC_LANGID_CZECH                        38
#define TT_MAC_LANGID_SLOVAK                       39
#define TT_MAC_LANGID_SLOVENIAN                    40
#define TT_MAC_LANGID_YIDDISH                      41
#define TT_MAC_LANGID_SERBIAN                      42
#define TT_MAC_LANGID_MACEDONIAN                   43
#define TT_MAC_LANGID_BULGARIAN                    44
#define TT_MAC_LANGID_UKRAINIAN                    45
#define TT_MAC_LANGID_BYELORUSSIAN                 46
#define TT_MAC_LANGID_UZBEK                        47
#define TT_MAC_LANGID_KAZAKH                       48
#define TT_MAC_LANGID_AZERBAIJANI                  49
#define TT_MAC_LANGID_AZERBAIJANI_CYRILLIC_SCRIPT  49
#define TT_MAC_LANGID_AZERBAIJANI_ARABIC_SCRIPT    50
#define TT_MAC_LANGID_ARMENIAN                     51
#define TT_MAC_LANGID_GEORGIAN                     52
#define TT_MAC_LANGID_MOLDAVIAN                    53
#define TT_MAC_LANGID_KIRGHIZ                      54
#define TT_MAC_LANGID_TAJIKI                       55
#define TT_MAC_LANGID_TURKMEN                      56
#define TT_MAC_LANGID_MONGOLIAN                    57
#define TT_MAC_LANGID_MONGOLIAN_MONGOLIAN_SCRIPT   57
#define TT_MAC_LANGID_MONGOLIAN_CYRILLIC_SCRIPT    58
#define TT_MAC_LANGID_PASHTO                       59
#define TT_MAC_LANGID_KURDISH                      60
#define TT_MAC_LANGID_KASHMIRI                     61
#define TT_MAC_LANGID_SINDHI                       62
#define TT_MAC_LANGID_TIBETAN                      63
#define TT_MAC_LANGID_NEPALI                       64
#define TT_MAC_LANGID_SANSKRIT                     65
#define TT_MAC_LANGID_MARATHI                      66
#define TT_MAC_LANGID_BENGALI                      67
#define TT_MAC_LANGID_ASSAMESE                     68
#define TT_MAC_LANGID_GUJARATI                     69
#define TT_MAC_LANGID_PUNJABI                      70
#define TT_MAC_LANGID_ORIYA                        71
#define TT_MAC_LANGID_MALAYALAM                    72
#define TT_MAC_LANGID_KANNADA                      73
#define TT_MAC_LANGID_TAMIL                        74
#define TT_MAC_LANGID_TELUGU                       75
#define TT_MAC_LANGID_SINHALESE                    76
#define TT_MAC_LANGID_BURMESE                      77
#define TT_MAC_LANGID_KHMER                        78
#define TT_MAC_LANGID_LAO                          79
#define TT_MAC_LANGID_VIETNAMESE                   80
#define TT_MAC_LANGID_INDONESIAN                   81
#define TT_MAC_LANGID_TAGALOG                      82
#define TT_MAC_LANGID_MALAY_ROMAN_SCRIPT           83
#define TT_MAC_LANGID_MALAY_ARABIC_SCRIPT          84
#define TT_MAC_LANGID_AMHARIC                      85
#define TT_MAC_LANGID_TIGRINYA                     86
#define TT_MAC_LANGID_GALLA                        87
#define TT_MAC_LANGID_SOMALI                       88
#define TT_MAC_LANGID_SWAHILI                      89
#define TT_MAC_LANGID_RUANDA                       90
#define TT_MAC_LANGID_RUNDI                        91
#define TT_MAC_LANGID_CHEWA                        92
#define TT_MAC_LANGID_MALAGASY                     93
#define TT_MAC_LANGID_ESPERANTO                    94
#define TT_MAC_LANGID_WELSH                       128
#define TT_MAC_LANGID_BASQUE                      129
#define TT_MAC_LANGID_CATALAN                     130
#define TT_MAC_LANGID_LATIN                       131
#define TT_MAC_LANGID_QUECHUA                     132
#define TT_MAC_LANGID_GUARANI                     133
#define TT_MAC_LANGID_AYMARA                      134
#define TT_MAC_LANGID_TATAR                       135
#define TT_MAC_LANGID_UIGHUR                      136
#define TT_MAC_LANGID_DZONGKHA                    137
#define TT_MAC_LANGID_JAVANESE                    138
#define TT_MAC_LANGID_SUNDANESE                   139


#if 0  /* these seem to be errors that have been dropped */

#define TT_MAC_LANGID_SCOTTISH_GAELIC             140
#define TT_MAC_LANGID_IRISH_GAELIC                141

#endif


  /* The following codes are new as of 2000-03-10 */
#define TT_MAC_LANGID_GALICIAN                    140
#define TT_MAC_LANGID_AFRIKAANS                   141
#define TT_MAC_LANGID_BRETON                      142
#define TT_MAC_LANGID_INUKTITUT                   143
#define TT_MAC_LANGID_SCOTTISH_GAELIC             144
#define TT_MAC_LANGID_MANX_GAELIC                 145
#define TT_MAC_LANGID_IRISH_GAELIC                146
#define TT_MAC_LANGID_TONGAN                      147
#define TT_MAC_LANGID_GREEK_POLYTONIC             148
#define TT_MAC_LANGID_GREELANDIC                  149
#define TT_MAC_LANGID_AZERBAIJANI_ROMAN_SCRIPT    150


  /*************************************************************************/
  /*                                                                       */
  /* Possible values of the language identifier field in the name records  */
  /* of the TTF `name' table if the `platform' identifier code is          */
  /* TT_PLATFORM_MICROSOFT.                                                */
  /*                                                                       */
  /* The canonical source for the MS assigned LCID's is at                 */
  /*                                                                       */
  /*   http://www.microsoft.com/typography/OTSPEC/lcid-cp.txt              */
  /*                                                                       */
#define TT_MS_LANGID_ARABIC_SAUDI_ARABIA               0x0401
#define TT_MS_LANGID_ARABIC_IRAQ                       0x0801
#define TT_MS_LANGID_ARABIC_EGYPT                      0x0c01
#define TT_MS_LANGID_ARABIC_LIBYA                      0x1001
#define TT_MS_LANGID_ARABIC_ALGERIA                    0x1401
#define TT_MS_LANGID_ARABIC_MOROCCO                    0x1801
#define TT_MS_LANGID_ARABIC_TUNISIA                    0x1c01
#define TT_MS_LANGID_ARABIC_OMAN                       0x2001
#define TT_MS_LANGID_ARABIC_YEMEN                      0x2401
#define TT_MS_LANGID_ARABIC_SYRIA                      0x2801
#define TT_MS_LANGID_ARABIC_JORDAN                     0x2c01
#define TT_MS_LANGID_ARABIC_LEBANON                    0x3001
#define TT_MS_LANGID_ARABIC_KUWAIT                     0x3401
#define TT_MS_LANGID_ARABIC_UAE                        0x3801
#define TT_MS_LANGID_ARABIC_BAHRAIN                    0x3c01
#define TT_MS_LANGID_ARABIC_QATAR                      0x4001
#define TT_MS_LANGID_BULGARIAN_BULGARIA                0x0402
#define TT_MS_LANGID_CATALAN_SPAIN                     0x0403
#define TT_MS_LANGID_CHINESE_TAIWAN                    0x0404
#define TT_MS_LANGID_CHINESE_PRC                       0x0804
#define TT_MS_LANGID_CHINESE_HONG_KONG                 0x0c04
#define TT_MS_LANGID_CHINESE_SINGAPORE                 0x1004
#define TT_MS_LANGID_CHINESE_MACAU                     0x1404
#define TT_MS_LANGID_CZECH_CZECH_REPUBLIC              0x0405
#define TT_MS_LANGID_DANISH_DENMARK                    0x0406
#define TT_MS_LANGID_GERMAN_GERMANY                    0x0407
#define TT_MS_LANGID_GERMAN_SWITZERLAND                0x0807
#define TT_MS_LANGID_GERMAN_AUSTRIA                    0x0c07
#define TT_MS_LANGID_GERMAN_LUXEMBOURG                 0x1007
#define TT_MS_LANGID_GERMAN_LIECHTENSTEI               0x1407
#define TT_MS_LANGID_GREEK_GREECE                      0x0408
#define TT_MS_LANGID_ENGLISH_UNITED_STATES             0x0409
#define TT_MS_LANGID_ENGLISH_UNITED_KINGDOM            0x0809
#define TT_MS_LANGID_ENGLISH_AUSTRALIA                 0x0c09
#define TT_MS_LANGID_ENGLISH_CANADA                    0x1009
#define TT_MS_LANGID_ENGLISH_NEW_ZEALAND               0x1409
#define TT_MS_LANGID_ENGLISH_IRELAND                   0x1809
#define TT_MS_LANGID_ENGLISH_SOUTH_AFRICA              0x1c09
#define TT_MS_LANGID_ENGLISH_JAMAICA                   0x2009
#define TT_MS_LANGID_ENGLISH_CARIBBEAN                 0x2409
#define TT_MS_LANGID_ENGLISH_BELIZE                    0x2809
#define TT_MS_LANGID_ENGLISH_TRINIDAD                  0x2c09
#define TT_MS_LANGID_ENGLISH_ZIMBABWE                  0x3009
#define TT_MS_LANGID_ENGLISH_PHILIPPINES               0x3409
#define TT_MS_LANGID_SPANISH_SPAIN_TRADITIONAL_SORT    0x040a
#define TT_MS_LANGID_SPANISH_MEXICO                    0x080a
#define TT_MS_LANGID_SPANISH_SPAIN_INTERNATIONAL_SORT  0x0c0a
#define TT_MS_LANGID_SPANISH_GUATEMALA                 0x100a
#define TT_MS_LANGID_SPANISH_COSTA_RICA                0x140a
#define TT_MS_LANGID_SPANISH_PANAMA                    0x180a
#define TT_MS_LANGID_SPANISH_DOMINICAN_REPUBLIC        0x1c0a
#define TT_MS_LANGID_SPANISH_VENEZUELA                 0x200a
#define TT_MS_LANGID_SPANISH_COLOMBIA                  0x240a
#define TT_MS_LANGID_SPANISH_PERU                      0x280a
#define TT_MS_LANGID_SPANISH_ARGENTINA                 0x2c0a
#define TT_MS_LANGID_SPANISH_ECUADOR                   0x300a
#define TT_MS_LANGID_SPANISH_CHILE                     0x340a
#define TT_MS_LANGID_SPANISH_URUGUAY                   0x380a
#define TT_MS_LANGID_SPANISH_PARAGUAY                  0x3c0a
#define TT_MS_LANGID_SPANISH_BOLIVIA                   0x400a
#define TT_MS_LANGID_SPANISH_EL_SALVADOR               0x440a
#define TT_MS_LANGID_SPANISH_HONDURAS                  0x480a
#define TT_MS_LANGID_SPANISH_NICARAGUA                 0x4c0a
#define TT_MS_LANGID_SPANISH_PUERTO_RICO               0x500a
#define TT_MS_LANGID_FINNISH_FINLAND                   0x040b
#define TT_MS_LANGID_FRENCH_FRANCE                     0x040c
#define TT_MS_LANGID_FRENCH_BELGIUM                    0x080c
#define TT_MS_LANGID_FRENCH_CANADA                     0x0c0c
#define TT_MS_LANGID_FRENCH_SWITZERLAND                0x100c
#define TT_MS_LANGID_FRENCH_LUXEMBOURG                 0x140c
#define TT_MS_LANGID_FRENCH_MONACO                     0x180c
#define TT_MS_LANGID_HEBREW_ISRAEL                     0x040d
#define TT_MS_LANGID_HUNGARIAN_HUNGARY                 0x040e
#define TT_MS_LANGID_ICELANDIC_ICELAND                 0x040f
#define TT_MS_LANGID_ITALIAN_ITALY                     0x0410
#define TT_MS_LANGID_ITALIAN_SWITZERLAND               0x0810
#define TT_MS_LANGID_JAPANESE_JAPAN                    0x0411
#define TT_MS_LANGID_KOREAN_EXTENDED_WANSUNG_KOREA     0x0412
#define TT_MS_LANGID_KOREAN_JOHAB_KOREA                0x0812
#define TT_MS_LANGID_DUTCH_NETHERLANDS                 0x0413
#define TT_MS_LANGID_DUTCH_BELGIUM                     0x0813
#define TT_MS_LANGID_NORWEGIAN_NORWAY_BOKMAL           0x0414
#define TT_MS_LANGID_NORWEGIAN_NORWAY_NYNORSK          0x0814
#define TT_MS_LANGID_POLISH_POLAND                     0x0415
#define TT_MS_LANGID_PORTUGUESE_BRAZIL                 0x0416
#define TT_MS_LANGID_PORTUGUESE_PORTUGAL               0x0816
#define TT_MS_LANGID_RHAETO_ROMANIC_SWITZERLAND        0x0417
#define TT_MS_LANGID_ROMANIAN_ROMANIA                  0x0418
#define TT_MS_LANGID_MOLDAVIAN_MOLDAVIA                0x0818
#define TT_MS_LANGID_RUSSIAN_RUSSIA                    0x0419
#define TT_MS_LANGID_RUSSIAN_MOLDAVIA                  0x0819
#define TT_MS_LANGID_CROATIAN_CROATIA                  0x041a
#define TT_MS_LANGID_SERBIAN_SERBIA_LATIN              0x081a
#define TT_MS_LANGID_SERBIAN_SERBIA_CYRILLIC           0x0c1a
#define TT_MS_LANGID_SLOVAK_SLOVAKIA                   0x041b
#define TT_MS_LANGID_ALBANIAN_ALBANIA                  0x041c
#define TT_MS_LANGID_SWEDISH_SWEDEN                    0x041d
#define TT_MS_LANGID_SWEDISH_FINLAND                   0x081d
#define TT_MS_LANGID_THAI_THAILAND                     0x041e
#define TT_MS_LANGID_TURKISH_TURKEY                    0x041f
#define TT_MS_LANGID_URDU_PAKISTAN                     0x0420
#define TT_MS_LANGID_INDONESIAN_INDONESIA              0x0421
#define TT_MS_LANGID_UKRAINIAN_UKRAINE                 0x0422
#define TT_MS_LANGID_BELARUSIAN_BELARUS                0x0423
#define TT_MS_LANGID_SLOVENE_SLOVENIA                  0x0424
#define TT_MS_LANGID_ESTONIAN_ESTONIA                  0x0425
#define TT_MS_LANGID_LATVIAN_LATVIA                    0x0426
#define TT_MS_LANGID_LITHUANIAN_LITHUANIA              0x0427
#define TT_MS_LANGID_CLASSIC_LITHUANIAN_LITHUANIA      0x0827
#define TT_MS_LANGID_MAORI_NEW_ZEALAND                 0x0428
#define TT_MS_LANGID_FARSI_IRAN                        0x0429
#define TT_MS_LANGID_VIETNAMESE_VIET_NAM               0x042a
#define TT_MS_LANGID_ARMENIAN_ARMENIA                  0x042b
#define TT_MS_LANGID_AZERI_AZERBAIJAN_LATIN            0x042c
#define TT_MS_LANGID_AZERI_AZERBAIJAN_CYRILLIC         0x082c
#define TT_MS_LANGID_BASQUE_SPAIN                      0x042d
#define TT_MS_LANGID_SORBIAN_GERMANY                   0x042e
#define TT_MS_LANGID_MACEDONIAN_MACEDONIA              0x042f
#define TT_MS_LANGID_SUTU_SOUTH_AFRICA                 0x0430
#define TT_MS_LANGID_TSONGA_SOUTH_AFRICA               0x0431
#define TT_MS_LANGID_TSWANA_SOUTH_AFRICA               0x0432
#define TT_MS_LANGID_VENDA_SOUTH_AFRICA                0x0433
#define TT_MS_LANGID_XHOSA_SOUTH_AFRICA                0x0434
#define TT_MS_LANGID_ZULU_SOUTH_AFRICA                 0x0435
#define TT_MS_LANGID_AFRIKAANS_SOUTH_AFRICA            0x0436
#define TT_MS_LANGID_GEORGIAN_GEORGIA                  0x0437
#define TT_MS_LANGID_FAEROESE_FAEROE_ISLANDS           0x0438
#define TT_MS_LANGID_HINDI_INDIA                       0x0439
#define TT_MS_LANGID_MALTESE_MALTA                     0x043a
#define TT_MS_LANGID_SAAMI_LAPONIA                     0x043b
#define TT_MS_LANGID_IRISH_GAELIC_IRELAND              0x043c
#define TT_MS_LANGID_SCOTTISH_GAELIC_UNITED_KINGDOM    0x083c
#define TT_MS_LANGID_MALAY_MALAYSIA                    0x043e
#define TT_MS_LANGID_MALAY_BRUNEI_DARUSSALAM           0x083e
#define TT_MS_LANGID_KAZAK_KAZAKSTAN                   0x043f
#define TT_MS_LANGID_SWAHILI_KENYA                     0x0441
#define TT_MS_LANGID_UZBEK_UZBEKISTAN_LATIN            0x0443
#define TT_MS_LANGID_UZBEK_UZBEKISTAN_CYRILLIC         0x0843
#define TT_MS_LANGID_TATAR_TATARSTAN                   0x0444
#define TT_MS_LANGID_BENGALI_INDIA                     0x0445
#define TT_MS_LANGID_PUNJABI_INDIA                     0x0446
#define TT_MS_LANGID_GUJARATI_INDIA                    0x0447
#define TT_MS_LANGID_ORIYA_INDIA                       0x0448
#define TT_MS_LANGID_TAMIL_INDIA                       0x0449
#define TT_MS_LANGID_TELUGU_INDIA                      0x044a
#define TT_MS_LANGID_KANNADA_INDIA                     0x044b
#define TT_MS_LANGID_MALAYALAM_INDIA                   0x044c
#define TT_MS_LANGID_ASSAMESE_INDIA                    0x044d
#define TT_MS_LANGID_MARATHI_INDIA                     0x044e
#define TT_MS_LANGID_SANSKRIT_INDIA                    0x044f
#define TT_MS_LANGID_KONKANI_INDIA                     0x0457


  /*************************************************************************/
  /*                                                                       */
  /* Possible values of the `name' identifier field in the name records of */
  /* the TTF `name' table.  These values are platform independent.         */
  /*                                                                       */
#define TT_NAME_ID_COPYRIGHT            0
#define TT_NAME_ID_FONT_FAMILY          1
#define TT_NAME_ID_FONT_SUBFAMILY       2
#define TT_NAME_ID_UNIQUE_ID            3
#define TT_NAME_ID_FULL_NAME            4
#define TT_NAME_ID_VERSION_STRING       5
#define TT_NAME_ID_PS_NAME              6
#define TT_NAME_ID_TRADEMARK            7

/* the following values are from the OpenType spec */
#define TT_NAME_ID_MANUFACTURER         8
#define TT_NAME_ID_DESIGNER             9
#define TT_NAME_ID_DESCRIPTION          10
#define TT_NAME_ID_VENDOR_URL           11
#define TT_NAME_ID_DESIGNER_URL         12
#define TT_NAME_ID_LICENSE              13
#define TT_NAME_ID_LICENSE_URL          14
/* number 15 is reserved */
#define TT_NAME_ID_PREFERRED_FAMILY     16
#define TT_NAME_ID_PREFERRED_SUBFAMILY  17
#define TT_NAME_ID_MAC_FULL_NAME        18

/* The following code is new as of 2000-01-21 */
#define TT_NAME_ID_SAMPLE_TEXT          19


  /*************************************************************************/
  /*                                                                       */
  /* Bit mask values for the Unicode Ranges from the TTF `OS2 ' table.     */
  /*                                                                       */
  /* Updated 02-Jul-2000.                                                  */
  /*                                                                       */

  /* General Scripts Area */

  /* Bit  0   C0 Controls and Basic Latin */
#define TT_UCR_BASIC_LATIN                     (1L <<  0) /* U+0020-U+007E */
  /* Bit  1   C1 Controls and Latin-1 Supplement */
#define TT_UCR_LATIN1_SUPPLEMENT               (1L <<  1) /* U+00A0-U+00FF */
  /* Bit  2   Latin Extended-A */
#define TT_UCR_LATIN_EXTENDED_A                (1L <<  2) /* U+0100-U+017F */
  /* Bit  3   Latin Extended-B */
#define TT_UCR_LATIN_EXTENDED_B                (1L <<  3) /* U+0180-U+024F */
  /* Bit  4   IPA Extensions */
#define TT_UCR_IPA_EXTENSIONS                  (1L <<  4) /* U+0250-U+02AF */
  /* Bit  5   Spacing Modifier Letters */
#define TT_UCR_SPACING_MODIFIER                (1L <<  5) /* U+02B0-U+02FF */
  /* Bit  6   Combining Diacritical Marks */
#define TT_UCR_COMBINING_DIACRITICS            (1L <<  6) /* U+0300-U+036F */
  /* Bit  7   Greek */
#define TT_UCR_GREEK                           (1L <<  7) /* U+0370-U+03FF */
  /* Bit  8 is reserved (was: Greek Symbols and Coptic) */
  /* Bit  9   Cyrillic */
#define TT_UCR_CYRILLIC                        (1L <<  9) /* U+0400-U+04FF */
  /* Bit 10   Armenian */
#define TT_UCR_ARMENIAN                        (1L << 10) /* U+0530-U+058F */
  /* Bit 11   Hebrew */
#define TT_UCR_HEBREW                          (1L << 11) /* U+0590-U+05FF */
  /* Bit 12 is reserved (was: Hebrew Extended) */
  /* Bit 13   Arabic */
#define TT_UCR_ARABIC                          (1L << 13) /* U+0600-U+06FF */
  /* Bit 14 is reserved (was: Arabic Extended) */
  /* Bit 15   Devanagari */
#define TT_UCR_DEVANAGARI                      (1L << 15) /* U+0900-U+097F */
  /* Bit 16   Bengali */
#define TT_UCR_BENGALI                         (1L << 16) /* U+0980-U+09FF */
  /* Bit 17   Gurmukhi */
#define TT_UCR_GURMUKHI                        (1L << 17) /* U+0A00-U+0A7F */
  /* Bit 18   Gujarati */
#define TT_UCR_GUJARATI                        (1L << 18) /* U+0A80-U+0AFF */
  /* Bit 19   Oriya */
#define TT_UCR_ORIYA                           (1L << 19) /* U+0B00-U+0B7F */
  /* Bit 20   Tamil */
#define TT_UCR_TAMIL                           (1L << 20) /* U+0B80-U+0BFF */
  /* Bit 21   Telugu */
#define TT_UCR_TELUGU                          (1L << 21) /* U+0C00-U+0C7F */
  /* Bit 22   Kannada */
#define TT_UCR_KANNADA                         (1L << 22) /* U+0C80-U+0CFF */
  /* Bit 23   Malayalam */
#define TT_UCR_MALAYALAM                       (1L << 23) /* U+0D00-U+0D7F */
  /* Bit 24   Thai */
#define TT_UCR_THAI                            (1L << 24) /* U+0E00-U+0E7F */
  /* Bit 25   Lao */
#define TT_UCR_LAO                             (1L << 25) /* U+0E80-U+0EFF */
  /* Bit 26   Georgian */
#define TT_UCR_GEORGIAN                        (1L << 26) /* U+10A0-U+10FF */
  /* Bit 27 is reserved (was Georgian Extended) */
  /* Bit 28   Hangul Jamo */
#define TT_UCR_HANGUL_JAMO                     (1L << 28) /* U+1100-U+11FF */
  /* Bit 29   Latin Extended Additional */
#define TT_UCR_LATIN_EXTENDED_ADDITIONAL       (1L << 29) /* U+1E00-U+1EFF */
  /* Bit 30   Greek Extended */
#define TT_UCR_GREEK_EXTENDED                  (1L << 30) /* U+1F00-U+1FFF */

  /* Symbols Area */

  /* Bit 31   General Punctuation */
#define TT_UCR_GENERAL_PUNCTUATION             (1L << 31) /* U+2000-U+206F */
  /* Bit 32   Superscripts And Subscripts */
#define TT_UCR_SUPERSCRIPTS_SUBSCRIPTS         (1L <<  0) /* U+2070-U+209F */
  /* Bit 33   Currency Symbols */
#define TT_UCR_CURRENCY_SYMBOLS                (1L <<  1) /* U+20A0-U+20CF */
  /* Bit 34   Combining Diacritical Marks For Symbols */
#define TT_UCR_COMBINING_DIACRITICS_SYMB       (1L <<  2) /* U+20D0-U+20FF */
  /* Bit 35   Letterlike Symbols */
#define TT_UCR_LETTERLIKE_SYMBOLS              (1L <<  3) /* U+2100-U+214F */
  /* Bit 36   Number Forms */
#define TT_UCR_NUMBER_FORMS                    (1L <<  4) /* U+2150-U+218F */
  /* Bit 37   Arrows */
#define TT_UCR_ARROWS                          (1L <<  5) /* U+2190-U+21FF */
  /* Bit 38   Mathematical Operators */
#define TT_UCR_MATHEMATICAL_OPERATORS          (1L <<  6) /* U+2200-U+22FF */
  /* Bit 39 Miscellaneous Technical */
#define TT_UCR_MISCELLANEOUS_TECHNICAL         (1L <<  7) /* U+2300-U+23FF */
  /* Bit 40   Control Pictures */
#define TT_UCR_CONTROL_PICTURES                (1L <<  8) /* U+2400-U+243F */
  /* Bit 41   Optical Character Recognition */
#define TT_UCR_OCR                             (1L <<  9) /* U+2440-U+245F */
  /* Bit 42   Enclosed Alphanumerics */
#define TT_UCR_ENCLOSED_ALPHANUMERICS          (1L << 10) /* U+2460-U+24FF */
  /* Bit 43   Box Drawing */
#define TT_UCR_BOX_DRAWING                     (1L << 11) /* U+2500-U+257F */
  /* Bit 44   Block Elements */
#define TT_UCR_BLOCK_ELEMENTS                  (1L << 12) /* U+2580-U+259F */
  /* Bit 45   Geometric Shapes */
#define TT_UCR_GEOMETRIC_SHAPES                (1L << 13) /* U+25A0-U+25FF */
  /* Bit 46   Miscellaneous Symbols */
#define TT_UCR_MISCELLANEOUS_SYMBOLS           (1L << 14) /* U+2600-U+26FF */
  /* Bit 47   Dingbats */
#define TT_UCR_DINGBATS                        (1L << 15) /* U+2700-U+27BF */

  /* CJK Phonetics and Symbols Area */

  /* Bit 48   CJK Symbols And Punctuation */
#define TT_UCR_CJK_SYMBOLS                     (1L << 16) /* U+3000-U+303F */
  /* Bit 49   Hiragana */
#define TT_UCR_HIRAGANA                        (1L << 17) /* U+3040-U+309F */
  /* Bit 50   Katakana */
#define TT_UCR_KATAKANA                        (1L << 18) /* U+30A0-U+30FF */
  /* Bit 51   Bopomofo + Extended Bopomofo */
#define TT_UCR_BOPOMOFO                        (1L << 19) /* U+3100-U+312F */
                                                          /* U+31A0-U+31BF */
  /* Bit 52   Hangul Compatibility Jamo */
#define TT_UCR_HANGUL_COMPATIBILITY_JAMO       (1L << 20) /* U+3130-U+318F */
  /* Bit 53   CJK Miscellaneous */
#define TT_UCR_CJK_MISC                        (1L << 21) /* U+3190-U+319F */
  /* Bit 54   Enclosed CJK Letters And Months */
#define TT_UCR_ENCLOSED_CJK_LETTERS_MONTHS     (1L << 22) /* U+3200-U+32FF */
  /* Bit 55   CJK Compatibility */
#define TT_UCR_CJK_COMPATIBILITY               (1L << 23) /* U+3300-U+33FF */

  /* Hangul Syllables Area */

  /* Bit 56   Hangul */
#define TT_UCR_HANGUL                          (1L << 24) /* U+AC00-U+D7A3 */

  /* Surrogates Area */

  /* Bit 57   Surrogates */
#define TT_UCR_SURROGATES                      (1L << 25) /* U+D800-U+DFFF */
  /* Bit 58 is reserved for Unicode SubRanges */

  /* CJK Ideographs Area */

  /* Bit 59   CJK Unified Ideographs             + */
  /*          CJK Radical Supplement             + */
  /*          Kangxi Radicals                    + */
  /*          Ideographic Description            + */
  /*          CJK Unified Ideographs Extension A   */
#define TT_UCR_CJK_UNIFIED_IDEOGRAPHS          (1L << 27) /* U+4E00-U+9FFF */
                                                          /* U+2E80-U+2EFF */
                                                          /* U+2F00-U+2FDF */
                                                          /* U+2FF0-U+2FFF */
                                                          /* U+34E0-U+4DB5 */

  /* Private Use Area */

  /* Bit 60   Private Use */
#define TT_UCR_PRIVATE_USE                     (1L << 28) /* U+E000-U+F8FF */

  /* Compatibility Area and Specials */

  /* Bit 61   CJK Compatibility Ideographs */
#define TT_UCR_CJK_COMPATIBILITY_IDEOGRAPHS    (1L << 29) /* U+F900-U+FAFF */
  /* Bit 62   Alphabetic Presentation Forms */
#define TT_UCR_ALPHABETIC_PRESENTATION_FORMS   (1L << 30) /* U+FB00-U+FB4F */
  /* Bit 63   Arabic Presentation Forms-A */
#define TT_UCR_ARABIC_PRESENTATIONS_A          (1L << 31) /* U+FB50-U+FDFF */
  /* Bit 64   Combining Half Marks */
#define TT_UCR_COMBINING_HALF_MARKS            (1L <<  0) /* U+FE20-U+FE2F */
  /* Bit 65   CJK Compatibility Forms */
#define TT_UCR_CJK_COMPATIBILITY_FORMS         (1L <<  1) /* U+FE30-U+FE4F */
  /* Bit 66   Small Form Variants */
#define TT_UCR_SMALL_FORM_VARIANTS             (1L <<  2) /* U+FE50-U+FE6F */
  /* Bit 67   Arabic Presentation Forms-B */
#define TT_UCR_ARABIC_PRESENTATIONS_B          (1L <<  3) /* U+FE70-U+FEFE */
  /* Bit 68   Halfwidth And Fullwidth Forms */
#define TT_UCR_HALFWIDTH_FULLWIDTH_FORMS       (1L <<  4) /* U+FF00-U+FFEF */
  /* Bit 69   Specials */
#define TT_UCR_SPECIALS                        (1L <<  5) /* U+FFF0-U+FFFD */
  /* Bit 70   Tibetan */
#define TT_UCR_TIBETAN                         (1L <<  6) /* U+0F00-U+0FCF */
  /* Bit 71   Syriac */
#define TT_UCR_SYRIAC                          (1L <<  7) /* U+0700-U+074F */
  /* Bit 72   Thaana */
#define TT_UCR_THAANA                          (1L <<  8) /* U+0780-U+07BF */
  /* Bit 73   Sinhala */
#define TT_UCR_SINHALA                         (1L <<  9) /* U+0D80-U+0DFF */
  /* Bit 74   Myanmar */
#define TT_UCR_MYANMAR                         (1L << 10) /* U+1000-U+109F */
  /* Bit 75   Ethiopic */
#define TT_UCR_ETHIOPIC                        (1L << 11) /* U+1200-U+12BF */
  /* Bit 76   Cherokee */
#define TT_UCR_CHEROKEE                        (1L << 12) /* U+13A0-U+13FF */
  /* Bit 77   Canadian Aboriginal Syllabics */
#define TT_UCR_CANADIAN_ABORIGINAL_SYLLABICS   (1L << 13) /* U+1400-U+14DF */
  /* Bit 78   Ogham */
#define TT_UCR_OGHAM                           (1L << 14) /* U+1680-U+169F */
  /* Bit 79   Runic */
#define TT_UCR_RUNIC                           (1L << 15) /* U+16A0-U+16FF */
  /* Bit 80   Khmer */
#define TT_UCR_KHMER                           (1L << 16) /* U+1780-U+17FF */
  /* Bit 81   Mongolian */
#define TT_UCR_MONGOLIAN                       (1L << 17) /* U+1800-U+18AF */
  /* Bit 82   Braille */
#define TT_UCR_BRAILLE                         (1L << 18) /* U+2800-U+28FF */
  /* Bit 83   Yi + Yi Radicals */
#define TT_UCR_YI                              (1L << 19) /* U+A000-U+A48C */
                                                          /* U+A490-U+A4CF */


  /*************************************************************************/
  /*                                                                       */
  /* Some compilers have a very limited length of identifiers.             */
  /*                                                                       */
#if defined( __TURBOC__ ) && __TURBOC__ < 0x0410 || defined( __PACIFIC__ )
#define HAVE_LIMIT_ON_IDENTS
#endif


#ifndef HAVE_LIMIT_ON_IDENTS


  /*************************************************************************/
  /*                                                                       */
  /* Here some alias #defines in order to be clearer.                      */
  /*                                                                       */
  /* These are not always #defined to stay within the 31 character limit   */
  /* which some compilers have.                                            */
  /*                                                                       */
  /* Credits go to Dave Hoo <dhoo@flash.net> for pointing out that modern  */
  /* Borland compilers (read: from BC++ 3.1 on) can increase this limit.   */
  /* If you get a warning with such a compiler, use the -i40 switch.       */
  /*                                                                       */
#define TT_UCR_ARABIC_PRESENTATION_FORMS_A      \
         TT_UCR_ARABIC_PRESENTATIONS_A
#define TT_UCR_ARABIC_PRESENTATION_FORMS_B      \
         TT_UCR_ARABIC_PRESENTATIONS_B

#define TT_UCR_COMBINING_DIACRITICAL_MARKS      \
         TT_UCR_COMBINING_DIACRITICS
#define TT_UCR_COMBINING_DIACRITICAL_MARKS_SYMB \
         TT_UCR_COMBINING_DIACRITICS_SYMB


#endif /* !HAVE_LIMIT_ON_IDENTS */


#ifdef __cplusplus
  }
#endif


#endif /* __TTNAMEID_H__ */


/* END */
