uniform sampler2D       diffuseTex;
uniform sampler2D       shadingTex;
uniform sampler2DShadow shadowTex;
uniform samplerCube     reflectTex;
uniform samplerCube     specularTex;

uniform mat4 shadowMat;
uniform vec4 shadowParams;

uniform vec4 unitTeamColor;
uniform vec4 unitAmbientColor;
uniform vec4 unitDiffuseColor;
uniform float unitShadowDensity;

void main() {
	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
