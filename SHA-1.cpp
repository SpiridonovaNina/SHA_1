#include <fstream>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string.h>

using namespace std;
bool getFileContent(char* fileName, vector<unsigned char> &fileContent);
void alignment(vector<unsigned char> &fileContent);
const char* getHash(vector<unsigned char> &fileContent);

int main(int argc, char* argv[])
{
	vector<unsigned char> inputContent;
	if (argc != 2)
	{
		cerr << "Bad command line!" << endl;
		return -1;
	}
	if (!getFileContent(argv[1], inputContent))
	{
		cerr << "Can't open input file: " << endl;
		return -1;
	}
	cout << getHash(inputContent) << endl;
	return 0;
}

bool getFileContent(char* fileName, vector<unsigned char> &fileContent)
{
	ifstream inputStream(fileName, std::ios::binary);
	if (inputStream.fail())
		return false;

	inputStream.seekg(0, std::ios::end);
	int inputSize = inputStream.tellg();
	inputStream.seekg(0, std::ios::beg);

	if (!inputSize)
	{
		fileContent.clear();
		return true;
	}

	fileContent.resize(inputSize);
	inputStream.read((char*)&fileContent.front(), inputSize);
	inputStream.close();

	return true;
}

unsigned long inv(unsigned long value)
{
	unsigned long res = 0;

	res |= ((value >> 0) & 0xFF) << 32;
	res |= ((value >> 8) & 0xFF) << 24;
	res |= ((value >> 16) & 0xFF) << 16;
	res |= ((value >> 24) & 0xFF) << 8;
	res |= ((value >> 32) & 0xFF) << 0;

	return res;
}

void alignment(vector<unsigned char> &fileContent)
{
	size_t size = fileContent.size() * 8;
	fileContent.push_back(0x80);

	int nZeroes = 56 - fileContent.size() % 64;
	if (nZeroes < 0)
		nZeroes += 64;
	fileContent.insert(fileContent.end(), nZeroes, 0);

	fileContent.insert(fileContent.end(), 4, 0);
	for (int i = 0; i < 4; i++)
		fileContent.push_back(size >> (3 - i) * 8);
}

unsigned long cir_shift(unsigned long val, int bits)
{
	return ((val << bits) & 0xFFFFFFFF) | ((val & 0xFFFFFFFF) >> (32 - bits));
}

const char* getHash(vector<unsigned char> &fileContent)
{
	alignment(fileContent);
	//Инициализация переменных
	unsigned long* buf = new unsigned long[5];
	buf[0] = 0x67452301;
	buf[1] = 0xEFCDAB89;
	buf[2] = 0x98BADCFE;
	buf[3] = 0x10325476;
	buf[4] = 0xC3D2E1F0;

	for (int j = 0; j < fileContent.size() / (16 * 4); j++)//номер блока
	{
		vector<unsigned long> w;
		w.resize(80);
		for (int i = 0; i < 16; i++)//номер элемента в блоке
		{
			unsigned long mi = 0;
			for (int k = 0; k < 4; k++)
			{
				mi |= fileContent[j * 16 * 4 + i * 4 + k] << (3 - k) * 8; 
			}
			w[i] = mi;
		}

		for (int i = 16; i < 80; i++)
		{
			//w[i] = (w[i-3] xor w[i-8] xor w[i-14] xor w[i-16]) циклический сдвиг влево 1
			w[i] = cir_shift(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1); 
		}

		unsigned long a = buf[0];
		unsigned long b = buf[1];
		unsigned long c = buf[2];
		unsigned long d = buf[3];
		unsigned long e = buf[4];
		unsigned long temp;
		
		for (int i = 0; i < 80; i++) 
		{
			unsigned long f;
			unsigned long k;
			if (i >= 0 && i <= 19)
			{
				f = (b & c) | ((~ b) & d); 
				k = 0x5A827999;
			}
			if (i >=20 && i <= 39)
			{
				f = b ^ c ^ d;
				k = 0x6ED9EBA1;
			}
			if (i >= 40 && i <= 59) 
			{
				f = (b & c) | (b & d) | (c & d);
				k = 0x8F1BBCDC;
			}
			if (i >= 60 && i <= 79)
			{
				f = b ^ c ^ d;
				k = 0xCA62C1D6;
			}
		
			temp = cir_shift(a, 5);
			temp = cir_shift(a, 5) + f + e + k + w[i];
			e = d;
			d = c;
			c = cir_shift(b, 30);
			b = a;
			a = temp;
		}
		//Добавляем хеш-значение этой части к результату:
		buf[0] = buf[0] + a;
		buf[1] = buf[1] + b; 
		buf[2] = buf[2] + c;
		buf[3] = buf[3] + d;
		buf[4] = buf[4] + e;
	}
	//Итоговое хеш-значение:
	ostringstream res;
	res.fill ('0'); //нулями будут «залиты» пустые позиции в результирующей строке
	for (int i = 0; i < 5; i++)
	{
		res << std::hex << setw(8) << buf[i] << " ";
	}

	char* result = new char[41];
	strcpy(result, (char*) res.str().c_str());
	return result;
}
