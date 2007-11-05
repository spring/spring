!!ARBfp1.0
OPTION ARB_fog_linear;
OPTION ARB_fragment_program_shadow;

TEMP temp;
TEMP shadow,tempColor,shadeColor,shadeTex;

TEX shadow, fragment.texcoord[0], texture[0], SHADOW2D;
#ADD shadow.x, 1,-shadow.w;
ADD_SAT shadow.x, shadow.x,program.env[11].w;
#ADD shadow.x, 1,-shadow.x;
LRP shadeColor, shadow.x, fragment.color, program.env[10];

TEX tempColor, fragment.texcoord[1], texture[1], 2D;
MUL result.color, tempColor,shadeColor;
END