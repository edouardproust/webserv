#include "utils/utils.hpp"
#include <sstream>

bool	utils::isNumeric(std::string const& str)
{
	if (str.empty())
		return false;
	for (size_t i = 0; i < str.size(); ++i) {
		if (!isdigit(str[i]))
			return false;
	}
	return true;
}

unsigned long	utils::parseSize(std::string const& value)
{
	if (value.empty())
		return 0;
	char unit = value[value.size() - 1];
	unsigned long multiplier = 1;
	static const std::string units = "KkMmGg";
	std::string numberStr =
		(units.find(unit) != std::string::npos)
			? value.substr(0, value.size() - 1)
			: value;
	if (!isNumeric(numberStr)) {
		throw std::runtime_error("Invalid size value: " + value);
	}
	unsigned long number = std::strtoul(numberStr.c_str(), NULL, 10);
	if (unit == 'K' || unit == 'k') {
		multiplier = 1024;
	} else if (unit == 'M' || unit == 'm') {
		multiplier = 1024 * 1024;
	} else if (unit == 'G' || unit == 'g') {
		multiplier = 1024 * 1024 * 1024;
	}
	return number * multiplier;
}
