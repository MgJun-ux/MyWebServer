/**
 * @author: MgJun
 * @brief:处理http响应
 * @date:23/3/21
*/

#pragma once

#include <fcntl.h>      //open
#include <unistd.h>     //closr
#include <sys/stat.h>   //stat
#include <sys/mman.h>  //mmap. munmap
#include <unordered_map>

#include "log.h"
#include "buffer.h"

class HttpResponse{
public:
    HttpResponse();
    ~HttpResponse();


    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    void MakeResponse(Buffer& buff);
    void UnmapFile();
    char* File();
    size_t FileLen() const;
    void ErrorContent(Buffer& buff, std::string message);
    int Code() const { return code_;}

private:
    void AddStatLine_(Buffer& buff);
    void AddHeader_(Buffer& buff);
    void AddContent_(Buffer& buff);

    void ErrorHtml_();
    std::string GetFileType_();

    int code_;
    bool isKeepAlive_;

    std::string path_;
    std::string srcDir_;

    char* mmFile_;
    struct stat mmFileStat_; //_stat结构体是文件（夹）信息的结构体，定义如下：以上信息就是可以通过_stat函数获取的所有相关信息，一般情况下，我们关心文件大小和创建时间、访问时间、修改时间。

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};