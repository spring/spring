!!ARBfp1.0
OPTION ARB_fog_linear;
OPTION ARB_fragment_program_shadow;

TEMP temp;
TEMP shadow,tempColor,shadeColor,shadeTex;

TEX shadow, fragment.texcoord[4], texture[4], SHADOW2D;
ADD shadow.x, 1,-shadow.x;
MUL shadow.x, shadow.x,program.env[11].w;
TEX shadeTex, fragment.texcoord[1], texture[1], 2D;
MUL shadow.x, shadow.x,shadeTex.w;
ADD shadow.x, 1,-shadow.x;
LRP shadeColor, shadow.x, shadeTex, program.env[10];

TEX tempColor, fragment.texcoord[0], texture[0], 2D;
TEX temp, fragment.texcoord[2], texture[2], 2D;
MAD temp, temp, {2,2,2,2},{-1,-1,-1,-1};
ADD tempColor, tempColor,temp;
MUL result.color, tempColor,shadeColor;
MAD result.color.w,fragment.texcoord[2].z,0.1,1;
END