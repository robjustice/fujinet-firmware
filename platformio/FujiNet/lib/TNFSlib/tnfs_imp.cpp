#include "tnfs_imp.h"

//tnfsPacket_t tnfsPacket;
//#include "tnfs_udp.h"

WiFiUDP UDP;
// byte tnfs_fd;
tnfsPacket_t tnfsPacket;
//byte sector[128];
//int dataidx = 0;

/* File System Implementation */

//TNFSImpl::TNFSImpl() {}

std::string TNFSImpl::host()
{

  if (_mountpoint != NULL)
  {
    char temp[36];
    int n = sscanf(_mountpoint, "%s", temp);
    if (n == 1)
      _host = temp;
    else
      _host.clear();
  }
  return _host;
}

uint16_t TNFSImpl::port()
{
  if (_mountpoint != NULL)
  {
    uint16_t temp;
    int n = sscanf(_mountpoint, "%*s %hu", &temp);
    if (n == 1)
      _port = temp;
  }
  return _port;
}

tnfsSessionID_t TNFSImpl::sid()
{
  if (_mountpoint != NULL)
  {
    byte lo;
    byte hi;
    int n = sscanf(_mountpoint, "%*s %*u %hhu %hhu", &lo, &hi);
    if (n == 1)
      _sid.session_idl = lo;
    _sid.session_idh = hi;
  }
  return _sid;
}

std::string TNFSImpl::location()
{
  if (_mountpoint != NULL)
  {
    char temp[36];
    int n = sscanf(_mountpoint, "%*s %*u %s", temp);
    if (n == 1)
      _location = temp;
    else
      _location.clear();
  }
  return _location;
}

std::string TNFSImpl::userid()
{
  if (_mountpoint != NULL)
  {
    char temp[36];
    int n = sscanf(_mountpoint, "%*s %*u %*s %s", temp);
    if (n == 1)
      _userid = temp;
    else
      _userid.clear();
  }
  return _userid;
}

std::string TNFSImpl::password()
{
  if (_mountpoint != NULL)
  {
    char temp[36];
    int n = sscanf(_mountpoint, "%*s %*u %*s %*s %s", temp);
    if (n == 1)
      _password = temp;
    else
      _password.clear();
  }
  return _password;
}

FileImplPtr TNFSImpl::open(const char *path, const char *mode)
{
  int ret_fd;
  byte fd;

  // TODO: path (filename) checking

  // translate C++ file mode to TNFS file flags
  uint16_t flag = TNFS_RDONLY;
  byte flag_lsb;
  byte flag_msb;
  if (strlen(mode) == 1)
  {
    switch (mode[0])
    {
    case 'r':
      flag = TNFS_RDONLY;
      break;
    case 'w':
      flag = TNFS_WRONLY | TNFS_CREAT | TNFS_TRUNC;
      break;
    case 'a':
      flag = TNFS_WRONLY | TNFS_CREAT | TNFS_APPEND;
      break;
    default:
      return nullptr;
    }
  }
  else if (strlen(mode) == 2)
  {
    if (mode[1] == '+')
    {
      switch (mode[0])
      {
      case 'r':
        flag = TNFS_RDWR;
        break;
      case 'w':
        flag = TNFS_RDWR | TNFS_CREAT | TNFS_TRUNC;
        break;
      case 'a':
        flag = TNFS_RDWR | TNFS_CREAT | TNFS_APPEND;
        break;
      default:
        return nullptr;
      }
    }
    else
    {
      return nullptr;
    }
  }
  flag_lsb = byte(flag & 0xff);
  flag_msb = byte(flag >> 8);

  // test if path is directory
  bool is_dir = tnfs_stat(this, path);
  if (is_dir)
    ret_fd = tnfs_opendir(this, path);
  else
    ret_fd = tnfs_open(this, path, flag_lsb, flag_msb);

  if (ret_fd != -1)
  {
    fd = (byte)ret_fd;
  }
  else
  {
    return nullptr;
  }
  return std::make_shared<TNFSFileImpl>(this, fd, path);
}

bool TNFSImpl::exists(const char *path)
{
  File f = open(path, "r");
  return (f == true && !f.isDirectory());
}

bool TNFSImpl::rename(const char *pathFrom, const char *pathTo) { return false; }
bool TNFSImpl::remove(const char *path) { return false; }
bool TNFSImpl::mkdir(const char *path) { return false; }
bool TNFSImpl::rmdir(const char *path) { return false; }

/* File Implementation */

TNFSFileImpl::TNFSFileImpl(TNFSImpl *fs, byte fd, const char *filename)
{
  this->fs = fs;
  this->fd = fd;
  strcpy(fn, filename);
}

size_t TNFSFileImpl::write(const uint8_t *buf, size_t size)
{
#ifdef DEBUG_S
  BUG_UART.println("calling tnfs_write");
#endif
  size_t ret = tnfs_write(fs, fd, buf, size);
  if (size == ret)
    return size;
  else
    return 0;
}

size_t TNFSFileImpl::read(uint8_t *buf, size_t size)
{
#ifdef DEBUG_S
  BUG_UART.println("calling tnfs_read");
#endif
  size_t ret = tnfs_read(fs, fd, buf, size);
  // move this part into tnfs_read and pass a buffer instead
  if (size == ret)
    return size;
  else
    return 0;
}

