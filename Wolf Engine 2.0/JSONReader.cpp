#include "JSONReader.h"

#include <fstream>
#include <stack>

#include "Debug.h"

Wolf::JSONReader::JSONReader(const std::string& filename)
{
	m_rootObject = new JSONObject;

	std::ifstream inputFile(filename);

	// JSON current state
	bool jsonStarted = false; // true inside main { }
	std::stack<JSONObject*>	currentObjectStack;
	currentObjectStack.push(m_rootObject);
	std::stack<JSONPropertyValue*> currentPropertyStack;

	enum class LookingFor { NextProperty, Colon, PropertyValue, Comma, NewArrayValue };
	LookingFor currentLookingFor = LookingFor::NextProperty;

	// helpers
	auto removeComments = [](std::string& line)
	{
		if (const size_t commentPos = line.find("//"); commentPos != std::string::npos)
		{
			line = line.substr(0, commentPos);
		}
	};
	auto removeSpaces = [](std::string& line)
	{
		std::erase_if(line, isspace);
	};

	auto lookForAFloat = [&removeSpaces](const std::string& line, float& outputFloat, size_t& outputLastCharacterPosition)
	{
		if (!line.empty())
		{
			std::string stringToTry;

			const size_t commaPos = line.find(',');
			const size_t endArray = line.find(']');
			const size_t endObject = line.find('}');

			const size_t minEndingPos = std::min(commaPos, std::min(endArray, endObject));

			if (minEndingPos != std::string::npos)
				stringToTry = line.substr(0, minEndingPos);
			else
				stringToTry = line;

			try
			{
				std::string stringWithoutSpaces = stringToTry; removeSpaces(stringWithoutSpaces);
				outputFloat = std::stof(stringWithoutSpaces);
				outputLastCharacterPosition = minEndingPos != std::string::npos ? minEndingPos : stringToTry.size();
				return true;
			}
			catch (std::invalid_argument const&)
			{}

			return false;
		}
	};

	std::string line;
	auto getLine = [&]()
	{
		if(line.empty())
		{
			return static_cast<bool>(std::getline(inputFile, line));
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
					std::string propertyName = line.substr(0, propertyNameEnding);

					if (currentObjectStack.top()->properties.contains(propertyName))
						Debug::sendError("JSON property " + propertyName + " is defined twice");

					currentObjectStack.top()->properties.insert({ propertyName, new JSONPropertyValue() });
					currentPropertyStack.push(currentObjectStack.top()->properties[propertyName]);

					line = line.substr(propertyNameEnding + 1);
					currentLookingFor = LookingFor::Colon;
				}
				else
					Debug::sendError("JSON " + filename + " seems wrong: property doesn't end");
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

			if (commaPos < endArrayPos)
			{
				line = line.substr(commaPos + 1);

				bool isLookingForNextObjectArrayItem = !currentPropertyStack.empty() && currentPropertyStack.top()->type == JSONPropertyType::ObjectArray && currentObjectStack.top() != currentPropertyStack.top()->objectArrayValue.back();
				if (!currentPropertyStack.empty() && (currentPropertyStack.top()->type == JSONPropertyType::FloatArray || isLookingForNextObjectArrayItem))
				{
					currentLookingFor = LookingFor::NewArrayValue;
				}
				else
				{
					currentLookingFor = LookingFor::NextProperty;
				}
			}
			else if (endArrayPos != std::string::npos)
			{
				if (currentPropertyStack.empty() ||
					(currentPropertyStack.top()->type != JSONPropertyType::FloatArray && currentPropertyStack.top()->type != JSONPropertyType::UnknownArray && currentPropertyStack.top()->type != JSONPropertyType::ObjectArray))
					Debug::sendError("JSON unexpected ']' when property is not array");

				currentPropertyStack.pop();
				line = line.substr(endArrayPos + 1);
				currentLookingFor = LookingFor::NextProperty;
			}
		}
		else if (currentLookingFor == LookingFor::PropertyValue)
		{
			const size_t bracePos = line.find('{');
			const size_t quotePos = line.find('"');
			const size_t bracketPos = line.find('[');

			if (bracePos == std::string::npos && quotePos == std::string::npos && bracketPos == std::string::npos)
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
			}
			else if(quotePos < bracePos && quotePos < bracketPos)
			{
				currentPropertyStack.top()->type = JSONPropertyType::String;
				line = line.substr(quotePos + 1);
				if (const size_t propertyStringEnding = line.find('"'); propertyStringEnding != std::string::npos)
				{
					std::string propertyValue = line.substr(0, propertyStringEnding);
					
					currentPropertyStack.top()->stringValue = propertyValue;

					line = line.substr(propertyStringEnding + 1);
					currentPropertyStack.pop();
					currentLookingFor = LookingFor::Comma;
				}
				else
					Debug::sendError("JSON " + filename + " seems wrong: property string value doesn't end");
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
			std::string lineWithoutSpace = line;
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
				Debug::sendError("JSON string array is not currently supported");
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

Wolf::JSONReader::JSONObjectInterface* Wolf::JSONReader::JSONObject::getArrayObjectItem(const std::string& propertyName, uint32_t idx)
{
	return properties[propertyName]->objectArrayValue[idx];
}
