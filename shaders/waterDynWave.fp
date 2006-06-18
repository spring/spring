!!ARBfp1.0
TEMP temp;
TEMP old,midFlow,flow1,flow2,flow3,flow4,flow,backgroundWave;

TEX old, fragment.texcoord[0], texture[0], 2D;
TEX midFlow, fragment.texcoord[1], texture[1], 2D;
TEX flow1, fragment.texcoord[2], texture[2], 2D;
TEX flow2, fragment.texcoord[3], texture[3], 2D;
TEX flow3, fragment.texcoord[4], texture[4], 2D;
TEX flow4, fragment.texcoord[5], texture[5], 2D;
TEX backgroundWave, fragment.texcoord[6], texture[6], 2D;

SUB old.x, old.x, old.z;

DP4 temp.x, midFlow,{1,1,1,1};
SUB old.x, old.x, temp.x;

MOV flow.x, flow1.x;
MOV flow.y, flow2.y;
MOV flow.z, flow3.z;
MOV flow.w, flow4.w;

DP4 temp.x, flow,{1,1,1,1};
ADD old.x, old.x, temp.x;

SUB temp, midFlow, flow;
DP4 temp.x, temp, temp;

MAD old.y, temp.x, 0.2, old.y;
MUL_SAT old.y, old.y, 0.99;

MIN temp.x, old.w, 2;
MUL_SAT temp.x, temp.x, 0.2;

LRP old.x, 0.995, old.x, backgroundWave.z;
LRP old.x, temp.x, -old.w, old.x;

MUL result.color, old, {1,1,0,0};
END