struct addresses
{
	int addresstodecipher;
	int processnumber;
	int pagenumber;
	struct addresses *next;
};


void userthread();

void memManager();

void faultHandler();

char* convertToBinary(int toconvert);

int convertToDecimal(long toconvert);

char *lastx(int s, char *input);