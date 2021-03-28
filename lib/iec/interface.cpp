//
// Title        : UNO2IEC - interface implementation, arduino side.
// Author       : Lars Wadefalk
// Version      : 0.1
// Target MCU   : Arduino Uno AtMega328(H, 5V) at 16 MHz, 2KB SRAM, 32KB flash, 1KB EEPROM.
//
// CREDITS:
// --------
// The UNO2IEC application is inspired by Lars Pontoppidan's MMC2IEC project.
// It has been ported to C++.
// The MMC2IEC application is inspired from Jan Derogee's 1541-III project for
// PIC: http://jderogee.tripod.com/
// This code is a complete reimplementation though, which includes some new
// features and excludes others.
//
// DESCRIPTION:
// This "interface" class is the main driving logic for the IEC command handling.
//
// Commands from the IEC communication are interpreted, and the appropriate data
// from either Native, D64, T64, M2I, x00 image formats is sent back.
//
// DISCLAIMER:
// The author is in no way responsible for any problems or damage caused by
// using this code. Use at your own risk.
//
// LICENSE:
// This code is distributed under the GNU Public License
// which can be found at http://www.gnu.org/licenses/gpl.txt
//

//#include "global_defines.h"
//#include "debug.h"
#include "interface.h"
#include "iec.h"

#include <stdarg.h>
#include <string.h>

#include "../hardware/fnSystem.h"
#include "../hardware/led.h"
#include "../../include/version.h"
#include "../utils/utils.h"

using namespace CBM;

namespace {

// Buffer for incoming and outgoing serial bytes and other stuff.
char serCmdIOBuf[MAX_BYTES_PER_REQUEST];

} // unnamed namespace

Interface::Interface() // (IEC &iec, FileSystem *fileSystem)
	: m_iec(iec)
	  // NOTE: Householding with RAM bytes: We use the middle of serial buffer for the ATNCmd buffer info.
	  // This is ok and won't be overwritten by actual serial data from the host, this is because when this ATNCmd data is in use
	  // only a few bytes of the actual serial data will be used in the buffer.
	  ,
	  m_atn_cmd(*reinterpret_cast<IEC::ATNCmd *>(&serCmdIOBuf[sizeof(serCmdIOBuf) / 2]))
//	,  m_device(&fileSystem)
//,  m_jsonHTTPBuffer(1024)
{
	// m_fileSystem = fileSystem;
	reset();
} // ctor


bool Interface::begin(IEC &iec, FileSystem *fileSystem)
{
	//	m_device.init(std::string(DEVICE_DB));
	//m_device.check();
	m_iec = iec;
	m_fileSystem = fileSystem;
	return true;
}

void Interface::reset(void)
{
	m_openState = O_NOTHING;
	m_queuedError = ErrIntro;
} // reset


void Interface::sendStatus(void)
{
	int i, readResult;
	
	std::string status("00, OK, 00, 08");
	readResult = status.length();

	Debug_printf("\r\nsendStatus: ");
	// Length does not include the CR, write all but the last one should be with EOI.
	for(i = 0; i < readResult - 2; ++i)
		m_iec.send(status[i]);

	// ...and last int in string as with EOI marker.
	m_iec.sendEOI(status[i]);
} // sendStatus



