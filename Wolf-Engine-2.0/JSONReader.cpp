#include "JSONReader.h"

#include <codecvt>
#include <fstream>
#include <locale>
#include <stack>

#include "Debug.h"

std::string ws2s(const std::wstring& wstr)
{
	if (wstr.empty())
		return "";

	if (wstr.size() > 512)
		Wolf::Debug::sendCriticalError("Input is too big");

	char str[512];
	if (std::wcstombs(str, wstr.c_str(), 512) == 0)
		Wolf::Debug::sendError("Conversion doesn't seem to work");

	return { str };
}

Wolf::JSONReader::JSONReader(const FileReadInfo& fileReadInfo)
{
	std::wifstream inputFile(fileReadInfo.filename);

	Lines lines;

	std::wstring line;
	while (std::getline(inputFile, line))
	{
		lines.addLine(line);
	}

	readFromLines(lines);
}

Wolf::JSONReader::JSONReader(const StringReadInfo& stringReadInfo)
{
	Lines lines;

	std::wstring line(stringReadInfo.jsonData.begin(), stringReadInfo.jsonData.end());
	lines.addLine(line);

	readFromLines(lines);
}

Wolf::JSONReader::~JSONReader()
{
	std::function<void(JSONObject* object)> deleteDataInsideObject;
	deleteDataInsideObject = [&deleteDataInsideObject](JSONObject* object)
	{
		for (auto it = object->properties.begin(); it != object->properties.end(); ++it)
		{
			const JSONPropertyValue* propertyValue = it->second;
			if (propertyValue->type == JSONPropertyType::Object)
				deleteDataInsideObject(propertyValue->objectValue);
			else if(propertyValue->type == JSONPropertyType::ObjectArray)
			{
				for(JSONObject* object : propertyValue->objectArrayValue)
				{
					deleteDataInsideObject(object);
				}
			}
		}

		delete object;
	};

	deleteDataInsideObject(m_rootObject);
}

