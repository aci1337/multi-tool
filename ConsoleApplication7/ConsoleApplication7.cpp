#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <tchar.h>
#include <regex>
#include <fstream>
#include <string>
#include <vector>


// Function to dump bytes from a file
void byteDump(std::string fileName)
{
	// Create a file stream object
	std::ifstream fileStream(fileName, std::ios::binary);

	// Create a buffer to read bytes into
	char buffer[16];

	// Print out the file name
	std::cout << "Dumping bytes from file " << fileName << std::endl;

	// Read bytes into the buffer until end of file is reached
	while (fileStream.read(buffer, 16))
	{
		// Print out the bytes read
		for (int i = 0; i < 16; i++)
		{
			std::cout << std::hex << (int)(unsigned char)buffer[i] << " ";
		}

		// Print a new line
		std::cout << std::endl;
	}

	// Close the file stream
	fileStream.close();
}

// Structure of packers
struct Packer {
	std::string name;
	std::vector<std::string> signatures;
	bool UnpackFunc(std::ifstream& file, std::vector<unsigned char>& out_data)
	{
		// Check for signature
		std::string signature;
		file.read(&signature[0], signature.length());

		if (signature != "PACKED") {
			std::cout << "Error: Invalid signature" << std::endl;
			return false;
		}

		// Read the size of the file
		uint32_t file_size;
		file.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));

		// Read the data
		out_data.resize(file_size);
		file.read(reinterpret_cast<char*>(out_data.data()), file_size);

		return true;
	}
};

// List of packers
const std::vector<Packer> packers = {
	{ "Themida", {
		"\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00"
	}},
	{ "VMProtect", {
		"\\x56\\x4d\\x50\\x72\\x6f\\x74\\x65\\x63\\x74",
		"\\x56\\x4d\\x50\\x72\\x6f\\x74\\x65\\x63\\x74\\x20\\x56\\x31\\x2e\\x30"
	}}
};

// Function to unpack a file

bool UnpackFile(std::string filename)
{
	std::cout << "Unpacking " << filename << std::endl;

	// Open the file
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		std::cout << "Error: Could not open " << filename << std::endl;
		return false;
	}

	// Determine the packer
	bool is_packed = false;
	Packer packer;
	for (const auto& p : packers)
	{
		bool found = true;
		for (const auto& sig : p.signatures)
		{
			std::string buffer;
			file.read(&buffer[0], sig.size());

			std::regex signature_regex(sig);
			if (!std::regex_match(buffer, signature_regex)) {
				found = false;
				break;
			}
		}

		if (found) {
			packer = p;
			is_packed = true;
			break;
		}
	}

	// If not packed, exit
	if (!is_packed) {
		std::cout << "Error: " << filename << " is not packed with a recognized packer" << std::endl;
		return false;
	}

	// Unpack the file
	std::vector<unsigned char> unpacked_file;
	if (!packer.UnpackFunc(file, unpacked_file)) {
		std::cout << "Error: Could not unpack " << filename << std::endl;
		return false;
	}

	// Delete the old file
	std::remove(filename.c_str());

	// Rename the unpacked file
	std::string new_filename = filename + ".unpacked.exe";
	std::ofstream out_file(new_filename, std::ios::binary);
	out_file.write(reinterpret_cast<const char*>(unpacked_file.data()), unpacked_file.size());
	out_file.close();

	std::cout << "Successfully unpacked " << filename << " with " << packer.name << std::endl;
	return true;
}
#include <iostream>
#include <string>
#include <windows.h>
#include <wincrypt.h>
#include <random>
using namespace std;

