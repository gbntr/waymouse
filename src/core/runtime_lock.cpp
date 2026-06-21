#include "core/runtime_lock.hpp"

#include <QLockFile>
#include <QString>

#include <cstdlib>
#include <filesystem>
#include <utility>

namespace waymouse {

namespace {

std::string default_lock_path()
{
    const char* runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    std::filesystem::path base = runtime_dir ? runtime_dir : "/tmp";
    return (base / "waymouse" / "shake-runtime.lock").string();
}

} // namespace

RuntimeLock::RuntimeLock()
    : RuntimeLock(default_lock_path())
{
}

RuntimeLock::RuntimeLock(std::string path)
    : m_path(std::move(path))
    , m_lock(std::make_unique<QLockFile>(QString::fromStdString(m_path)))
{
    std::filesystem::create_directories(std::filesystem::path(m_path).parent_path());
    m_lock->setStaleLockTime(0);
}

RuntimeLock::~RuntimeLock()
{
    release();
}

const std::string& RuntimeLock::path() const
{
    return m_path;
}

void RuntimeLock::set_path(std::string path)
{
    release();
    m_path = std::move(path);
    m_lock = std::make_unique<QLockFile>(QString::fromStdString(m_path));
    std::filesystem::create_directories(std::filesystem::path(m_path).parent_path());
    m_lock->setStaleLockTime(0);
}

bool RuntimeLock::acquire()
{
    if (!m_lock)
        m_lock = std::make_unique<QLockFile>(QString::fromStdString(m_path));

    std::filesystem::create_directories(std::filesystem::path(m_path).parent_path());
    m_lock->setStaleLockTime(0);
    return m_lock->tryLock();
}

void RuntimeLock::release()
{
    if (m_lock && m_lock->isLocked())
        m_lock->unlock();
}

bool RuntimeLock::is_locked() const
{
    return m_lock && m_lock->isLocked();
}

} // namespace waymouse
