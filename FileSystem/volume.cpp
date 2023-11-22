#include "volume.h"

Volume::Volume() {}

Volume::~Volume() {}

// create blank volume
bool Volume::_createBlankVolume() {
	ifstream fileCheck(this->volumeName + this->extentionTail);
	if (fileCheck.is_open()) {
		fileCheck.close();
		return false;  // If file existed return false
	}

	fstream f;
	f.open(this->volumeName + this->extentionTail, ios::trunc | ios::out);
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
bool Volume::_verifyVolumePassword(string password) {
	string hash = md5(password);
	for (int i = 0; i < 16; i++)
	{
		string pair = hash.substr(i * 2, 2);
		BYTE t1 = hexToByte(pair);
		BYTE t2 = this->spBlock.password[i];
		if (t1 != t2)
			return false;
	}
	return true;
}
bool Volume::_changeVolumePassword(string newPassword)
{
	string hash = md5(newPassword);
	for (int i = 0; i < 16; i++)
	{
		string pair = hash.substr(i * 2, 2);
		this->spBlock.password[i] = hexToByte(pair);
	}
	string path = this->volumeName + this->extentionTail;
	wstring temp = wstring(path.begin(), path.end());
	LPCWSTR sw = temp.c_str();
	if (!this->_writeSuperBlock(this->spBlock, sw))
		return false;
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

entry Volume::_createEntry(string name, string format, string password, int size, int start)
{
	entry et;

	for (int i = 0; i < 8; i++)
	{
		if (i < name.length())
			et.fileName[i] = name[i];
		else
			et.fileName[i] = BYTE(0);
	}
	for (int i = 0; i < 3; i++)
	{
		if (i < format.length())
			et.fileFormat[i] = format[i];
		else
			et.fileFormat[i] = BYTE(0);
	}
	et.fileStatus[0] = BYTE('a');
	BYTE* temp = decToHexaLE(size, 4);
	for (int i = 0; i < 4; i++)
		et.fileSize[i] = temp[i];
	BYTE* temp1 = decToHexaLE(start, 4);
	for (int i = 0; i < 4; i++)
		et.startBlock[i] = temp1[i];
	//password
	if (password != "0")
	{
		string hash = md5(password);
		for (int i = 0; i < 12; i++)
		{
			string pair = hash.substr(i * 2, 2);
			et.password[i] = hexToByte(pair);
		}
	}
	else
	{
		for (int i = 0; i < 12; i++)
			et.password[i] = BYTE(0);
	}

	return et;
}

bool Volume::_writeEntryTable(vector<entry> entryTable, LPCWSTR fileName) {
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
	int i = 0;
	for (auto& element : entryTable) {
		SetFilePointer(hFile, 32 + i, NULL, FILE_BEGIN);
		flag = WriteFile(hFile, &element, 32, &bytesRead, NULL);
		i += 32;
	}
	CloseHandle(hFile);
	return flag;
}
/*
Input password: ad
superBlockSize: 32
bytePerBlock: 512
blocksInVolume: 4096
entryTableSize: 131104
password: 523af537946b79c4f8369ed39ba78605
unused: 0 0 0 0 0
*/
bool Volume::_readEntryTable(vector<entry>& entryTable, LPCWSTR fileName) {
	BYTE* buffer = new BYTE[BytePerBlock];
	int entryBlocks = reverseByte(this->spBlock.entryTableSize,4)/ BytePerBlock;
	for (int i = 0; i < entryBlocks; i++)
	{
		buffer = this->_readBlock(i, fileName);
		if (isBufferEmpty(buffer, BytePerBlock))
			break;

		for (int j = 0; j < BytePerBlock; j += 32) { // j/32
			if (isEntryEmpty(j, buffer))
				goto endLoops;

			entry et;
			memcpy(et.fileName, buffer + j, 8);
			memcpy(et.fileFormat, buffer + j + 8, 3);
			memcpy(et.fileStatus, buffer + j + 11, 1);
			memcpy(et.fileSize, buffer + j + 12, 4);
			memcpy(et.startBlock, buffer + j + 16, 4);
			memcpy(et.password, buffer + j + 20, 12);
			entryTable.push_back(et);
		}
	}
	endLoops:
	entryTable.erase(entryTable.begin());
	delete[] buffer;
	return true;
}

entry* Volume::_searchEntry(string fn) {
	// Copy string characters to unsigned char array
	for (entry& e : this->entryTable) {
		if (memcmp(e.fileName, fn.c_str(), sizeof(e.fileName)) == 0) {
			return &e; // Returning a pointer to the found entry
		}
	}
	return nullptr; // Returning nullptr if entry is not found
}
bool Volume::_verifyFilePassword(string password, entry e) {
	string hash = md5(password);
	for (int i = 0; i < 12; i++)
	{
		string pair = hash.substr(i * 2, 2);
		BYTE t1 = hexToByte(pair);
		BYTE t2 = e.password[i];
		if (t1 != t2)
			return false;
	}
	return true;
}
bool Volume::_changeFilePassword(entry e, string password) {
	string hash = md5(password);
	for (int i = 0; i < 12; i++)
	{
		string pair = hash.substr(i * 2, 2);
		e.password[i] = hexToByte(pair);
	}
	for (entry& en : this->entryTable) {
		if (memcmp(en.fileName, e.fileName, sizeof(en.fileName)) == 0) {
			en = e;
		}
	}
	string path = this->volumeName + this->extentionTail;
	wstring temp = wstring(path.begin(), path.end());
	LPCWSTR sw = temp.c_str();

	if (!this->_writeEntryTable(this->entryTable, sw))
		return false;

	return true;
}

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

// console open/create volume
bool Volume::createNewVolume() {
	string vN;
	int vS;
	cout << "Volume name: ";
	cin >> vN;
	cout << "Volume size (MB): ";
	cin >> vS;
	
	// xoa phan mo trong
	if (vN.length() >= 4 && vN.compare(vN.length() - 4, 4, this->extentionTail) == 0) {
		// Remove the suffix
		vN.erase(vN.length() - 4);
	}
	this->volumeName = vN;
	this->volumeSize = vS;

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

		string t = this->volumeName + this->extentionTail;
		wstring temp = wstring(t.begin(), t.end());
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
	string vN; //volume name
	cout << "Volume name: ";
	cin >> vN;
	
	if (vN.length() >= 4 && vN.substr(vN.length() - 4) != ".drs") {
		// If it doesn't end with ".drs", append it
		vN += this->extentionTail;
	}
	//kiem tra file co ton tai
	ifstream fileCheck(vN);
	if (!fileCheck.is_open()) {
		fileCheck.close();
		cout << "Volume isn't existed!" << endl;
		return false;  // If file existed return false
	}
	if (!this->_readSuperBlock(vN))
	{
		cout << "Fail to read volume!" << endl;
		return false;
	}
	//check volume password
	if (this->spBlock.password[0] != NULL)
	{
		string pw;
		cout << "Input password: ";
		cin >> pw;
		if (!this->_verifyVolumePassword(pw))
		{
			cout << "Wrong password!" << endl;
			return false;
		}
	}
	
	// read entry table
	wstring temp = wstring(vN.begin(), vN.end());
	LPCWSTR sw = temp.c_str();
	if (!this->_readEntryTable(this->entryTable, sw))
		return false;

	// xoa phan mo trong
	if (vN.length() >= 4 && vN.compare(vN.length() - 4, 4, this->extentionTail) == 0) {
		// Remove the suffix
		vN.erase(vN.length() - 4);
	}
	this->volumeName = vN;

	return true;
}

// console modify password
bool Volume::changeVolumePassword() {
	if (this->spBlock.password[0] != BYTE(0))
	{
		string pw;
		cout << "Input password: ";
		cin >> pw;
		if (!this->_verifyVolumePassword(pw))
		{
			cout << "Wrong password!" << endl;
			return false;
		}
	}
	string newPw;
	cout << "Input new password: ";
	cin >> newPw;
	if (!this->_changeVolumePassword(newPw))
	{
		cout << "Fail to change password!" << endl;
		return false;
	}
	cout << "Change password successfully!" << endl;
	return true;
}

void Volume::printSuperBlock(superBlock sb) {
	cout << "superBlockSize: " << reverseByte(sb.superBlockSize, 1) << endl;
	cout << "bytePerBlock: " << reverseByte(sb.bytePerBlock,2) << endl;
	cout << "blocksInVolume: " << reverseByte(sb.blocksInVolume,4) << endl;
	cout << "entryTableSize: " << reverseByte(sb.entryTableSize,4) << endl;

	cout << "password: ";
	for (int i = 0; i < 16; ++i) {
		cout << byteToHex(sb.password[i]);
	}
	cout << endl;

	cout << "unused: ";
	for (int i = 0; i < 5; ++i) {
		cout << static_cast<int>(sb.unused[i]) << " ";
	}
	cout << endl;
}

void Volume::printEntry(entry en) {
	for (int i = 0; i < sizeof(en.fileName); ++i)
		cout << en.fileName[i];
	cout << "\t\t";
	for (int i = 0; i < sizeof(en.fileFormat); ++i)
		cout << en.fileFormat[i];
	cout << "\t\t";
	//cout << en.fileStatus[0] << "\t";
	cout << reverseByte(en.fileSize, 4) << "\t";
	/*cout << reverseByte(en.startBlock, 4) << "\t";
	for (int i = 0; i < sizeof(en.password); ++i)
		cout << en.password[i];*/
	cout << endl;
}

//console print entry table
void Volume::printEntryTable() {
	cout << "Name\t\tFormat\t\tSize" << endl;
	for (auto& element : this->entryTable) {
		this->printEntry(element);
	}
}
bool Volume::changeFilePassword() {
	string fn;
	cout << "Input file name: ";
	cin >> fn;
	entry* e = this->_searchEntry(fn);
	if (e == nullptr) {
		cout << "File not found!" << endl;
		return false;
	}
	if (e->password[0] != BYTE(0))
	{
		string pw;
		cout << "Input password: ";
		cin >> pw;
		if (!this->_verifyFilePassword(pw, *e))
		{
			cout << "Wrong password!" << endl;
			return false;
		}
	}
	string newPw;
	cout << "Input new password: ";
	cin >> newPw;
	if (!this->_changeFilePassword(*e, newPw))
	{
		cout << "Fail to change password!" << endl;
		return false;
	}
	cout << "Change password successfully!" << endl;
	return true;
}


// USE FOR TESTING
void Volume::testEntry(){
	string vN = this->volumeName + this->extentionTail;
	wstring temp = wstring(vN.begin(), vN.end());
	LPCWSTR sw = temp.c_str();
	/*
	entry t = this->_createEntry("filename", "txt", 8, 257);
	this->printEntry(t);
	cout << endl;

	
	vector<entry> enT;
	enT.push_back(t);
	if(this->_writeEntryTable(enT,sw))
		cout << "write entry table successfully" << endl;
	else
		cout << "fail to write entry table" << endl;
	*/
	vector<entry> enT2;
	if (this->_readEntryTable(enT2, sw))
		cout << "read entry table successfully" << endl;
	else
		cout << "fail to read entry table" << endl;
	cout << endl << enT2.size() <<endl;
	if(enT2.size() > 0)
		this->printEntry(enT2[0]);
}