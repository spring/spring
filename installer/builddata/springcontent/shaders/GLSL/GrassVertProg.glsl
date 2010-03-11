void main() {
	gl_Position = ftransform();
}

/*
GRASS.VP
	!!ARBvp1.0
	ATTRIB pos = vertex.position;
	PARAM mat[4]  = { state.matrix.projection };
	PARAM mat2[4] = { state.matrix.modelview };
	PARAM mat3[4] = { state.matrix.program[0] };
	OUTPUT opos = result.position;
	TEMP worldPos,temp,temp2;

	#position to world space
	DP4 worldPos.x, pos, mat2[0];
	DP4 worldPos.y, pos, mat2[1];
	DP4 worldPos.z, pos, mat2[2];
	DP4 worldPos.w, pos, mat2[3];

	#calculate texture coords
	MUL result.texcoord[0], worldPos.xzyw, program.env[13];					#ground shade texture
	MUL result.texcoord[2], worldPos.xzyw, program.env[14];					#main ground texture
	MOV result.texcoord[3], vertex.texcoord[0];								#grass texture

	#tex coords for shadow texture
	DP4 temp.x, worldPos, mat3[0];
	DP4 temp.y, worldPos, mat3[1];

	ABS temp2,temp;
	ADD temp2,temp2,program.env[17];
	RSQ temp2.x, temp2.x;
	RSQ temp2.y, temp2.y;
	ADD temp2,temp2,program.env[18];
	MAD result.texcoord[1], temp, temp2,program.env[16];

	DP4 result.texcoord[1].z, worldPos, mat3[2];
	DP4 result.texcoord[1].w, worldPos, mat3[3];

	#world space to clip space
	DP4 opos.x, worldPos, mat[0];
	DP4 opos.y, worldPos, mat[1];
	DP4 opos.z, worldPos, mat[2];
	DP4 opos.w, worldPos, mat[3];

	DP4 result.fogcoord.x, worldPos, mat[2];
	MOV result.color, vertex.color;

	END

GRASSFAR.VP
	!!ARBvp1.0
	ATTRIB pos = vertex.position;
	PARAM mat[4] = { state.matrix.mvp };
	PARAM mat3[4] = { state.matrix.program[0] };
	OUTPUT opos = result.position;
	TEMP worldPos,temp,temp2;

	MAD worldPos, program.env[8], vertex.normal.x, pos;
	MAD worldPos, program.env[9], vertex.normal.y, worldPos;
	ADD worldPos, program.env[11], worldPos;

	#calculate texture coords
	MUL result.texcoord[0], worldPos.xzyw, program.env[13];			#ground shade texture
	MUL result.texcoord[2], worldPos.xzyw, program.env[14];			#main ground texture
	ADD result.texcoord[3], vertex.texcoord,program.env[10];		#grass billboard texture

	#tex coords for shadow texture
	DP4 temp.x, worldPos, mat3[0];
	DP4 temp.y, worldPos, mat3[1];

	ABS temp2,temp;
	ADD temp2,temp2,program.env[17];
	RSQ temp2.x, temp2.x;
	RSQ temp2.y, temp2.y;
	ADD temp2,temp2,program.env[18];
	MAD result.texcoord[1], temp, temp2,program.env[16];

	DP4 temp2.z, worldPos, mat3[2];
	ADD result.texcoord[1].z, temp2.z, {0,0,-0.0005,0};	#grass would sometimes be shadowed by the ground beneath it
	DP4 result.texcoord[1].w, worldPos, mat3[3];

	#world space to clip space
	DP4 opos.x, worldPos, mat[0];
	DP4 opos.y, worldPos, mat[1];
	DP4 opos.z, worldPos, mat[2];
	DP4 opos.w, worldPos, mat[3];

	DP4 result.fogcoord.x, worldPos, mat[2];
	MOV result.color, vertex.color;

	END

GRASSFARNS.VP
	!!ARBvp1.0
	ATTRIB pos = vertex.position;
	PARAM mat[4] = { state.matrix.mvp };
	OUTPUT opos = result.position;
	TEMP modpos;

	MAD modpos, program.env[8], vertex.normal.x, pos;
	MAD modpos, program.env[9], vertex.normal.y, modpos;
	ADD modpos, program.env[11], modpos;

	MUL result.texcoord[1], modpos.xzyw, program.env[12];
	MUL result.texcoord[2], modpos.xzyw, program.env[12];
	MUL result.texcoord[3], modpos.xzyw, program.env[13];

	#modelview/projection transformation
	DP4 opos.x, modpos, mat[0];
	DP4 opos.y, modpos, mat[1];
	DP4 opos.z, modpos, mat[2];
	DP4 opos.w, modpos, mat[3];

	DP4 result.fogcoord.x, modpos, mat[2];
	MOV result.color, vertex.color;
	ADD result.texcoord, vertex.texcoord,program.env[10];
	END
*/
