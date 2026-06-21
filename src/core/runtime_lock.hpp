#pragma once

#include <memory>
#include <string>

class QLockFile;

namespace waymouse {

class RuntimeLock
{
public:
    RuntimeLock();
    explicit RuntimeLock(std::string path);
    ~RuntimeLock();

    const std::string& path() const;
    void set_path(std::string path);

    bool acquire();
    void release();
    bool is_locked() const;

private:
    std::string m_path;
    std::unique_ptr<QLockFile> m_lock;
};

} // namespace waymouse
