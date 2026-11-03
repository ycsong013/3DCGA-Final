#version 410
in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

uniform samplerCube prefilteredMap;
uniform samplerCube prefilteredMapNight;
uniform vec3 cameraPosition;
uniform float interpolationFactor;

out vec4 fragColor;

void main() {
	vec3 N = normalize(fragNormal);
	vec3 V = normalize(cameraPosition - fragPosition);
	vec3 R = reflect(-V, N); // environment reflection
	R.y = abs(R.y); // Reflection might point downward when fragment normal is facing behind
	// lod = 0.0 --> mirror sharp, higher --> blurrier
	vec3 reflection = textureLod(prefilteredMap, R, 1.0).rgb;
	vec3 reflectionNight = textureLod(prefilteredMapNight, R, 1.0).rgb;
	vec3 reflectionDayAndNight = mix(reflection, reflectionNight, interpolationFactor);

	// Schlick approximation: F(n, l) = F0 + (1 - F0) * (1 - cosTheta) ^ 5
	float f0 = 0.02f;
	float fresnel = f0 + (1 - f0) * pow(1 - max(dot(N, V), 0.0), 5.0);

	vec3 waterBase = vec3(0.02, 0.12, 0.2); // deep water base color
	vec3 mix = mix(waterBase, reflectionDayAndNight, fresnel);

	fragColor = vec4(mix, 1.0);
}