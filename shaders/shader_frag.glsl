#version 410

// Note: We have followed LearnOpenGL's PBR shading tutorial to understand
// how it works. After finishing reading the tutorial, we tried to come up with the code on our own,
// without looking at the sample code given in the tutorial, following the instructions of the website.
// https://learnopengl.com/PBR/Theory -> Tutorial we used 

layout(std140) uniform Material // Must match the GPUMaterial defined in src/mesh.h
{
    vec3 kd;
	vec3 ks;
	float shininess;
	float transparency;
};

uniform sampler2D colorMap;
uniform bool hasTexCoords;
uniform bool useMaterial;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;
uniform vec3 lightColor;
uniform int useIBL; // If we use IBL or not (environment mapping)

// Texture-based information
uniform sampler2D textureAlbedo;
uniform sampler2D textureNormal;
uniform sampler2D textureMetallic;
uniform sampler2D textureRoughness;
uniform sampler2D textureAO;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

// Normal distribution function. The formula was taken from the PBR tutorial. This tutorial uses the function approximation called "Trowbridge-Reitz GGX"
float NDF(float pi, float roughness, vec3 normal, vec3 halfwayVector) {
    float ndfResult = pow(roughness, 4); // Mentioned on the tutorial (https://learnopengl.com/PBR/Lighting) 
    // that big animation companies like Disney and Epic Games prefer using the SQUARE of the roughness as the normal roughness. So I took the 4th power.
    // This is the case for the normal distribution function and the geometry function.

    ndfResult /= pi;
    ndfResult /= pow(pow( max(dot(normal, halfwayVector), 0.0), 2) * (pow(roughness, 4) - 1) + 1, 2);
    return ndfResult;
}

// As LearnOpenGL mentions, this function is based on the Schlick-GGX and the Smith method.
float geometryFunction(float roughness, vec3 normal, vec3 fragToCamera, vec3 fragToLight) {
    float k;
    if (useIBL == 0) {
        // No IBL is used
        k = pow(roughness + 1, 2) / 8;
    } else {
        // IBL is used
        k = pow(roughness, 2) / 2;
    }

    float normalDotFragToCamera = max(dot(normal, fragToCamera), 0.0);
    float normalDotFragToLight = max(dot(normal, fragToLight), 0.0);
    float g_nvk = normalDotFragToCamera / ((normalDotFragToCamera) * (1 - k) + k);
    float g_nlk = normalDotFragToLight / ((normalDotFragToLight) * (1 - k) + k);

    return g_nvk * g_nlk;
}

// Using the Fresnel-Schlick approximation of LearnOpenGL:
// Note that this also represents the ks (the fraction of light that is reflected), as LearnOpenGL mentions.
vec3 fresnel(vec3 halfwayVector, vec3 fragToCamera, vec3 F0) {
    return F0 + (1 - F0) * pow(clamp(1 - max(dot(halfwayVector,fragToCamera), 0.0), 0.0, 1.0), 5);
}



void main()
{
    float pi = radians(180.0); // Needed for the BRDF.
    vec3 normal = normalize(fragNormal);


    if (hasTexCoords)       { 

        // fragColor = vec4(texture(colorMap, fragTexCoord).rgb, 1);
        // Albedos found online are usually given the sRGB space. To convert
        // them to the linear space, LearnOpenGL mentioned that we need to take its power to 2.2.
        vec3 albedo = pow(texture(textureAlbedo, fragTexCoord).rgb, 2.2);
        // normal = normalize(texture(textureNormal, fragTexCoord).xyz); // Note to Yamaç: This will NOT give you the normal yet. Check the normal mapping lecture.
        float metallic = texture(textureMetallic, fragTexCoord).x;
        float roughness = texture(textureRougness, fragTexCoord).x; 
        float ambientOcclusion = texture(textureAO, fragTexCoord).x;

        vec3 fragToLight = normalize(lightPosition - fragPosition);
        vec3 fragToCamera = normalize(cameraPosition - fragPosition);
        vec3 halfwayVector = (fragToLight + fragToCamera) / length((fragToLight + fragToCamera));

        float fragToLightDistance = length(lightPosition - fragPosition);
        vec3 lightRadiance = lightColor / (fragToLightDistance * fragToLightDistance);

        vec3 F0 = vec3(0.04); // LearnOpenGL mentions that we can approximate the F0 of dielectric materials as a vector of 0.4s.
        F0 = mix(F0, albedo, metallic); // For metallic surfaces, the F0 value is changed using linear interpolation.

        vec3 ks = fresnel(halfwayVector, fragToCamera, F0); // Fraction of specular light (reflections)
        vec3 kd = vec3(1.0) - ks; // Fraction of diffuse light (refractions)

        // According to LearnOpenGL, we need to enforce no diffusion in metallic surfaces, so we need to modify kd with this equation:
        kd *= (1-metallic);
        
        // Calculate the rest of the function values
        float ndf = NDF(pi, roughness, normal, halfwayVector);
        float geometry = geometryFunction(roughness, normal, fragToCamera, fragToLight);
        float cosTheta = max(dot(fragToLight, normal), 0.0);
        // LearnOpenGL advises to add a 0.0001 to the denominator to ensure no divisions by zero
        vec3 outgoingRadiance = (kd * (albedo / pi) + ((ndf * geometry * F0) / (4 * max(dot(normal, fragToCamera), 0.0) * cosTheta + 0.0001))) * lightRadiance * cosTheta;

        vec3 ambient = vec3(0.03) * albedo * ambientOcclusion; // LearnOpenGL advises to add an ambient term
        vec3 color = ambient + outGoingRadiance;

        // Finally, implement gamma correcting for more realistic colors to the eye:

        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0/2.2));

        fragColor = vec4(color, 1.0);




    }
    else if (useMaterial)   { fragColor = vec4(kd, 1);}
    else                    { fragColor = vec4(normal, 1); } // Output color value, change from (1, 0, 0) to something else
}
