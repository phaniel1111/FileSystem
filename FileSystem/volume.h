#include "header.h"
#include "utils.h"

struct superBlock {//32 bytes
	BYTE superBlockSize[1];
	BYTE bytePerBlock[2];
	BYTE blocksInVolume[4];
	BYTE entryTableSize[4];
	BYTE password[16];
	BYTE unused[5];
};

struct entryTable { //32 bytes
	BYTE fileName[8];
	BYTE fileFormat[4];
	BYTE password[12];
	BYTE fileSize[4];
	BYTE startBlock[4];
};

struct dataBlock {
	BYTE data[512];
};

class Volume {
	private:
		
		bool _createBlankVolume(); // tao o dia trong

		bool _createSupperBlock(int volumeSize);
		bool _readSuperBlock(string volumeName);
		bool _writeSuperBlock(superBlock sb, LPCWSTR volumeName);
		//superBlock _readableSuperBlock(superBlock sb);
		// read and write entry table
		// read and write file

		BYTE* _readBlock(int block, LPCWSTR fileName);
		bool _writeBlock(int block, BYTE* buffer, LPCWSTR fileName);
	public:
		string volumeName;
		int volumeSize; //in MB
		vector<entryTable> entryTable;
		superBlock spBlock;
		string extentionTail = ".drs";
		Volume();
		~Volume();

		bool createNewVolume();
		bool readVolume();


		void printSuperBlock(const superBlock& sb);
};