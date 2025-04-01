#version 460
#extension GL_EXT_ray_tracing : enable

// 채윤님 dist 함수
float RandomValue(inout uint state) {
    state *= (state + 195439) * (state + 124395) * (state + 845921);
    return state / 4294967295.0; // 2^31 - 1 (uint 최댓값으로 나눔) -> 0~1 사이의 실수
}

float RandomValueNormalDistribution(inout uint state) {
    float theta = 2 * 3.1415926 * RandomValue(state);
    float rho = sqrt(-2 * log(RandomValue(state)));
    return rho * cos(theta);
}

vec3 RandomDirection(inout uint state) {
    float x = RandomValueNormalDistribution(state);
    float y = RandomValueNormalDistribution(state);
    float z = RandomValueNormalDistribution(state);
    return normalize(vec3(x, y, z));
}

vec3 RandomHemisphereDirection(vec3 normal, inout uint state) {
    vec3 dir = RandomDirection(state);
    return dir * sign(dot(normal, dir));
}

struct Vertex {
    vec4 pos;
    vec4 normal;
    vec2 tex;
};

layout(set = 0, binding = 0) uniform accelerationStructureEXT TLAS;
layout(set = 1, binding = 2) uniform sampler2D textureSampler;
layout(std430, set = 1, binding = 0) buffer Vertices      { Vertex vertices[]; };
layout(std430, set = 1, binding = 1) buffer Indices       { uint indices[]; };
layout(std430, set = 1, binding = 3) buffer RandomSamples { vec4 samples[]; };

layout(shaderRecordEXT) buffer CustomData { vec4 customColor; };

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 1) rayPayloadEXT uint missCount;

void main() {
    missCount = 0;

    uint idx0 = indices[gl_PrimitiveID * 3    ];
    uint idx1 = indices[gl_PrimitiveID * 3 + 1];
    uint idx2 = indices[gl_PrimitiveID * 3 + 2];

    // barycentric coordinate pos
    float bA = 1.0f - attribs.x - attribs.y;
    float bB = attribs.x;
    float bC = attribs.y;

    // local vertices pos
    vec3 p0 = vec3(vertices[idx0].pos);
    vec3 p1 = vec3(vertices[idx1].pos);
    vec3 p2 = vec3(vertices[idx2].pos);

    // local normal pos
    vec3 n0 = vec3(vertices[idx0].normal);
    vec3 n1 = vec3(vertices[idx1].normal);
    vec3 n2 = vec3(vertices[idx2].normal);

    // local pos & normal (interpolated)
    vec3 hitPosition = bA * p0 + bB * p1 + bC * p2;
    vec3 interpolatedN = bA * n0 + bB * n1 + bC * n2;

    vec3 worldHitPos = vec3(gl_ObjectToWorldEXT * vec4(hitPosition, 1.0f));
    vec3 worldNormal = normalize(transpose(inverse(mat3(gl_ObjectToWorldEXT))) * interpolatedN);

    uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    uint rayCount = 512;

    // 채윤님 dist
    uvec2 pixelCoord = gl_LaunchIDEXT.xy;
    uvec2 screenSize = gl_LaunchSizeEXT.xy;
    uint rngState = pixelCoord.y * screenSize.x + pixelCoord.x;

    for (uint i = 0; i < rayCount; ++i) {
        vec3 rayDir = RandomHemisphereDirection(worldNormal, rngState);     // 채윤님 dist
        vec3 rayFrom = worldHitPos + 0.34f * worldNormal;

        // hit group 1개 (closest hit, idx 0)
        // geometry 4개 (각 geometry에 closest hit만 적용할 것이므로 stride 1)

        // miss group 2개 (bgMiss, idx 0 | aoMiss, idx 1)
        // second ray는 aoMiss 사용 (idx 1)
        traceRayEXT(
            TLAS,                               // topLevel
            flags, 0xFF,                        // rayFlags, cullMask
            0, 1, 1,                            // sbtRecordOffset, sbtRecordStride, missIndex
            rayFrom, 0.34f, rayDir, 100.0f,     // origin, tmin, direction, tmax
            1                                   // payload location
        );
    }

    hitValue = vec3(1.0f, 1.0f, 1.0f) * (float(missCount) / rayCount);
}