void Interface::sendDeviceInfo()
{
	Debug_printf("\r\nsendDeviceInfo:\r\n");

	// Reset basic memory pointer:
	uint16_t basicPtr = C64_BASIC_START;

	// FSInfo64 fs_info;
	// m_fileSystem->info64 ( fs_info );

	// char floatBuffer[10]; // buffer
	// dtostrf(getFragmentation(), 3, 2, floatBuffer);

	// Send load address
	m_iec.send(C64_BASIC_START bitand 0xff);
	m_iec.send((C64_BASIC_START >> 8) bitand 0xff);
	Debug_println("");

	// Send List HEADER
	sendLine(basicPtr, 0, "\x12 %s V%s ", FN_VERSION_DATE, FN_VERSION_FULL);

	// CPU
	sendLine(basicPtr, 0, "SYSTEM ---");
	// std::string sdk(esp_get_idf_version());
	// util_string_toupper(sdk);
	sendLine(basicPtr, 0, "SDK VER    : %s", fnSystem.get_sdk_version());
	// TODO: sendLine(basicPtr, 0, "BOOT VER   : %08X", ESP.getBootVersion());
	// TODO: sendLine(basicPtr, 0, "BOOT MODE  : %08X", ESP.getBootMode());
	// TODO: sendLine(basicPtr, 0, "CHIP ID    : %08X", ESP.getChipId());
	sendLine(basicPtr, 0, "CPU MHZ    : %d MHZ", fnSystem.get_cpu_frequency());
	// TODO: sendLine(basicPtr, 0, "CYCLES     : %u", ESP.getCycleCount());

	// POWER
	sendLine(basicPtr, 0, "POWER ---");
	sendLine(basicPtr, 0, "VOLTAGE    : %d.%d V", fnSystem.get_sio_voltage() / 1000, fnSystem.get_sio_voltage() % 1000);

	// RAM
	sendLine(basicPtr, 0, "MEMORY ---");
	sendLine(basicPtr, 0, "PSRAM SIZE  : %5d B", fnSystem.get_psram_size());
	sendLine(basicPtr, 0, "HEAP FREE   : %5d B", fnSystem.get_free_heap_size());
	// sendLine(basicPtr, 0, "RAM >BLK   : %5d B", getLargestAvailableBlock());
	// sendLine(basicPtr, 0, "RAM FRAG   : %s %%", floatBuffer);

	// ROM
	// sendLine(basicPtr, 0, "ROM SIZE   : %5d B", ESP.getSketchSize() + ESP.getFreeSketchSpace());
	// sendLine(basicPtr, 0, "ROM USED   : %5d B", ESP.getSketchSize());
	// sendLine(basicPtr, 0, "ROM FREE   : %5d B", ESP.getFreeSketchSpace());

	// FLASH
	// sendLine(basicPtr, 0, "STORAGE ---");
	// sendLine(basicPtr, 0, "FLASH SIZE : %5d B", ESP.getFlashChipRealSize());
	// sendLine(basicPtr, 0, "FLASH SPEED: %d MHZ", ( ESP.getFlashChipSpeed() / 1000000 ));

	// FILE SYSTEM
	// sendLine(basicPtr, 0, "FILE SYSTEM ---");
	// sendLine(basicPtr, 0, "TYPE       : %s", FS_TYPE);
	// sendLine(basicPtr, 0, "SIZE       : %5d B", fs_info.totalBytes);
	// sendLine(basicPtr, 0, "USED       : %5d B", fs_info.usedBytes);
	// sendLine(basicPtr, 0, "FREE       : %5d B", fs_info.totalBytes - fs_info.usedBytes);

	// NETWORK
	sendLine(basicPtr, 0, "NETWORK ---");
	// char ip[16];
	// sprintf ( ip, "%s", ipToString ( WiFi.softAPIP() ).c_str() );
	std::string hn(fnSystem.Net.get_hostname());
	util_string_toupper(hn);
	sendLine(basicPtr, 0, "HOSTNAME : %s", hn.c_str());
	sendLine(basicPtr, 0, "IP       : %s", fnSystem.Net.get_ip4_address_str().c_str());
	sendLine(basicPtr, 0, "GATEWAY  : %s", fnSystem.Net.get_ip4_gateway_str().c_str());
	sendLine(basicPtr, 0, "DNS      : %s", fnSystem.Net.get_ip4_dns_str().c_str());

	// End program with two zeros after last line. Last zero goes out as EOI.
	m_iec.send(0);
	m_iec.sendEOI(0);

	fnLedManager.set(LED_SIO);
} // sendDeviceInfo

void Interface::sendDeviceStatus()
{
	Debug_printf("\r\nsendDeviceStatus:\r\n");

	// Reset basic memory pointer:
	uint16_t basicPtr = C64_BASIC_START;

	// Send load address
	m_iec.send(C64_BASIC_START bitand 0xff);
	m_iec.send((C64_BASIC_START >> 8) bitand 0xff);
	Debug_println("");

	// Send List HEADER
	// sendLine(basicPtr, 0, "\x12 %s V%s ", PRODUCT_ID, FW_VERSION);
	sendLine(basicPtr, 0, "\x12 %s V%s ", FN_VERSION_DATE, FN_VERSION_FULL);

	// Current Config
	// TODO: sendLine(basicPtr, 0, "DEVICE    : %d", m_device.device());
	// TODO: sendLine(basicPtr, 0, "DRIVE     : %d", m_device.drive());
	// TODO: sendLine(basicPtr, 0, "PARTITION : %d", m_device.partition());
	// TODO: sendLine(basicPtr, 0, "URL       : %s", m_device.url().c_str());
	// TODO: sendLine(basicPtr, 0, "PATH      : %s", m_device.path().c_str());
	// TODO: sendLine(basicPtr, 0, "IMAGE     : %s", m_device.image().c_str());
	// TODO: sendLine(basicPtr, 0, "FILENAME  : %s", m_filename.c_str());

	// End program with two zeros after last line. Last zero goes out as EOI.
	m_iec.send(0);
	m_iec.sendEOI(0);

	fnLedManager.set(LED_SIO);
} // sendDeviceStatus


