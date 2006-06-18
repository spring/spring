!!ARBfp1.0
OPTION ARB_fog_linear;
TEMP temp,temp2;

TEX temp, fragment.texcoord[0], texture[0], 2D;
MUL temp2, temp, program.env[0];
TEX temp, fragment.texcoord[1], texture[1], 2D;
MAD temp2, temp, program.env[1],temp2;
TEX temp, fragment.texcoord[2], texture[2], 2D;
MAD temp2, temp, program.env[2],temp2;
TEX temp, fragment.texcoord[3], texture[3], 2D;
MAD temp2, temp, program.env[3],temp2;
TEX temp, fragment.texcoord[4], texture[4], 2D;
MAD temp2, temp, program.env[4],temp2;
TEX temp, fragment.texcoord[5], texture[5], 2D;
MAD temp2, temp, program.env[5],temp2;
TEX temp, fragment.texcoord[6], texture[6], 2D;
MAD temp2, temp, program.env[6],temp2;
TEX temp, fragment.texcoord[7], texture[7], 2D;
MAD temp2, temp, program.env[7],temp2;

MOV result.color, temp2;

END