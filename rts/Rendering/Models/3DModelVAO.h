/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <memory>

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VAO.h"

struct S3DModel;
struct S3DModelPiece;
struct SInstanceData;

class CUnit;
class CFeature;
struct UnitDef;

struct SDrawElementsIndirectCommand;

struct SIndexAndCount {
	SIndexAndCount() = default;
	SIndexAndCount(uint32_t index_, uint32_t count_)
		: index{ index_ }
		, count{ count_ }
	{}
	bool operator==(const SIndexAndCount& o) const { return index == o.index && count == o.count; }
	uint64_t operator()(const SIndexAndCount& o) const {
		return ((uint64_t)o.index << 32 | o.count);
	}
	uint32_t index;
	uint32_t count;
};

// singleton
class S3DModelVAO {
public:
	static void Init();
	static void Kill();
	static S3DModelVAO& GetInstance() { return *instance; }
public:
	static constexpr size_t INSTANCE_BUFFER_NUM_BATCHED = 2 << 15;
	static constexpr size_t INSTANCE_BUFFER_NUM_IMMEDIATE = 2 << 10;
	static constexpr size_t INSTANCE_BUFFER_NUM_ELEMS = INSTANCE_BUFFER_NUM_BATCHED + INSTANCE_BUFFER_NUM_IMMEDIATE;
public:
	S3DModelVAO();

	void Bind() const;
	void Unbind() const;

	bool AddToSubmission(const S3DModel* model, uint8_t teamID, uint8_t drawFlags);

	bool AddToSubmission(const CUnit* unit);
	bool AddToSubmission(const CFeature* feature);

	bool AddToSubmission(const UnitDef* unitDef, uint8_t teamID);
	void Submit(GLenum mode = GL_TRIANGLES, bool bindUnbind = false);

	bool SubmitImmediately(const S3DModel* model, uint8_t teamID, uint8_t drawFlags, GLenum mode = GL_TRIANGLES, bool bindUnbind = false);
	bool SubmitImmediately(const CUnit* unit, GLenum mode = GL_TRIANGLES, bool bindUnbind = false);
	bool SubmitImmediately(const CFeature* feature, GLenum mode = GL_TRIANGLES, bool bindUnbind = false);
	bool SubmitImmediately(const UnitDef* unitDef, int teamID, GLenum mode = GL_TRIANGLES, bool bindUnbind = false);

	const VBO* GetVertVBO() const { return &vertVBO; }
	      VBO* GetVertVBO()       { return &vertVBO; }
	const VBO* GetIndxVBO() const { return &indxVBO; }
	      VBO* GetIndxVBO()       { return &indxVBO; }
private:
	template<typename TObj>
	bool SubmitImmediatelyImpl(
		const TObj* obj,
		uint32_t indexStart,
		uint32_t indexCount,
		uint8_t teamID,
		uint8_t drawFlags,
		GLenum mode = GL_TRIANGLES,
		bool bindUnbind = false
	);
	template<typename TObj>
	bool AddToSubmissionImpl(
		const TObj* obj,
		uint32_t indexStart,
		uint32_t indexCount,
		uint8_t teamID,
		uint8_t drawFlags
	);
	void EnableAttribs(bool inst) const;
	void DisableAttribs() const;
private:
	inline static S3DModelVAO* instance = nullptr;
private:
	uint32_t batchedBaseInstance;
	uint32_t immediateBaseInstance; //note relative index

	VBO vertVBO;
	VBO indxVBO;

	VBO instVBO;
	VAO vao;

	std::unordered_map<SIndexAndCount, std::vector<SInstanceData>, SIndexAndCount> modelDataToInstance;
};