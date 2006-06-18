!!ARBfp1.0
# 3DO Unit rendering with shadows

OPTION ARB_fog_linear;
OPTION ARB_fragment_program_shadow;

TEMP temp,reflect,texColor,specular;
TEMP shadow,tempColor,shadeColor,shadeTex;

#get unit texture
TEX texColor, fragment.texcoord[1], texture[1], 2D;

#get shadow status
TEX shadow, fragment.texcoord[0], texture[0], SHADOW2D;

#normalize surface normal
DP3 temp.x, fragment.texcoord[2], fragment.texcoord[2];
RSQ temp.x, temp.x;
MUL temp, fragment.texcoord[2], temp.x;

#calc reflection dir
DP3 reflect.x, temp, fragment.texcoord[3];
MUL reflect.x, reflect.x,{-2};
MAD reflect, temp, reflect.x, fragment.texcoord[3];

#get specular highlight and remove if in shadow
TEX specular, reflect, texture[3], CUBE;
MUL specular, specular, {4,4,4,1};
MUL specular, specular, shadow.x;
MUL specular, specular, texColor.w;

#soften shadow with shadow intensity
ADD shadow.x, 1,-shadow.x;
MUL shadow.x, shadow.x,program.env[10].w;
ADD shadow.x, 1,-shadow.x;

#change color depending on if in shadow
LRP shadeColor, shadow.x, fragment.color, program.env[11];

TEX reflect, reflect, texture[2], CUBE;
LRP shadeColor, texColor.w, reflect, shadeColor;

MAD result.color, texColor, shadeColor, specular;
END