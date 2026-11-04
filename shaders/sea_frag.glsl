#version 410
in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

uniform samplerCube prefilteredMap;
uniform samplerCube prefilteredMapNight;
uniform vec3 cameraPosition;
uniform float interpolationFactor;
uniform sampler2D shadowMap;
uniform vec3 lightColor;
uniform vec3 sunDirection;
uniform bool useDirectionalLight;
uniform float sunIntensity;
uniform mat4 lightMVPMatrix;
uniform bool useShadows;
uniform bool dayNightSystem;
uniform float shininess;

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

	// Add the boat's shadow if shadowing has been turned on for use.
	float shadowFactor = 0.0f;
        // Shadow calculation: (Similar implementation to my answer to assignment 4.)
        if (useDirectionalLight && useShadows) {
            vec4 fragLightCoord = lightMVPMatrix * vec4(fragPosition, 1.0);
            // Divide by w because fragLightCoord are homogeneous coordinates
            fragLightCoord.xyz /= fragLightCoord.w;
            // The resulting value is in NDC space (-1 to +1),
            //  we transform them to texture space (0 to 1).
            fragLightCoord.xyz = fragLightCoord.xyz * 0.5 + 0.5;
            // Depth of the fragment with respect to the light
            float fragLightDepth = fragLightCoord.z;
            // Shadow map coordinate corresponding to this fragment
            vec2 shadowMapCoord = fragLightCoord.xy;
            // Shadow map value from the corresponding shadow map position
            float shadowMapDepth = texture(shadowMap, shadowMapCoord).x;

            vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
            // Using the 3x3 nearest neighbor technique for shadows, again my implementation from assignment 4.
            float shadow = 0.0f;
            for (int x= -1; x <= 1; x++) {
                for (int y= -1; y<= 1; y++){
                    float neighborDepth = texture(shadowMap, shadowMapCoord + vec2(x,y) * texelSize).x;
                    shadow += fragLightDepth - 0.01 > neighborDepth ? 1.0f : 0.0f;
                }
            }

                shadowFactor = shadow / 9;
        }

	vec3 sunColor = vec3(0.0f);
	// Add in the sun's color and the sun's reflections to the base color of the sea if the sun has been turned on.
	if (useDirectionalLight) {
		// Simple Lambert and Blinn Phong shading, as detailed in assignment 3. This is from my implementation of assignment 3.
		vec3 diffuseColor = waterBase * lightColor * sunIntensity * max(dot(N, normalize(sunDirection)), 0.0);
		vec3 halfwayVector = normalize(V + normalize(sunDirection));
		vec3 specularColor = lightColor * sunIntensity * pow(max(dot(N, halfwayVector), 0.0), shininess);
		sunColor = (1 - shadowFactor) * (diffuseColor + specularColor);
	}

	fragColor = vec4(mix + sunColor, 1.0);
}