int Interface::loop(void)
{
//#ifdef HAS_RESET_LINE
//	if(m_iec.checkRESET()) {
//		// IEC reset line is in reset state, so we should set all states in reset.
//		reset();
//		
//
//		return IEC::ATN_RESET;
//	}
//#endif
	// Wait for it to get out of reset.
	// while (m_iec.checkRESET())
	// {
	// 	Debug_println("ATN_RESET");
	// }

//	noInterrupts();
	IEC::ATNCheck retATN = m_iec.checkATN( m_atn_cmd);
//	interrupts();

	if(retATN == IEC::ATN_ERROR)
	{
		//Debug_printf("\r\n[ERROR]");
		reset();
		retATN = IEC::ATN_IDLE;
	}
	// Did anything happen from the host side?
	else if(retATN not_eq IEC::ATN_IDLE)
	 {
		switch( m_atn_cmd.command) {
			case IEC::ATN_CODE_OPEN:
				if ( m_atn_cmd.channel == 0 )
					Debug_printf("\r\n[OPEN] LOAD \"%s\",%d ", m_atn_cmd.str, m_atn_cmd.device);
				if ( m_atn_cmd.channel == 1 )
					Debug_printf("\r\n[OPEN] SAVE \"%s\",%d ", m_atn_cmd.str, m_atn_cmd.device);

				// Open either file or prg for reading, writing or single line command on the command channel.
				// In any case we just issue an 'OPEN' to the host and let it process.
				// Note: Some of the host response handling is done LATER, since we will get a TALK or LISTEN after this.
				// Also, simply issuing the request to the host and not waiting for any response here makes us more
				// responsive to the CBM here, when the DATA with TALK or LISTEN comes in the next sequence.
				handleATNCmdCodeOpen( m_atn_cmd );
			break;

			case IEC::ATN_CODE_DATA:  // data channel opened
				Debug_printf("\r\n[DATA] ");
				if(retATN == IEC::ATN_CMD_TALK) {
					 // when the CMD channel is read (status), we first need to issue the host request. The data channel is opened directly.
					if( m_atn_cmd.channel == CMD_CHANNEL)
						handleATNCmdCodeOpen( m_atn_cmd); // This is typically an empty command,
					handleATNCmdCodeDataTalk( m_atn_cmd.channel); // ...but we do expect a response from PC that we can send back to CBM.
				}
				else if(retATN == IEC::ATN_CMD_LISTEN)
					handleATNCmdCodeDataListen();
				else if(retATN == IEC::ATN_CMD) // Here we are sending a command to PC and executing it, but not sending response
					handleATNCmdCodeOpen( m_atn_cmd);	// back to CBM, the result code of the command is however buffered on the PC side.
				break;

			case IEC::ATN_CODE_CLOSE:
				Debug_printf("\r\n[CLOSE] ");
				// handle close with host.
				handleATNCmdClose();
				break;

			case IEC::ATN_CODE_LISTEN:
				Debug_printf("\r\n[LISTEN] ");
				break;
			case IEC::ATN_CODE_TALK:
				Debug_printf("\r\n[TALK] ");
				break;
			case IEC::ATN_CODE_UNLISTEN:
				Debug_printf("\r\n[UNLISTEN] ");
				break;
			case IEC::ATN_CODE_UNTALK:
				Debug_printf("\r\n[UNTALK] ");
				break;
		} // switch
	} // IEC not idle

	return retATN;
} // handler


