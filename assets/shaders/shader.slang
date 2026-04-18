struct VertexInput {
    [[vk::location(0)]] float2 pos;
    [[vk::location(1)]] float3 color;
    [[vk::location(2)]] float2 texCoord;
};

struct UniformBuffer {
    float2 qiekn;
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
ConstantBuffer<UniformBuffer> ubo;

struct VertexOutput {
    float4 pos : SV_Position;
    float3 color;
    float2 texCoord;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input) {
    VertexOutput output;
    output.pos = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.pos, 0.0, 1.0))));
    output.color = input.color;
    output.texCoord = input.texCoord;
    return output;
}

Sampler2D tex;

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target {
    // texCoord is [0,1]; quad center is at texCoord (0.5, 0.5)
    float2 uv = inVert.texCoord * 2.0 - 1.0;
    float dist = length(uv);

    float fade = 0.005;
    float outerRadius = 1.0;
    float innerRadius = 0.0; // small center hole like a vinyl spindle (just remove, use full)

    float outer = 1.0 - smoothstep(outerRadius - fade, outerRadius, dist);
    float inner = smoothstep(innerRadius - fade, innerRadius, dist);
    float mask = outer * inner;

    if (mask < 0.001) discard;

    float4 sampled = tex.Sample(inVert.texCoord);
    return float4(sampled.rgb, sampled.a * mask);
}
