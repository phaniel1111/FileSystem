#include "header.h"
#include "volume.h"

int main() {

	Volume myFS;
	bool btn2 = 0;
	int btn1 = 0;

	// mo hoac tao volume
	do {
		do {
			cout << "-----FILE SYSTEM-----" << endl;
			cout << "Enter 1: Create new volume (Size: 2 MB)" << endl;
			cout << "Enter 2: Open existed volume" << endl;
			cout << "Enter your choice: ";
			cin >> btn1;
		} while (btn1 != 1 && btn1 != 2);

		if (btn1 == 1) {
			// create new volume
			btn2 = myFS.createNewVolume();
		}
		else {
			// open existed volume
			btn2 = myFS.readVolume();
		}
		system("pause");
		system("cls");
	} while (!btn2);

	// cac tinh nang truy xuat khac


	system("pause");
	return 0;
}