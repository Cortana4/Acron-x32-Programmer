#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>

#include <FTDI/ftd2xx.h>

int main(int argC, char* argV[])
{
	// check number of arguments
	if (argC == 1)
	{
		std::cout << "Error: no source file specified!" << std::endl;
		return -1;
	}
	else if (argC != 2)
	{
		std::cout << "Error: invalid number of arguments!" << std::endl;
		return -1;
	}

	// open source file
	std::ifstream file;
	file.open(argV[1], std::ios::binary);

	if (!file.is_open())
	{
		std::cout << "Error: cannot open source file '" << argV[1] << "'!" << std::endl;
		return -1;
	}

	// read source file into vector
	std::vector<char> data{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
	file.close();

	// init and open the serial device
	FT_HANDLE deviceHandle;
	FT_STATUS deviceStatus;
	LONG comPort;

	deviceStatus = FT_Open(0, &deviceHandle);
	deviceStatus |= FT_GetComPortNumber(deviceHandle, &comPort);
	deviceStatus |= FT_SetUSBParameters(deviceHandle, 16384, 16384);
	deviceStatus |= FT_SetTimeouts(deviceHandle, 120000, 120000);
	deviceStatus |= FT_SetFlowControl(deviceHandle, FT_FLOW_RTS_CTS, 0x00, 0x00);
	deviceStatus |= FT_SetBaudRate(deviceHandle, 230400);
	deviceStatus |= FT_SetDataCharacteristics(deviceHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_EVEN);	

	if (deviceStatus != FT_OK)
	{
		std::cout << "Error: cannot open serial device! (0x"
			<< std::hex << std::setw(8) << std::setfill('0') << deviceStatus
			<< ")" << std::endl;
		return -1;
	}

	if (comPort == -1)
	{
		std::cout << "Error: invalid COM port!" << std::endl;
		return -1;
	}

	std::cout << "Info: device opened on COM" << comPort << ". Transmitting data..." << std::endl;
	
	// transmit data
	DWORD bytesWritten;
	deviceStatus |= FT_Write(deviceHandle, data.data(), data.size(), &bytesWritten);

	if (deviceStatus != FT_OK)
	{
		std::cout << "Error: data transmission failed! (0x" << std::endl
			<< std::hex << std::setw(8) << std::setfill('0') << deviceStatus
			<< std::endl;
		FT_Close(deviceHandle);
		return -1;
	}

	if (bytesWritten != data.size())
	{
		std::cout << "Error: timeout during data transmission!" << std::endl;
		FT_Close(deviceHandle);
		return -1;
	}

	std::cout << "Info: data transmitted. Reading status byte..." << std::endl;

	// read status byte
	DWORD bytesReceived;
	char returnByte = 0x00;
	deviceStatus |= FT_Read(deviceHandle, &returnByte, 1, &bytesReceived);

	if (deviceStatus != FT_OK)
	{
		std::cout << "Error: cannot read status byte! (0x" << std::endl
			<< std::hex << std::setw(8) << std::setfill('0') << deviceStatus
			<< std::endl;
		FT_Close(deviceHandle);
		return -1;
	}

	if (bytesReceived == 0)
	{
		std::cout << "Error: no status byte received!" << std::endl;
		FT_Close(deviceHandle);
		return -1;
	}

	if (returnByte != 0x00)
	{
		std::cout << "Data transmission completed with errors. ("
			<< (((returnByte & 0x10) == 0x10) ? "FE, " : "0, ")
			<< (((returnByte & 0x08) == 0x08) ? "PE, " : "0, ")
			<< (((returnByte & 0x04) == 0x04) ? "NE, " : "0, ")
			<< (((returnByte & 0x02) == 0x02) ? "UE, " : "0, ")
			<< (((returnByte & 0x01) == 0x01) ? "OE)" : "0)")
			<< std::endl;

		FT_Close(deviceHandle);
		return -1;
	}		

	std::cout << "Data transmission completed successfully." << std::endl;
	FT_Close(deviceHandle);
	return 0;
}