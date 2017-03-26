/*
The MIT License (MIT)

Copyright (c) 2013-2015 SRS(ossrs)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <srs_kernel_file.hpp>

// for srs-librtmp, @see https://github.com/ossrs/srs/issues/213
//#ifndef _WIN32
#include <unistd.h>
#include <sys/uio.h>
//#endif
#include <st.h>

#include <fcntl.h>
#include <sstream>
using namespace std;

#include <srs_kernel_log.hpp>
#include <srs_kernel_error.hpp>

SrsFileWriter::SrsFileWriter()
{
#ifdef WIN32
    fd = NULL;
#else
	fd = -1;
#endif
}

SrsFileWriter::~SrsFileWriter()
{
    close();
}

int SrsFileWriter::open(string p)
{
    int ret = ERROR_SUCCESS;
    
    if (fd > 0) {
        ret = ERROR_SYSTEM_FILE_ALREADY_OPENED;
        srs_error("file %s already opened. ret=%d", path.c_str(), ret);
        return ret;
    }
    
    //O_WRONLY 以只写方式打开文件
    //O_CREAT 若欲打开的文件不存在则自动建立该文件.
    //O_TRUNC 若文件存在并且以可写的方式打开时, 此旗标会令文件长度清为0, 而原来存于该文件的资料也会消失.
    int flags = O_CREAT|O_WRONLY|O_TRUNC;
#ifdef WIN32
    //mode_t mode =_S_IREAD|S_IWRITE;
    if ((fd = fopen(p.c_str(), "w+b")) == 0) { //, mode
#else
    //  S_IRWXU00700 权限, 代表该文件所有者具有可读、可写及可执行的权限.
    //  S_IRUSR 或S_IREAD, 00400 权限, 代表该文件所有者具有可读取的权限.
    //  S_IWUSR 或S_IWRITE, 00200 权限, 代表该文件所有者具有可写入的权限.
    //  S_IXUSR 或S_IEXEC, 00100 权限, 代表该文件所有者具有可执行的权限.
    //  S_IRWXG 00070 权限, 代表该文件用户组具有可读、可写及可执行的权限.
    //  S_IRGRP 00040 权限, 代表该文件用户组具有可读的权限.
    //  S_IWGRP 00020 权限, 代表该文件用户组具有可写入的权限.
    //  S_IXGRP 00010 权限, 代表该文件用户组具有可执行的权限.
    //  S_IRWXO 00007 权限, 代表其他用户具有可读、可写及可执行的权限.
    //  S_IROTH 00004 权限, 代表其他用户具有可读的权限
    //  S_IWOTH 00002 权限, 代表其他用户具有可写入的权限.
    //  S_IXOTH 00001 权限, 代表其他用户具有可执行的权限.
    mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH;
    if ((fd = ::open(p.c_str(), flags, mode)) < 0) {
#endif
        ret = ERROR_SYSTEM_FILE_OPENE;
        srs_error("open file %s failed. ret=%d", p.c_str(), ret);
        return ret;
    }
    
    path = p;
    
    return ret;
}

int SrsFileWriter::open_append(string p)
{
    int ret = ERROR_SUCCESS;
    
    if (fd > 0) {
        ret = ERROR_SYSTEM_FILE_ALREADY_OPENED;
        srs_error("file %s already opened. ret=%d", path.c_str(), ret);
        return ret;
    }
    
    int flags = O_APPEND|O_WRONLY;
#ifdef _WIN32
    //mode_t mode =_S_IREAD|S_IWRITE;
    if ((fd = fopen(p.c_str(), "a+b")) == 0) { //, mode
#else
    mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH;
    if ((fd = ::open(p.c_str(), flags, mode)) < 0) {
#endif
        ret = ERROR_SYSTEM_FILE_OPENE;
        srs_error("open file %s failed. ret=%d", p.c_str(), ret);
        return ret;
    }
    
    path = p;
    
    return ret;
}

void SrsFileWriter::close()
{
    int ret = ERROR_SUCCESS;
    
 #ifdef _WIN32   
    if (fd == NULL) {
        return;
    }
    fclose(fd);
	fd = NULL;
#else
    if (fd < 0) {
        return;
    }
	if (::close(fd) < 0) {

        ret = ERROR_SYSTEM_FILE_CLOSE;
        srs_error("close file %s failed. ret=%d", path.c_str(), ret);
        return;
    }
    fd = -1;
#endif    
    return;
}

bool SrsFileWriter::is_open()
{
    return fd > 0;
}

void SrsFileWriter::lseek(int64_t offset)
{
 #ifdef _WIN32 
	_fseeki64(fd, offset, SEEK_SET);
#else
	::lseek(fd, (off_t)offset, SEEK_SET);
#endif
}

int64_t SrsFileWriter::tellg()
{
 #ifdef _WIN32 
	return (int64_t) _ftelli64(fd);
#else
	return (int64_t)::lseek(fd, 0, SEEK_CUR);
#endif
}

int SrsFileWriter::write(void* buf, size_t count, ssize_t* pnwrite)
{
    int ret = ERROR_SUCCESS;
    
    ssize_t nwrite;
    // TODO: FIXME: use st_write.
 #ifdef _WIN32 
	if (fwrite(buf, (uint32_t) count, 1, fd) != 1) {
#else
	if ((nwrite = ::_write(fd, buf, count)) < 0) {
#endif
        ret = ERROR_SYSTEM_FILE_WRITE;
        srs_error("write to file %s failed. ret=%d", path.c_str(), ret);
        return ret;
    }
#ifdef _WIN32 
	 nwrite = count; 
#endif
    if (pnwrite != NULL) {
        *pnwrite = nwrite;
    }
    
    return ret;
}

int SrsFileWriter::writev(iovec* iov, int iovcnt, ssize_t* pnwrite)
{
    int ret = ERROR_SUCCESS;
    
    ssize_t nwrite = 0;
    for (int i = 0; i < iovcnt; i++) {
        iovec* piov = iov + i;
        ssize_t this_nwrite = 0;
        if ((ret = write(piov->iov_base, piov->iov_len, &this_nwrite)) != ERROR_SUCCESS) {
            return ret;
        }
        nwrite += this_nwrite;
    }
    
    if (pnwrite) {
        *pnwrite = nwrite;
    }
    
    return ret;
}

SrsFileReader::SrsFileReader()
{
#ifdef _WIN32 
	fd = NULL;
#else
    fd = -1;
#endif
}

SrsFileReader::~SrsFileReader()
{
    close();
}

int SrsFileReader::open(string p)
{
    int ret = ERROR_SUCCESS;
    
    if (fd > 0) {
        ret = ERROR_SYSTEM_FILE_ALREADY_OPENED;
        srs_error("file %s already opened. ret=%d", path.c_str(), ret);
        return ret;
    }
#ifdef _WIN32 
    if ((fd = fopen(p.c_str(), "rb")) == 0) {
        ret = ERROR_SYSTEM_FILE_OPENE;
        srs_error("open file %s failed. ret=%d", p.c_str(), ret);
        return ret;
    }
#else
    if ((fd = ::open(p.c_str(), O_RDONLY)) < 0) {
        ret = ERROR_SYSTEM_FILE_OPENE;
        srs_error("open file %s failed. ret=%d", p.c_str(), ret);
        return ret;
    }
#endif
    path = p;
    
    return ret;
}

void SrsFileReader::close()
{
    int ret = ERROR_SUCCESS;
 #ifdef _WIN32    
    if (fd == 0) {
        return;
    }
    
    fclose(fd);
    fd = NULL;
#else
    if (fd == 0) {
        return;
    }
    
    if (fclose(fd) < 0) {
        ret = ERROR_SYSTEM_FILE_CLOSE;
        srs_error("close file %s failed. ret=%d", path.c_str(), ret);
        return;
    }
    fd = -1;
#endif
    return;
}

bool SrsFileReader::is_open()
{
    return fd > 0;
}

int64_t SrsFileReader::tellg()
{
 #ifdef _WIN32 
	return (int64_t) _ftelli64(fd);
#else
	return (int64_t)::lseek(fd, 0, SEEK_CUR);
#endif
}

void SrsFileReader::skip(int64_t size)
{
#ifdef _WIN32 
	_fseeki64(fd, size, SEEK_CUR);
#else
    ::lseek(fd, (off_t)size, SEEK_CUR);
#endif

}

int64_t SrsFileReader::lseek(int64_t offset)
{
#ifdef _WIN32 
	if(_fseeki64(fd, offset, SEEK_SET) == 0)
	{
		return offset;
	}
	return -1;
#else
    return (int64_t)::lseek(fd, (off_t)offset, SEEK_SET);
#endif
}

int64_t SrsFileReader::filesize()
{
    int64_t cur = tellg();
#ifdef _WIN32 
	 _fseeki64(fd, 0, SEEK_END);
	 int64_t size = tellg();
	  _fseeki64(fd, cur, SEEK_SET);
#else
    int64_t size = (int64_t)::lseek(fd, 0, SEEK_END);
    ::lseek(fd, (off_t)cur, SEEK_SET);
#endif
    return size;
}

int SrsFileReader::read(void* buf, size_t count, ssize_t* pnread)
{
    int ret = ERROR_SUCCESS;
    
    ssize_t nread;
    // TODO: FIXME: use st_read.
#ifdef _WIN32 
	if (fread(buf, (uint32_t) count, 1, fd) != 1) {
        ret = ERROR_SYSTEM_FILE_READ;
        srs_error("read from file %s failed. ret=%d", path.c_str(), ret);
        return ret;
    }
	nread = count;
#else
    if ((nread = ::read(fd, buf, count)) < 0) {
        ret = ERROR_SYSTEM_FILE_READ;
        srs_error("read from file %s failed. ret=%d", path.c_str(), ret);
        return ret;
    }
#endif
    if (nread == 0) {
        ret = ERROR_SYSTEM_FILE_EOF;
        return ret;
    }
    
    if (pnread != NULL) {
        *pnread = nread;
    }
    
    return ret;
}

