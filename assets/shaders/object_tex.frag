#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D uDiffuseTex;
uniform sampler2D uNormalTex;

out vec4 FragColor;

vec3 normal_from_map()
{
    vec3 N = normalize(vNormal);

    vec3 nTS = texture(uNormalTex, vTexCoord).xyz * 2.0 - 1.0;

    vec3 dp1 = dFdx(vWorldPos);
    vec3 dp2 = dFdy(vWorldPos);
    vec2 duv1 = dFdx(vTexCoord);
    vec2 duv2 = dFdy(vTexCoord);

    float det = duv1.x * duv2.y - duv1.y * duv2.x;
    if (abs(det) < 1e-8)
    {
        return N;
    }

    float invDet = 1.0 / det;

    vec3 T = (dp1 * duv2.y - dp2 * duv1.y) * invDet;
    T = normalize(T - N * dot(N, T));

    float sign = (det < 0.0) ? -1.0 : 1.0;
    vec3 B = sign * normalize(cross(N, T));

    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * nTS);
}

void main()
{
    vec3 albedo = texture(uDiffuseTex, vTexCoord).rgb;

    vec3 N = normal_from_map();
    vec3 L = normalize(vec3(0.35, 0.55, 0.75));
    float diff = max(dot(N, L), 0.0);

    vec3 base = albedo * (0.20 + 0.80 * diff);

    FragColor = vec4(base, 1.0);
}
