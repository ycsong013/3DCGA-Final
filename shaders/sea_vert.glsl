#version 410

const float PI = 3.1415926;

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
// Normals should be transformed differently than positions:
// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
uniform mat3 normalModelMatrix;
uniform float time;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoord;

float height(vec2 posXZ, out vec2 gradient) {
	float h = 0.0;
	gradient = vec2(0.0);

	const int numOfWaves = 2;
	// Directions of waves in XZ plane
	vec2 directions[numOfWaves] = vec2[](normalize(vec2(1, 1)), normalize(vec2(-1, 1)));
	// Amplitudes of waves
	float amplitudes[numOfWaves] = float[](0.1, 0.05);
	// Wavelengths of waves
	float wavelengths[numOfWaves] = float[](1, 2);
	// Speeds of waves
	float speeds[numOfWaves] = float[](1, 1);

	for (int i = 0; i < numOfWaves; i++) {
		// wave number
		float k = 2 * PI / wavelengths[i];
		float phase = k * dot(directions[i], posXZ) + speeds[i] * time;
		h += amplitudes[i] * sin(phase);
		// dh/dx = a * cos(phase) * k * dir, dh/dz = a * cos(phase) * k * dir
		gradient += amplitudes[i] * cos(phase) * k * directions[i];
	}

	return h;
}

void main() {
	vec2 gradient;

	vec3 newPosition = position;
	newPosition.y += height(newPosition.xz, gradient);

	vec3 newNormal = normalize(vec3(-gradient.x, 1.0, -gradient.y));

	gl_Position = mvpMatrix * vec4(newPosition, 1);
    fragPosition    = (modelMatrix * vec4(newPosition, 1)).xyz;
    fragNormal      = normalModelMatrix * newNormal;
    fragTexCoord    = texCoord;
}