void TNFSFileImpl::flush() {}

bool TNFSFileImpl::seek(uint32_t pos, SeekMode mode)
{
  tnfs_seek(fs, fd, pos); // implement SeekMode
  return true;
}

void TNFSFileImpl::close()
{
  tnfs_close(fs, fd, fn);
}

const char *TNFSFileImpl::name() const
{
  return fn;
}

boolean TNFSFileImpl::isDirectory(void)
{
  bool is_dir = tnfs_stat(fs, fn);
  return is_dir;
}

// not written yet
size_t TNFSFileImpl::position() const { return 0; }
size_t TNFSFileImpl::size() const { 
  
// this requires a STAT call

  return 0; }
time_t TNFSFileImpl::getLastWrite() { return 0; }
FileImplPtr TNFSFileImpl::openNextFile(const char *mode) { 

// call READDIR and the TNFS.open
// but RTFM about this on ESP32

  char nextfn[36];
  byte nextfd=0;
  return std::make_shared<TNFSFileImpl>(this, nextfd, nextfn); }
void TNFSFileImpl::rewindDirectory(void) {

// call tnfs_closedir and tnfs_opendir
// but RTFM about this on ESP32

}
TNFSFileImpl::operator bool() { return true; }

// TNFS calls

/*
-------------------------
MOUNT - Command ID 0x00
-------------------------
  Format:
  Standard header followed by:
  Bytes 4+: 16 bit version number, little endian, LSB = minor, MSB = major
            NULL terminated string: mount location
            NULL terminated string: user id (optional - NULL if no user id)
            NULL terminated string: password (optional - NULL if no passwd)

  The server responds with the standard header:
  Bytes 0,1       Connection ID (ignored for client's "mount" command)
  Byte  2         Retry number
  Byte  3         Command
  If the operation was successful, the standard header contains the session number.
  Byte 4 contains the command or error code. Then there is the
  TNFS protocol version that the server is using following the header, followed by the 
  minimum retry time in milliseconds as a little-endian 16 bit number.

  Return cases:
  valid nonzero session ID
  invalid zero session ID
*/
tnfsSessionID_t tnfs_mount(FSImplPtr hostPtr) //(unsigned char hostSlot)
{

  tnfsSessionID_t tempID;

  std::string mp(hostPtr->mountpoint());

  // extract the parameters
  //host + sep + numstr + sep + "0 0 " + location + sep + userid + sep + password;
  char host[36];
  uint16_t port;
  char location[36];
  char userid[36];
  char password[36];
  sscanf(mp.c_str(), "%s %hu %*u %*u %s %s %s", host, &port, location, userid, password);

  int start = millis();
  int dur = millis() - start;
  unsigned char retries = 0;

  while (retries < 5)
  {
    memset(tnfsPacket.rawData, 0, sizeof(tnfsPacket.rawData));

    // Do not mount, if we already have a session ID, just bail.
    // if (tnfsSessionIDs[hostSlot].session_idl != 0 && tnfsSessionIDs[hostSlot].session_idh != 0)
    //   return true;

    tnfsPacket.session_idl = 0;
    tnfsPacket.session_idh = 0;
    tnfsPacket.retryCount = 0;
    tnfsPacket.command = 0;
    tnfsPacket.data[0] = 0x01; // vers
    tnfsPacket.data[1] = 0x00; // "  "
    // todo: need to strcpy location, userid and password
    tnfsPacket.data[2] = 0x2F; // /
    tnfsPacket.data[3] = 0x00; // nul
    tnfsPacket.data[4] = 0x00; // no username
    tnfsPacket.data[5] = 0x00; // no password

#ifdef DEBUG_VERBOSE
    Debug_print("Mounting / from ");
    Debug_println((char *)hostSlots.host[hostSlot]);
    for (int i = 0; i < 32; i++)
      Debug_printf("%02x ", hostSlots.host[hostSlot][i]);
    Debug_printf("\n\n");
    Debug_print("Req Packet: ");
    for (int i = 0; i < 10; i++)
    {
      Debug_print(tnfsPacket.rawData[i], HEX);
      Debug_print(" ");
    }
    Debug_println("");
#endif /* DEBUG_S */

    UDP.beginPacket(host, port);
    UDP.write(tnfsPacket.rawData, 10);
    UDP.endPacket();

#ifdef DEBUG_VERBOSE
    Debug_println("Wrote the packet");
#endif

    while (dur < 5000)
    {
      dur = millis() - start;
      yield();
      if (UDP.parsePacket())
      {
        int l = UDP.read(tnfsPacket.rawData, 516);
#ifdef DEBUG_VERBOSE
        Debug_print("Resp Packet: ");
        for (int i = 0; i < l; i++)
        {
          Debug_print(tnfsPacket.rawData[i], HEX);
          Debug_print(" ");
        }
        Debug_println("");
#endif /* DEBUG_S */
        if (tnfsPacket.data[0] == 0x00)
        {
// Successful
#ifdef DEBUG_VERBOSE
          Debug_print("Successful, Session ID: ");
          Debug_print(tnfsPacket.session_idl, HEX);
          Debug_println(tnfsPacket.session_idh, HEX);
#endif /* DEBUG_S */
          // Persist the session ID.
          tempID.session_idl = tnfsPacket.session_idl;
          tempID.session_idh = tnfsPacket.session_idh;
          return tempID;
        }
        else
        {
// Error
#ifdef DEBUG_VERBOSE
          Debug_print("Error #");
          Debug_println(tnfsPacket.data[0], HEX);
#endif /* DEBUG_S */
          tempID.session_idh = 0;
          tempID.session_idl = 0;
          return tempID;
        }
      }
    }
// Otherwise we timed out.
#ifdef DEBUG_VERBOSE
    Debug_println("Timeout after 5000ms");
#endif /* DEBUG_S */
    retries++;
    tnfsPacket.retryCount--;
  }
#ifdef DEBUG
  Debug_printf("Failed.\n");
#endif
  tempID.session_idh = 0;
  tempID.session_idl = 0;
  return tempID;
}

