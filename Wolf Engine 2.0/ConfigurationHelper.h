#pragma once

#include <cstdint>
#include <string>

namespace Wolf
{
	class ConfigurationHelper
	{
	public:
		[[nodiscard]] static std::string readInfoFromFile(const std::string& filepath, const std::string& token);

		static void writeInfoToFile(const std::string& filepath, const std::string& token, const std::string& value);
		static void writeInfoToFile(const std::string& filepath, const std::string& token, bool value);
		static void writeInfoToFile(const std::string& filepath, const std::string& token, uint32_t value);
	};
}