void Interface::handleATNCmdCodeOpen(IEC::ATNCmd& atn_cmd)
{
	m_device.select( atn_cmd.device );
	m_filename = std::string((char *)atn_cmd.str);
	util_string_trim(m_filename);
	// m_filetype = m_filename.substring(m_filename.lastIndexOf(".") + 1);
	m_filetype = m_filename.substr(m_filename.find_last_of("."));
	util_string_toupper(m_filetype); // .toUpperCase();
	if ( m_filetype.length() > 4 || m_filetype.length() == m_filename.length() )
		m_filetype = "";

	// TODO this whole directory handling needs to be
	// rewritten using the fnFs** classes. in fact it
	// might be handled by the fuji device class
	// that is called by the sio config routines

	// FILE* local_file = m_fileSystem->file_open( std::string(m_device.path() + m_filename).c_str() );
	
	//Serial.printf("\r\n$IEC: DEVICE[%d] DRIVE[%d] PARTITION[%d] URL[%s] PATH[%s] IMAGE[%s] FILENAME[%s] FILETYPE[%s] COMMAND[%s]\r\n", m_device.device(), m_device.drive(), m_device.partition(), m_device.url().c_str(), m_device.path().c_str(), m_device.image().c_str(), m_filename.c_str(), m_filetype.c_str(), atn_cmd.str);
	if (m_filename[0] == '$')
	{
		m_openState = O_DIR;
	}
	// else if ( local_file.isDirectory() )
	// {
	// 	// Enter directory
	// 	Debug_printf("\r\nchangeDir: [%s] >", m_filename.c_str());
	// 	m_device.path(m_device.path() + m_filename.substr(3) + "/");
	// 	m_openState = O_DIR;	
	// }
	// else if (std::string( IMAGE_TYPES ).find(m_filetype) >= 0 && m_filetype.length() > 0 )
	else if (std::string(IMAGE_TYPES).find(m_filetype) < std::string::npos && m_filetype.length() > 0)
	{
		// Mount image file
		Debug_printf("\r\nmount: [%s] >", m_filename.c_str());
		m_device.image( m_filename );

		m_openState = O_DIR;
	}
	else if (util_starts_with(m_filename, "HTTP://"))
	{
		// Mount url
		Debug_printf("\r\nmount: [%s] >", m_filename.c_str());
		m_device.partition(0);
		m_device.url(m_filename.substr(7).c_str());
		m_device.path("/");
		m_device.image("");

		m_openState = O_DIR;
	}
	else if (util_starts_with(m_filename, "CD"))
	{
		if (m_filename.back() == '_')
		{
			if (m_device.image().length())
			{
				// Unmount image file
				//Debug_printf("\r\nunmount: [%s] <", m_device.image().c_str());
				m_device.image("");
			}
			else if ( m_device.url().length() && m_device.path() == "/" )
			{
				// Unmount url
				//Debug_printf("\r\nunmount: [%s] <", m_device.url().c_str());
				m_device.url("");				
			}
			else
			{
				// Go back a directory
				//Debug_printf("\r\nchangeDir: [%s] <", m_filename.c_str());
				m_device.path(m_device.path().substr(0, m_device.path().find_last_of("/", m_device.path().length() - 2) + 1));

				if (!m_device.path().length())
				{
					m_device.path("/");
				}				
			}
		}
		else if (m_filename.length() > 3)
		{
			// Switch to root
			if (util_starts_with(m_filename, "CD//"))
			{
				m_device.path("");
				m_device.image("");
			}

			if (std::string(IMAGE_TYPES).find(m_filetype) < std::string::npos && !m_filetype.empty())
			{
				// Mount image file
				//Debug_printf("\r\nmount: [%s] >", m_filename.c_str());
				m_device.image(m_filename.substr(3));
			}
			else
			{
				// Enter directory
				//Debug_printf("\r\nchangeDir: [%s] >", m_filename.c_str());
				m_device.path(m_device.path() + m_filename.substr(3) + "/");
			}
		}
		
		if ( atn_cmd.channel == 0x00 )
		{
			m_openState = O_DIR;
		}
	}
	else if (util_starts_with(m_filename, "@INFO"))
	{
		m_filename = "";
		m_openState = O_DEVICE_INFO;
	}
	else if (util_starts_with(m_filename, "@STAT"))
	{
		m_filename = "";
		m_openState = O_DEVICE_STATUS;
	}
	else
	{
		m_openState = O_FILE;
	}

	if ( m_openState == O_DIR )
	{
		m_filename = "$";
		m_filetype = "";
		m_atn_cmd.str[0] = '\0';
		m_atn_cmd.strLen = 0;
	}

	//Debug_printf("\r\nhandleATNCmdCodeOpen: %d (M_OPENSTATE) [%s]", m_openState, m_atn_cmd.str);
	Debug_printf("\r\n$IEC: DEVICE[%d] DRIVE[%d] PARTITION[%d] URL[%s] PATH[%s] IMAGE[%s] FILENAME[%s] FILETYPE[%s] COMMAND[%s]\r\n", m_device.device(), m_device.drive(), m_device.partition(), m_device.url().c_str(), m_device.path().c_str(), m_device.image().c_str(), m_filename.c_str(), m_filetype.c_str(), atn_cmd.str);

} // handleATNCmdCodeOpen