/*
----------------------------------
OPEN - Opens a file - Command 0x29
----------------------------------
Format: Standard header, flags, mode, then the null terminated filename.
Flags are a bit field.

The flags are:
O_RDONLY        0x0001  Open read only
O_WRONLY        0x0002  Open write only
O_RDWR          0x0003  Open read/write
O_APPEND        0x0008  Append to the file, if it exists (write only)
O_CREAT         0x0100  Create the file if it doesn't exist (write only)
O_TRUNC         0x0200  Truncate the file on open for writing
O_EXCL          0x0400  With O_CREAT, returns an error if the file exists

The modes are the same as described by CHMOD (i.e. POSIX modes). These
may be modified by the server process's umask. The mode only applies
when files are created (if the O_CREAT flag is specified)

Examples: 
Open a file called "/foo/bar/baz.bas" for reading:

0xBEEF 0x00 0x29 0x0001 0x0000 /foo/bar/baz.bas 0x00

Open a file called "/tmp/foo.dat" for writing, creating the file but
returning an error if it exists. Modes set are S_IRUSR, S_IWUSR, S_IRGRP
and S_IWOTH (read/write for owner, read-only for group, read-only for
others):

0xBEEF 0x00 0x29 0x0102 0x01A4 /tmp/foo.dat 0x00

The server returns the standard header and a result code in response.
If the operation was successful, the byte following the result code
is the file descriptor:

0xBEEF 0x00 0x29 0x00 0x04 - Successful file open, file descriptor = 4
0xBEEF 0x00 0x29 0x01 - File open failed with "permssion denied"

(HISTORICAL NOTE: OPEN used to have command id 0x20, but with the
addition of extra flags, the id was changed so that servers could
support both the old style OPEN and the new OPEN)
 */

//bool tnfs_open(unsigned char deviceSlot, unsigned char options, bool create)
int tnfs_open(TNFSImpl *F, const char *mountPath, byte flag_lsb, byte flag_msb)
{

  tnfsSessionID_t sessionID = F->sid();

  int start = millis();
  int dur = millis() - start;
  int c = 0;
  unsigned char retries = 0;

  while (retries < 5)
  {
    //strcpy(mountPath, deviceSlots.slot[deviceSlot].file);
    //tnfsPacket.session_idl = tnfsSessionIDs[deviceSlots.slot[deviceSlot].hostSlot].session_idl;
    //tnfsPacket.session_idh = tnfsSessionIDs[deviceSlots.slot[deviceSlot].hostSlot].session_idh;
    tnfsPacket.session_idl = sessionID.session_idl;
    tnfsPacket.session_idh = sessionID.session_idh;
    tnfsPacket.retryCount++;   // increase sequence #
    tnfsPacket.command = 0x29; // OPEN

    // if (options == 0x01)
    //   tnfsPacket.data[c++] = 0x01;
    // else if (options == 0x02)
    //   tnfsPacket.data[c++] = 0x03;
    // else
    //   tnfsPacket.data[c++] = 0x03;
    tnfsPacket.data[c++] = flag_lsb;

    // tnfsPacket.data[c++] = (create == true ? 0x01 : 0x00); // Create flag
    tnfsPacket.data[c++] = flag_msb;

    tnfsPacket.data[c++] = 0xFF; // mode
    tnfsPacket.data[c++] = 0x01; // 0777
    // tnfsPacket.data[c++] = '/'; // Filename start

    for (int i = 0; i < strlen(mountPath); i++)
    {
      tnfsPacket.data[i + 5] = mountPath[i];
      c++;
    }

    tnfsPacket.data[c++] = 0x00;
    tnfsPacket.data[c++] = 0x00;
    tnfsPacket.data[c++] = 0x00;

#ifdef DEBUG_VERBOSE
    Debug_printf("Opening /%s\n", mountPath);
    Debug_println("");
    Debug_print("Req Packet: ");
    for (int i = 0; i < c + 4; i++)
    {
      Debug_print(tnfsPacket.rawData[i], HEX);
      Debug_print(" ");
    }
#endif /* DEBUG_S */

    //UDP.beginPacket(hostSlots.host[deviceSlots.slot[deviceSlot].hostSlot], 16384);
    UDP.beginPacket(F->host().c_str(), F->port());
    UDP.write(tnfsPacket.rawData, c + 4);
    UDP.endPacket();

    while (dur < 5000)
    {
      dur = millis() - start;
      yield();
      if (UDP.parsePacket())
      {
        int l = UDP.read(tnfsPacket.rawData, 516);
#ifdef DEBUG_VERBOSE
        Debug_print("Resp packet: ");
        for (int i = 0; i < l; i++)
        {
          Debug_print(tnfsPacket.rawData[i], HEX);
          Debug_print(" ");
        }
        Debug_println("");
#endif // DEBUG_S
        if (tnfsPacket.data[0] == 0x00)
        {
          // Successful
          //tnfs_fds[deviceSlot] = tnfsPacket.data[1];
          int fid = tnfsPacket.data[1];
          return fid;
#ifdef DEBUG_VERBOSE
          Debug_print("Successful, file descriptor: #");
          Debug_println(tnfs_fds[deviceSlot], HEX);
#endif /* DEBUG_S */
          return true;
        }
        else
        {
// unsuccessful
#ifdef DEBUG
          Debug_print("Error code #");
          Debug_println(tnfsPacket.data[0], HEX);
#endif /* DEBUG_S*/
          return -1;
        }
      }
    }
    // Otherwise, we timed out.
    retries++;
    tnfsPacket.retryCount--;
#ifdef DEBUG
    Debug_println("Timeout after 5000ms.");
#endif /* DEBUG_S */
  }
#ifdef DEBUG
  Debug_printf("Failed\n");
#endif
  return -1;
}

