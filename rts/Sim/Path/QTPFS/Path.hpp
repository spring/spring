/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATH_HDR
#define QTPFS_PATH_HDR

#include <vector>

#include "System/float3.h"

namespace QTPFS {
	struct IPath {
		IPath(): pathID(0), ownerID(-1U), pointID(0), hash(-1U), radius(0.0f), synced(true) {}
		virtual ~IPath() {}

		void SetID(unsigned int pathID) { this->pathID = pathID; }
		void SetOwnerID(unsigned int ownerID) { this->ownerID = ownerID; }
		void SetPointID(unsigned int pointID) { this->pointID = pointID; }
		unsigned int GetID() const { return pathID; }
		unsigned int GetOwnerID() const { return ownerID; }
		unsigned int GetPointID() const { return pointID; }

		void SetHash(boost::uint64_t hash) { this->hash = hash; }
		void SetRadius(float radius) { this->radius = radius; }
		void SetSynced(bool synced) { this->synced = synced; }

		float GetRadius() const { return radius; }
		boost::uint64_t GetHash() const { return hash; }
		bool GetSynced() const { return synced; }

		virtual void SetBoundingBox() {}
		virtual void SetPoint(unsigned int, const float3&) {}
		virtual const float3& GetBoundingBoxMins() const {}
		virtual const float3& GetBoundingBoxMaxs() const {}
		virtual const float3& GetPoint(unsigned int) const { return ZeroVector; }

		virtual void SetObjectPoint(const float3&) {}
		virtual void SetSourcePoint(const float3&) {}
		virtual void SetTargetPoint(const float3&) {}
		virtual const float3& GetSourcePoint() const { return ZeroVector; }
		virtual const float3& GetTargetPoint() const { return ZeroVector; }
		virtual const float3& GetObjectPoint() const { return ZeroVector; }

		virtual unsigned int NumPoints() const { return 0; }
		virtual void AllocPoints(unsigned int) {}
		virtual void CopyPoints(const IPath&) {}

	protected:
		unsigned int pathID;
		unsigned int ownerID; // ID of the CSolidObject that requested us (unused)
		unsigned int pointID; // ID (index) of the next waypoint to be visited

		boost::uint64_t hash;
		float radius;
		bool synced;
	};

	struct Path: public IPath {
	public:
		Path(): IPath() {}
		Path(const Path& p) { *this = p; }
		Path& operator = (const Path& p) {
			pathID = p.GetID();
			ownerID = p.GetOwnerID();
			pointID = p.GetPointID();

			hash   = p.GetHash();
			synced = p.GetSynced();
			radius = p.GetRadius();

			points = p.GetPoints();
			point = p.GetObjectPoint();

			mins = p.GetBoundingBoxMins();
			maxs = p.GetBoundingBoxMaxs();
			return *this;
		}
		~Path() { points.clear(); }

		void SetBoundingBox() {
			mins.x = 1e6f; maxs.x = -1e6f;
			mins.z = 1e6f; maxs.z = -1e6f;

			for (unsigned int n = 0; n < points.size(); n++) {
				mins.x = std::min(mins.x, points[n].x);
				mins.z = std::min(mins.z, points[n].z);
				maxs.x = std::max(maxs.x, points[n].x);
				maxs.z = std::max(maxs.z, points[n].z);
			}
		}

		void SetPoint(unsigned int i, const float3& p) { points[std::min(i, NumPoints() - 1)] = p; }
		const float3& GetBoundingBoxMins() const { return mins; }
		const float3& GetBoundingBoxMaxs() const { return maxs; }
		const float3& GetPoint(unsigned int i) const { return points[std::min(i, NumPoints() - 1)]; }

		void SetObjectPoint(const float3& p) {                             point                     = p; }
		void SetSourcePoint(const float3& p) { assert(points.size() >= 2); points[                0] = p; }
		void SetTargetPoint(const float3& p) { assert(points.size() >= 2); points[points.size() - 1] = p; }
		const float3& GetObjectPoint() const { return point;                     }
		const float3& GetSourcePoint() const { return points[                0]; }
		const float3& GetTargetPoint() const { return points[points.size() - 1]; }

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

	private:
		std::vector<float3> points;

		// corners of the bounding-box containing all our points
		float3 mins;
		float3 maxs;

		// where on the map our owner (CSolidObject*) currently is
		// (normally lies roughly between two consecutive waypoints)
		float3 point;
	};
};

#endif

