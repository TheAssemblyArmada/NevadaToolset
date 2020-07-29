#include "cstraw.h"
#include <algorithm>
#include <cstring>

int CacheStraw::Get(void *source, int slen)
{
    int total = 0;

    if (m_bufferPtr.Get_Buffer() == nullptr || source == nullptr || slen <= 0) {
        return total;
    }

    while (slen > 0) {
        if (m_length > 0) {
            // How much data to read into destination buffer?
            int readlen = std::min(slen, m_length);

            // Move from our cache buffer to destination
            memmove(source, m_bufferPtr.Get_Buffer() + m_index, readlen);

            // move our buffer positions and increase the total by the read amount
            m_index += readlen;
            total += readlen;
            source = static_cast<char *>(source) + readlen;

            // decrement the amount of data we have left in the cache and slen
            // left to fill in destination
            m_length -= readlen;
            slen -= readlen;
        }

        // If we have read requested amount to destination, exit loop
        if (!slen) {
            break;
        }

        // Read some more data from linked straw
        m_length = Straw::Get(m_bufferPtr.Get_Buffer(), m_bufferPtr.Get_Size());
        m_index = 0;

        // If we didn't get any more data, exit the loop
        if (!m_length) {
            break;
        }
    }

    return total;
}