void Interface::handleATNCmdCodeDataTalk(int chan)
{
	// process response into m_queuedError.
	// Response: ><code in binary><CR>

	Debug_printf("\r\nhandleATNCmdCodeDataTalk: %d (CHANNEL) %d (M_OPENSTATE)", chan, m_openState);

	if(chan == CMD_CHANNEL) {
		// Send status message
		sendStatus();
		// go back to OK state, we have dispatched the error to IEC host now.
		m_queuedError = ErrOK;
	}
	else {

		//Debug_printf("\r\nm_openState: %d", m_openState);

		switch(m_openState) {
			case O_NOTHING:
				// Say file not found
				m_iec.sendFNF();
				break;

			case O_INFO:
				// Reset and send SD card info
				reset();
				sendListing();
				break;

			case O_FILE:
				// Send program file
				if(m_device.url().length())
				{
					// sendFileHTTP();
				}
				else
				{
					sendFile();
				}
				break;				

			case O_DIR:
				// Send listing
				if(m_device.url().length())
				{
					// sendListingHTTP();
				}
				else
				{
					sendListing();
				}
				break;

			case O_FILE_ERR:
				// FIXME: interface with Host for error info.
				//sendListing(/*&send_file_err*/);
				m_iec.sendFNF();
				break;

			case O_DEVICE_INFO:
				// Send device info
				sendDeviceInfo();
				break;

			case O_DEVICE_STATUS:
				// Send device info
				sendDeviceStatus();
				break;
		}
	}

} // handleATNCmdCodeDataTalk


void Interface::handleATNCmdCodeDataListen()
{
	int lengthOrResult = 0;
	bool wasSuccess = false;

	// process response into m_queuedError.
	// Response: ><code in binary><CR>

	serCmdIOBuf[0] = 0;

	Debug_printf("\r\nhandleATNCmdCodeDataListen: %s", serCmdIOBuf);

	if(not lengthOrResult or '>' not_eq serCmdIOBuf[0])
	 {
		// FIXME: Check what the drive does here when things go wrong. FNF is probably not right.
		m_iec.sendFNF();
		strcpy(serCmdIOBuf, "response not sync.");
	}
	else {
		if (lengthOrResult == fnUartDebug.readBytes(serCmdIOBuf, 2))
		{
			if(2 == lengthOrResult) {
				lengthOrResult = serCmdIOBuf[0];
				wasSuccess = true;
			}
			else
			{
				//Log(Error, FAC_IFACE, serCmdIOBuf);	
			}
		}
		m_queuedError = wasSuccess ? lengthOrResult : ErrSerialComm;

		if(ErrOK == m_queuedError)
			saveFile();
//		else // FIXME: Check what the drive does here when saving goes wrong. FNF is probably not right. Dummyread entire buffer from CBM?
//			m_iec.sendFNF();
	}
} // handleATNCmdCodeDataListen


void Interface::handleATNCmdClose()
{
	Debug_printf("\r\nhandleATNCmdClose: Success!");

	//Serial.printf("\r\nIEC: DEVICE[%d] DRIVE[%d] PARTITION[%d] URL[%s] PATH[%s] IMAGE[%s] FILENAME[%s] FILETYPE[%s]\r\n", m_device.device(), m_device.drive(), m_device.partition(), m_device.url().c_str(), m_device.path().c_str(), m_device.image().c_str(), m_filename.c_str(), m_filetype.c_str());
	Debug_printf("\r\n=================================\r\n\r\n");

	m_filename = "";
} // handleATNCmdClose


// send single basic line, including heading basic pointer and terminating zero.
uint16_t Interface::sendLine(uint16_t &basicPtr, uint16_t blocks, const char* format, ...)
{
	// Format our string
	va_list args;
  	va_start(args, format);
	char text[vsnprintf(NULL, 0, format, args) + 1];
	vsnprintf(text, sizeof text, format, args);
	va_end(args);

	return sendLine(basicPtr, blocks, text);
}

uint16_t Interface::sendLine(uint16_t &basicPtr, uint16_t blocks, char* text)
{
	int i;
	uint16_t b_cnt = 0;

	Debug_printf("%d %s ", blocks, text);

	// Get text length
	int len = strlen(text);

	// Increment next line pointer
	basicPtr += len + 5;

	// Send that pointer
	m_iec.send(basicPtr bitand 0xFF);
	m_iec.send(basicPtr >> 8);

	// Send blocks
	m_iec.send(blocks bitand 0xFF);
	m_iec.send(blocks >> 8);

	// Send line contents
	for(i = 0; i < len; i++)
		m_iec.send(text[i]);

	// Finish line
	m_iec.send(0);

	Debug_println("");
	
	b_cnt += (len + 5);

	return b_cnt;
} // sendLine