void Wolf::JSONReader::readFromLines(const Lines& lines)
{
	m_rootObject = new JSONObject;

	// JSON current state
	bool jsonStarted = false; // true inside main { }
	std::stack<JSONObject*>	currentObjectStack;
	currentObjectStack.push(m_rootObject);
	std::stack<JSONPropertyValue*> currentPropertyStack;

	enum class LookingFor { NextProperty, Colon, PropertyValue, Comma, NewArrayValue };
	LookingFor currentLookingFor = LookingFor::NextProperty;

	// helpers
	auto removeComments = [](std::wstring& line)
		{
			if (const size_t commentPos = line.find(L"//"); commentPos != std::wstring::npos)
			{
				// Check if it's not in a string
				bool isInString = false;
				size_t beginQuotePos = line.find('"');
				while (beginQuotePos != std::wstring::npos)
				{
					if (const size_t endQuotePos = line.find('"', beginQuotePos + 1); endQuotePos != std::wstring::npos)
					{
						if (beginQuotePos < commentPos && endQuotePos > commentPos)
						{
							isInString = true;
							break;
						}

						beginQuotePos = line.find('"', endQuotePos + 1);
					}
				}
					

				if (!isInString)
				{
					line = line.substr(0, commentPos);
				}
			}
		};
	auto removeSpaces = [](std::wstring& line)
		{
			std::erase_if(line, isspace);
		};

	auto lookForAFloat = [&removeSpaces](const std::wstring& line, float& outputFloat, size_t& outputLastCharacterPosition)
		{
			if (!line.empty())
			{
				std::wstring stringToTry;

				const size_t commaPos = line.find(',');
				const size_t endArray = line.find(']');
				const size_t endObject = line.find('}');

				const size_t minEndingPos = std::min(commaPos, std::min(endArray, endObject));

				if (minEndingPos != std::wstring::npos)
					stringToTry = line.substr(0, minEndingPos);
				else
					stringToTry = line;

				try
				{
					std::wstring stringWithoutSpaces = stringToTry; removeSpaces(stringWithoutSpaces);
					outputFloat = std::stof(stringWithoutSpaces);
					outputLastCharacterPosition = minEndingPos != std::wstring::npos ? minEndingPos : stringToTry.size();
					return true;
				}
				catch (std::invalid_argument const&)
				{
				}

				return false;
			}

			return false;
		};

	std::wstring line;
	int32_t currentLineNumber = -1;
	auto getLine = [&]()
		{
			if (line.empty())
			{
				currentLineNumber++;
				return lines.getNextLine(line);
			}
			return true;
		};

	while (getLine())
	{
		removeComments(line);

		if (!jsonStarted)
		{
			if (const size_t jsonStartingPos = line.find('{'); jsonStartingPos != std::string::npos)
			{
				line = line.substr(jsonStartingPos + 1);
				jsonStarted = true;
			}
			continue;
		}

		// Get next property
		if (currentLookingFor == LookingFor::NextProperty)
		{
			const size_t quotePos = line.find('"');
			const size_t endObjectPos = line.find('}');

			if (quotePos < endObjectPos)
			{
				line = line.substr(quotePos + 1);
				if (const size_t propertyNameEnding = line.find('"'); propertyNameEnding != std::string::npos)
				{
					std::wstring propertyName = line.substr(0, propertyNameEnding);

					if (currentObjectStack.top()->properties.contains(ws2s(propertyName)))
						Debug::sendError("JSON property " + ws2s(propertyName) + " is defined twice");

					currentObjectStack.top()->properties.insert({ ws2s(propertyName), new JSONPropertyValue() });
					currentPropertyStack.push(currentObjectStack.top()->properties[ws2s(propertyName)]);

					line = line.substr(propertyNameEnding + 1);
					currentLookingFor = LookingFor::Colon;
				}
				else
					Debug::sendError("JSON seems wrong: property doesn't end");
			}
			else if (endObjectPos != std::string::npos)
			{
				currentObjectStack.pop();

				if (currentObjectStack.empty())
					break;

				currentLookingFor = LookingFor::Comma;
				line = line.substr(endObjectPos + 1);
			}
		}
		else if (currentLookingFor == LookingFor::Colon)
		{
			if (const size_t colonPos = line.find(':'); colonPos != std::string::npos)
			{
				line = line.substr(colonPos + 1);
				currentLookingFor = LookingFor::PropertyValue;
			}
		}
		else if (currentLookingFor == LookingFor::Comma)
		{
			const size_t endArrayPos = line.find(']');
			const size_t commaPos = line.find(',');
			const size_t endObjectPos = line.find('}');

			if (commaPos < endArrayPos && commaPos < endObjectPos)
			{
				line = line.substr(commaPos + 1);

				bool isLookingForNextObjectArrayItem = !currentPropertyStack.empty() && currentPropertyStack.top()->type == JSONPropertyType::ObjectArray && currentObjectStack.top() != currentPropertyStack.top()->objectArrayValue.back();
				if (!currentPropertyStack.empty() && (currentPropertyStack.top()->type == JSONPropertyType::FloatArray || currentPropertyStack.top()->type == JSONPropertyType::StringArray || isLookingForNextObjectArrayItem))
				{
					currentLookingFor = LookingFor::NewArrayValue;
				}
				else
				{
					currentLookingFor = LookingFor::NextProperty;
				}
			}
			else if (endArrayPos != std::string::npos && endArrayPos < endObjectPos)
			{
				if (currentPropertyStack.empty() ||
					(currentPropertyStack.top()->type != JSONPropertyType::FloatArray && currentPropertyStack.top()->type != JSONPropertyType::StringArray && currentPropertyStack.top()->type != JSONPropertyType::UnknownArray && 
						currentPropertyStack.top()->type != JSONPropertyType::ObjectArray))
					Debug::sendError("JSON unexpected ']' when property is not array");

				currentPropertyStack.pop();
				line = line.substr(endArrayPos + 1);
				currentLookingFor = LookingFor::Comma;
			}
			else if (endObjectPos != std::string::npos)
			{
				JSONObject* removedObject = currentObjectStack.top();
				currentObjectStack.pop();

				if (currentObjectStack.empty())
					break;

				// If the object is a property, we need to remove it
				JSONPropertyValue* topProperty = currentPropertyStack.top();
				if (topProperty->objectValue == removedObject)
				{
					currentPropertyStack.pop();
				}

				currentLookingFor = LookingFor::Comma;
				line = line.substr(endObjectPos + 1);
			}
		}
		else if (currentLookingFor == LookingFor::PropertyValue)
		{
			const size_t bracePos = line.find('{');
			const size_t quotePos = line.find('"');
			const size_t bracketPos = line.find('[');

			std::wstring duplicatedLine = line;
			removeSpaces(duplicatedLine);
			char nextCharacter = static_cast<char>(toupper(duplicatedLine[0]));

			if (nextCharacter == 'F' || nextCharacter == 'T')
			{
				currentPropertyStack.top()->type = JSONPropertyType::Bool;
				if (nextCharacter == 'F')
				{
					currentPropertyStack.top()->boolValue = false;
				}
				else
				{
					currentPropertyStack.top()->boolValue = true;
				}
				const size_t propertyValueEnding = std::min(line.find('e'), line.find('E'));
				line = line.substr(propertyValueEnding + 1);
				currentPropertyStack.pop();
				currentLookingFor = LookingFor::Comma;
			}
			else if (isdigit(nextCharacter) || nextCharacter == '-')
			{
				float floatValue;
				size_t lastCharacterPos;
				if (lookForAFloat(line, floatValue, lastCharacterPos))
				{
					currentPropertyStack.top()->type = JSONPropertyType::Float;
					currentPropertyStack.top()->floatValue = floatValue;
					line = line.substr(lastCharacterPos);

					currentPropertyStack.pop();
					currentLookingFor = LookingFor::Comma;
				}
			}
			else if (bracePos < quotePos && bracePos < bracketPos)
			{
				currentPropertyStack.top()->type = JSONPropertyType::Object;
				currentPropertyStack.top()->objectValue = new JSONObject;

				currentObjectStack.push(currentPropertyStack.top()->objectValue);
				line = line.substr(line.find('{') + 1);
				currentLookingFor = LookingFor::NextProperty;
			}
			else if (quotePos < bracePos && quotePos < bracketPos)
			{
				currentPropertyStack.top()->type = JSONPropertyType::String;
				line = line.substr(quotePos + 1);
				size_t propertyStringEnding = line.find('"');

				while (propertyStringEnding != std::string::npos)
				{
					if (propertyStringEnding > 0 && line[propertyStringEnding - 1] == '\\')
					{
						line.erase(propertyStringEnding - 1, 1);
						propertyStringEnding = line.find('"', propertyStringEnding + 1);
						continue;
					}
					break;
				}

				if (propertyStringEnding != std::string::npos)
				{
					std::wstring propertyValue = line.substr(0, propertyStringEnding);

					currentPropertyStack.top()->stringValue = ws2s(propertyValue);

					line = line.substr(propertyStringEnding + 1);
					currentPropertyStack.pop();
					currentLookingFor = LookingFor::Comma;
				}
				else
					Debug::sendError("JSON seems wrong: property string value doesn't end");
			}
			else if (bracketPos < bracePos && bracketPos < quotePos)
			{
				currentPropertyStack.top()->type = JSONPropertyType::UnknownArray;

				line = line.substr(bracketPos + 1);
				currentLookingFor = LookingFor::NewArrayValue;
			}
		}
		else if (currentLookingFor == LookingFor::NewArrayValue)
		{
			std::wstring lineWithoutSpace = line;
			removeSpaces(lineWithoutSpace);

			const size_t bracePos = lineWithoutSpace.find('{');
			const size_t quotePos = lineWithoutSpace.find('"');

			if (bracePos == 0)
			{
				currentPropertyStack.top()->type = JSONPropertyType::ObjectArray;
				currentPropertyStack.top()->objectArrayValue.push_back(new JSONObject);

				currentObjectStack.push(currentPropertyStack.top()->objectArrayValue.back());
				line = line.substr(line.find('{') + 1);
				currentLookingFor = LookingFor::NextProperty;
			}
			else if (quotePos == 0)
			{
				currentPropertyStack.top()->type = JSONPropertyType::StringArray;
				line = line.substr(line.find('"') + 1);
				if (const size_t propertyStringEnding = line.find('"'); propertyStringEnding != std::string::npos)
				{
					std::wstring propertyValue = line.substr(0, propertyStringEnding);

					currentPropertyStack.top()->stringValue = ws2s(propertyValue);

					line = line.substr(propertyStringEnding + 1);
					currentLookingFor = LookingFor::Comma;
				}
				else
					Debug::sendError("JSON seems wrong: property string value doesn't end");
			}
			else
			{
				float floatValue;
				size_t lastCharacterPos;
				if (lookForAFloat(line, floatValue, lastCharacterPos))
				{
					currentPropertyStack.top()->type = JSONPropertyType::FloatArray;
					currentPropertyStack.top()->floatArrayValue.push_back(floatValue);
					line = line.substr(lastCharacterPos);
					currentLookingFor = LookingFor::Comma;
				}
			}
		}
	}
}

