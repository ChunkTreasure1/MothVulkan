layout(std140, set = 1, binding = 0) uniform CameraBuffer
{
    CameraData u_cameraData;
};

layout(std140, set = 1, binding = 2) uniform PassBuffer
{
    PassData u_passData;
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

layout(std140, set = 0, binding = 1) uniform DirectionalLightBuffer
{
    DirectionalLight u_directionalLight;
};