uint16_t Interface::sendHeader(uint16_t &basicPtr)
{
	uint16_t byte_count = 0;

	// Send List HEADER
	// "      MEAT LOAF 64      "
	//	int space_cnt = (16 - strlen(PRODUCT_ID)) / 2;
	int space_cnt = (16 - strlen(FN_VERSION_FULL)) / 2;
	byte_count += sendLine(basicPtr, 0, "\x12\"%*s%s%*s\" %.02d 2A", space_cnt, "", FN_VERSION_FULL, space_cnt, "", m_device.device());

	// Send Extra INFO
	if (m_device.url().length())
	{
		byte_count += sendLine(basicPtr, 0, "%*s\"%-*s\" NFO", 3, "", 16, "[URL]");
		byte_count += sendLine(basicPtr, 0, "%*s\"%-*s\" NFO", 3, "", 16, m_device.url().c_str());
	}
	if (m_device.path().length() > 1)
	{
		byte_count += sendLine(basicPtr, 0, "%*s\"%-*s\" NFO", 3, "", 16, "[PATH]");
		byte_count += sendLine(basicPtr, 0, "%*s\"%-*s\" NFO", 3, "", 16, m_device.path().c_str());		
	}
	if (m_device.url().length() + m_device.path().length() > 1)
	{
		byte_count += sendLine(basicPtr, 0, "%*s\"----------------\" NFO", 3, "");
	}

    return byte_count;
}

void Interface::sendListing()
{
	Debug_printf("\r\nsendListing:\r\n");

	uint16_t byte_count = 0;
	std::string extension = "DIR";

	// Reset basic memory pointer:
	uint16_t basicPtr = C64_BASIC_START;

	// Send load address
	m_iec.send(C64_BASIC_START bitand 0xff);
	m_iec.send((C64_BASIC_START >> 8) bitand 0xff);
	byte_count += 2;
	Debug_println("");

    byte_count += sendHeader( basicPtr );

	// TODO directory handling!!!!!

	// Send List ITEMS
	//sendLine(1, "\"THIS IS A FILE\"     PRG", basicPtr);
	//sendLine(5, "\"THIS IS A FILE 2\"   PRG", basicPtr);
/* 	Dir dir = m_fileSystem->openDir(m_device.path());
	while (dir.next()) {
		uint16_t block_cnt = dir.fileSize() / 256;
		int block_spc = 3;
		if (block_cnt > 9) block_spc--;
		if (block_cnt > 99) block_spc--;
		if (block_cnt > 999) block_spc--;

		int space_cnt = 21 - (dir.fileName().length() + 5);
		if (space_cnt > 21)
			space_cnt = 0;
		
		if(dir.fileSize()) {
			block_cnt = dir.fileSize()/256;

			uint8_t ext_pos = dir.fileName().lastIndexOf(".") + 1;
			if (ext_pos && ext_pos != dir.fileName().length())
			{
				extension = dir.fileName().substring(ext_pos);
				util_string_toupper(extension);
				//extension.toUpperCase();
			}
			else
			{
				extension = "PRG";
			}
		}
		else
		{
			extension = "DIR";
		}

		// Don't show hidden folders or files
		if(!dir.fileName().startsWith("."))
		{
			byte_count += sendLine(basicPtr, block_cnt, "%*s\"%s\"%*s %3s", block_spc, "", dir.fileName().c_str(), space_cnt, "", extension.c_str());
		}
		
		//Debug_printf(" (%d, %d)\r\n", space_cnt, byte_count);
		fnLedManager.toggle(LED_SIO);
		//toggleLED(true);
	}	
 */
    byte_count += sendFooter( basicPtr );

	// End program with two zeros after last line. Last zero goes out as EOI.
	m_iec.send(0);
	m_iec.sendEOI(0);

	Debug_printf("\r\nBytes Sent: %d\r\n", byte_count);

	//ledON();
	fnLedManager.set(LED_SIO);
} // sendListing

uint16_t Interface::sendFooter(uint16_t &basicPtr)
{
	// Send List FOOTER
	// todo TODO figure out fnFS equivalents
	// vfs stat?
	//FSInfo64 fs_info;
	//m_fileSystem->info64(fs_info);
	return sendLine(basicPtr, 0, "END.");
	// return sendLine(basicPtr, (fs_info.totalBytes-fs_info.usedBytes)/256, "BLOCKS FREE.");
	//Debug_println("");
}


