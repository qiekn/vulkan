struct VertexInput {
    [[vk::location(0)]] float3 pos;
    [[vk::location(1)]] float3 color;
    [[vk::location(2)]] float2 texCoord;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

struct VertexOutput {
    float4 pos : SV_Position;
    float3 color;
    float2 texCoord;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input, uniform UniformBuffer *ubo) {
    VertexOutput output;
    output.pos = mul(ubo->proj, mul(ubo->view, mul(ubo->model, float4(input.pos, 1.0))));
    output.color = input.color;
    output.texCoord = input.texCoord;
    return output;
}

Sampler2D tex;

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target {
    return tex.Sample(inVert.texCoord);
}