/*
------------------------------------
CLOSE - Closes a file - Command 0x23
------------------------------------
  Closes an open file. Consists of the standard header, followed by
  the file descriptor. Example:

  0xBEEF 0x00 0x23 0x04 - Close file descriptor 4

  The server replies with the standard header followed by the return
  code:

  0xBEEF 0x00 0x23 0x00 - File closed.
  0xBEEF 0x00 0x23 0x06 - Operation failed with EBADF, "bad file descriptor"
*/
//bool tnfs_close(unsigned char deviceSlot)
bool tnfs_close(TNFSImpl *F, byte fd, const char *mountPath)
{
  tnfsSessionID_t sessionID = F->sid();

  int start = millis();
  int dur = millis() - start;
  int c = 0;
  unsigned char retries = 0;

  while (retries < 5)
  {
    //strcpy(mountPath, deviceSlots.slot[deviceSlot].file);
    tnfsPacket.session_idl = sessionID.session_idl;
    tnfsPacket.session_idh = sessionID.session_idh;
    tnfsPacket.retryCount++;   // increase sequence #
    tnfsPacket.command = 0x23; // CLOSE

    //tnfsPacket.data[c++] = tnfs_fds[deviceSlot];
    tnfsPacket.data[c++] = fd;

    for (int i = 0; i < strlen(mountPath); i++)
    {
      tnfsPacket.data[i + 5] = mountPath[i];
      c++;
    }

    UDP.beginPacket(F->host().c_str(), F->port());
    UDP.write(tnfsPacket.rawData, c + 4);
    UDP.endPacket();

    while (dur < 5000)
    {
      dur = millis() - start;
      yield();
      if (UDP.parsePacket())
      {
        int l = UDP.read(tnfsPacket.rawData, 516);
#ifdef DEBUG_VERBOSE
        Debug_print("Resp packet: ");
        for (int i = 0; i < l; i++)
        {
          Debug_print(tnfsPacket.rawData[i], HEX);
          Debug_print(" ");
        }
        Debug_println("");
#endif // DEBUG_S
        if (tnfsPacket.data[0] == 0x00)
        {
          // Successful
          return true;
        }
        else
        {
// unsuccessful
#ifdef DEBUG_VERBOSE
          Debug_print("Error code #");
          Debug_println(tnfsPacket.data[0], HEX);
#endif /* DEBUG_S*/
          return false;
        }
      }
    }
    // Otherwise, we timed out.
    retries++;
    tnfsPacket.retryCount--;
#ifdef DEBUG
    Debug_println("Timeout after 5000ms.");
#endif /* DEBUG_S */
  }
#ifdef DEBUG
  Debug_printf("Failed\n");
#endif
  return false;
}

/*
--------------------------------------------------------
OPENDIR - Open a directory for reading - Command ID 0x10
--------------------------------------------------------

  Format:
  Standard header followed by a null terminated absolute path.
  The path delimiter is always a "/". Servers whose underlying 
  file system uses other delimiters, such as Acorn ADFS, should 
  translate. Note that any recent version of Windows understands "/" 
  to be a path delimiter, so a Windows server does not need
  to translate a "/" to a "\".
  Clients should keep track of their own current working directory.

  Example:
  0xBEEF 0x00 0x10 /home/tnfs 0x00 - Open absolute path "/home/tnfs"

  The server responds with the standard header, with byte 4 set to the
  return code which is 0x00 for success, and if successful, byte 5 
  is set to the directory handle.

  Example:
  0xBEEF 0x00 0x10 0x00 0x04 - Successful, handle is 0x04
  0xBEEF 0x00 0x10 0x1F - Failed with code 0x1F
*/
int tnfs_opendir(TNFSImpl *F, const char *dirName)
{
  tnfsSessionID_t sessionID = F->sid();

  int start = millis();
  int dur = millis() - start;
  unsigned char retries = 0;

  while (retries < 5)
  {
    tnfsPacket.session_idl = sessionID.session_idl;
    tnfsPacket.session_idh = sessionID.session_idh;
    tnfsPacket.retryCount++;   // increase sequence #
    tnfsPacket.command = 0x10; // OPENDIR
    strcpy((char *)&tnfsPacket.data[0], dirName);
//tnfsPacket.data[0] = '/';  // Open root dir
//tnfsPacket.data[1] = 0x00; // nul terminated

#ifdef DEBUG
    Debug_println("TNFS Open directory /");
#endif

    UDP.beginPacket(F->host().c_str(), F->port());
    UDP.write(tnfsPacket.rawData, 2 + 4);
    UDP.endPacket();

    while (dur < 5000)
    {
      dur = millis() - start;
      yield();
      if (UDP.parsePacket())
      {
        int l = UDP.read(tnfsPacket.rawData, 516);
        if (tnfsPacket.data[0] == 0x00)
        {
          // Successful
          int handle = tnfsPacket.data[1];
#ifdef DEBUG_VERBOSE
          Debug_printf("Opened dir on slot #%d - fd = %02x\n", hostSlot, tnfs_dir_fds[hostSlot]);
#endif
          return handle;
        }
        else
        {
          // Unsuccessful
          return -1;
        }
      }
    }
// Otherwise, we timed out.
#ifdef DEBUG
    Debug_println("Timeout after 5000ms.");
#endif
    retries++;
    tnfsPacket.retryCount--;
  }
#ifdef DEBUG
  Debug_printf("Failed.");
#endif
  return -1;
}

