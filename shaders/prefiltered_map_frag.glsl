#version 410

// Shader used to create the pre-filtered environment map for the specular part of the lighting equation.
// As some other shaders, we have used the tutorial included in LearnOpenGL: https://learnopengl.com/PBR/IBL/Specular-IBL.
// Note that we followed the tutorial, noted down the main concepts, and built the code accordingly without looking
// at the tutorial's code.

// One other note: Generating the complete reversal of an integer's bit representation was not something we thought
// had a value to learn for this specific course. This function is thus directly taken from the following tutorial
// to create the Hammersley sequence, a low-discrepancy sequence commonly used in computer graphics: https://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html

uniform samplerCube skyboxMap;
uniform float roughness; // The "roughness" would now be accessed according to the mipmap layer, as 
// the pre-filtered map has different mip layers for different roughness values (increasing as we go on higher layers).

out vec4 fragColor;
in vec3 fragPosition;


// This function is used to reverse the bits of an integer, and then convert it again to an integer. Only this part
// of the shader code is copied and pasted from the above resource.

// LearnOpenGL warns that this operation may not work on older hardware (though the tutorial was published in 2012, so probably not a lot of computers these days.)
float reverseBits(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // Convert the bits to an integer representation
}

vec2 hammersleyFunction(uint currentSample, uint totalSamples) {
    return vec2 (float(currentSample) / float(totalSamples), reverseBits(currentSample));
}

vec3 importanceSample(vec2 lowDiscrepancySample, float roughness, vec3 zDirection) {
    float phi = 2 * radians(180) * lowDiscrepancySample.x;
    // In the normal formula, the roughness is squared, however, as per the explanation we gave in the default shader,
    // Epic Games and Disney's research led to the fact that roughness is more realistic when squared initially, so
    // it becomes a power of 4 (Source: LearnOpenGL)
    float costheta = sqrt((1 - lowDiscrepancySample.y) / (1 + (pow(roughness, 4)) * lowDiscrepancySample.y));
    float sintheta = sqrt((1 - pow(costheta, 2)));

    // Convert from the spherical coordinates to the cartesian coordinates:

    vec3 cartesianVector = vec3(cos(phi) * costheta, sin(phi)*sintheta, costheta);

    // Again, like the cubemap convolution, this code builds a new coordinate space around the normal of the position
    // of the fragment, with a higher chance that the vector we sample is sampled taking into consideration
    // that specular light reflects around a smaller blob. So instead of sampling the entire hemisphere LearnOpenGL
    // advises to find samples in this blob.

    vec3 yDirection;

    if (zDirection < 0.999) {
        yDirection = vec3(0.0f, 0.0f, 1.0f);
    } else {
        yDirection = vec3(1.0f, 0.0f, 0.0f);
    }

    vec3 xDirection = normalize(cross(yDirection, zDirection));
    yDirection = normalize(cross(zDirection, xDirection));

    vec3 sampleVector = xDirection * cartesianVector.x + yDirection * cartesianVector.y + zDirection * cartesianVector.z;
    return normalize(sampleVector);
}

void main()
{
    uint sampleCount = 1024u;
    vec3 fragNormal = normalize(fragPosition);

    vec3 color = vec3(0.0f);
    float totalSampleWeight = 0.0f;

    for (uint i = 0u; i < sampleCount; i = i + 1u) {
        vec2 lowDiscrepancySequence = hammersleyFunction(i, sampleCount);
        vec3 sampledVector = importanceSample(lowDiscrepancySequence, roughness, fragNormal);
        vec3 surfaceToLight = reflect(-fragNormal, sampledVector); // We have the normal and the reflected light, so do the reverse to find the light.

        float sampleWeight = max(dot(fragNormal, surfaceToLight), 0.0); // Treat the dot product between the light and the normal as how much weight that sample has. Light that arrives exactly parallel to the normal will contribute 100% to the specularity.

        if (sampleWeight > 0.0) {
            color += texture(skyboxMap, sampleWeight).xyz;
            totalSampleWeight += sampleWeight;
        }

    }

    color /= totalSampleWeight;

    fragColor = vec4(color, 1.0);
}