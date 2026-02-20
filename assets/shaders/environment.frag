#version 330 core

in vec3 vWorldPos;

uniform sampler2D uEnvironmentTex;
uniform vec3 uCameraPos;

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

void main()
{
    vec3 dir = normalize(vWorldPos - uCameraPos);
    vec3 env = texture(uEnvironmentTex, direction_to_equirect_uv(dir)).rgb;
    FragColor = vec4(env, 1.0);
}