/**
---------------------------------------------------
READDIR - Reads a directory entry - Command ID 0x11
---------------------------------------------------

  Format:
  Standard header plus directory handle.

  Example:
  0xBEEF 0x00 0x11 0x04 - Read an entry with directory handle 0x04

  The server responds with the standard header, followed by the directory
  entry. Example:

  0xBEEF 0x17 0x11 0x00 . 0x00 - Directory entry for the current working directory
  0xBEEF 0x18 0x11 0x00 .. 0x00 - Directory entry for parent
  0xBEEF 0x19 0x11 0x00 foo 0x00 - File named "foo"

  If the end of directory is reached, or another error occurs, then the
  status byte is set to the error number as for other commands.
  0xBEEF 0x1A 0x11 0x21 - EOF
  0xBEEF 0x1B 0x11 0x1F - Error code 0x1F
*/
bool tnfs_readdir(TNFSImpl *F, byte fd, char *nextFile)
{
  tnfsSessionID_t sessionID = F->sid();

  int start = millis();
  int dur = millis() - start;
  unsigned char retries = 0;

  while (retries < 5)
  {
    tnfsPacket.session_idl = sessionID.session_idl;
    tnfsPacket.session_idh = sessionID.session_idh;
    tnfsPacket.retryCount++;   // increase sequence #
    tnfsPacket.command = 0x11; // READDIR
    tnfsPacket.data[0] = fd;   // dir handle

#ifdef DEBUG_VERBOSE
    Debug_printf("TNFS Read next dir entry, slot #%d - fd %02x\n\n", hostSlot, tnfs_dir_fds[hostSlot]);
#endif

    UDP.beginPacket(F->host().c_str(), F->port());
    UDP.write(tnfsPacket.rawData, 1 + 4);
    UDP.endPacket();

    while (dur < 5000)
    {
      dur = millis() - start;
      yield();
      if (UDP.parsePacket())
      {
        int l = UDP.read(tnfsPacket.rawData, 516);
        if (tnfsPacket.data[0] == 0x00)
        {
          // Successful
          strcpy(nextFile, (char *)&tnfsPacket.data[1]);
          return true;
        }
        else
        {
          // Unsuccessful
          return false;
        }
      }
    }
// Otherwise, we timed out.
#ifdef DEBUG
    Debug_println("Timeout after 5000ms.");
#endif /* DEBUG_S */
    retries++;
    tnfsPacket.retryCount--;
  }
#ifdef DEBUG
  Debug_printf("Failed.\n");
#endif
  return false;
}

/**
-----------------------------------------------------
CLOSEDIR - Close a directory handle - Command ID 0x12
-----------------------------------------------------

  Format:
  Standard header plus directory handle.

  Example, closing handle 0x04:
  0xBEEF 0x00 0x12 0x04

  The server responds with the standard header, with byte 4 set to the
  return code which is 0x00 for success, or something else for an error.
  Example:
  0xBEEF 0x00 0x12 0x00 - Close operation succeeded.
  0xBEEF 0x00 0x12 0x1F - Close failed with error code 0x1F
*/
bool tnfs_closedir(TNFSImpl *F, byte fd)
{
  tnfsSessionID_t sessionID = F->sid();

  int start = millis();
  int dur = millis() - start;
  unsigned char retries = 0;

  while (retries < 5)
  {
    tnfsPacket.session_idl = sessionID.session_idl;
    tnfsPacket.session_idh = sessionID.session_idh;
    tnfsPacket.retryCount++;                     // increase sequence #
    tnfsPacket.command = 0x12;                   // CLOSEDIR
    tnfsPacket.data[0] = fd; // Open root dir

#ifdef DEBUG_VERBOSE
    Debug_println("TNFS dir close");
#endif

    UDP.beginPacket(F->host().c_str(), F->port());
    UDP.write(tnfsPacket.rawData, 1 + 4);
    UDP.endPacket();

    while (dur < 5000)
    {
      dur = millis() - start;
      yield();
      if (UDP.parsePacket())
      {
        int l = UDP.read(tnfsPacket.rawData, 516);
        if (tnfsPacket.data[0] == 0x00)
        {
          // Successful
          return true;
        }
        else
        {
          // Unsuccessful
          return false;
        }
      }
    }
// Otherwise, we timed out.
#ifdef DEBUG
    Debug_println("Timeout after 5000ms.");
    retries++;
    tnfsPacket.retryCount--;
#endif /* DEBUG_S */
  }
#ifdef DEBUG
  Debug_printf("Failed.\n");
#endif
  return false;
}

