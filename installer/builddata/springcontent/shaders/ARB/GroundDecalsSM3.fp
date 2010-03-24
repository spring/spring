!!ARBfp1.0
OPTION ARB_fog_linear;
OPTION ARB_fragment_program_shadow;

TEMP temp;
TEMP shadow, tempColor, shadeColor, shadeTex;

TEX shadow, fragment.texcoord[2], texture[2], SHADOW2D;
ADD shadow.x, 1, -shadow.x;
MUL shadow.x, shadow.x, program.env[11].w;

ADD shadow.x, 1, -shadow.x;
MOV shadeTex, {1.0, 1.0, 1.0, 1.0};
LRP shadeColor, shadow.x, shadeTex, program.env[10];

TEX tempColor, fragment.texcoord[0], texture[0], 2D;
MUL result.color, tempColor, shadeColor;
MUL result.color.w, tempColor.w, fragment.color.w;
END
