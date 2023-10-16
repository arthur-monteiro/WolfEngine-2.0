#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Wolf
{
	class JSONReader
	{
	public:
		JSONReader(const std::string& filename);
		~JSONReader();

		class JSONObjectInterface
		{
		public:
			virtual float getPropertyFloat(const std::string& propertyName) = 0;
			virtual const std::vector<float>& getPropertyFloatArray(const std::string& propertyName) = 0;
			virtual const std::string& getPropertyString(const std::string& propertyName) = 0;
			virtual JSONObjectInterface* getArrayObjectItem(const std::string& propertyName, uint32_t idx) = 0;
		};
		JSONObjectInterface* getRoot() { return m_rootObject; }

	private:
		class JSONObject;

		enum class JSONPropertyType { String, Object, Float, UnknownArray, ObjectArray, FloatArray, Unknown };
		struct JSONPropertyValue
		{
			JSONPropertyType type;

			// Possible values
			float floatValue = 0;
			std::string stringValue;
			JSONObject* objectValue = nullptr;
			std::vector<JSONObject*> objectArrayValue;
			std::vector<float> floatArrayValue;

			JSONPropertyValue() : type(JSONPropertyType::Unknown) {};
		};

		class JSONObject final : public JSONObjectInterface
		{
		public:
			std::map<std::string, JSONPropertyValue*> properties;

			float getPropertyFloat(const std::string& propertyName) override;
			const std::vector<float>& getPropertyFloatArray(const std::string& propertyName) override;
			const std::string& getPropertyString(const std::string& propertyName) override;
			JSONObjectInterface* getArrayObjectItem(const std::string& propertyName, uint32_t idx) override;
		};

		JSONObject* m_rootObject;
	};
}

