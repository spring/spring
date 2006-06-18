!!ARBfp1.0
OPTION ARB_fog_exp;

PARAM detailMul = { 0.05, 0.0, 0.0, 0.1};
PARAM dp3sub = { -1, -1, -1, -1};
PARAM expStuff = {16, 1, 1, 0};
TEMP temp,temp2,temp3;
TEMP cloudColor,tempColor;
TEX temp, fragment.texcoord[2], texture[2], 2D;
TEX temp2, fragment.texcoord[1], texture[1], 2D;
MAD temp, temp, {2,2,2,1},dp3sub;
MAD temp2, temp2,{2,2,2,1}, dp3sub;
MUL tempColor, temp.x, program.env[10];
MUL cloudColor, tempColor,temp2.x;
MUL tempColor, temp.y, program.env[11];
MAD cloudColor, tempColor,temp2.y,cloudColor;
MUL tempColor, temp.z, program.env[12];
MAD_SAT cloudColor, tempColor,temp2.z,cloudColor;
TEX temp3, fragment.texcoord[3], texture[3], 2D;
MAD temp3.w, temp3.x, detailMul.x, temp.w;
MUL temp3.w, temp3.w, temp3.w;
MUL temp3.w, temp3.w, temp3.w;
MUL_SAT temp3.w, temp3.w, expStuff.x;
TEX temp, fragment.texcoord[0], texture[0], 2D;
LRP result.color, temp3.w, temp, cloudColor;
END