void packer() {
	string inputFilename;
	// check for valid command line parameters
	if (__argc < 2)
	{
		cout << "Please enter a filename to pack: ";
		cin >> inputFilename;
	}
	else
	{
		inputFilename = __argv[1];
	}

	// check if the file is an exe
	if (inputFilename.substr(inputFilename.find_last_of(".") + 1) != "exe")
	{
		cout << "The file you entered is not an exe file. Please enter an exe file." << endl;
	}

	// open explorer to select the exe file
	ShellExecute(NULL, "open", "explorer.exe", NULL, NULL, SW_SHOW);

	cout << "Please select the exe file to pack: ";
	cin >> inputFilename;

	// open the executable for reading
	HANDLE hFile = CreateFile(inputFilename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		cout << "Error opening file " << inputFilename << endl;
	}

	// get the file size
	DWORD fileSize = GetFileSize(hFile, NULL);

	// allocate memory to store the executable
	BYTE* fileBuffer = new BYTE[fileSize];

	// read the executable into memory
	DWORD bytesRead;
	ReadFile(hFile, fileBuffer, fileSize, &bytesRead, NULL);
	CloseHandle(hFile);

	// create a buffer to store the packed executable
	BYTE* packedBuffer = new BYTE[fileSize];

	// generate a random key
	BYTE key[32];
	CryptGenRandom(NULL, 32, key);

	// seed random number generator
	random_device rd;
	mt19937 gen(rd());
	uniform_int_distribution<> dist(0, 255);

	// loop through each byte in the executable and XOR it with the random key
	// to hide the strings
	for (DWORD i = 0; i < fileSize; i++)
	{
		packedBuffer[i] = fileBuffer[i] ^ key[i % 32] ^ dist(gen);
	}

	// create the output filename
	string outputFilename = inputFilename.substr(0, inputFilename.find_last_of(".")) + "_packed.exe";

	// open the output file for writing
	HANDLE hOutputFile = CreateFile(outputFilename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

	if (hOutputFile == INVALID_HANDLE_VALUE)
	{
		cout << "Error creating file " << outputFilename << endl;
		
	}

	// write the packed executable to the output file
	DWORD bytesWritten;
	WriteFile(hOutputFile, packedBuffer, fileSize, &bytesWritten, NULL);
	CloseHandle(hOutputFile);

	// execute the packed executable
	ShellExecute(NULL, "open", outputFilename.c_str(), NULL, NULL, SW_SHOW);

	// clean up
	delete[] fileBuffer;
	delete[] packedBuffer;

	cout << "Packed file written to " << outputFilename << endl;

}

void cummy() {
	std::string cacawawa = "input.exe";
	UnpackFile(cacawawa);
}

void bytedumpper() {
	// Ask user for file name
	std::string cececece = "input.exe";

	// Call byteDump() to dump bytes from the file
	byteDump(cececece);
}

#include <fstream>
#include <sstream>
#include <cstdlib>



//Function to read a file and store it in a string variable
string ReadFile(string FileName) {
	// Create a stringstream variable
	stringstream ss;
	// Create a variable to store the contents of the file
	string FileData;
	// Open the file
	ifstream File(FileName.c_str());
	// Read the file
	ss << File.rdbuf();
	// Store the file data in the variable
	FileData = ss.str();
	// Close the file
	File.close();
	// Return the file data
	return FileData;
}

//Function to download a file from a URL and store it in a string variable
string DownloadFile(string URL) {
	// Create a stringstream variable
	stringstream ss;
	// Create a variable to store the contents of the file
	string FileData;
	// Open the file
	ifstream File(URL.c_str());
	// Read the file
	ss << File.rdbuf();
	// Store the file data in the variable
	FileData = ss.str();
	// Close the file
	File.close();
	// Return the file data
	return FileData;
}

//Function to run a file from memory
void RunFileFromMemory(string FileData) {
	// Create a vector to store the file lines
	vector<string> FileLines;
	// Create a stringstream variable
	stringstream ss;
	// Store the file data in the stringstream
	ss << FileData;
	// Create a variable to store the current line
	string Line;
	// Loop through the file lines
	while (getline(ss, Line)) {
		// Add the line to the vector
		FileLines.push_back(Line);
	}
	// Create a variable to store the current line number
	int LineNumber = 0;
	// Create a variable to store the current command
	string Command;
	// Create a variable to store the command arguments
	vector<string> Arguments;
	// Loop through the file lines
	while (LineNumber < FileLines.size()) {
		// Get the current line
		Line = FileLines[LineNumber];
		// Check if the line is empty
		if (Line.length() == 0) {
			// Increment the line number
			LineNumber++;
			// Skip to the next line
			continue;
		}
		// Create a stringstream variable
		stringstream ss;
		// Store the line in the stringstream
		ss << Line;
		// Get the command
		ss >> Command;
		// Check if the command is "print"
		if (Command == "print") {
			// Get the argument
			string Argument;
			ss >> Argument;
			// Print the argument
			cout << Argument << endl;
		}
		// Check if the command is "exit"
		else if (Command == "exit") {
			// Exit the program
			exit(0);
		}
		// Increment the line number
		LineNumber++;
	}
}

void memory() {
	// Get the URL of the file to download
	string URL;
	cout << "Enter the URL of the file to download: ";
	cin >> URL;
	// Download the file
	string FileData = DownloadFile(URL);
	// Run the file from memory
	RunFileFromMemory(FileData);
}

int main()
{
	// Show a loading screen
	std::cout << "Loading...\n" << std::endl;
	system("cls");
	std::cout << "Make sure your file it's named input.exe\n";
	std::cout << "[1] Unpacker\n";
	std::cout << "[2] Bytedumper\n";
	std::cout << "[3] Packer [.exe packer ISN'T READY]\n";
	std::cout << "[4] Download file from memory\n";
	std::cout << "[4] Get the strings of an application\n";
	std::cout << "\n";
	std::cout << "Your choose:\n";

	int options;
	std::cin >> options;
	switch (options) {
	case 1:
		// Unpack the file
		cummy();
		break;
	case 2:

		bytedumpper();
		break;

	case 3:
		packer();
		break;
	case 4:
		memory();
		break;
	case 5:
		// open the input executable file
		std::wstring filename = L"input.exe";
		HANDLE hFile = CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			std::cout << "Error: Could not open the input file!";
			return 1;
		}

		// create output file
		std::ofstream outputFile("output.txt");

		// read the data
		DWORD dwBytesRead;
		char pBuffer[1024];
		do {
			if (!ReadFile(hFile, pBuffer, 1024, &dwBytesRead, NULL)) {
				std::cout << "Error: Could not read the file!";
				return 1;
			}

			// extract the strings
			std::string line;
			for (DWORD i = 0; i < dwBytesRead; ++i) {
				if (pBuffer[i] != 0) {
					line += pBuffer[i];
				}
				else {
					if (line.length() > 0) {
						// write the string to the output file
						outputFile << line << std::endl;
						line.clear();
					}
				}
			}
		} while (dwBytesRead > 0);

		// close the handle
		CloseHandle(hFile);

		// close the output file
		outputFile.close();

		// success
		std::cout << "Successfully extracted strings to output.txt!";

		return 0;
		break;
	}
}