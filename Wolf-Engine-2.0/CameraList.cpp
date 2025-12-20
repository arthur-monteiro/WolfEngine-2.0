#include "CameraList.h"

#include "ProfilerCommon.h"

void Wolf::CameraList::addCameraForThisFrame(CameraInterface* camera, uint32_t idx)
{
	if (idx >= m_nextFrameCameras.size())
		m_nextFrameCameras.resize(idx + 1);

	m_nextFrameCameras[idx] = camera;
}

const Wolf::CameraInterface* Wolf::CameraList::getCamera(uint32_t idx) const
{
	if (idx >= m_currentCameras.size())
		return nullptr;

	return m_currentCameras[idx];
}

void Wolf::CameraList::moveToNextFrame(const CameraUpdateContext& context)
{
	PROFILE_FUNCTION

	m_currentCameras.clear();
	m_nextFrameCameras.swap(m_currentCameras);

	for (CameraInterface* camera : m_currentCameras)
	{
		// Camera can be null if not set for this index
		if (camera)
		{
			camera->update(context);
		}
	}
}
