#include "UltralightViewListener.h"

#include "Debug.h"
#include "InputHandler.h"
#include "Window.h"

using namespace Wolf;

UltralightViewListener::UltralightViewListener(const ResourceNonOwner<InputHandler>& inputHandler) : m_inputHandler(inputHandler)
{
}

void UltralightViewListener::OnAddConsoleMessage(ultralight::View* caller, const ultralight::ConsoleMessage& message)
{
    const std::string logMessage = "Ultralight console: " + std::string(message.message().utf8().data()) + " in " + std::string(message.source_id().utf8().data()) + " line " + std::to_string(message.line_number());

    switch (message.level())
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

        m_inputHandler->setCursorType(windowCursorType);
    }
}