/*
---------------------------------------
WRITE - Writes to a file - Command 0x22
---------------------------------------
  Writes a block of data to a file. Consists of the standard header,
  followed by the file descriptor, followed by a 16 bit little endian
  value containing the size of the data, followed by the data. The
  entire message must fit in a single datagram.

  Examples:
  Write to fd 4, 256 bytes of data:

  0xBEEF 0x00 0x22 0x04 0x00 0x01 ...data...

  The server replies with the standard header, followed by the return
  code, and the number of bytes actually written. For example:

  0xBEEF 0x00 0x22 0x00 0x00 0x01 - Successful write of 256 bytes
  0xBEEF 0x00 0x22 0x06 - Failed write, error is "bad file descriptor"
*/

size_t tnfs_write(TNFSImpl *F, byte fd, const uint8_t *buf, unsigned short len)
{
  tnfsSessionID_t sessionID = F->sid();

  int start = millis();
  int dur = millis() - start;
  unsigned char retries = 0;

  while (retries < 5)
  {
    tnfsPacket.session_idl = sessionID.session_idl;
    tnfsPacket.session_idh = sessionID.session_idh;
    tnfsPacket.retryCount++;   // Increase sequence
    tnfsPacket.command = 0x22; // READ
    tnfsPacket.data[0] = fd;   // returned file descriptor
    tnfsPacket.data[1] = len & 0xFF;
    tnfsPacket.data[2] = len >> 8;

#ifdef DEBUG_VERBOSE
    Debug_print("Writing to File descriptor: ");
    Debug_println(tnfs_fds[deviceSlot]);
    Debug_print("Req Packet: ");
    for (int i = 0; i < 7; i++)
    {
      Debug_print(tnfsPacket.rawData[i], HEX);
      Debug_print(" ");
    }
    Debug_println("");
#endif /* DEBUG_S */

    UDP.beginPacket(F->host().c_str(), F->port());
    UDP.write(tnfsPacket.rawData, 4 + 3);
    UDP.write(buf, len);
    UDP.endPacket();

    while (dur < 5000)
    {
      dur = millis() - start;
      yield();
      if (UDP.parsePacket())
      {
        int l = UDP.read(tnfsPacket.rawData, sizeof(tnfsPacket.rawData));
#ifdef DEBUG_VERBOSE
        Debug_print("Resp packet: ");
        for (int i = 0; i < l; i++)
        {
          Debug_print(tnfsPacket.rawData[i], HEX);
          Debug_print(" ");
        }
        Debug_println("");
#endif /* DEBUG_S */
        if (tnfsPacket.data[0] == 0x00)
        {
// Successful
#ifdef DEBUG_VERBOSE
          Debug_println("Successful.");
#endif /* DEBUG_S */
          return len;
        }
        else
        {
// Error
#ifdef DEBUG
          Debug_print("Error code #");
          Debug_println(tnfsPacket.data[0], HEX);
#endif /* DEBUG_S*/
          return 0;
        }
      }
    }
#ifdef DEBUG
    Debug_println("Timeout after 5000ms.");
#endif /* DEBUG_S */
    retries++;
    tnfsPacket.retryCount--;
  }
#ifdef DEBUG
  Debug_printf("Failed.\n");
#endif
  return 0;
}

/*
---------------------------------------
READ - Reads from a file - Command 0x21
---------------------------------------
  Reads a block of data from a file. Consists of the standard header
  followed by the file descriptor as returned by OPEN, then a 16 bit
  little endian integer specifying the size of data that is requested.

  The server will only reply with as much data as fits in the maximum
  TNFS datagram size of 1K when using UDP as a transport. For the
  TCP transport, sequencing and buffering etc. are just left up to
  the TCP stack, so a READ operation can return blocks of up to 64K. 

  If there is less than the size requested remaining in the file, 
  the server will return the remainder of the file.  Subsequent READ 
  commands will return the code EOF.

  Examples:
  Read from fd 4, maximum 256 bytes:

  0xBEEF 0x00 0x21 0x04 0x00 0x01

  The server will reply with the standard header, followed by the single
  byte return code, the actual amount of bytes read as a 16 bit unsigned
  little endian value, then the data, for example, 256 bytes:

  0xBEEF 0x00 0x21 0x00 0x00 0x01 ...data...

  End-of-file reached:

  0xBEEF 0x00 0x21 0x21
*/

