#pragma once

#include <vector>

#include "CameraInterface.h"

namespace Wolf
{
	class WolfEngine;

	class CameraList
	{
	public:
		void addCameraForThisFrame(CameraInterface* camera, uint32_t idx);

		void moveToNextFrame();

		const CameraInterface* getCamera(uint32_t idx) const;
		const std::vector<CameraInterface*>& getCurrentCameras() const { return m_currentCameras; }

	private:
		friend WolfEngine;

		CameraList() = default;

		void update(const CameraUpdateContext& context) const;

		std::vector<CameraInterface*> m_currentCameras;
		std::vector<CameraInterface*> m_nextFrameCameras;
	};
}