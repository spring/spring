!!ARBfp1.0
TEMP height;

TEX height, fragment.texcoord[0], texture[0], 2D;

MOV result.color, height.x;
END