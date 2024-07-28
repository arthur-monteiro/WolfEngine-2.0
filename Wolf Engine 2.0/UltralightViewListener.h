#pragma once

#include <string>
#include <Ultralight/Listener.h>

#include "ResourceNonOwner.h"

namespace Wolf
{
	class InputHandler;

	class UltralightViewListener : public ultralight::ViewListener
    {
    public:
	    explicit UltralightViewListener(const ResourceNonOwner<InputHandler>& inputHandler);

	    void OnAddConsoleMessage(ultralight::View* caller, const ultralight::ConsoleMessage& message) override;

	    void OnChangeCursor(ultralight::View* caller, ultralight::Cursor cursor) override;

    private:
	    ultralight::Cursor m_currentCursor = ultralight::kCursor_Pointer;
		ResourceNonOwner<InputHandler> m_inputHandler;
    };
}
