#version 460

#extension GL_EXT_nonuniform_qualifier : enable

#include "Common.h" //! #include "../../Includes/Common.h"
#include "Buffers.h" //! #include "../../Includes/Buffers.h"
#include "Bindless.h" //! #include "../../Includes/Bindless.h"

layout(location = 0) out vec4 o_color;

layout(push_constant) uniform PushConstant
{
    mat4 transform;
    uint u_materialId;
};

layout(location = 0) in inData
{
    vec2 texCoords;

} v_inData;

void main()
{
    const Material material = GetMaterial(u_materialId);
    o_color = ReadTexture(material.albedo, v_inData.texCoords);
}