size_t tnfs_read(TNFSImpl *F, byte fd, uint8_t *buf, unsigned short len)
{

  tnfsSessionID_t sessionID = F->sid();

  int start = millis();
  int dur = millis() - start;
  unsigned char retries = 0;

  while (retries < 5)
  {
    tnfsPacket.session_idl = sessionID.session_idl;
    tnfsPacket.session_idh = sessionID.session_idh;
    tnfsPacket.retryCount++;         // Increase sequence
    tnfsPacket.command = 0x21;       // READ
    tnfsPacket.data[0] = fd;         // returned file descriptor
    tnfsPacket.data[1] = len & 0xFF; // len bytes
    tnfsPacket.data[2] = len >> 8;   //

#ifdef DEBUG_VERBOSE
    Debug_print("Reading from File descriptor: ");
    Debug_println(tnfs_fds[deviceSlot]);
    Debug_print("Req Packet: ");
    for (int i = 0; i < 7; i++)
    {
      Debug_print(tnfsPacket.rawData[i], HEX);
      Debug_print(" ");
    }
    Debug_println("");
#endif /* DEBUG_S */

    UDP.beginPacket(F->host().c_str(), F->port());
    UDP.write(tnfsPacket.rawData, 4 + 3);
    UDP.endPacket();
    start = millis();
    dur = millis() - start;
    while (dur < 5000)
    {
      dur = millis() - start;
      yield();
      if (UDP.parsePacket())
      {
        int l = UDP.read(tnfsPacket.rawData, sizeof(tnfsPacket.rawData));
#ifdef DEBUG_VERBOSE
        Debug_print("Resp packet: ");
        for (int i = 0; i < l; i++)
        {
          Debug_print(tnfsPacket.rawData[i], HEX);
          Debug_print(" ");
        }
        Debug_println("");
#endif /* DEBUG_S */
        if (tnfsPacket.data[0] == 0x00)
        {
// Successful
#ifdef DEBUG_VERBOSE
          Debug_println("Successful.");
#endif /* DEBUG_S */
          uint8_t *s = &tnfsPacket.data[3];
          memcpy(buf, s, len);
          return len;
        }
        else
        {
// Error
#ifdef DEBUG
          Debug_print("Error code #");
          Debug_println(tnfsPacket.data[0], HEX);
#endif /* DEBUG_S*/
          return 0;
        }
      }
    }
#ifdef DEBUG
    Debug_println("Timeout after 5000ms.");
    if (retries < 5)
      Debug_printf("Retrying...\n");
#endif /* DEBUG_S */
    retries++;
    tnfsPacket.retryCount--;
  }
#ifdef DEBUG
  Debug_printf("Failed.\n");
#endif
  return 0;
}

/*
--------------------------------------------------------
LSEEK - Seeks to a new position in a file - Command 0x25
--------------------------------------------------------
  Seeks to an absolute position in a file, or a relative offset in a file,
  or to the end of a file.
  The request consists of the header, followed by the file descriptor,
  followed by the seek type (SEEK_SET, SEEK_CUR or SEEK_END), followed
  by the position to seek to. The seek position is a signed 32 bit integer,
  little endian. (2GB file sizes should be more than enough for 8 bit
  systems!)

  The seek types are defined as follows:
  0x00            SEEK_SET - Go to an absolute position in the file
  0x01            SEEK_CUR - Go to a relative offset from the current position
  0x02            SEEK_END - Seek to EOF

  Example:

  File descriptor is 4, type is SEEK_SET, and position is 0xDEADBEEF:
  0xBEEF 0x00 0x25 0x04 0x00 0xEF 0xBE 0xAD 0xDE

  Note that clients that buffer reads for single-byte reads will have
  to make a calculation to implement SEEK_CUR correctly since the server's
  file pointer will be wherever the last read block made it end up.
*/
bool tnfs_seek(TNFSImpl *F, byte fd, long offset)
{
  tnfsSessionID_t sessionID = F->sid();

  int start = millis();
  int dur = millis() - start;
  byte offsetVal[4];
  unsigned char retries = 0;

  while (retries < 5)
  {
    offsetVal[0] = (int)((offset & 0xFF000000) >> 24);
    offsetVal[1] = (int)((offset & 0x00FF0000) >> 16);
    offsetVal[2] = (int)((offset & 0x0000FF00) >> 8);
    offsetVal[3] = (int)((offset & 0X000000FF));

    tnfsPacket.retryCount++;
    tnfsPacket.session_idl = sessionID.session_idl;
    tnfsPacket.session_idh = sessionID.session_idh;
    tnfsPacket.command = 0x25; // LSEEK
    tnfsPacket.data[0] = fd;
    tnfsPacket.data[1] = 0x00; // SEEK_SET
    tnfsPacket.data[2] = offsetVal[3];
    tnfsPacket.data[3] = offsetVal[2];
    tnfsPacket.data[4] = offsetVal[1];
    tnfsPacket.data[5] = offsetVal[0];
#ifdef DEBUG
    Debug_print("Seek requested to offset: ");
    Debug_println(offset);
#endif /* DEBUG */
#ifdef DEBUG_VERBOSE
    Debug_print("Req packet: ");
    for (int i = 0; i < 10; i++)
    {
      Debug_print(tnfsPacket.rawData[i], HEX);
      Debug_print(" ");
    }
    Debug_println("");
#endif /* DEBUG_S*/

    UDP.beginPacket(F->host().c_str(), F->port());
    UDP.write(tnfsPacket.rawData, 6 + 4);
    UDP.endPacket();

    while (dur < 5000)
    {
      dur = millis() - start;
      yield();
      if (UDP.parsePacket())
      {
        int l = UDP.read(tnfsPacket.rawData, sizeof(tnfsPacket.rawData));
#ifdef DEBUG
        Debug_print("Resp packet: ");
        for (int i = 0; i < l; i++)
        {
          Debug_print(tnfsPacket.rawData[i], HEX);
          Debug_print(" ");
        }
        Debug_println("");
#endif /* DEBUG_S */

        if (tnfsPacket.data[0] == 0)
        {
// Success.
#ifdef DEBUG_VERBOSE
          Debug_println("Successful.");
#endif /* DEBUG_S */
          return true;
        }
        else
        {
// Error.
#ifdef DEBUG
          Debug_print("Error code #");
          Debug_println(tnfsPacket.data[0], HEX);
#endif /* DEBUG_S*/
          return false;
        }
      }
    }
#ifdef DEBUG
    Debug_println("Timeout after 5000ms.");
    if (retries < 5)
      Debug_printf("Retrying...\n");
#endif /* DEBUG_S */
    tnfsPacket.retryCount--;
    retries++;
  }
#ifdef DEBUG
  Debug_printf("Failed.\n");
#endif
  return false;
}

