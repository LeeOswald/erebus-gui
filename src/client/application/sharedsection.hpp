#pragma once

#include <erebus/exception.hxx>
#include <erebus/noncopyable.hxx>

#include <erebus-gui/erebus-gui.hpp>

#include <QSharedMemory>


namespace Erc
{

namespace Private
{


template <typename T>
class SharedSection final
    : public Er::NonCopyable
{
public:
    using Type = T;

    class Lock final
        : public Er::NonCopyable
    {
    public:
        using Type = T;

        ~Lock()
        {
            s.m_section->unlock();
        }

        explicit Lock(SharedSection& s)
            : s(s)
        {
            if (!s.m_section->lock())
                throw Er::Exception(ER_HERE(), "Failed to lock the shared memory section", Er::ExceptionProps::DecodedError(Erc::toUtf8(s.m_section->errorString())));
        }

        size_t size() const
        {
            return s.m_section->size();
        }

        T* get()
        {
            return static_cast<T*>(s.m_section->data());
        }

        const T* get() const
        {
            return static_cast<const T*>(s.m_section->data());
        }

    private:
        SharedSection& s;
    };


    ~SharedSection()
    {
    }

    explicit SharedSection(const QString& key)
    {
#if ER_LINUX
        // by explicitly attaching it and then deleting it we make sure that the
        // memory is deleted even after the process has crashed on Unix
        m_section.reset(new QSharedMemory(key));
        m_section->attach();
        m_section.reset();
#endif

        m_section.reset(new QSharedMemory(key));

        if (m_section->create(sizeof(T)))
        {
            m_owner = true;
        }
        else
        {
            if (m_section->error() == QSharedMemory::AlreadyExists)
            {
                if (!m_section->attach())
                    throw Er::Exception(ER_HERE(), "Failed to attach to the shared memory section", Er::ExceptionProps::DecodedError(Erc::toUtf8(m_section->errorString())));
            }
            else
                throw Er::Exception(ER_HERE(), "Failed to create a shared memory section", Er::ExceptionProps::DecodedError(Erc::toUtf8(m_section->errorString())));
        }
    }

    bool owner() const
    {
        return m_owner;
    }

    Lock lock()
    {
        return Lock(*this);
    }

private:
    friend class Lock;

    std::unique_ptr<QSharedMemory> m_section;
    bool m_owner = false;
};


} // namespace Private {}

} // namespace Erc {}