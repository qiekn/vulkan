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
    return tex.Sample(inVert.texCoord);
}
