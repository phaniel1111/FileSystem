#include "utils.h"

BYTE* decToHexaLE(unsigned int num, int n) {
	BYTE* byte = createBlankOffets(n);
	while (1) {
		unsigned int dec = num;
		int n = 0;
		while (dec > 256) {
			dec = dec >> 8;
			n++;
		}
		byte[n] = BYTE(dec);
		num = num - (dec << (8 * n));
		if (n == 0)
			break;
	}
	return byte;
}
BYTE* createBlankOffets(int n) {
	BYTE* offsets = new BYTE[n];

	for (int i = 0; i < n; i++) {
		offsets[i] = BYTE(0);
	}
	return offsets;
}

BYTE hexToByte(const std::string& hexString) {
	if (hexString.size() != 2) {
		// Handle invalid input
		throw std::invalid_argument("Input must be a two-character hexadecimal string");
	}
	try {
		// Convert the hexadecimal string to an integer
		int hexValue = std::stoi(hexString, 0, 16);

		// Convert the integer to an unsigned char
		unsigned char result = static_cast<unsigned char>(hexValue);

		return result;
	}
	catch (const std::out_of_range& e) {
		// Handle out-of-range error
		throw std::out_of_range("Hexadecimal value out of range for unsigned char");
	}
	catch (const std::invalid_argument& e) {
		// Handle invalid argument error
		throw std::invalid_argument("Invalid hexadecimal string");
	}
}

string byteToHex(unsigned char byte) {
	std::ostringstream oss;
	oss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(byte);
	return oss.str();
}
int reverseByte(BYTE* byte, unsigned int count)
{
	int result = 0;
	for (int i = count - 1; i >= 0; i--)
		result = (result << 8) | byte[i];
	return result;
}

LPCWSTR stringToLPCWSTR(string orig) {
	wstring t = wstring(orig.begin(), orig.end());
	return  t.c_str();

}

