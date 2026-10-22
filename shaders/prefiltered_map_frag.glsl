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

float NDF(float pi, float roughness, vec3 normal, vec3 halfwayVector) {
    float ndfResult = pow(roughness, 4); // Mentioned on the tutorial (https://learnopengl.com/PBR/Lighting) 
    // that big animation companies like Disney and Epic Games prefer using the SQUARE of the roughness as the normal roughness. So I took the 4th power.
    // This is the case for the normal distribution function and the geometry function.

    ndfResult /= pi;
    ndfResult /= pow(pow( max(dot(normal, halfwayVector), 0.0), 2) * (pow(roughness, 4) - 1) + 1, 2);
    return ndfResult;
}

void main()
{
    uint sampleCount = 1024u;
    vec3 fragNormal = normalize(fragPosition);

    vec3 color = vec3(0.0f);
    float totalSampleWeight = 0.0f;

    for (uint i = 0u; i < sampleCount; i = i + 1u) {
        vec2 lowDiscrepancySequence = hammersleyFunction(i, sampleCount);
        vec3 sampledVector = importanceSample(lowDiscrepancySequence, roughness, fragNormal); // Halfway vector
        vec3 surfaceToLight = reflect(-fragNormal, sampledVector); // We have the normal and the reflected light, so do the reverse to find the light.

        float sampleWeight = max(dot(fragNormal, surfaceToLight), 0.0); // Treat the dot product between the light and the normal as how much weight that sample has. Light that arrives exactly parallel to the normal will contribute 100% to the specularity.

        // The following part is used to find the mip location within the skybox that we will use to access the texture. The function formulas were
        // taken from LearnOpenGL's tutorial above. The LearnOpenGL tutorial itself uses this tutorial to find the mip level: https://chetanjags.wordpress.com/2015/08/26/image-based-lighting/.
        // Again, this part was written from understanding the tutorial, then writing the code without looking at the tutorial.
        if (sampleWeight > 0.0) {
            float ndf = NDF(radians(180), roughness, fragNormal, sampledVector);
            float pdf = (ndf * max(sampleWeight, fragNormal, 0.0) / ( 4 * max(dot(sampleWeight, surfaceToLight), 0.0))) + 0.0001;
            float solidAngleArea = 4 * radians(180) / (6 * pow(512, 2)); // 512 due to the resolution of the original cube map. This is to find the area of one solid angle.
            float monteCarloArea = 1 / (float(sampleCount) * pdf + 0.0001);
            float mipMapLevel = (roughness == 0.0f ? 0.0f : 0.5f * log2(solidAngleArea / monteCarloArea));

            color += texture(skyboxMap, surfaceToLight, mipMapLevel).xyz * sampleWeight;
            totalSampleWeight += sampleWeight;
        }

    }

    color /= totalSampleWeight;

    fragColor = vec4(color, 1.0);
}