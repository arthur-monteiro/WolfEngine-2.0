#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string.h>

#include "Debug.h"

void readArguments(bool& isGraphicTest, int argc, char* argv[])
{
	for (int i = 0; i < argc; ++i)
	{
		if (strcmp(argv[i], "graphictests") == 0)
		{
			isGraphicTest = true;
		}
	}
}

constexpr auto CURRENT_FILENAME = "graphicTestExecution.jpg";
constexpr auto REFERENCE_FILENAME = "referenceGraphicTest.jpg";

bool checkGraphicTest()
{
	struct stat buffer;
	if (stat(REFERENCE_FILENAME, &buffer) != 0)
	{
		Wolf::Debug::sendInfo("GraphicTest: reference doesn't exist");

		std::ifstream current(CURRENT_FILENAME, std::ios::binary);
		std::ofstream ref(REFERENCE_FILENAME, std::ios::binary);

		ref << current.rdbuf();
	}
	else
	{
		std::ifstream current(CURRENT_FILENAME, std::ios::binary);
		std::ifstream ref(REFERENCE_FILENAME, std::ios::binary);

		const std::ifstream::pos_type fileSize = current.tellg();

		if (fileSize != ref.tellg())
		{
			Wolf::Debug::sendInfo("GraphicTest: different image size");
			return false; // different file size
		}

		current.seekg(0); // rewind
		ref.seekg(0); // rewind

		std::istreambuf_iterator<char> beginCurrent(current);
		std::istreambuf_iterator<char> beginRef(ref);

		std::istreambuf_iterator<char> end;

		if (!std::equal(beginCurrent, std::istreambuf_iterator<char>(), beginRef))
		{
			Wolf::Debug::sendInfo("GraphicTest: reference and test are not the same");
			return false;
		}
	}

	std::filesystem::remove(CURRENT_FILENAME);

	return true;
}