void Interface::sendFile()
{
	uint16_t i = 0;
	bool success = true;

	//uint16_t bi = 0;
	char b[1];
	//int ba[9];

	//ba[8] = '\0';

	// Find first program
	//if(m_filename.endsWith("*"))
	if (m_filename.back() == '*')
	{
		m_filename = "";

		if (m_device.path() == "/" && m_device.image().length() == 0)
		{
			m_filename = "FB64";
		}
		// TODO directory handling
		// else
		// {
		// 	Dir dir = m_fileSystem->openDir(m_device.path());
		// 	while (dir.next() && dir.isDirectory())
		// 	{
		// 		Debug_printf("\r\nsendFile: %s", dir.fileName().c_str());
		// 	}
		// 	if(dir.isFile())
		// 		m_filename = dir.fileName();			
		// }
	}
	std::string inFile = std::string(m_device.path()+m_filename);
	

	FILE* file = m_fileSystem->file_open(inFile.c_str(), "rb");
	
	if (!file)
	{
		Debug_printf("\r\nsendFile: %s (File Not Found)\r\n", inFile.c_str());
		m_iec.sendFNF();
	}
	else
	{
		size_t len = m_fileSystem->filesize(file);
		//.size();

		Debug_printf("\r\nsendFile: [%s] (%d bytes)\r\n=================================\r\n", inFile.c_str(), len);
		for(i = 0; success and i < len; ++i) { // End if sending to CBM fails.
			//success = file.readBytes(b, 1);
			success = fread(&b, 1, 1, file);
			if(i == len - 1)
			{
				success = m_iec.sendEOI(b[0]); // indicate end of file.				
			}
			else
			{
				success = m_iec.send(b[0]);				
			}

#ifdef DATA_STREAM
			// Show ASCII Data
			if (b[0] < 32 || b[0] == 127) 
				b[0] = 46;

			ba[bi] = b[0];
			bi++;
			if(bi == 8)
			{
				Debug_printf(" %s\r\n", ba);
				bi = 0;
			}
#endif

			// Toggle LED
			if(i % 50 == 0)
				fnLedManager.toggle(LED_SIO);

			//printProgress(len, i);
			Debug_printf("progress: %d %d", len, i);
		}
		fclose(file);
		Debug_println("");
		Debug_printf("%d bytes sent\r\n", i);
		fnLedManager.set(LED_SIO);
		//ledON();

		if( !success )
		{
			//bool s1 = m_iec.status(IEC_PIN_ATN);
			//bool s2 = m_iec.status(IEC_PIN_CLK);
			//bool s3 = m_iec.status(IEC_PIN_DATA);

			//Debug_printf("Transfer failed! %d, %d, %d\r\n", s1, s2, s3);
			Debug_println("Transfer failed!");
		}
	}
} // sendFile

void Interface::saveFile()
{
	std::string outFile = std::string(m_device.path()+m_filename);
	int b;
	
	Debug_printf("\r\nsaveFile: %s", outFile.c_str());

	FILE* file = m_fileSystem->file_open(outFile.c_str(), "w");
//	noInterrupts();
	if (!file)
	{
		Debug_printf("\r\nsaveFile: %s (Error)\r\n", outFile.c_str());
	}
	else
	{	
		bool done = false;
		// Recieve bytes until a EOI is detected
		do {
			b = m_iec.receive();
			done = (m_iec.state() bitand IEC::eoiFlag) or (m_iec.state() bitand IEC::errorFlag);

			//file.write(b);
			fwrite(&b, 2, 1, file);
		} while(not done);
		fclose(file);
	}
//	interrupts();
} // saveFile


