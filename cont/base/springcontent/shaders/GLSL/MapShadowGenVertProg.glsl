uniform mat4 shadowViewMat;
uniform mat4 shadowProjMat;
uniform vec4 shadowParams;

layout(location = 0) in vec3 vtxPositionAttr;


void main() {
	vec4 vertexShadowPos = shadowViewMat * vec4(vtxPositionAttr, 1.0);

		vertexShadowPos.xy *= (inversesqrt(abs(vertexShadowPos.xy) + shadowParams.zz) + shadowParams.ww);
		vertexShadowPos.xy += shadowParams.xy;

	gl_Position = shadowProjMat * vertexShadowPos;
}

