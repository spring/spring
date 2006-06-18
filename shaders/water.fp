!!ARBfp1.0
OPTION ARB_fog_linear;
PARAM bumpAdd = { -0.5, -0.5, 0, 0};
PARAM bumpMul = { 0.02, 0.02, 0.0, 0.0};
PARAM alphaMul = { 0.03, 0.0, 0.0, 1};
TEMP temp,temp2,temp3;
TEX temp3, fragment.texcoord[1], texture[1], 2D;
ADD temp3, temp3, bumpAdd;
DP4 temp.x, temp3, program.env[0];
DP4 temp.y, temp3, program.env[1];
MAD temp2, temp, bumpMul,fragment.texcoord[0];
TEX temp3, temp2, texture[0], 2D;
MOV temp2, fragment.color;
MAD temp2.w, temp.y, alphaMul.x, temp2.w;
MUL result.color, temp3, temp2;
END