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
		struct FileReadInfo
		{
			std::string filename;
		};
		JSONReader(const FileReadInfo& fileReadInfo);

		struct StringReadInfo
		{
			std::string jsonData;
		};
		JSONReader(const StringReadInfo& stringReadInfo);
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
		class Lines
		{
		public:
			void addLine(const std::wstring& line) { m_lines.push_back(line); }

			bool getNextLine(std::wstring& outLine) const
			{
				if (m_currentLine == m_lines.size())
					return false;

				outLine = m_lines[m_currentLine++];
				return true;
			}

		private:
			std::vector<std::wstring> m_lines;
			mutable uint32_t m_currentLine = 0;
		};

		void readFromLines(const Lines& lines);

		class JSONObject;

		enum class JSONPropertyType { String, Object, Float, UnknownArray, ObjectArray, FloatArray, StringArray, Bool, Unknown };
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