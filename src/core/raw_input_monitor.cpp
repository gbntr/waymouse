#include "core/raw_input_monitor.hpp"

#include <libudev.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <thread>

namespace waymouse {

// Maximum number of open device fds
static constexpr int MAX_DEVICES = 16;
// epoll wait timeout in milliseconds
static constexpr int EPOLL_TIMEOUT_MS = 100;

RawInputMonitor::RawInputMonitor()
    : m_running(false)
    , m_functional(false)
    , m_permission_denied(false)
    , m_needs_rescan(true)
    , m_last_error()
{
}

RawInputMonitor::~RawInputMonitor()
{
    stop();
}

void RawInputMonitor::start()
{
    if (m_running.load())
        return;

    m_running.store(true);
    m_needs_rescan.store(true);
    m_permission_denied.store(false);
    m_functional.store(false);
    m_last_error.clear();
    publish_state();
    m_thread = std::thread(&RawInputMonitor::run, this);
}

void RawInputMonitor::stop()
{
    if (!m_running.load())
        return;

    m_running.store(false);

    if (m_thread.joinable())
        m_thread.join();

    // Close all fds
    for (int fd : m_fds)
    {
        if (fd >= 0)
            close(fd);
    }
    m_fds.clear();
    m_functional.store(false);
    publish_state();
}

bool RawInputMonitor::is_running() const
{
    return m_running.load();
}

bool RawInputMonitor::has_functional_input() const
{
    return m_functional.load();
}

bool RawInputMonitor::permission_denied() const
{
    return m_permission_denied.load();
}

std::string RawInputMonitor::last_error() const
{
    return m_last_error;
}

RawInputMonitor::State RawInputMonitor::state() const
{
    State s;
    s.running = m_running.load();
    s.functional = m_functional.load();
    s.permission_denied = m_permission_denied.load();
    s.needs_rescan = m_needs_rescan.load();
    s.last_error = m_last_error;
    return s;
}

void RawInputMonitor::publish_state()
{
    if (on_state_change)
        on_state_change(state());
}

void RawInputMonitor::scan_and_open()
{
    // Close any previously opened fds
    for (int fd : m_fds)
    {
        if (fd >= 0)
            close(fd);
    }
    m_fds.clear();
    m_functional.store(false);
    m_permission_denied.store(false);
    m_last_error.clear();

    auto* udev = udev_new();
    if (!udev)
    {
        std::cerr << "RawInputMonitor: failed to create udev context\n";
        m_last_error = "failed to create udev context";
        return;
    }

    auto* enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_add_match_property(enumerate, "ID_INPUT_MOUSE", "1");
    udev_enumerate_scan_devices(enumerate);

    auto* devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry* entry;
    udev_list_entry_foreach(entry, devices)
    {
        const char* syspath = udev_list_entry_get_name(entry);
        if (!syspath)
            continue;

        // Exclude touchpads
        auto* dev = udev_device_new_from_syspath(udev, syspath);
        if (dev)
        {
            const char* touchpad = udev_device_get_property_value(dev, "ID_INPUT_TOUCHPAD");
            if (touchpad && std::strcmp(touchpad, "1") == 0)
            {
                udev_device_unref(dev);
                continue;
            }
            udev_device_unref(dev);
        }

        // Build the event device path
        std::string name = std::filesystem::path(syspath).filename().string();
        std::string devnode = "/dev/input/" + name;

        // Validate it's an event node and accessible
        if (!std::filesystem::exists(devnode))
            continue;

        int fd = open(devnode.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0)
        {
            if (errno == EACCES)
            {
                std::cerr << "RawInputMonitor: permission denied on "
                          << devnode << " (user not in 'input' group?)\n";
                m_permission_denied.store(true);
                m_last_error = "permission denied opening " + devnode;
            }
            continue;
        }

        // Verify the device supports REL_X/REL_Y
        unsigned long evbit = 0;
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit) < 0)
        {
            close(fd);
            continue;
        }
        if (!(evbit & (1ULL << EV_REL)))
        {
            close(fd);
            continue;
        }

        unsigned long relbit = 0;
        if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relbit)), &relbit) < 0)
        {
            close(fd);
            continue;
        }
        if (!(relbit & (1ULL << REL_X)) && !(relbit & (1ULL << REL_Y)))
        {
            close(fd);
            continue;
        }

        m_fds.push_back(fd);
        m_functional.store(true);
        std::cout << "RawInputMonitor: opened " << devnode << "\n";
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    if (m_fds.empty())
    {
        std::cerr << "RawInputMonitor: no mouse devices found\n";
        if (m_last_error.empty())
            m_last_error = m_permission_denied.load() ? "permission denied opening input devices"
                                                      : "no mouse devices found";
    }

    publish_state();
}

void RawInputMonitor::run()
{
    while (m_running.load())
    {
        if (m_needs_rescan.load() || m_fds.empty())
        {
            scan_and_open();
            m_needs_rescan.store(false);
            if (m_fds.empty())
            {
                publish_state();
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }
        }

        // Create epoll instance
        int epfd = epoll_create1(0);
        if (epfd < 0)
        {
            std::cerr << "RawInputMonitor: epoll_create1 failed: " << strerror(errno) << "\n";
            m_last_error = "epoll_create1 failed";
            publish_state();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        // Add all device fds to epoll
        for (int fd : m_fds)
        {
            epoll_event ev{};
            ev.events = EPOLLIN;
            ev.data.fd = fd;
            if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0)
            {
                std::cerr << "RawInputMonitor: epoll_ctl ADD failed: " << strerror(errno) << "\n";
            }
        }

        epoll_event events[MAX_DEVICES];

        int nfds = epoll_wait(epfd, events, MAX_DEVICES, EPOLL_TIMEOUT_MS);
        if (nfds < 0)
        {
            if (errno == EINTR)
            {
                close(epfd);
                continue;
            }
            std::cerr << "RawInputMonitor: epoll_wait error: " << strerror(errno) << "\n";
            m_last_error = "epoll_wait error";
            publish_state();
            break;
        }

        for (int i = 0; i < nfds; ++i)
        {
            int fd = events[i].data.fd;

            input_event ie{};
            while (true)
            {
                ssize_t n = read(fd, &ie, sizeof(ie));
                if (n < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        break; // no more data
                    if (errno == ENODEV)
                    {
                        std::cerr << "RawInputMonitor: device removed, fd=" << fd << "\n";
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                        close(fd);
                        m_needs_rescan.store(true);
                        m_functional.store(false);
                        publish_state();
                        break;
                    }
                    break;
                }
                if (n != sizeof(ie))
                    break; // partial read, skip

                // Only process REL_X and REL_Y
                if (ie.type != EV_REL)
                    continue;
                if (ie.code != REL_X && ie.code != REL_Y)
                    continue;

                // Convert timeval to microseconds
                uint64_t timestamp_us =
                    static_cast<uint64_t>(ie.time.tv_sec) * 1000000ULL +
                    static_cast<uint64_t>(ie.time.tv_usec);

                // Build event and deliver
                InputEvent ev{};
                ev.timestamp_us = timestamp_us;
                if (ie.code == REL_X)
                    ev.rel_x = ie.value;
                else
                    ev.rel_y = ie.value;

                if (on_event)
                    on_event(ev);
            }
        }

        close(epfd);
    }

    m_running.store(false);
    publish_state();
}

} // namespace waymouse
