#include "volume.h"

Volume::Volume() {}

Volume::~Volume() {}

// create blank volume
bool Volume::_createBlankVolume() {
	ifstream fileCheck(this->volumeName);
	if (fileCheck.is_open()) {
		fileCheck.close();
		return false;  // If file existed return false
	}

	fstream f;
	f.open(this->volumeName, ios::trunc | ios::out);
	BYTE block[BytePerBlock]; // each block take 512 byte
	for (int i = 0; i < BytePerBlock; i++) {
		block[i] = BYTE(0);
	}
	int totalBLock = this->volumeSize * 2048; //1 MB = 1048576B
	for (int i = 0; i < totalBLock; i++) {
		f.write((char*)&block, BytePerBlock);
	}
	f.close();
	return true;
}

// create super block
bool Volume::_createSupperBlock(int volumeSize) {
	this->spBlock.superBlockSize[0] = hexToByte("20"); // little endian 32 = 0x20
	this->spBlock.bytePerBlock[0] = BYTE(0); // little endian 512 = 0x0200
	this->spBlock.bytePerBlock[1] = BYTE(2);
	int biv = volumeSize * 2048; // don vi: block
	BYTE* temp = decToHexaLE(biv, 4);
	for (int i = 0; i < 4; i++)
		this->spBlock.blocksInVolume[i] = temp[i];

	int fs = biv * 32 + 32;
	BYTE* temp2 = decToHexaLE(fs, 4);
	for (int i = 0; i < 4; i++)
		this->spBlock.entryTableSize[i] = temp2[i];

	return true;
}
// read super block
bool Volume::_readSuperBlock(string volumeName) {
	wstring temp = wstring(volumeName.begin(), volumeName.end());
	LPCWSTR sw = temp.c_str();
	BYTE* buffer = this->_readBlock(0, sw);
	if (buffer == NULL)
		return false;
	this->spBlock.superBlockSize[0] = buffer[0];
	this->spBlock.bytePerBlock[0] = buffer[1];
	this->spBlock.bytePerBlock[1] = buffer[2];
	for (int i = 0; i < 4; i++) {
		this->spBlock.blocksInVolume[i] = buffer[3 + i];
	};
	for (int i = 0; i < 4; i++) {
		this->spBlock.entryTableSize[i] = buffer[7 + i];
	};
	for (int i = 0; i < 16; i++) {
		this->spBlock.password[i] = buffer[11 + i];
	};
	return true;
}


