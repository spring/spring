!!ARBfp1.0
TEMP cross1,cross2,normal,temp;
TEMP height1,height2,height3,height4,foam;

TEX height1, fragment.texcoord[0], texture[0], 2D;
TEX height2, fragment.texcoord[1], texture[1], 2D;
TEX height3, fragment.texcoord[2], texture[2], 2D;
TEX height4, fragment.texcoord[3], texture[3], 2D;
ADD foam.x, height1.y, height2.y;

MOV cross1,program.env[0];
SUB cross1.y, height1.x, height2.x;
MOV cross2,program.env[1];
SUB cross2.y, height3.x, height4.x;

XPD normal, cross1,cross2;
DP3 temp.x, normal, normal;
RSQ temp.x, temp.x;
MUL normal, normal, temp.x;

MOV result.color, normal;
MUL result.color.w, foam.x, 0.5;

END