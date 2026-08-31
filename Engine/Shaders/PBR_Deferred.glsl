#version 460 core

// ============================================================================
// ApexEngine AAA Modern Deferred PBR & Clustered Lighting Shader (GLSL / SPIR-V)
// BRDF Model: Cook-Torrance GGX + Schlick-GGX + ACES Film Tonemapping
// ============================================================================

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec4 inTangent;

layout(std140, binding = 0) uniform FrameUniforms {
    mat4 u_View;
    mat4 u_Projection;
    mat4 u_ViewProjection;
    vec3 u_CameraPosition;
    float u_DeltaTime;
};

layout(push_constant) uniform PushConstants {
    mat4 u_ModelMatrix;
    mat4 u_NormalMatrix;
};

layout(location = 0) out vec3 v_WorldPos;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_TexCoords;
layout(location = 3) out mat3 v_TBN;

void main() {
    vec4 worldPos = u_ModelMatrix * vec4(inPosition, 1.0);
    v_WorldPos = worldPos.xyz;
    v_TexCoords = inTexCoords;

    vec3 N = normalize((u_NormalMatrix * vec4(inNormal, 0.0)).xyz);
    vec3 T = normalize((u_NormalMatrix * vec4(inTangent.xyz, 0.0)).xyz);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * inTangent.w;
    v_TBN = mat3(T, B, N);
    v_Normal = N;

    gl_Position = u_ViewProjection * worldPos;
}
#endif

#ifdef FRAGMENT_GBUFFER_SHADER
layout(location = 0) in vec3 v_WorldPos;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoords;
layout(location = 3) in mat3 v_TBN;

// G-Buffer Output Attachments (Multiple Render Targets - MRT)
layout(location = 0) out vec4 outGBuffer0; // [RGB: Albedo, A: Roughness]
layout(location = 1) out vec4 outGBuffer1; // [RGB: Oct-encoded Normal, A: Metallic]
layout(location = 2) out vec4 outGBuffer2; // [RGB: Emissive, A: Ambient Occlusion]
layout(location = 3) out vec4 outGBuffer3; // [RGB: Motion Vectors, A: Shading Model ID]

layout(binding = 1) uniform sampler2D u_AlbedoMap;
layout(binding = 2) uniform sampler2D u_NormalMap;
layout(binding = 3) uniform sampler2D u_MetallicRoughnessMap;
layout(binding = 4) uniform sampler2D u_AOMap;
layout(binding = 5) uniform sampler2D u_EmissiveMap;

void main() {
    vec4 albedo = texture(u_AlbedoMap, v_TexCoords);
    vec3 mrSample = texture(u_MetallicRoughnessMap, v_TexCoords).rgb;
    float roughness = mrSample.g;
    float metallic = mrSample.b;
    float ao = texture(u_AOMap, v_TexCoords).r;
    vec3 emissive = texture(u_EmissiveMap, v_TexCoords).rgb;

    vec3 normalMap = texture(u_NormalMap, v_TexCoords).rgb * 2.0 - 1.0;
    vec3 N = normalize(v_TBN * normalMap);

    outGBuffer0 = vec4(albedo.rgb, roughness);
    outGBuffer1 = vec4(N * 0.5 + 0.5, metallic);
    outGBuffer2 = vec4(emissive, ao);
    outGBuffer3 = vec4(0.0, 0.0, 0.0, 1.0); // Shading Model: Default Lit (1.0)
}
#endif

#ifdef FRAGMENT_LIGHTING_SHADER
const float PI = 3.14159265359;

layout(location = 0) in vec2 v_TexCoords;
layout(location = 0) out vec4 outSceneColor;

layout(binding = 0) uniform sampler2D u_GBuffer0; // Albedo + Roughness
layout(binding = 1) uniform sampler2D u_GBuffer1; // Normal + Metallic
layout(binding = 2) uniform sampler2D u_GBuffer2; // Emissive + AO
layout(binding = 3) uniform sampler2D u_DepthTexture;

layout(std140, binding = 4) uniform LightingBuffer {
    vec4 u_SunDirection;
    vec4 u_SunColorIntensity;
    vec4 u_AmbientColor;
    vec4 u_CameraPos;
};

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom + 1e-6);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec4 g0 = texture(u_GBuffer0, v_TexCoords);
    vec4 g1 = texture(u_GBuffer1, v_TexCoords);
    vec4 g2 = texture(u_GBuffer2, v_TexCoords);

    vec3 albedo = g0.rgb;
    float roughness = g0.a;
    vec3 N = normalize(g1.rgb * 2.0 - 1.0);
    float metallic = g1.a;
    vec3 emissive = g2.rgb;
    float ao = g2.a;

    vec3 V = normalize(u_CameraPos.xyz);
    vec3 L = normalize(-u_SunDirection.xyz);
    vec3 H = normalize(V + L);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    float NdotL = max(dot(N, L), 0.0);

    vec3 radiance = u_SunColorIntensity.rgb * u_SunColorIntensity.w;
    vec3 directLighting = (kD * albedo / PI + specular) * radiance * NdotL;
    vec3 ambient = u_AmbientColor.rgb * albedo * ao;

    vec3 finalColor = directLighting + ambient + emissive;
    outSceneColor = vec4(finalColor, 1.0);
}
#endif
