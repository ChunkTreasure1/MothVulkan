#version 460

layout(set = 0, binding = 0, rgba8) restrict writeonly uniform image2D o_output;
layout(set = 0, binding = 1) uniform sampler2D u_inputTexture;

layout(push_constant) uniform PushConstant
{
	int u_redTarget;
	int u_greenTarget;
	int u_blueTarget;
	int u_alphaTarget;

	vec2 u_targetSize;
};


layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main()
{
	const vec2 texCoords = vec2(gl_GlobalInvocationID.xy) / u_targetSize;
	const vec4 inputColor = texture(u_inputTexture, texCoords);

	vec4 result = vec4(0);

	switch(u_redTarget)
	{
		case 0:
		{
			result.r = inputColor.r;
			break;
		}
		
		case 1:
		{
			result.g = inputColor.r;
			break;
		}

		case 2:
		{
			result.b = inputColor.r;
			break;
		}

		case 3:
		{
			result.a = inputColor.r;
		}

		default:
		{
			break;
		}
	}

	switch(u_greenTarget)
	{
		case 0:
		{
			result.r = inputColor.g;
			break;
		}
		
		case 1:
		{
			result.g = inputColor.g;
			break;
		}

		case 2:
		{
			result.b = inputColor.g;
			break;
		}

		case 3:
		{
			result.a = inputColor.g;
		}

		default:
		{
			break;
		}
	}

	switch(u_blueTarget)
	{
		case 0:
		{
			result.r = inputColor.b;
			break;
		}
		
		case 1:
		{
			result.g = inputColor.b;
			break;
		}

		case 2:
		{
			result.b = inputColor.b;
			break;
		}

		case 3:
		{
			result.a = inputColor.b;
		}

		default:
		{
			break;
		}
	}

	switch(u_alphaTarget)
	{
		case 0:
		{
			result.r = inputColor.a;
			break;
		}
		
		case 1:
		{
			result.g = inputColor.a;
			break;
		}

		case 2:
		{
			result.b = inputColor.a;
			break;
		}

		case 3:
		{
			result.a = inputColor.a;
		}

		default:
		{
			break;
		}
	}

	imageStore(o_output, ivec2(gl_GlobalInvocationID.xy), result);
}