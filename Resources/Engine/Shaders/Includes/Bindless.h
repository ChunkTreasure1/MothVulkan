vec4 ReadTexture(uint index, vec2 texCoord)
{
	return texture(u_textures[index], texCoord);
}

Material GetMaterial(uint index)
{
	return u_materials[index];
}