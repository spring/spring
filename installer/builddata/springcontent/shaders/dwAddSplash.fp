!!ARBfp1.0
OPTION ARB_fog_linear;
TEMP temp,temp2,temp3;

TEX temp, fragment.texcoord[0], texture[0], 2D;

MUL result.color, temp, fragment.texcoord[1];

END