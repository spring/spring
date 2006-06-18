!!ARBfp1.0

# 3DO unit rendering without shadows
OPTION ARB_fog_linear;

TEMP temp,reflect,texColor,specular;
TEMP shadeColor;

#get unit texture
TEX texColor, fragment.texcoord[1], texture[1], 2D;

#normalize surface normal
DP3 temp.x, fragment.texcoord[2], fragment.texcoord[2];
RSQ temp.x, temp.x;
MUL temp, fragment.texcoord[2], temp.x;

#calc reflection dir
DP3 reflect.x, temp, fragment.texcoord[3];
MUL reflect.x, reflect.x,{-2};
MAD reflect, temp, reflect.x, fragment.texcoord[3];

#get specular highlight
TEX specular, reflect, texture[3], CUBE;
MUL specular, specular, {4,4,4,1};
MUL specular, specular, texColor.w;

TEX reflect, reflect, texture[2], CUBE;
LRP shadeColor, texColor.w, reflect, fragment.color;

MAD result.color, texColor, shadeColor, specular;
END