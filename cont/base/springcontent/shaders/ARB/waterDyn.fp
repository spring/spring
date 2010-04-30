!!ARBfp1.0
OPTION ARB_fog_linear;
OPTION ARB_fragment_program_shadow;
TEMP temp,camdir,reflectdir,waterNormal,angle,reflectColor;
TEMP refractdir,refractColor,clearcolor,specularvalue;
TEMP groundHeight,refractMul,foamColor,lightColor,foamIntensity;
TEMP detailNormal,shadow,camdist,waterNormal2,angle2,reflectdir2;

TEX shadow, fragment.texcoord[1], texture[7], SHADOW2D;

SUB camdir, fragment.texcoord, program.env[1];
DP3 temp.x, camdir, camdir;
RSQ temp.x, temp.x;
MUL camdir, camdir, temp.x;
RCP camdist.x, temp.x;

TEX waterNormal, fragment.texcoord[2], texture[0], 2D;
MAD_SAT foamIntensity.x, waterNormal.w, 5, -1;
TEX groundHeight, fragment.texcoord[4], texture[4], 2D;
MAD groundHeight.x, groundHeight.a, 0.4, -0.4;
TEX temp, fragment.texcoord[3], texture[5], 2D;
MUL foamIntensity.x , foamIntensity.x, temp.x;
TEX detailNormal, fragment.texcoord[3], texture[6], 2D;

MAD waterNormal, detailNormal.xzyw, {1,0,1,0}, waterNormal;
DP3 temp.x, waterNormal, waterNormal;
RSQ temp.x, temp.x;
MUL waterNormal, waterNormal, temp.x;

DP3 angle.x, camdir, waterNormal;
MUL temp.x, angle.x, -2;
MAD reflectdir, waterNormal, temp.x, camdir;

DP3_SAT specularvalue.x, reflectdir, program.env[9];
POW temp, specularvalue.x, {16,0,0,0}.x;
POW specularvalue, specularvalue.x, {4096,0,0,0}.x;
MUL specularvalue, specularvalue, 10;
MUL specularvalue, specularvalue, shadow.x;
LRP shadow.x, 0.3, 1, shadow.x;
MUL temp, temp, shadow.x;
MAD specularvalue, temp, 0.5, specularvalue;

MAD waterNormal2, camdist.x, {0,0.003,0,0}, waterNormal;
DP3 temp.x, waterNormal2, waterNormal2;
RSQ temp.x, temp.x;
MUL waterNormal2, waterNormal2, temp.x;

DP3 angle2.x, camdir, waterNormal2;
MUL temp.x, angle2.x, -2;
MAD reflectdir2, waterNormal2, temp.x, camdir;
MAX reflectdir2.y, reflectdir2.y, 0;

DP3 temp.x, reflectdir2, program.env[5];
RCP temp.x, temp.x;
MUL reflectdir2, reflectdir2, temp.x;

DP3 temp.x, reflectdir2, program.env[2];
DP3 temp.y, reflectdir2, program.env[3];
MAD temp, temp, program.env[4], {0.5, 0.5, 0, 0};
TEX reflectColor, temp, texture[1], 2D;
MAD reflectColor, specularvalue, program.env[10], reflectColor;

MUL refractMul, groundHeight.x, {1,0,1,0};
MAD refractdir,waterNormal2, refractMul,camdir;
DP3 temp.x, refractdir, program.env[15];
RCP temp.x, temp.x;
MUL refractdir, refractdir, temp.x;

DP3 temp.x, refractdir, program.env[12];
DP3 temp.y, refractdir, program.env[13];
MAD temp, temp, program.env[14], {0.5, 0.5, 0, 0};
TEX refractColor, temp, texture[3], 2D;

ADD temp.x, 1, angle.x;
MUL temp.y, temp.x, temp.x;
MUL temp.y, temp.y, temp.y;
MUL temp.x, temp.x, temp.y;
MAD temp.x, temp.x, program.env[6].y, program.env[6].x; 
LRP clearcolor, temp.x, reflectColor, refractColor;

DP3_SAT temp.x, waterNormal, program.env[9];
MUL temp.x, temp.x, shadow.x;
MAD lightColor, temp.x, program.env[10], program.env[11];
MUL temp, lightColor, program.env[8];
LRP temp, program.env[7].x, temp, clearcolor;

LRP result.color, foamIntensity.x, lightColor, temp;

#MUL temp, fragment.texcoord[2],1;
#TEX temp, temp, texture[0], 2D;
#MAD result.color, temp, 0.5, 0.5;

END