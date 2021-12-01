#include "TexturesSet.h"

#include <cassert>
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"


TypedTexture::TypedTexture()
	: id{0}
	, type{GL_TEXTURE_2D}
{}

TypedTexture::TypedTexture(uint32_t id_)
	: id{ id_ }
	, type{ GL_TEXTURE_2D }
{}

TypedTexture::TypedTexture(uint32_t id_, uint32_t type_)
	: id{ id_ }
	, type{ type_ }
{}

TexturesSet::TexturesSet(const std::initializer_list<std::pair<uint8_t, TypedTexture>>& texturesList)
{
	for (const auto& [relBinding, texture] : texturesList) {
		assert(texture.id  < MAX_TEXTURE_ID);
		assert(relBinding < CGlobalRendering::MAX_TEXTURE_UNITS);
		auto [it, result] = textures.try_emplace(relBinding, texture);
		assert(result);
	}
}

TexturesSet::TexturesSet(const std::initializer_list<TypedTexture>& texturesList) {
	uint8_t relBinding = 0u;

	for (const auto& texture : texturesList) {
		assert(texture.id < MAX_TEXTURE_ID);
		assert(relBinding < CGlobalRendering::MAX_TEXTURE_UNITS);
		auto [it, result] = textures.try_emplace(relBinding, texture);
		assert(result);

		++relBinding;
	}
}

TexturesSet& TexturesSet::operator=(const TexturesSet& rhs)
{
	this->textures = rhs.textures;
	this->perfectHash = rhs.perfectHash;

	return *this;
}

TexturesSet& TexturesSet::operator=(TexturesSet&& rhs) noexcept
{
	std::swap(this->textures, rhs.textures);
	this->perfectHash = rhs.perfectHash;

	return *this;
}

void TexturesSet::UpsertTexture(uint8_t relBinding, TypedTexture texture)
{
	assert(texture.id  < MAX_TEXTURE_ID);
	assert(relBinding < CGlobalRendering::MAX_TEXTURE_UNITS);
	textures[relBinding] = texture.id;
	perfectHash = 0;
}

bool TexturesSet::RemoveTexture(uint8_t relBinding)
{
	assert(relBinding < CGlobalRendering::MAX_TEXTURE_UNITS);
	perfectHash = 0;
	return (textures.erase(relBinding) > 0);
}

std::size_t TexturesSet::GetHash() const
{
	if (textures.empty())
		return 0;

	if (perfectHash > 0)
		return perfectHash;

	for (const auto& [relBinding, texture] : textures) {
		perfectHash += relBinding * MAX_TEXTURE_ID + texture.id; //ignore the texture.type
	}

	return perfectHash;
}