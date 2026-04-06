
#version 450
#extension GL_ARB_separate_shader_objects : enable

#define dSzBuffer 120
#define dSzWorkgroup 7
layout (local_size_x = dSzWorkgroup, local_size_y = 1, local_size_z = 1 ) in;

layout(std430, binding = 0) buffer buf
{
	uint buff[];
};

void main()
{
	uint idx = gl_GlobalInvocationID.x;

	if(idx >= dSzBuffer)
		return;

	buff[idx] = idx + 2;
}

