/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CAMERA_CONTROLLER_H
#define _CAMERA_CONTROLLER_H

#include <algorithm>
#include <array>
#include <string>

#include "System/float3.h"


class CCameraController
{
public:
	struct StateMap {
	public:
		typedef std::pair<std::string, float> MapPair;
		typedef std::array<MapPair, 32> ArrayMap;

		typedef ArrayMap::iterator iterator;
		typedef ArrayMap::const_iterator const_iterator;

		iterator begin() { return (pairsMap.begin()); }
		iterator end() { return (pairsMap.begin() + numPairs); }

		const_iterator cbegin() const { return (pairsMap.cbegin()); }
		const_iterator cend() const { return (pairsMap.cbegin() + numPairs); }

		const_iterator find(const std::string& s) const {
			const auto pair = std::make_pair(s, 0.0f);
			const auto pred = [](const MapPair& a, const MapPair& b) { return (a.first < b.first); };
			const auto iter = std::lower_bound(cbegin(), cend(), pair, pred);
			if (iter == cend() || iter->first != s)
				return (cend());
			return iter;
		}

		float operator [](const std::string& s) const {
			const auto iter = find(s);
			if (iter == cend())
				return 0.0f;
			return iter->second;
		}

		float& operator [](const std::string& s) {
			const auto iter = find(s);

			if (iter == cend()) {
				// Spring.SetCameraState might be handed a table larger than we can hold
				if (numPairs == pairsMap.size())
					return dummyPair.second;

				pairsMap[numPairs++] = std::make_pair(s, 0.0f);

				// inefficient on repeated insertions, but map size is a small constant
				for (size_t i = numPairs - 1; i > 0; i--) {
					if (pairsMap[i - 1].first < pairsMap[i].first)
						return pairsMap[i].second;

					std::swap(pairsMap[i - 1], pairsMap[i]);
				}

				return pairsMap[0].second;
			}

			return pairsMap[iter - cbegin()].second;
		}

		bool operator == (const StateMap& sm) const {
			return (numPairs == sm.numPairs && pairsMap == sm.pairsMap);
		}

		bool empty() const { return (numPairs == 0); }
		void clear() {
			numPairs = 0;

			for (auto& pair: pairsMap) {
				pair = {};
			}
		}

	private:
		ArrayMap pairsMap;
		MapPair dummyPair;

		size_t numPairs = 0;
	};

public:
	CCameraController();
	virtual ~CCameraController() {}

	virtual const std::string GetName() const = 0;

	virtual void KeyMove(float3 move) = 0;
	virtual void MouseMove(float3 move) = 0;
	virtual void ScreenEdgeMove(float3 move) = 0;
	virtual void MouseWheelMove(float move) = 0;

	virtual void Update() {}

	float GetFOV() const { return fov; } //< In degrees!

	virtual float3 GetPos() const { return pos; }
	virtual float3 GetDir() const { return dir; }
	virtual float3 GetRot() const;

	virtual void SetPos(const float3& newPos) { pos = newPos; }
	virtual void SetDir(const float3& newDir) { dir = newDir; }
	virtual bool DisableTrackingByKey() { return true; }

	// return the position to send to new controllers SetPos
	virtual float3 SwitchFrom() const = 0;
	virtual void SwitchTo(const int oldCam, const bool showText = true) = 0;

	virtual void GetState(StateMap& sm) const;
	virtual bool SetState(const StateMap& sm);
	virtual void SetTrackingInfo(const float3& pos, float radius) { SetPos(pos); }

	/**
	 * Whether the camera's distance to the ground or to the units
	 * should be used to determine whether to show them as icons or normal.
	 * This is called every frame, so it can be dynamically,
	 * eg depend on the current position/angle.
	 */
	virtual bool GetUseDistToGroundForIcons();

	/// should this mode appear when we toggle the camera controller?
	bool enabled;

protected:
	bool SetStateBool(const StateMap& sm, const std::string& name, bool& var);
	bool SetStateFloat(const StateMap& sm, const std::string& name, float& var);


	float fov;
	float3 pos;
	float3 dir;

	/**
	* @brief scrollSpeed
	* scales the scroll speed in general
	* (includes middleclick, arrowkey, screenedge scrolling)
	*/
	float scrollSpeed;

	/**
	 * @brief switchVal
	 * Where to switch from Camera-Unit-distance to Camera-Ground-distance
	 * for deciding whether to draw 3D view or icon of units.
	 * * 1.0 = 0 degree  = overview
	 * * 0.0 = 90 degree = first person
	 */
	float switchVal;

	float pixelSize;
};


#endif // _CAMERA_CONTROLLER_H
