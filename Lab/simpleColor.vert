#version 330 core

// input data
layout(location = 0) in vec3 aPosition;

// uniform input data
uniform mat4 uModelViewProjectionMatrix;

// model space matrix
uniform mat4 uModelMatrix;

out vec3 vPosition;

void main()
{
	// set vertex position
    gl_Position = uModelViewProjectionMatrix * vec4(aPosition, 1.0f);

	vPosition = (uModelMatrix * vec4(aPosition, 1.0f)).xyz;
}
