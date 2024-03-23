#pragma once

#include <memory>
#include <string>
#include <unordered_map>
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
			virtual const std::string& getPropertyString(uint32_t propertyIdx) = 0;
			virtual bool getPropertyBool(const std::string& propertyName) = 0;
			virtual JSONObjectInterface* getPropertyObject(const std::string& propertyName) = 0;
			virtual JSONObjectInterface* getArrayObjectItem(const std::string& propertyName, uint32_t idx) = 0;

			virtual uint32_t getArraySize(const std::string& propertyName) = 0;
			virtual uint32_t getPropertyCount() = 0;
		};
		JSONObjectInterface* getRoot() { return m_rootObject; }

	private:
		class JSONObject;

		enum class JSONPropertyType { String, Object, Float, UnknownArray, ObjectArray, FloatArray, Bool, Unknown };
		struct JSONPropertyValue
		{
			JSONPropertyType type;

			// Possible values
			float floatValue = 0;
			std::string stringValue;
			bool boolValue = false;;
			JSONObject* objectValue = nullptr;
			std::vector<JSONObject*> objectArrayValue;
			std::vector<float> floatArrayValue;

			JSONPropertyValue() : type(JSONPropertyType::Unknown) {};
		};

		class JSONObject final : public JSONObjectInterface
		{
		public:
			std::unordered_map<std::string, JSONPropertyValue*> properties;

			float getPropertyFloat(const std::string& propertyName) override;
			const std::vector<float>& getPropertyFloatArray(const std::string& propertyName) override;
			const std::string& getPropertyString(const std::string& propertyName) override;
			const std::string& getPropertyString(uint32_t propertyIdx) override;
			bool getPropertyBool(const std::string& propertyName) override;
			JSONObjectInterface* getPropertyObject(const std::string& propertyName) override;
			JSONObjectInterface* getArrayObjectItem(const std::string& propertyName, uint32_t idx) override;

			uint32_t getArraySize(const std::string& propertyName) override;
			uint32_t getPropertyCount() override;
		};

		JSONObject* m_rootObject;
	};
}