// check if superblock is written successfully
bool Volume::_writeSuperBlock(superBlock sb, LPCWSTR volumeName) {
	DWORD bytesRead;
	HANDLE headerFile;
	headerFile = CreateFile(volumeName,
		GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	SetFilePointer(headerFile, 0, NULL, FILE_BEGIN);
	bool isOK = WriteFile(headerFile, &sb, sizeof(superBlock), &bytesRead, NULL);
	CloseHandle(headerFile);
	return isOK;
}


/*superBlock Volume::_readableSuperBlock(superBlock sb) {
	superBlock spB;

	spB.superBlockSize = reverseByte(sb.superBlockSize, 1);
	spB.bytePerBlock = reverseByte(sb.bytePerBlock, 2);
	spB.blocksInVolume = reverseByte(sb.blocksInVolume, 4);
	spB.entryTableSize = reverseByte(sb.entryTableSize, 4);
	for (int i = 0; i < 8; i++) {
		spB.password[i] = sb.password[i];
	}
	return spB;
}*/

BYTE* Volume::_readBlock(int block, LPCWSTR fileName) {
	DWORD bytesRead;
	HANDLE hFile;
	bool flag;
	BYTE* buffer = new BYTE[BytePerBlock];
	hFile = CreateFile(fileName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	SetFilePointer(hFile, block * BytePerBlock, NULL, FILE_BEGIN);
	flag = ReadFile(hFile, buffer, BytePerBlock, &bytesRead, NULL);
	CloseHandle(hFile);
	if (flag)
		return buffer;
	return NULL;
}
bool Volume::_writeBlock(int block, BYTE* buffer, LPCWSTR fileName) {
	DWORD bytesRead;
	HANDLE hFile;
	bool flag;
	hFile = CreateFile(fileName,
		GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	SetFilePointer(hFile, block * BytePerBlock, NULL, FILE_BEGIN);
	flag = WriteFile(hFile, buffer, BytePerBlock, &bytesRead, NULL);
	CloseHandle(hFile);
	return flag;
}

// console 1st function
bool Volume::createNewVolume() {
	string volumeName;
	int volumeSize;
	cout << "Volume name: ";
	cin >> volumeName;
	cout << "Volume size (MB): ";
	cin >> volumeSize;
	if (volumeName.length() >= 4 && volumeName.substr(volumeName.length() - 4) != ".drs") {
		// If it doesn't end with ".drs", append it
		volumeName += this->extentionTail;
	}
	this->volumeName = volumeName;
	this->volumeSize = volumeSize;

	if (this->_createBlankVolume()) {
		this->_createSupperBlock(volumeSize);

		string password;
		cout << "Set password (if not, enter 0):";
		cin >> password;
		if (password != "0") {
			string hash = md5(password);
			for (int i = 0; i < 16; i++)
			{
				string pair = hash.substr(i * 2, 2);
				this->spBlock.password[i] = hexToByte(pair);
			}
		}

		wstring temp = wstring(volumeName.begin(), volumeName.end());
		LPCWSTR sw = temp.c_str();
		if (this->_writeSuperBlock(this->spBlock, sw)) {
			cout << "Create volume successfully!" << endl;
			return true;
		}
		else {
			cout << "Fail to create volume !" << endl;
			return false;
		}
	}
	else {
		cout << "Volume existed!" << endl;
		return false;
	}
}

bool Volume::readVolume() {
	string t;

	cout << "Volume name: ";
	cin >> volumeName;
	
	if (volumeName.length() >= 4 && volumeName.substr(volumeName.length() - 4) != ".drs") {
		// If it doesn't end with ".drs", append it
		volumeName += this->extentionTail;
	}
	//kiem tra file co ton tai
	ifstream fileCheck(volumeName);
	if (!fileCheck.is_open()) {
		fileCheck.close();
		cout << "Volume isn't existed!" << endl;
		return false;  // If file existed return false
	}
	if (!this->_readSuperBlock(volumeName))
	{
		cout << "Fail to read volume!" << endl;
		return false;
	}
	// read entry table !!

	// xoa phan mo trong
	if (volumeName.length() >= 4 && volumeName.compare(volumeName.length() - 4, 4, this->extentionTail) == 0) {
		// Remove the suffix
		volumeName.erase(volumeName.length() - 4);
	}
	this->volumeName = volumeName;

	printSuperBlock(this->spBlock);

	return true;
}

void Volume::printSuperBlock(const superBlock& sb) {
	std::cout << "superBlockSize: " << static_cast<int>(sb.superBlockSize[0]) << std::endl;
	std::cout << "bytePerBlock: " << (sb.bytePerBlock[1] << 8 | sb.bytePerBlock[0]) << std::endl;
	std::cout << "blocksInVolume: " << (sb.blocksInVolume[3] << 24 | sb.blocksInVolume[2] << 16 | sb.blocksInVolume[1] << 8 | sb.blocksInVolume[0]) << std::endl;
	std::cout << "entryTableSize: " << (sb.entryTableSize[3] << 24 | sb.entryTableSize[2] << 16 | sb.entryTableSize[1] << 8 | sb.entryTableSize[0]) << std::endl;

	std::cout << "password: ";
	for (int i = 0; i < 16; ++i) {
		std::cout << byteToHex(sb.password[i]);
	}
	std::cout << std::endl;

	std::cout << "unused: ";
	for (int i = 0; i < 5; ++i) {
		std::cout << static_cast<int>(sb.unused[i]) << " ";
	}
	std::cout << std::endl;
}