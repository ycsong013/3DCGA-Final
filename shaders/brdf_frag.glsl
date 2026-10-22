#version 410

out vec2 fragColor;

in vec2 fragTexCoord;

// The concepts behind this shader were learned from https://learnopengl.com/PBR/IBL/Specular-IBL. This shader is
// used to create the second part of the split-sum integral, namely the integration of the BRDF term. The sampling
// concepts are similar to the pre-filtered environment map creation.

// This shader works in 2D as the BRDF map is 2D.

// The "reverseBits", "hammersleyFunction", and "importanceSample" functions were copied and pastied from the code that we already have from the prefiltered map
// creation shader. 

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

// This function is copied and pasted from the main PBR shader.
float geometryFunction(float roughness, vec3 normal, vec3 fragToCamera, vec3 fragToLight) {
    float k;
    // IBL is used: Difference from the main PBR shader
    k = pow(roughness, 2) / 2;

    float normalDotFragToCamera = max(dot(normal, fragToCamera), 0.0);
    float normalDotFragToLight = max(dot(normal, fragToLight), 0.0);
    float g_nvk = normalDotFragToCamera / ((normalDotFragToCamera) * (1 - k) + k);
    float g_nlk = normalDotFragToLight / ((normalDotFragToLight) * (1 - k) + k);

    return g_nvk * g_nlk;
}

void main() 
{
    vec3 fragToCamera = vec3(sqrt((1 - pow(fragTexCoord.x, 2))), 0.0, fragTexCoord.x);
    float roughness = fragTexCoord.y;

    float scaleTerm = 0;
    float biasTerm = 0;

    vec3 fragNormal = (0.0f, 0.0f, 1.0f);

    uint sampleCount = 1024u;

    for (uint i = 0u; i < sampleCount; i = i + 1u) {
        vec2 lowDiscrepancySequence = hammersleyFunction(i, sampleCount);
        vec3 sampledVector = importanceSample(lowDiscrepancySequence, roughness, fragNormal); // sampled halfway vector.

        vec3 fragToLight = reflect(-fragNormal, sampledVector);

        // Here, N: fragNormal, H: halfway vector, V: surfaceToLight
        float NdotL = max(dot(fragNormal, fragToLight), 0.0);
        float NdotH = max(dot(fragNormal, sampledVector), 0.0);
        float VdotH = max(dot(fragToLight, sampledVector), 0.0); // 

        if (NdotL > 0.0) {
            float geometry = geometryFunction(roughness, fragNormal, fragToCamera, fragToLight);
            float brdfTerm = (geometry * VdotH) / (NdotH * NdotV + 0.0001);
            float fresnelTerm = pow(1 - VdotH, 5);
            scaleTerm += (1 - fresnelTerm) * brdfTerm;
            biasTerm += fresnelTerm * brdfTerm;
        }
    }

    scaleTerm /= float(sampleCount);
    biasTerm /= float(sampleCount);

    fragColor = vec2(scaleTerm, biasTerm);
}