/*
-----------------------------------------------
STAT - Get information on a file - Command 0x24
-----------------------------------------------
  Reads the file's information, such as size, datestamp etc. The TNFS
  stat contains less data than the POSIX stat - information that is unlikely
  to be of use to 8 bit systems are omitted.
  The request consists of the standard header, followed by the full path
  of the file to stat, terminated by a NULL. Example:

  0xBEEF 0x00 0x24 /foo/bar/baz.txt 0x00

  The server replies with the standard header, followed by the return code.
  On success, the file information follows this. Stat information is returned
  in this order. Not all values are used by all servers. At least file
  mode and size must be set to a valid value (many programs depend on these).

  File mode       - 2 bytes: file permissions - little endian byte order
  uid             - 2 bytes: Numeric UID of owner
  gid             - 2 bytes: Numeric GID of owner
  size            - 4 bytes: Unsigned 32 bit little endian size of file in bytes
  atime           - 4 bytes: Access time in seconds since the epoch, little end.
  mtime           - 4 bytes: Modification time in seconds since the epoch,
                            little endian
  ctime           - 4 bytes: Time of last status change, as above.
  uidstring       - 0 or more bytes: Null terminated user id string
  gidstring       - 0 or more bytes: Null terminated group id string

  Fields that don't apply to the server in question should be left as 0x00.
  The ´mtime' field and 'size' fields are unsigned 32 bit integers.
  The uidstring and gidstring are helper fields so the client doesn't have
  to then ask the server for the string representing the uid and gid.

  File mode flags will be most useful for code that is showing a directory
  listing, and for programs that need to find out what kind of file (regular
  file or directory, etc) a particular file may be. They follow the POSIX
  convention which is:

  Flags           Octal representation
  S_IFDIR         0040000         Directory


  Most of these won't be of much interest to an 8 bit client, but the
  read/write/execute permissions can be used for a client to determine whether
  to bother even trying to open a remote file, or to automatically execute
  certain types of files etc. (Further file metadata such as load and execution
  addresses are platform specific and should go into a header of the file
  in question). Note the "trivial" bit in TNFS means that the client is
  unlikely to do anything special with a FIFO, so writing to a file of that
  type is likely to have effects on the server, and not the client! It's also
  worth noting that the server is responsible for enforcing read and write
  permissions (although the permission bits can help the client work out
  whether it should bother to send a request).
*/
bool tnfs_stat(TNFSImpl *F, const char *filename)
{
  tnfsSessionID_t sessionID = F->sid();

  int start = millis();
  int dur = millis() - start;
  int c = 0;
  unsigned char retries = 0;

  while (retries < 5)
  {
    tnfsPacket.session_idl = sessionID.session_idl;
    tnfsPacket.session_idh = sessionID.session_idh;
    tnfsPacket.retryCount++;   // increase sequence #
    tnfsPacket.command = 0x24; // STAT

    for (int i = 0; i < strlen(filename); i++)
    {
      tnfsPacket.data[c++] = filename[i];
      c++;
    }

    tnfsPacket.data[c++] = 0x00;

    UDP.beginPacket(F->host().c_str(), F->port());
    UDP.write(tnfsPacket.rawData, c + 4);
    UDP.endPacket();

    while (dur < 5000)
    {
      dur = millis() - start;
      yield();
      if (UDP.parsePacket())
      {
        int l = UDP.read(tnfsPacket.rawData, 516);
#ifdef DEBUG_VERBOSE
        Debug_print("Resp packet: ");
        for (int i = 0; i < l; i++)
        {
          Debug_print(tnfsPacket.rawData[i], HEX);
          Debug_print(" ");
        }
        Debug_println("");
#endif // DEBUG_S
        if (tnfsPacket.data[0] == 0x00)
        {
          // Successful
          bool is_dir = (tnfsPacket.data[2] == 0x40);
          return is_dir;
        }
        else
        {
// unsuccessful
#ifdef DEBUG
          Debug_print("Error code #");
          Debug_println(tnfsPacket.data[0], HEX);
#endif /* DEBUG_S*/
          return false;
        }
      }
    }
    // Otherwise, we timed out.
    retries++;
    tnfsPacket.retryCount--;
#ifdef DEBUG
    Debug_println("Timeout after 5000ms.");
#endif /* DEBUG_S */
  }
#ifdef DEBUG
  Debug_printf("Failed\n");
#endif
  return false;
}
