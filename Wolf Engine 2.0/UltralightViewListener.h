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

	    void OnAddConsoleMessage(ultralight::View* caller,
	                             ultralight::MessageSource source,
	                             ultralight::MessageLevel level,
	                             const ultralight::String& message,
	                             uint32_t line_number,
	                             uint32_t column_number,
	                             const ultralight::String& source_id) override;

	    void OnChangeCursor(ultralight::View* caller, ultralight::Cursor cursor) override;

    private:
	    ultralight::Cursor m_currentCursor = ultralight::kCursor_Pointer;
		ResourceNonOwner<InputHandler> m_inputHandler;
    };
}
