#ifndef CSTRAW_H
#define CSTRAW_H

#include "always.h"
#include "buffer.h"
#include "straw.h"

class CacheStraw : public Straw
{
public:
    CacheStraw(int length) : Straw(), m_bufferPtr(nullptr, length), m_index(0), m_length(0) {}
    CacheStraw(void *buffer, int length) : Straw(), m_bufferPtr((uint8_t *)buffer, length), m_index(0), m_length(0) {}
    CacheStraw(BufferClass &buffer) : Straw(), m_bufferPtr(buffer), m_index(0), m_length(0) {}

    virtual ~CacheStraw() {}
    virtual int Get(void *source, int slen);

    bool Is_Valid() const { return m_bufferPtr.Is_Valid(); }

protected:
    BufferClass m_bufferPtr;
    int m_index;
    int m_length;
};

#endif // CSTRAW_H
