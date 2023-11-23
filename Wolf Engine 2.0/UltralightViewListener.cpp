#include "UltralightViewListener.h"

#include "Debug.h"
#include "InputHandler.h"
#include "Window.h"

using namespace Wolf;

UltralightViewListener::UltralightViewListener(const InputHandler& inputHandler) : m_inputHandler(inputHandler)
{
}

void UltralightViewListener::OnAddConsoleMessage(ultralight::View* caller, ultralight::MessageSource source,
                                       ultralight::MessageLevel level, const ultralight::String& message, uint32_t line_number, uint32_t column_number,
                                       const ultralight::String& source_id)
{
    const std::string logMessage = "Ultralight console: " + std::string(message.utf8().data()) + " in " + std::string(source_id.utf8().data()) + " line " + std::to_string(line_number);

    switch (level)
    {
    case ultralight::kMessageLevel_Log:
    case ultralight::kMessageLevel_Debug:
    case ultralight::kMessageLevel_Info:
        Debug::sendInfo(logMessage);
        break;
    case ultralight::kMessageLevel_Warning:
        Debug::sendWarning(logMessage);
        break;
    case ultralight::kMessageLevel_Error: 
        Debug::sendError(logMessage);
        break;
    }
}

void UltralightViewListener::OnChangeCursor(ultralight::View* caller, ultralight::Cursor cursor)
{
    if (cursor != m_currentCursor)
    {
        m_currentCursor = cursor;

        Window::CursorType windowCursorType = Window::CursorType::POINTER;
        switch (cursor)
        {
        case ultralight::kCursor_Pointer:
            windowCursorType = Window::CursorType::POINTER;
            break;
        case ultralight::kCursor_Hand:
            windowCursorType = Window::CursorType::HAND;
            break;
        case ultralight::kCursor_IBeam:
            windowCursorType = Window::CursorType::IBEAM;
            break;
        case ultralight::kCursor_ColumnResize:
            windowCursorType = Window::CursorType::HRESIZE;
            break;
        default:
	        Debug::sendError("Unhandled cursor type");
            break;
        }

        m_inputHandler.setCursorType(windowCursorType);
    }
}
