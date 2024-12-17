#pragma once

#include "ivi-logging-console.h"

namespace logging {

class FileLogData : public StreamLogData {
  public:
    ~FileLogData() {
        flushLog();
    }
};

class FileLogContext : public StreamLogContextAbstract {
  public:
    typedef FileLogData LogDataType;

    FILE* getFile(logging::StreamLogData&) override {
        return getFileStatic();
    }

    static void openFile(char const* fileName) {
        setFilePath(fileName);
    }

    static void setFilePath(char const* fileName) {
        if (getFileStatic() == nullptr) {
            getFileStatic() = fopen(fileName, "w");
        }
        assert(getFileStatic() != nullptr);
    }

  private:
    static FILE*& getFileStatic() {
        static FILE* m_file;
        return m_file;
    }
};

} // namespace logging
