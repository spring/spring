!!ARBfp1.0

# water reflective+refraction program for cards supporting GL_ARB_texture_rectangle
# program.env[2] is texture scale

OPTION ARB_fog_linear;
PARAM bumpAdd = { -0.5, -0.5, 0, 0};
PARAM bumpMul = { 0.02, 0.02, 0.0, 0.0};
TEMP temp,temp2,temp3,subsurfcol,sstexc,waterDepth,blendfactor;
TEX temp3, fragment.texcoord[1], texture[1], 2D;
ADD temp3, temp3, bumpAdd;
DP4 temp.x, temp3, program.env[0];
DP4 temp.y, temp3, program.env[1];
MAD temp2, temp, bumpMul,fragment.texcoord[0];
TEX temp3, temp2, texture[0], 2D;
MUL temp3, temp3, fragment.color;

# Shading texture with waterdepth (stored as 255+groundheight) encoded in alpha
TEX waterDepth, fragment.texcoord[3], texture[3], 2D;

# More depth gives more texture coordinate offset
SUB temp2, {1,1,1,1}, waterDepth.a;
MUL temp2, temp2, program.env[2];  # Texture coordinates for RECT mode are in pixels
MUL temp2, temp2, fragment.position.w;
MAD sstexc, temp, temp2, fragment.position;
TEX subsurfcol, sstexc, texture[2], RECT;

# deeper water has darker underwater surface color
MAD blendfactor, waterDepth.a, {0.5,0.5,0.5,0.0}, {0.4, 0.4, 0.4, 0.0};
LRP result.color, blendfactor, subsurfcol, temp3;
END
