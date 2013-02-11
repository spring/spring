#version 420 compatibility
//#extension GL_ARB_uniform_buffer_object : enable
//#extension GL_EXT_gpu_shader4 : enable

struct SDecal {
	vec3 pos;
	float rot;
	vec4 texcoords;
	vec2 size;
	float alpha;
};

/*layout(std140) uniform decals
{
	SDecal decals_[1000];
};*/

layout(binding=7) uniform samplerBuffer decalInfo;

SDecal GetDecalInfo() {
	//instanceID = gl_InstanceID;
	//return decals_[gl_InstanceID];

	vec4 info1 = texelFetch(decalInfo, gl_InstanceID*3 + 0);
	vec4 info2 = texelFetch(decalInfo, gl_InstanceID*3 + 1);
	vec4 info3 = texelFetch(decalInfo, gl_InstanceID*3 + 2);
	SDecal decal;
	decal.pos       = info1.xyz;
	decal.rot       = info1.w;
	decal.texcoords = info2.xyzw;
	decal.size      = info3.xy;
	decal.alpha     = info3.z;
	return decal;
}


uniform vec3 camPos;

//flat out int instanceID;
flat out SDecal decal;
flat out mat2 rot;

void main() {
	decal = GetDecalInfo();

	// invert vertex winding when being inside
	// we have culling enabled and normally draw the frontfaces, when being inside we need to draw the backfaces
	float vertex_winding = mix(1.0, -1.0, float(any(lessThanEqual(abs(decal.pos - camPos), decal.size.xxy))));

	rot = mat2(cos(decal.rot), sin(decal.rot), -sin(decal.rot), cos(decal.rot));
	vec3 v = gl_Vertex.xyz * decal.size.xxy;
	v *= vertex_winding;
	v.xz = rot * v.xz;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(decal.pos + v, 1.0);
}