bool Wolf::JSONReader::JSONObject::hasProperty(const std::string& propertyName)
{
	return properties.contains(propertyName);
}

float Wolf::JSONReader::JSONObject::getPropertyFloat(const std::string& propertyName)
{
	return properties[propertyName]->floatValue;
}

const std::vector<float>& Wolf::JSONReader::JSONObject::getPropertyFloatArray(const std::string& propertyName)
{
	return properties[propertyName]->floatArrayValue;
}

const std::string& Wolf::JSONReader::JSONObject::getPropertyString(const std::string& propertyName)
{
	return properties[propertyName]->stringValue;
}

const std::string& Wolf::JSONReader::JSONObject::getPropertyString(uint32_t propertyIdx)
{
	auto it = properties.begin();
	std::advance(it, propertyIdx);
	return it->first;
}

bool Wolf::JSONReader::JSONObject::getPropertyBool(const std::string& propertyName)
{
	if (properties[propertyName]->type != JSONPropertyType::Bool)
		Debug::sendError("Property is not a bool");
	return properties[propertyName]->boolValue;
}

Wolf::JSONReader::JSONObjectInterface* Wolf::JSONReader::JSONObject::getPropertyObject(const std::string& propertyName)
{
	if (!properties.contains(propertyName))
		return nullptr;

	if (properties[propertyName]->type != JSONPropertyType::Object)
		Debug::sendError("Property is not an object");
	return properties[propertyName]->objectValue;
}

Wolf::JSONReader::JSONObjectInterface* Wolf::JSONReader::JSONObject::getArrayObjectItem(const std::string& propertyName, uint32_t idx)
{
	return properties[propertyName]->objectArrayValue[idx];
}

uint32_t Wolf::JSONReader::JSONObject::getArraySize(const std::string& propertyName)
{
	if (!properties.contains(propertyName))
		return 0;
	return static_cast<uint32_t>(properties[propertyName]->objectArrayValue.size());
}

uint32_t Wolf::JSONReader::JSONObject::getPropertyCount()
{
	return static_cast<uint32_t>(properties.size());
}
