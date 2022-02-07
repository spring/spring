/*
 * Copyright 2008  Veselin Georgiev,
 * anrieffNOSPAM @ mgail_DOT.com (convert to gmail)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "libcpuid.h"
#include "libcpuid_util.h"
#include "libcpuid_internal.h"
#include "recog_amd.h"

const struct amd_code_str { amd_code_t code; char *str; } amd_code_str[] = {
	#define CODE(x) { x, #x }
	#define CODE2(x, y) CODE(x)
	#include "amd_code_t.h"
	#undef CODE
};

struct amd_code_and_bits_t {
	int code;
	uint64_t bits;
};


const struct match_entry_t cpudb_amd[] = {
//     F   M   S  EF  EM  #cores  L2$   L3$  BC  ModelBits     ModelCode  Name
	{ -1, -1, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Unknown AMD CPU"               },

	/* 486 and the likes */
	{  4, -1, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Unknown AMD 486"               },
	{  4,  3, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "AMD 486DX2"                    },
	{  4,  7, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "AMD 486DX2WB"                  },
	{  4,  8, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "AMD 486DX4"                    },
	{  4,  9, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "AMD 486DX4WB"                  },

	/* Pentia clones */
	{  5, -1, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Unknown AMD 586"               },
	{  5,  0, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "K5"                            },
	{  5,  1, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "K5"                            },
	{  5,  2, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "K5"                            },
	{  5,  3, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "K5"                            },

	/* The K6 */
	{  5,  6, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "K6"                            },
	{  5,  7, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "K6"                            },

	{  5,  8, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "K6-2"                          },
	{  5,  9, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "K6-III"                        },
	{  5, 10, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Unknown K6"                    },
	{  5, 11, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Unknown K6"                    },
	{  5, 12, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Unknown K6"                    },
	{  5, 13, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "K6-2+"                         },

	/* Athlon et al. */
	{  6,  1, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Athlon (Slot-A)"               },
	{  6,  2, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Athlon (Slot-A)"               },
	{  6,  3, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Duron (Spitfire)"              },
	{  6,  4, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Athlon (ThunderBird)"          },

	{  6,  6, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Unknown Athlon"                },
	{  6,  6, -1, -1,   -1,   1,    -1,    -1, NC, ATHLON_             ,     0, "Athlon (Palomino)"             },
	{  6,  6, -1, -1,   -1,   1,    -1,    -1, NC, ATHLON_|_MP_        ,     0, "Athlon MP (Palomino)"          },
	{  6,  6, -1, -1,   -1,   1,    -1,    -1, NC, DURON_              ,     0, "Duron (Palomino)"              },
	{  6,  6, -1, -1,   -1,   1,    -1,    -1, NC, ATHLON_|_XP_        ,     0, "Athlon XP"                     },

	{  6,  7, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Unknown Athlon XP"             },
	{  6,  7, -1, -1,   -1,   1,    -1,    -1, NC, DURON_              ,     0, "Duron (Morgan)"                },

	{  6,  8, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Athlon XP"                     },
	{  6,  8, -1, -1,   -1,   1,    -1,    -1, NC, ATHLON_             ,     0, "Athlon XP (Thoroughbred)"      },
	{  6,  8, -1, -1,   -1,   1,    -1,    -1, NC, ATHLON_|_XP_        ,     0, "Athlon XP (Thoroughbred)"      },
	{  6,  8, -1, -1,   -1,   1,    -1,    -1, NC, DURON_              ,     0, "Duron (Applebred)"             },
	{  6,  8, -1, -1,   -1,   1,    -1,    -1, NC, SEMPRON_            ,     0, "Sempron (Thoroughbred)"        },
	{  6,  8, -1, -1,   -1,   1,   128,    -1, NC, SEMPRON_            ,     0, "Sempron (Thoroughbred)"        },
	{  6,  8, -1, -1,   -1,   1,   256,    -1, NC, SEMPRON_            ,     0, "Sempron (Thoroughbred)"        },
	{  6,  8, -1, -1,   -1,   1,    -1,    -1, NC, ATHLON_|_MP_        ,     0, "Athlon MP (Thoroughbred)"      },
	{  6,  8, -1, -1,   -1,   1,    -1,    -1, NC, ATHLON_|_XP_|_M_    ,     0, "Mobile Athlon (T-Bred)"        },
	{  6,  8, -1, -1,   -1,   1,    -1,    -1, NC, ATHLON_|_XP_|_M_|_LV_,    0, "Mobile Athlon (T-Bred)"        },

	{  6, 10, -1, -1,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Athlon XP (Barton)"            },
	{  6, 10, -1, -1,   -1,   1,   512,    -1, NC, ATHLON_|_XP_        ,     0, "Athlon XP (Barton)"            },
	{  6, 10, -1, -1,   -1,   1,   512,    -1, NC, SEMPRON_            ,     0, "Sempron (Barton)"              },
	{  6, 10, -1, -1,   -1,   1,   256,    -1, NC, SEMPRON_            ,     0, "Sempron (Thorton)"             },
	{  6, 10, -1, -1,   -1,   1,   256,    -1, NC, ATHLON_|_XP_        ,     0, "Athlon XP (Thorton)"           },
	{  6, 10, -1, -1,   -1,   1,    -1,    -1, NC, ATHLON_|_MP_        ,     0, "Athlon MP (Barton)"            },
	{  6, 10, -1, -1,   -1,   1,    -1,    -1, NC, ATHLON_|_XP_|_M_    ,     0, "Mobile Athlon (Barton)"        },
	{  6, 10, -1, -1,   -1,   1,    -1,    -1, NC, ATHLON_|_XP_|_M_|_LV_,    0, "Mobile Athlon (Barton)"        },

	/* K8 Architecture */
	{ 15, -1, -1, 15,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Unknown K8"                    },
	{ 15, -1, -1, 16,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Unknown K9"                    },

	{ 15, -1, -1, 15,   -1,   1,    -1,    -1, NC, 0                   ,     0, "Unknown A64"                   },
	{ 15, -1, -1, 15,   -1,   1,    -1,    -1, NC, OPTERON_            ,     0, "Opteron"                       },
	{ 15, -1, -1, 15,   -1,   2,    -1,    -1, NC, OPTERON_|_X2        ,     0, "Opteron (Dual Core)"           },
	{ 15,  3, -1, 15,   -1,   1,    -1,    -1, NC, OPTERON_            ,     0, "Opteron"                       },
	{ 15,  3, -1, 15,   -1,   2,    -1,    -1, NC, OPTERON_|_X2        ,     0, "Opteron (Dual Core)"           },
	{ 15, -1, -1, 15,   -1,   1,   512,    -1, NC, ATHLON_|_64_        ,     0, "Athlon 64 (512K)"              },
	{ 15, -1, -1, 15,   -1,   1,  1024,    -1, NC, ATHLON_|_64_        ,     0, "Athlon 64 (1024K)"             },
	{ 15, -1, -1, 15,   -1,   1,    -1,    -1, NC, ATHLON_|_FX         ,     0, "Athlon FX"                     },
	{ 15, -1, -1, 15,   -1,   1,    -1,    -1, NC, ATHLON_|_64_|_FX    ,     0, "Athlon 64 FX"                  },
	{ 15,  3, -1, 15,   35,   2,    -1,    -1, NC, ATHLON_|_64_|_FX    ,     0, "Athlon 64 FX X2 (Toledo)"      },
	{ 15, -1, -1, 15,   -1,   2,   512,    -1, NC, ATHLON_|_64_|_X2    ,     0, "Athlon 64 X2 (512K)"           },
	{ 15, -1, -1, 15,   -1,   2,  1024,    -1, NC, ATHLON_|_64_|_X2    ,     0, "Athlon 64 X2 (1024K)"          },
	{ 15, -1, -1, 15,   -1,   1,   512,    -1, NC, TURION_|_64_        ,     0, "Turion 64 (512K)"              },
	{ 15, -1, -1, 15,   -1,   1,  1024,    -1, NC, TURION_|_64_        ,     0, "Turion 64 (1024K)"             },
	{ 15, -1, -1, 15,   -1,   2,   512,    -1, NC, TURION_|_X2         ,     0, "Turion 64 X2 (512K)"           },
	{ 15, -1, -1, 15,   -1,   2,  1024,    -1, NC, TURION_|_X2         ,     0, "Turion 64 X2 (1024K)"          },
	{ 15, -1, -1, 15,   -1,   1,   128,    -1, NC, SEMPRON_            ,     0, "A64 Sempron (128K)"            },
	{ 15, -1, -1, 15,   -1,   1,   256,    -1, NC, SEMPRON_            ,     0, "A64 Sempron (256K)"            },
	{ 15, -1, -1, 15,   -1,   1,   512,    -1, NC, SEMPRON_            ,     0, "A64 Sempron (512K)"            },
	{ 15, -1, -1, 15, 0x4f,   1,   512,    -1, NC, ATHLON_|_64_        ,     0, "Athlon 64 (Orleans/512K)"      },
	{ 15, -1, -1, 15, 0x5f,   1,   512,    -1, NC, ATHLON_|_64_        ,     0, "Athlon 64 (Orleans/512K)"      },
	{ 15, -1, -1, 15, 0x2f,   1,   512,    -1, NC, ATHLON_|_64_        ,     0, "Athlon 64 (Venice/512K)"       },
	{ 15, -1, -1, 15, 0x2c,   1,   512,    -1, NC, ATHLON_|_64_        ,     0, "Athlon 64 (Venice/512K)"       },
	{ 15, -1, -1, 15, 0x1f,   1,   512,    -1, NC, ATHLON_|_64_        ,     0, "Athlon 64 (Winchester/512K)"   },
	{ 15, -1, -1, 15, 0x0c,   1,   512,    -1, NC, ATHLON_|_64_        ,     0, "Athlon 64 (Newcastle/512K)"    },
	{ 15, -1, -1, 15, 0x27,   1,   512,    -1, NC, ATHLON_|_64_        ,     0, "Athlon 64 (San Diego/512K)"    },
	{ 15, -1, -1, 15, 0x37,   1,   512,    -1, NC, ATHLON_|_64_        ,     0, "Athlon 64 (San Diego/512K)"    },
	{ 15, -1, -1, 15, 0x04,   1,   512,    -1, NC, ATHLON_|_64_        ,     0, "Athlon 64 (ClawHammer/512K)"   },

	{ 15, -1, -1, 15, 0x5f,   1,  1024,    -1, NC, ATHLON_|_64_        ,     0, "Athlon 64 (Orleans/1024K)"     },
	{ 15, -1, -1, 15, 0x27,   1,  1024,    -1, NC, ATHLON_|_64_        ,     0, "Athlon 64 (San Diego/1024K)"   },
	{ 15, -1, -1, 15, 0x04,   1,  1024,    -1, NC, ATHLON_|_64_        ,     0, "Athlon 64 (ClawHammer/1024K)"  },

	{ 15, -1, -1, 15, 0x4b,   2,   256,    -1, NC, SEMPRON_            ,     0, "Athlon 64 X2 (Windsor/256K)"   },

	{ 15, -1, -1, 15, 0x23,   2,   512,    -1, NC, ATHLON_|_64_|_X2    ,     0, "Athlon 64 X2 (Toledo/512K)"    },
	{ 15, -1, -1, 15, 0x4b,   2,   512,    -1, NC, ATHLON_|_64_|_X2    ,     0, "Athlon 64 X2 (Windsor/512K)"   },
	{ 15, -1, -1, 15, 0x43,   2,   512,    -1, NC, ATHLON_|_64_|_X2    ,     0, "Athlon 64 X2 (Windsor/512K)"   },
	{ 15, -1, -1, 15, 0x6b,   2,   512,    -1, NC, ATHLON_|_64_|_X2    ,     0, "Athlon 64 X2 (Brisbane/512K)"  },
	{ 15, -1, -1, 15, 0x2b,   2,   512,    -1, NC, ATHLON_|_64_|_X2    ,     0, "Athlon 64 X2 (Manchester/512K)"},

	{ 15, -1, -1, 15, 0x23,   2,  1024,    -1, NC, ATHLON_|_64_|_X2    ,     0, "Athlon 64 X2 (Toledo/1024K)"   },
	{ 15, -1, -1, 15, 0x43,   2,  1024,    -1, NC, ATHLON_|_64_|_X2    ,     0, "Athlon 64 X2 (Windsor/1024K)"  },

	{ 15, -1, -1, 15, 0x08,   1,   128,    -1, NC, MOBILE_|SEMPRON_    ,     0, "Mobile Sempron 64 (Dublin/128K)"},
	{ 15, -1, -1, 15, 0x08,   1,   256,    -1, NC, MOBILE_|SEMPRON_    ,     0, "Mobile Sempron 64 (Dublin/256K)"},
	{ 15, -1, -1, 15, 0x0c,   1,   256,    -1, NC, SEMPRON_            ,     0, "Sempron 64 (Paris)"            },
	{ 15, -1, -1, 15, 0x1c,   1,   128,    -1, NC, SEMPRON_            ,     0, "Sempron 64 (Palermo/128K)"     },
	{ 15, -1, -1, 15, 0x1c,   1,   256,    -1, NC, SEMPRON_            ,     0, "Sempron 64 (Palermo/256K)"     },
	{ 15, -1, -1, 15, 0x1c,   1,   128,    -1, NC, MOBILE_| SEMPRON_   ,     0, "Mobile Sempron 64 (Sonora/128K)"},
	{ 15, -1, -1, 15, 0x1c,   1,   256,    -1, NC, MOBILE_| SEMPRON_   ,     0, "Mobile Sempron 64 (Sonora/256K)"},
	{ 15, -1, -1, 15, 0x2c,   1,   128,    -1, NC, SEMPRON_            ,     0, "Sempron 64 (Palermo/128K)"     },
	{ 15, -1, -1, 15, 0x2c,   1,   256,    -1, NC, SEMPRON_            ,     0, "Sempron 64 (Palermo/256K)"     },
	{ 15, -1, -1, 15, 0x2c,   1,   128,    -1, NC, MOBILE_| SEMPRON_   ,     0, "Mobile Sempron 64 (Albany/128K)"},
	{ 15, -1, -1, 15, 0x2c,   1,   256,    -1, NC, MOBILE_| SEMPRON_   ,     0, "Mobile Sempron 64 (Albany/256K)"},
	{ 15, -1, -1, 15, 0x2f,   1,   128,    -1, NC, SEMPRON_            ,     0, "Sempron 64 (Palermo/128K)"     },
	{ 15, -1, -1, 15, 0x2f,   1,   256,    -1, NC, SEMPRON_            ,     0, "Sempron 64 (Palermo/256K)"     },
	{ 15, -1, -1, 15, 0x4f,   1,   128,    -1, NC, SEMPRON_            ,     0, "Sempron 64 (Manila/128K)"      },
	{ 15, -1, -1, 15, 0x4f,   1,   256,    -1, NC, SEMPRON_            ,     0, "Sempron 64 (Manila/256K)"      },
	{ 15, -1, -1, 15, 0x5f,   1,   128,    -1, NC, SEMPRON_            ,     0, "Sempron 64 (Manila/128K)"      },
	{ 15, -1, -1, 15, 0x5f,   1,   256,    -1, NC, SEMPRON_            ,     0, "Sempron 64 (Manila/256K)"      },
	{ 15, -1, -1, 15, 0x6b,   2,   256,    -1, NC, SEMPRON_            ,     0, "Sempron 64 Dual (Sherman/256K)"},
	{ 15, -1, -1, 15, 0x6b,   2,   512,    -1, NC, SEMPRON_            ,     0, "Sempron 64 Dual (Sherman/512K)"},
	{ 15, -1, -1, 15, 0x7f,   1,   256,    -1, NC, SEMPRON_            ,     0, "Sempron 64 (Sparta/256K)"      },
	{ 15, -1, -1, 15, 0x7f,   1,   512,    -1, NC, SEMPRON_            ,     0, "Sempron 64 (Sparta/512K)"      },
	{ 15, -1, -1, 15, 0x4c,   1,   256,    -1, NC, MOBILE_| SEMPRON_   ,     0, "Mobile Sempron 64 (Keene/256K)"},
	{ 15, -1, -1, 15, 0x4c,   1,   512,    -1, NC, MOBILE_| SEMPRON_   ,     0, "Mobile Sempron 64 (Keene/512K)"},
	{ 15, -1, -1, 15,   -1,   2,    -1,    -1, NC, SEMPRON_            ,     0, "Sempron Dual Core"             },

	{ 15, -1, -1, 15, 0x24,   1,   512,    -1, NC, TURION_|_64_        ,     0, "Turion 64 (Lancaster/512K)"    },
	{ 15, -1, -1, 15, 0x24,   1,  1024,    -1, NC, TURION_|_64_        ,     0, "Turion 64 (Lancaster/1024K)"   },
	{ 15, -1, -1, 15, 0x48,   2,   256,    -1, NC, TURION_|_X2         ,     0, "Turion X2 (Taylor)"            },
	{ 15, -1, -1, 15, 0x48,   2,   512,    -1, NC, TURION_|_X2         ,     0, "Turion X2 (Trinidad)"          },
	{ 15, -1, -1, 15, 0x4c,   1,   512,    -1, NC, TURION_|_64_        ,     0, "Turion 64 (Richmond)"          },
	{ 15, -1, -1, 15, 0x68,   2,   256,    -1, NC, TURION_|_X2         ,     0, "Turion X2 (Tyler/256K)"        },
	{ 15, -1, -1, 15, 0x68,   2,   512,    -1, NC, TURION_|_X2         ,     0, "Turion X2 (Tyler/512K)"        },
	{ 15, -1, -1, 17,    3,   2,   512,    -1, NC, TURION_|_X2         ,     0, "Turion X2 (Griffin/512K)"      },
	{ 15, -1, -1, 17,    3,   2,  1024,    -1, NC, TURION_|_X2         ,     0, "Turion X2 (Griffin/1024K)"     },

	/* K10 Architecture (2007) */
	{ 15, -1, -1, 16,   -1,   1,    -1,    -1, PHENOM, 0               ,     0, "Unknown AMD Phenom"            },
	{ 15,  2, -1, 16,   -1,   1,    -1,    -1, PHENOM, 0               ,     0, "Phenom"                        },
	{ 15,  2, -1, 16,   -1,   3,    -1,    -1, PHENOM, 0               ,     0, "Phenom X3 (Toliman)"           },
	{ 15,  2, -1, 16,   -1,   4,    -1,    -1, PHENOM, 0               ,     0, "Phenom X4 (Agena)"             },
	{ 15,  2, -1, 16,   -1,   3,   512,    -1, PHENOM, 0               ,     0, "Phenom X3 (Toliman/256K)"      },
	{ 15,  2, -1, 16,   -1,   3,   512,    -1, PHENOM, 0               ,     0, "Phenom X3 (Toliman/512K)"      },
	{ 15,  2, -1, 16,   -1,   4,   128,    -1, PHENOM, 0               ,     0, "Phenom X4 (Agena/128K)"        },
	{ 15,  2, -1, 16,   -1,   4,   256,    -1, PHENOM, 0               ,     0, "Phenom X4 (Agena/256K)"        },
	{ 15,  2, -1, 16,   -1,   4,   512,    -1, PHENOM,  0              ,     0, "Phenom X4 (Agena/512K)"        },
	{ 15,  2, -1, 16,   -1,   2,   512,    -1, NC, ATHLON_|_64_|_X2    ,     0, "Athlon X2 (Kuma)"              },
	/* Phenom II derivates: */
	{ 15,  4, -1, 16,   -1,   4,    -1,    -1, NC, 0                   ,     0, "Phenom (Deneb-based)"          },
	{ 15,  4, -1, 16,   -1,   1,  1024,    -1, NC, SEMPRON_            ,     0, "Sempron (Sargas)"              },
	{ 15,  4, -1, 16,   -1,   2,   512,    -1, PHENOM2, 0              ,     0, "Phenom II X2 (Callisto)"       },
	{ 15,  4, -1, 16,   -1,   3,   512,    -1, PHENOM2, 0              ,     0, "Phenom II X3 (Heka)"           },
	{ 15,  4, -1, 16,   -1,   4,   512,    -1, PHENOM2, 0              ,     0, "Phenom II X4"                  },
	{ 15,  4, -1, 16,    4,   4,   512,    -1, PHENOM2, 0              ,     0, "Phenom II X4 (Deneb)"          },
	{ 15,  5, -1, 16,    5,   4,   512,    -1, PHENOM2, 0              ,     0, "Phenom II X4 (Deneb)"          },
	{ 15,  4, -1, 16,   10,   4,   512,    -1, PHENOM2, 0              ,     0, "Phenom II X4 (Zosma)"          },
	{ 15,  4, -1, 16,   10,   6,   512,    -1, PHENOM2, 0              ,     0, "Phenom II X6 (Thuban)"         },
	/* Athlon II derivates: */
	{ 15,  6, -1, 16,    6,   2,   512,    -1, NC, ATHLON_|_X2         ,     0, "Athlon II (Champlain)"         },
	{ 15,  6, -1, 16,    6,   2,   512,    -1, NC, ATHLON_|_64_|_X2    ,     0, "Athlon II X2 (Regor)"          },
	{ 15,  6, -1, 16,    6,   2,  1024,    -1, NC, ATHLON_|_64_|_X2    ,     0, "Athlon II X2 (Regor)"          },
	{ 15,  5, -1, 16,    5,   3,   512,    -1, NC, ATHLON_|_64_|_X3    ,     0, "Athlon II X3 (Rana)"           },
	{ 15,  5, -1, 16,    5,   4,   512,    -1, NC, ATHLON_|_64_|_X4    ,     0, "Athlon II X4 (Propus)"         },
	/* Llano APUs (2011): */
	{ 15,  1, -1, 18,    1,   2,    -1,    -1, FUSION_EA, 0            ,     0, "Llano X2"                      },
	{ 15,  1, -1, 18,    1,   3,    -1,    -1, FUSION_EA, 0            ,     0, "Llano X3"                      },
	{ 15,  1, -1, 18,    1,   4,    -1,    -1, FUSION_EA, 0            ,     0, "Llano X4"                      },

	/* Family 14h: Bobcat Architecture (2011) */
	{ 15,  2, -1, 20,   -1,   1,    -1,    -1, FUSION_C, 0             ,     0, "Brazos Ontario"                },
	{ 15,  2, -1, 20,   -1,   2,    -1,    -1, FUSION_C, 0             ,     0, "Brazos Ontario (Dual-core)"    },
	{ 15,  1, -1, 20,   -1,   1,    -1,    -1, FUSION_E, 0             ,     0, "Brazos Zacate"                 },
	{ 15,  1, -1, 20,   -1,   2,    -1,    -1, FUSION_E, 0             ,     0, "Brazos Zacate (Dual-core)"     },
	{ 15,  2, -1, 20,   -1,   2,    -1,    -1, FUSION_Z, 0             ,     0, "Brazos Desna (Dual-core)"      },

	/* Family 15h: Bulldozer Architecture (2011) */
	{ 15, -1, -1, 21,    0,   4,    -1,    -1, NC, 0                   ,     0, "Bulldozer X2"                  },
	{ 15, -1, -1, 21,    1,   4,    -1,    -1, NC, 0                   ,     0, "Bulldozer X2"                  },
	{ 15, -1, -1, 21,    1,   6,    -1,    -1, NC, 0                   ,     0, "Bulldozer X3"                  },
	{ 15, -1, -1, 21,    1,   8,    -1,    -1, NC, 0                   ,     0, "Bulldozer X4"                  },
	/* 2nd-gen, Piledriver core (2012): */
	{ 15, -1, -1, 21,    2,   4,    -1,    -1, NC, 0                   ,     0, "Vishera X2"                    },
	{ 15, -1, -1, 21,    2,   6,    -1,    -1, NC, 0                   ,     0, "Vishera X3"                    },
	{ 15, -1, -1, 21,    2,   8,    -1,    -1, NC, 0                   ,     0, "Vishera X4"                    },
	{ 15,  0, -1, 21,   16,   2,    -1,    -1, FUSION_A, 0             ,     0, "Trinity X2"                    },
	{ 15,  0, -1, 21,   16,   4,    -1,    -1, FUSION_A, 0             ,     0, "Trinity X4"                    },
	{ 15,  3, -1, 21,   19,   2,    -1,    -1, FUSION_A, 0             ,     0, "Richland X2"                   },
	{ 15,  3, -1, 21,   19,   4,    -1,    -1, FUSION_A, 0             ,     0, "Richland X4"                   },
	/* 3rd-gen, Steamroller core (2014): */
	{ 15,  0, -1, 21,   48,   2,    -1,    -1, FUSION_A, 0             ,     0, "Kaveri X2"                     },
	{ 15,  0, -1, 21,   48,   4,    -1,    -1, FUSION_A, 0             ,     0, "Kaveri X4"                     },
	{ 15,  8, -1, 21,   56,   4,    -1,    -1, FUSION_A, 0             ,     0, "Godavari X4"                   },
	/* 4th-gen, Excavator core (2015): */
	{ 15,  1, -1, 21,   96,   2,    -1,    -1, FUSION_A, 0             ,     0, "Carrizo X2"                    },
	{ 15,  1, -1, 21,   96,   4,    -1,    -1, FUSION_A, 0             ,     0, "Carrizo X4"                    },
	{ 15,  5, -1, 21,  101,   2,    -1,    -1, FUSION_A, 0             ,     0, "Bristol Ridge X2"              },
	{ 15,  5, -1, 21,  101,   4,    -1,    -1, FUSION_A, 0             ,     0, "Bristol Ridge X4"              },
	{ 15,  0, -1, 21,  112,   2,    -1,    -1, FUSION_A, 0             ,     0, "Stoney Ridge X2"               },
	{ 15,  0, -1, 21,  112,   2,    -1,    -1, FUSION_E, 0             ,     0, "Stoney Ridge X2"               },

	/* Family 16h: Jaguar Architecture (2013) */
	{ 15,  0, -1, 22,    0,   2,    -1,    -1, FUSION_A, 0             ,     0, "Kabini X2"                     },
	{ 15,  0, -1, 22,    0,   4,    -1,    -1, FUSION_A, 0             ,     0, "Kabini X4"                     },
	/* 2nd-gen, Puma core (2013): */
	{ 15,  0, -1, 22,   48,   2,    -1,    -1, FUSION_E, 0             ,     0, "Mullins X2"                    },
	{ 15,  0, -1, 22,   48,   4,    -1,    -1, FUSION_A, 0             ,     0, "Mullins X4"                    },

	/* Family 17h: Zen Architecture (2017) => https://en.wikichip.org/wiki/amd/microarchitectures/zen */
	{ 15, -1, -1, 23,    1,  -1,    -1,    -1, NC, EPYC_               ,     0, "EPYC (Naples)"                 },
	{ 15, -1, -1, 23,    1,  -1,    -1,    -1, NC, RYZEN_TR_           ,     0, "Threadripper (Whitehaven)"     },
	{ 15, -1, -1, 23,    1,  -1,    -1,    -1, NC, RYZEN_|_7           ,     0, "Ryzen 7 (Summit Ridge)"        },
	{ 15, -1, -1, 23,    1,  -1,    -1,    -1, NC, RYZEN_|_5           ,     0, "Ryzen 5 (Summit Ridge)"        },
	{ 15, -1, -1, 23,    1,  -1,    -1,    -1, NC, RYZEN_|_3           ,     0, "Ryzen 3 (Summit Ridge)"        },
	{ 15, -1, -1, 23,   17,  -1,    -1,    -1, NC, RYZEN_|_7           ,     0, "Ryzen 7 (Raven Ridge)"         },
	{ 15, -1, -1, 23,   17,  -1,    -1,    -1, NC, RYZEN_|_5           ,     0, "Ryzen 5 (Raven Ridge)"         },
	{ 15, -1, -1, 23,   17,  -1,    -1,    -1, NC, RYZEN_|_3           ,     0, "Ryzen 3 (Raven Ridge)"         },
	{ 15, -1, -1, 23,   17,  -1,    -1,    -1, NC, ATHLON_             ,     0, "Athlon (Raven Ridge)"          },
	/* Zen+ (2018) => https://en.wikichip.org/wiki/amd/microarchitectures/zen%2B */
	{ 15, -1, -1, 23,    8,  -1,    -1,    -1, NC, RYZEN_TR_           ,     0, "Threadripper (Colfax)"         },
	{ 15, -1, -1, 23,    8,  -1,    -1,    -1, NC, RYZEN_|_7           ,     0, "Ryzen 7 (Pinnacle Ridge)"      },
	{ 15, -1, -1, 23,    8,  -1,    -1,    -1, NC, RYZEN_|_5           ,     0, "Ryzen 5 (Pinnacle Ridge)"      },
	{ 15, -1, -1, 23,    8,  -1,    -1,    -1, NC, RYZEN_|_3           ,     0, "Ryzen 3 (Pinnacle Ridge)"      },
	{ 15, -1, -1, 23,   24,  -1,    -1,    -1, NC, RYZEN_|_7           ,     0, "Ryzen 7 (Picasso)"             },
	{ 15, -1, -1, 23,   24,  -1,    -1,    -1, NC, RYZEN_|_5           ,     0, "Ryzen 5 (Picasso)"             },
	{ 15, -1, -1, 23,   24,  -1,    -1,    -1, NC, RYZEN_|_3           ,     0, "Ryzen 3 (Picasso)"             },
	{ 15, -1, -1, 23,   24,  -1,    -1,    -1, NC, ATHLON_             ,     0, "Athlon (Picasso)"              },
	/* Zen 2 (2019) => https://en.wikichip.org/wiki/amd/microarchitectures/zen_2 */
	{ 15, -1, -1, 23,   49,  -1,    -1,    -1, NC, EPYC_               ,     0, "EPYC (Rome)"                   },
	{ 15, -1, -1, 23,   49,  -1,    -1,    -1, NC, RYZEN_TR_           ,     0, "Threadripper (Castle Peak)"    },
	{ 15, -1, -1, 23,  113,  -1,    -1,    -1, NC, RYZEN_|_9           ,     0, "Ryzen 9 (Matisse)"             },
	{ 15, -1, -1, 23,  113,  -1,    -1,    -1, NC, RYZEN_|_7           ,     0, "Ryzen 7 (Matisse)"             },
	{ 15, -1, -1, 23,  113,  -1,    -1,    -1, NC, RYZEN_|_5           ,     0, "Ryzen 5 (Matisse)"             },
	{ 15, -1, -1, 23,  113,  -1,    -1,    -1, NC, RYZEN_|_3           ,     0, "Ryzen 3 (Matisse)"             },
	{ 15, -1, -1, 23,   96,  -1,    -1,    -1, NC, RYZEN_|_9           ,     0, "Ryzen 9 (Renoir)"              },
	{ 15, -1, -1, 23,   96,  -1,    -1,    -1, NC, RYZEN_|_7           ,     0, "Ryzen 7 (Renoir)"              },
	{ 15, -1, -1, 23,   96,  -1,    -1,    -1, NC, RYZEN_|_5           ,     0, "Ryzen 5 (Renoir)"              },
	{ 15, -1, -1, 23,   96,  -1,    -1,    -1, NC, RYZEN_|_3           ,     0, "Ryzen 3 (Renoir)"              },
	{ 15, -1, -1, 23,  104,  -1,    -1,    -1, NC, RYZEN_|_7           ,     0, "Ryzen 7 (Lucienne)"            },
	{ 15, -1, -1, 23,  104,  -1,    -1,    -1, NC, RYZEN_|_5           ,     0, "Ryzen 5 (Lucienne)"            },
	{ 15, -1, -1, 23,  104,  -1,    -1,    -1, NC, RYZEN_|_3           ,     0, "Ryzen 3 (Lucienne)"            },
	/* Zen 3 (2020) => https://en.wikichip.org/wiki/amd/microarchitectures/zen_3 */
	{ 15, -1, -1, 25,    1,  -1,    -1,    -1, NC, EPYC_               ,     0, "EPYC (Milan)"                  },
	{ 15, -1, -1, 25,   33,  -1,    -1,    -1, NC, RYZEN_|_9           ,     0, "Ryzen 9 (Vermeer)"             },
	{ 15, -1, -1, 25,   33,  -1,    -1,    -1, NC, RYZEN_|_7           ,     0, "Ryzen 7 (Vermeer)"             },
	{ 15, -1, -1, 25,   33,  -1,    -1,    -1, NC, RYZEN_|_5           ,     0, "Ryzen 5 (Vermeer)"             },
	{ 15, -1, -1, 25,   33,  -1,    -1,    -1, NC, RYZEN_|_3           ,     0, "Ryzen 3 (Vermeer)"             },
	{ 15, -1, -1, 25,   80,  -1,    -1,    -1, NC, RYZEN_|_9           ,     0, "Ryzen 9 (Cezanne)"             },
	{ 15, -1, -1, 25,   80,  -1,    -1,    -1, NC, RYZEN_|_7           ,     0, "Ryzen 7 (Cezanne)"             },
	{ 15, -1, -1, 25,   80,  -1,    -1,    -1, NC, RYZEN_|_5           ,     0, "Ryzen 5 (Cezanne)"             },
	{ 15, -1, -1, 25,   80,  -1,    -1,    -1, NC, RYZEN_|_3           ,     0, "Ryzen 3 (Cezanne)"             },
	/* F   M   S  EF    EM  #cores  L2$   L3$  BC  ModelBits          ModelCode  Name                           */

	{ 15, -1, -1, 24,    0,  -1,    -1,    -1, NC, C86_|_7              ,     0, "C86 7 (Dhyana)"               },
	{ 15, -1, -1, 24,    0,  -1,    -1,    -1, NC, C86_|_5              ,     0, "C86 5 (Dhyana)"               },
	{ 15, -1, -1, 24,    0,  -1,    -1,    -1, NC, C86_|_3              ,     0, "C86 3 (Dhyana)"               },


	/* Newer Opterons: */
	{ 15,  9, -1, 22,    9,   8,    -1,    -1, NC, OPTERON_            ,     0, "Magny-Cours Opteron"           },
};


static void load_amd_features(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	const struct feature_map_t matchtable_edx81[] = {
		{ 20, CPU_FEATURE_NX },
		{ 22, CPU_FEATURE_MMXEXT },
		{ 25, CPU_FEATURE_FXSR_OPT },
		{ 30, CPU_FEATURE_3DNOWEXT },
		{ 31, CPU_FEATURE_3DNOW },
	};
	const struct feature_map_t matchtable_ecx81[] = {
		{  1, CPU_FEATURE_CMP_LEGACY },
		{  2, CPU_FEATURE_SVM },
		/* id 5 is handled in common */
		{  6, CPU_FEATURE_SSE4A },
		{  7, CPU_FEATURE_MISALIGNSSE },
		{  8, CPU_FEATURE_3DNOWPREFETCH },
		{  9, CPU_FEATURE_OSVW },
		{ 10, CPU_FEATURE_IBS },
		{ 11, CPU_FEATURE_XOP },
		{ 12, CPU_FEATURE_SKINIT },
		{ 13, CPU_FEATURE_WDT },
		{ 16, CPU_FEATURE_FMA4 },
		{ 21, CPU_FEATURE_TBM },
	};
	const struct feature_map_t matchtable_edx87[] = {
		{  0, CPU_FEATURE_TS },
		{  1, CPU_FEATURE_FID },
		{  2, CPU_FEATURE_VID },
		{  3, CPU_FEATURE_TTP },
		{  4, CPU_FEATURE_TM_AMD },
		{  5, CPU_FEATURE_STC },
		{  6, CPU_FEATURE_100MHZSTEPS },
		{  7, CPU_FEATURE_HWPSTATE },
		/* id 8 is handled in common */
		{  9, CPU_FEATURE_CPB },
		{ 10, CPU_FEATURE_APERFMPERF },
		{ 11, CPU_FEATURE_PFI },
		{ 12, CPU_FEATURE_PA },
	};
	if (raw->ext_cpuid[0][EAX] >= 0x80000001) {
		match_features(matchtable_edx81, COUNT_OF(matchtable_edx81), raw->ext_cpuid[1][EDX], data);
		match_features(matchtable_ecx81, COUNT_OF(matchtable_ecx81), raw->ext_cpuid[1][ECX], data);
	}
	if (raw->ext_cpuid[0][EAX] >= 0x80000007)
		match_features(matchtable_edx87, COUNT_OF(matchtable_edx87), raw->ext_cpuid[7][EDX], data);
	if (raw->ext_cpuid[0][EAX] >= 0x8000001a) {
		/* We have the extended info about SSE unit size
		Extracted from BKDG, about CPUID_Fn8000001A_EAX [Performance Optimization Identifiers] (Core::X86::Cpuid::PerfOptId):
		- bit 2: FP256
		- bit 1: MOVU
		- bit 0: FP128 */
		data->detection_hints[CPU_HINT_SSE_SIZE_AUTH] = 1;
		if ((raw->ext_cpuid[0x1a][EAX] >> 2) & 1)
			data->sse_size = 256;
		else if ((raw->ext_cpuid[0x1a][EAX]) & 1)
			data->sse_size = 128;
		else
			data->sse_size = 64;
	}
}

static void decode_amd_cache_info(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int l3_result, l3_assoc;
	const int assoc_table[16] = {
		0, 1, 2, 0, 4, 0, 8, 0, 16, 16, 32, 48, 64, 96, 128, 255
	};
	unsigned n = raw->ext_cpuid[0][EAX];

	if (n >= 0x80000005) {
		/* L1 Data Cache */
		data->l1_data_cache     = EXTRACTS_BITS(raw->ext_cpuid[5][ECX], 31, 24); // L1DcSize
		data->l1_data_assoc     = EXTRACTS_BITS(raw->ext_cpuid[5][ECX], 23, 16); // L1DcAssoc
		data->l1_data_cacheline = EXTRACTS_BITS(raw->ext_cpuid[5][ECX],  7,  0); // L1DcLineSize

		/* L1 Instruction Cache */
		data->l1_instruction_cache     = EXTRACTS_BITS(raw->ext_cpuid[5][EDX], 31, 24); // L1IcSize
		data->l1_instruction_assoc     = EXTRACTS_BITS(raw->ext_cpuid[5][EDX], 23, 16); // L1IcAssoc
		data->l1_instruction_cacheline = EXTRACTS_BITS(raw->ext_cpuid[5][EDX],  7,  0); // L1IcLineSize
	}
	if (n >= 0x80000006) {
		data->l2_cache = (raw->ext_cpuid[6][ECX] >> 16) & 0xffff;
		data->l2_assoc = assoc_table[(raw->ext_cpuid[6][ECX] >> 12) & 0xf];
		data->l2_cacheline = (raw->ext_cpuid[6][ECX]) & 0xff;

		l3_result = (raw->ext_cpuid[6][EDX] >> 18);
		if (l3_result > 0) {
			l3_result = 512 * l3_result; /* AMD spec says it's a range,
			                                but we take the lower bound */
			l3_assoc = (raw->ext_cpuid[6][EDX] >> 12) & 0xf;
			data->l3_cache = l3_result;

			if(l3_assoc == 0x9) {
				/* Since Zen 2, CPUID_Fn80000006_EDX[15:12] is invalid (0x9)
				According to page 74 on
				Processor Programming Reference (PPR) for AMD Family 17h Model 71h, Revision B0 Processors:
				"There are insufficient available encodings to represent all possible L3
				associativities. Please refer to Core::X86::Cpuid::CachePropEbx3[CacheNumWays]."
				Note: we do not read CPUID_Fn80000001_ECX[22] (AKA TopologyExtensions) to allow backward compatibility with existing tests */
				data->l3_assoc     = EXTRACTS_BITS(raw->amd_fn8000001dh[0x3][EBX], 31, 22) + 1; // Cache number of ways is CacheNumWays + 1
				data->l3_cacheline = EXTRACTS_BITS(raw->amd_fn8000001dh[0x3][EBX], 11, 0) + 1; // Cache line size in bytes is CacheLineSize + 1
			} else {
				data->l3_assoc = assoc_table[l3_assoc];
				data->l3_cacheline = (raw->ext_cpuid[6][EDX]) & 0xff;
			}
		} else {
			data->l3_cache = 0;
		}
	}
}

static void decode_amd_number_of_cores(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int logical_cpus = -1, num_cores = -1;

	if (raw->basic_cpuid[0][EAX] >= 1) {
		logical_cpus = (raw->basic_cpuid[1][EBX] >> 16) & 0xff;
		if (raw->ext_cpuid[0][EAX] >= 8) {
			num_cores = 1 + (raw->ext_cpuid[8][ECX] & 0xff);
		}
	}
	if (data->flags[CPU_FEATURE_HT]) {
		if (num_cores > 1) {
			if ((data->ext_family >= 23) && (raw->ext_cpuid[0][EAX] >= 30))
				/* Ryzen 3 has SMT flag, but in fact cores count is equal to threads count.
				Ryzen 5/7 reports twice as many "real" cores (e.g. 16 cores instead of 8) because of SMT. */
				/* On PPR 17h, page 82:
				CPUID_Fn8000001E_EBX [Core Identifiers][15:8] is ThreadsPerCore
				ThreadsPerCore: [...] The number of threads per core is ThreadsPerCore+1 */
				num_cores /= ((raw->ext_cpuid[30][EBX] >> 8) & 0xff) + 1;
			data->num_cores = num_cores;
			data->num_logical_cpus = logical_cpus;
		} else {
			data->num_cores = 1;
			data->num_logical_cpus = (logical_cpus >= 2 ? logical_cpus : 2);
		}
	} else {
		data->num_cores = data->num_logical_cpus = 1;
	}
}

static int amd_has_turion_modelname(const char *bs)
{
	/* We search for something like TL-60. Ahh, I miss regexes...*/
	int i, l, k;
	char code[3] = {0};
	const char* codes[] = { "ML", "MT", "MK", "TK", "TL", "RM", "ZM", "" };
	l = (int) strlen(bs);
	for (i = 3; i < l - 2; i++) {
		if (bs[i] == '-' &&
		    isupper(bs[i-1]) && isupper(bs[i-2]) && !isupper(bs[i-3]) &&
		    isdigit(bs[i+1]) && isdigit(bs[i+2]) && !isdigit(bs[i+3]))
		{
			code[0] = bs[i-2];
			code[1] = bs[i-1];
			for (k = 0; codes[k][0]; k++)
				if (!strcmp(codes[k], code)) return 1;
		}
	}
	return 0;
}

static struct amd_code_and_bits_t decode_amd_codename_part1(const char *bs)
{
	amd_code_t code = (amd_code_t) NC;
	struct amd_code_and_bits_t result;
	uint64_t bits = 0;
	int i = 0;

	const struct { amd_code_t c; const char *search; } code_matchtable[] = {
		{ PHENOM2, "Phenom(tm) II" },
		{ PHENOM, "Phenom(tm)" },
		{ FUSION_C, "C-##" },
		{ FUSION_E, "E-###" },
		{ FUSION_Z, "Z-##" },
		{ FUSION_EA, "[EA]#-####" },
	};

	const struct { uint64_t bit; const char *search; } bit_matchtable[] = {
		{ _X2, "Dual[- ]Core" },
		{ _X2, " X2 " },
		{ _X3, " X3 " },
		{ _X4, " X4 " },
		{ OPTERON_, "Opteron" },
		{ ATHLON_, "Athlon" },
		{ SEMPRON_, "Sempron(tm)" },
		{ DURON_, "Duron" },
		{ _64_, " 64 " },
		{ _FX, " FX" },
		{ _MP_, " MP" },
		{ ATHLON_|_64_, "Athlon(tm) 64" },
		{ ATHLON_|_64_, "Athlon(tm) II X" },
		{ ATHLON_|_64_, "Athlon(tm) X#" },
		{ TURION_, "Turion" },
		{ MOBILE_, "[mM]obile" },
		{ _XP_, "XP" },
		{ _M_, "XP-M" },
		{ _LV_, "(LV)" },
		{ _APU_, " APU " },
		{ EPYC_, "EPYC" },
		{ RYZEN_TR_, "Ryzen Threadripper" },
		{ C86_, "C86" },
	};

	for (i = 0; i < COUNT_OF(bit_matchtable); i++) {
		if (match_pattern(bs, bit_matchtable[i].search))
			bits |= bit_matchtable[i].bit;
	}
	if (amd_has_turion_modelname(bs)) {
		bits |= TURION_;
	}
	if ((i = match_pattern(bs, "Ryzen [3579]")) != 0) {
		bits |= RYZEN_;
		i--;
		switch (bs[i + 6]) {
			case '3': bits |= _3; break;
			case '5': bits |= _5; break;
			case '7': bits |= _7; break;
			case '9': bits |= _9; break;
		}
	}

	if ((i = match_pattern(bs, "C86 [357]")) != 0) {
		bits |= C86_;
		i--;
		switch (bs[i + 6]) {
			case '3': bits |= _3; break;
			case '5': bits |= _5; break;
			case '7': bits |= _7; break;
		}
	}

	for (i = 0; i < COUNT_OF(code_matchtable); i++)
		if (match_pattern(bs, code_matchtable[i].search)) {
			code = code_matchtable[i].c;
			break;
		}

	result.code = code;
	result.bits = bits;
	return result;
}

static void decode_amd_codename(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct internal_id_info_t* internal)
{
	struct amd_code_and_bits_t code_and_bits = decode_amd_codename_part1(data->brand_str);
	int i = 0;
	char* code_str = NULL;
	int model_code = 0;

	for (i = 0; i < COUNT_OF(amd_code_str); i++) {
		if (code_and_bits.code == amd_code_str[i].code) {
			code_str = amd_code_str[i].str;
			break;
		}
	}
	if (/*code == ATHLON_64_X2*/ match_all(code_and_bits.bits, ATHLON_|_64_|_X2) && data->l2_cache < 512) {
		code_and_bits.bits &= ~(ATHLON_ | _64_);
		code_and_bits.bits |= SEMPRON_;
	}
	if (code_str)
		debugf(2, "Detected AMD brand code: %d (%s)\n", code_and_bits.code, code_str);
	else
		debugf(2, "Detected AMD brand code: %d\n", code_and_bits.code);

	if (code_and_bits.bits) {
		debugf(2, "Detected AMD bits: ");
		debug_print_lbits(2, code_and_bits.bits);
	}

	internal->code.amd = code_and_bits.code;
	internal->bits = code_and_bits.bits;
	internal->score = match_cpu_codename(cpudb_amd, COUNT_OF(cpudb_amd), data, code_and_bits.code,
	                                     code_and_bits.bits, model_code);
}

int cpuid_identify_amd(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct internal_id_info_t* internal)
{
	load_amd_features(raw, data);
	decode_amd_cache_info(raw, data);
	decode_amd_number_of_cores(raw, data);
	decode_amd_codename(raw, data, internal);
	return 0;
}

void cpuid_get_list_amd(struct cpu_list_t* list)
{
	generic_get_cpu_list(cpudb_amd, COUNT_OF(cpudb_amd), list);
}
