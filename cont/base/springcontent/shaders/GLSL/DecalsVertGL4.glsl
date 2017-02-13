#version 420 compatibility
#extension GL_ARB_uniform_buffer_object : enable
//#extension GL_EXT_gpu_shader4 : enable
//#define MAX_DECALS_PER_GROUP 48
//#define MAX_DECALS_GROUPS 300

struct SDecalGroup {
	vec4 boundAABB[2];
	int ids[MAX_DECALS_PER_GROUP];
};

//FIXME layout(std140)
layout(packed) uniform decalGroupsUBO
{
	SDecalGroup groups[MAX_DECALS_GROUPS];
};


uniform vec3 camPos;
uniform mat4 viewProjMatrix;

flat out int decalGroupId;


void main() {
	decalGroupId = gl_InstanceID;
	SDecalGroup g = groups[gl_InstanceID];
	vec3 v = mix(g.boundAABB[0].xyz, g.boundAABB[1].xyz, step(vec3(0.0), gl_Vertex.xyz) );
	gl_Position = viewProjMatrix * vec4(v, 1.0);
}
