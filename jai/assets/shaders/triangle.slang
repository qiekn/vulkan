/* A minimal colored triangle.
 *
 * The vertex positions and colors are baked into the shader and indexed with
 * SV_VertexID, so the CPU side needs no vertex buffer -- and therefore no GPU
 * memory allocation at all. That is what lets the host program stay this small.
 */

// Vulkan's clip space has Y pointing down, so -0.5 is the top of the screen.
static const float2 POSITIONS[3] = {
    float2( 0.0, -0.5),
    float2( 0.5,  0.5),
    float2(-0.5,  0.5),
};

static const float3 COLORS[3] = {
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0),
};

struct VSOutput {
    float4 position : SV_Position;
    float3 color;
};

[shader("vertex")]
VSOutput vertex_main(uint vertex_index : SV_VertexID) {
    VSOutput output;
    output.position = float4(POSITIONS[vertex_index], 0.0, 1.0);
    output.color    = COLORS[vertex_index];
    return output;
}

[shader("fragment")]
float4 fragment_main(VSOutput input) : SV_Target {
    // The rasterizer interpolates `color` across the three vertices for us.
    // That interpolation is the whole "colored" part of the colored triangle.
    return float4(input.color, 1.0);
}
