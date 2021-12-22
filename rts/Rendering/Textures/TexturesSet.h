#pragma once

#include <cstdint>
#include <unordered_map>

struct TypedTexture {
	TypedTexture();
	TypedTexture(uint32_t id_);
	TypedTexture(uint32_t id_, uint32_t type_);
	uint32_t id;
	uint32_t type;
};

class TexturesSet {
public:
	TexturesSet() = default;
	TexturesSet(const std::initializer_list<std::pair<uint8_t, TypedTexture>>& texturesList);
	TexturesSet(const std::initializer_list<TypedTexture>& texturesList);

	TexturesSet(const TexturesSet& rhs) { *this = rhs; }
	TexturesSet(TexturesSet&& rhs) noexcept { *this = std::move(rhs); }

	TexturesSet& operator= (const TexturesSet& rhs);
	TexturesSet& operator= (TexturesSet&& rhs) noexcept;

	bool operator==(const TexturesSet& rhs) const { return (GetHash() == rhs.GetHash()); }
	bool operator!=(const TexturesSet& rhs) const { return (GetHash() != rhs.GetHash()); }

	void UpsertTexture(const std::pair <uint8_t, TypedTexture>& tex) { UpsertTexture(tex.first, tex.second); }
	void UpsertTexture(uint8_t relBinding, TypedTexture texture);
	bool RemoveTexture(const std::pair <uint8_t, TypedTexture>& tex) { return RemoveTexture(tex.first); }
	bool RemoveTexture(uint8_t relBinding);

	std::size_t GetHash() const;
private:
	mutable std::size_t perfectHash = 0u;
	std::unordered_map<uint8_t, TypedTexture> textures; //relBinding, textureID

	static constexpr uint32_t MAX_TEXTURE_ID = 1 << 17; //enough for everyone (c)
};

class TexturesSetHash {
public:
	std::size_t operator()(const TexturesSet& ts) const { return ts.GetHash(); }
};
