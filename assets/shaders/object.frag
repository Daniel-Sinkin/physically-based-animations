#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uColor;
uniform sampler2D uEnvironmentTex;
uniform vec3 uCameraPos;
uniform float uEnvLightStrength;

out vec4 FragColor;

const float kInvPi = 0.31830988618;
const float kInvTwoPi = 0.15915494309;

vec2 direction_to_equirect_uv(vec3 dir)
{
    vec3 d = normalize(dir);
    float u = atan(d.y, d.x) * kInvTwoPi + 0.5;
    float v = acos(clamp(d.z, -1.0, 1.0)) * kInvPi;
    return vec2(u, v);
}

vec3 sample_environment_rough(vec3 dir, float roughness)
{
    const float kMaxLod = 8.0;
    float lod = clamp(roughness, 0.0, 1.0) * kMaxLod;
    return textureLod(uEnvironmentTex, direction_to_equirect_uv(dir), lod).rgb;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(vec3(0.35, 0.55, 0.75));
    vec3 H = normalize(V + L);

    float diff = max(dot(N, L), 0.0);
    float ndotv = max(dot(N, V), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 64.0) * step(0.0, diff);

    vec3 envDiffuse = sample_environment_rough(N, 1.0);
    vec3 envSpecular = sample_environment_rough(reflect(-V, N), 0.8);
    vec3 F0 = vec3(0.04);
    vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - ndotv, 5.0);

    vec3 base = uColor * (0.20 + 0.70 * diff);
    base += uColor * envDiffuse * (0.18 * uEnvLightStrength);
    base += envSpecular * fresnel * (0.14 * uEnvLightStrength + 0.10 * spec);

    FragColor = vec4(base, 1.0);
}
