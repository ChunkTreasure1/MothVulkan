layout(std140, set = 1, binding = 0) uniform CameraBuffer
{
    CameraData u_cameraData;
};

layout(std140, set = 1, binding = 1) uniform TargetBuffer
{
    TargetData u_targetData;
};

layout(std140, set = 1, binding = 2) uniform PassBuffer
{
    PassData u_passData;
};

layout(std140, set = 1, binding = 3) uniform DirectionalLightBuffer
{
	DirectionalLight u_directionalLight;
};

//layout(std430, set = 0, binding = 4) readonly buffer ObjectMapBuffer
//{
//	uint u_objectMap[];
//};
//
//layout(std430, set = 0, binding = 3) readonly buffer ObjectBuffer
//{
//    ObjectData u_objectBuffer[];
//};

layout(set = 0, binding = 0) uniform sampler2D u_textures[];

layout(std430, set = 0, binding = 1) readonly buffer MaterialBuffer
{
    Material u_materials[];
};