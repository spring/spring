/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATH_HDR
#define QTPFS_PATH_HDR

#include <vector>

#include "System/float3.h"

namespace QTPFS {
	struct IPath {
		IPath(): pathID(0), pointID(0), hash(-1U), radius(0.0f), synced(true) {}
		IPath(const IPath& p) { *this = p; }
		IPath& operator = (const IPath& p) {
			pathID = p.GetID();
			pointID = p.GetPointID();

			hash   = p.GetHash();
			radius = p.GetRadius();
			synced = p.GetSynced();
			points = p.GetPoints();

			boundingBoxMins = p.GetBoundingBoxMins();
			boundingBoxMaxs = p.GetBoundingBoxMaxs();

			objectPoint = p.GetObjectPoint();
			return *this;
		}
		~IPath() { points.clear(); }

		void SetID(unsigned int pathID) { this->pathID = pathID; }
		void SetPointID(unsigned int pointID) { this->pointID = pointID; }
		unsigned int GetID() const { return pathID; }
		unsigned int GetPointID() const { return pointID; }

		void SetHash(boost::uint64_t hash) { this->hash = hash; }
		void SetRadius(float radius) { this->radius = radius; }
		void SetSynced(bool synced) { this->synced = synced; }

		float GetRadius() const { return radius; }
		boost::uint64_t GetHash() const { return hash; }
		bool GetSynced() const { return synced; }

		void SetBoundingBox() {
			boundingBoxMins.x = 1e6f; boundingBoxMaxs.x = -1e6f;
			boundingBoxMins.z = 1e6f; boundingBoxMaxs.z = -1e6f;

			for (unsigned int n = 0; n < points.size(); n++) {
				boundingBoxMins.x = std::min(boundingBoxMins.x, points[n].x);
				boundingBoxMins.z = std::min(boundingBoxMins.z, points[n].z);
				boundingBoxMaxs.x = std::max(boundingBoxMaxs.x, points[n].x);
				boundingBoxMaxs.z = std::max(boundingBoxMaxs.z, points[n].z);
			}
		}

		const float3& GetBoundingBoxMins() const { return boundingBoxMins; }
		const float3& GetBoundingBoxMaxs() const { return boundingBoxMaxs; }

		void SetPoint(unsigned int i, const float3& p) { points[std::min(i, NumPoints() - 1)] = p; }
		const float3& GetPoint(unsigned int i) const { return points[std::min(i, NumPoints() - 1)]; }

		void SetSourcePoint(const float3& p) { assert(points.size() >= 2); points[                0] = p; }
		void SetTargetPoint(const float3& p) { assert(points.size() >= 2); points[points.size() - 1] = p; }
		const float3& GetSourcePoint() const { return points[                0]; }
		const float3& GetTargetPoint() const { return points[points.size() - 1]; }

		void SetObjectPoint(const float3& p) { objectPoint = p; }
		const float3& GetObjectPoint() const { return objectPoint; }

		unsigned int NumPoints() const { return (points.size()); }
		void AllocPoints(unsigned int n) {
			points.clear();
			points.resize(n);
		}
		void CopyPoints(const IPath& p) {
			AllocPoints(p.NumPoints());

			for (unsigned int n = 0; n < p.NumPoints(); n++) {
				points[n] = p.GetPoint(n);
			}
		}

		const std::vector<float3>& GetPoints() const { return points; }

	protected:
		unsigned int pathID;
		unsigned int pointID; // ID (index) of the next waypoint to be visited

		boost::uint64_t hash;
		float radius;
		bool synced;

		std::vector<float3> points;

		// corners of the bounding-box containing all our points
		float3 boundingBoxMins;
		float3 boundingBoxMaxs;

		// where on the map our owner (CSolidObject*) currently is
		// (normally lies roughly between two consecutive waypoints)
		float3 objectPoint;
	};
};

#endif

