!!ARBfp1.0
TEMP temp;
TEMP old,midHeight,height1,height2,height3,height4,gheight;

TEX old, fragment.texcoord[0], texture[0], 2D;
TEX midHeight, fragment.texcoord[0], texture[1], 2D;
TEX height1, fragment.texcoord[1], texture[2], 2D;
TEX height2, fragment.texcoord[2], texture[3], 2D;
TEX height3, fragment.texcoord[3], texture[4], 2D;
TEX height4, fragment.texcoord[4], texture[5], 2D;
TEX gheight, fragment.texcoord[5], texture[6], 2D;

SUB temp.x, midHeight.x, height1.x;
MAD old.x, temp.x, 0.02, old.x;
SUB temp.x, midHeight.x, height2.x;
MAD old.y, temp.x, 0.02, old.y;
SUB temp.x, midHeight.x, height3.x;
MAD old.z, temp.x, 0.02, old.z;
SUB temp.x, midHeight.x, height4.x;
MAD old.w, temp.x, 0.02, old.w;

LRP temp.x, gheight.w, 0.008, 0.998;
MUL old, old, temp.x;

MOV result.color, old;
END