/* 
void Interface::sendListingHTTP()
{
	Debug_printf("\r\nsendListingHTTP: ");

	uint16_t byte_count = 0;

	std::string user_agent( std::string(PRODUCT_ID) + " [" + std::string(FW_VERSION) + "]" );
	std::string url("http://"+m_device.url()+"/api/");
	std::string post_data("p="+urlencode(m_device.path())+"&i="+urlencode(m_device.image())+"&f="+urlencode(m_filename));

	// Connect to HTTP server
	HTTPClient client;
	client.setUserAgent( user_agent );
	client.setFollowRedirects(true);
	client.setTimeout(10000);
	if (!client.begin(url)) {
		Debug_println("\r\nConnection failed");
		m_iec.sendFNF();
		return;
	}
	client.addHeader("Content-Type", "application/x-www-form-urlencoded");	

	Debug_printf("\r\nConnected!\r\n--------------------\r\n%s\r\n%s\r\n%s\r\n", user_agent.c_str(), url.c_str(), post_data.c_str());

	int httpCode = client.POST(post_data);            //Send the request
	WiFiClient payload = client.getStream();    //Get the response payload as Stream
	//std::string payload = client.getString();    //Get the response payload as std::string

	Debug_printf("HTTP Status: %d\r\n", httpCode);   //Print HTTP return code
	if(httpCode != 200)
	{
		Debug_println("Error");
		m_iec.sendFNF();
		return;
	}

	//Serial.println(payload);    //Print request response payload
	m_lineBuffer = payload.readStringUntil('\n');

	// Reset basic memory pointer:
	uint16_t basicPtr = C64_BASIC_START;

	// Send load address
	m_iec.send(C64_BASIC_START bitand 0xff);
	m_iec.send((C64_BASIC_START >> 8) bitand 0xff);
	byte_count += 2;
	Debug_println("");

	do
	{
		// Parse JSON object
		DeserializationError error = deserializeJson(m_jsonHTTP, m_lineBuffer);
		if (error) {
			Serial.print("\r\ndeserializeJson() failed: ");
			Serial.println(error.c_str());
			break;
		}

		byte_count += sendLine(basicPtr, m_jsonHTTP["blocks"], "%s", urldecode(m_jsonHTTP["line"].as<std::string>()).c_str());
		toggleLED(true);
		m_lineBuffer = payload.readStringUntil('\n');
		//Serial.printf("\r\nlinebuffer: %d %s", m_lineBuffer.length(), m_lineBuffer.c_str());
	} while (m_lineBuffer.length() > 1);

	//End program with two zeros after last line. Last zero goes out as EOI.
	m_iec.send(0);
	m_iec.sendEOI(0);

	Debug_printf("\r\nBytes Sent: %d\r\n", byte_count);

	client.end();  //Close connection		

	ledON();
} // sendListingHTTP
 */
/* 
void Interface::sendFileHTTP()
{
	uint16_t i = 0;
	bool success = true;

	uint16_t bi = 0;
	char b[1];
	int ba[9];

	ba[8] = '\0';

	Debug_printf("\r\nsendFileHTTP: ");

	std::string user_agent( std::string(PRODUCT_ID) + " [" + std::string(FW_VERSION) + "]" );
	std::string url("http://"+m_device.url()+"/api/");
	std::string post_data("p="+urlencode(m_device.path())+"&i="+urlencode(m_device.image())+"&f="+urlencode(m_filename));

	// Connect to HTTP server
	HTTPClient client;
	client.setUserAgent( user_agent );
	client.setFollowRedirects(true);
	client.setTimeout(10000);
	if (!client.begin(url)) {
		Debug_println("\r\nConnection failed");
		m_iec.sendFNF();
		return;
	}
	client.addHeader("Content-Type", "application/x-www-form-urlencoded");	

	Debug_printf("\r\nConnected!\r\n--------------------\r\n%s\r\n%s\r\n%s\r\n", user_agent.c_str(), url.c_str(), post_data.c_str());


	int httpCode = client.POST(post_data);            //Send the request
	WiFiClient file = client.getStream();    //Get the response payload as Stream
	
	if (!file.available())
	{
		Debug_printf("\r\nsendFileHTTP: %s (File Not Found)\r\n", url.c_str());
		m_iec.sendFNF();
	}
	else
	{
		size_t len = client.getSize();

		Debug_printf("\r\nsendFileHTTP: %d bytes\r\n=================================\r\n", len);
		for(i = 0; success and i < len; ++i) { // End if sending to CBM fails.
			success = file.readBytes(b, 1);
			if(i == len - 1)
			{
				success = m_iec.sendEOI(b[0]); // indicate end of file.				
			}
			else
			{
				success = m_iec.send(b[0]);				
			}

#ifdef DATA_STREAM
			// Show ASCII Data
			if (b[0] < 32 || b[0] == 127) 
				b[0] = 46;

			ba[bi] = b[0];
			bi++;
			if(bi == 8)
			{
				Debug_printf(" %s\r\n", ba);
				bi = 0;
			}
#endif

			// Toggle LED
			if(i % 50 == 0)
				toggleLED(true);

			printProgress(len, i);
		}
		client.end();
		Debug_println("");
		Debug_printf("%d bytes sent\r\n", i);
		ledON();

		if( !success )
		{
			bool s1 = m_iec.status(IEC_PIN_ATN);
			bool s2 = m_iec.status(IEC_PIN_CLK);
			bool s3 = m_iec.status(IEC_PIN_DATA);

			Debug_printf("Transfer failed! %d, %d, %d\r\n", s1, s2, s3);
		}
	}
}
 */