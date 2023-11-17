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

		const CameraInterface* getCamera(uint32_t idx) const;

	private:
		friend WolfEngine;

		CameraList() = default;

		void moveToNextFrame(const CameraUpdateContext& context);

		std::vector<CameraInterface*> m_currentCameras;
		std::vector<CameraInterface*> m_nextFrameCameras;
	};
}