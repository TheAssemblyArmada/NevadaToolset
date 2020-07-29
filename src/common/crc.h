/**
 * @file
 *
 * @author CCHyper
 * @author OmniBlade
 *
 * @brief CRC calculating classes.
 *
 * @copyright Chronoshift is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#ifndef CRC_H
#define CRC_H

#include "always.h"

/**
 * @brief Generates simple 32bit hashes of data.
 *
 * Generates simple 32bit hashes of data. Unlike the TS equivalent, this appears to be something Westwood rolled themselves
 * rather than being a standard CRC32.
 */
class CRCEngine
{
public:
    CRCEngine(int32_t initial = 0) : m_crc(initial), m_index(0) { m_buffer.integer = 0; }

    int32_t operator()(const void *buffer, unsigned length = 0);
    operator int32_t() { return Value(); }

private:
    void operator()(char datnum);
    int32_t Value();
    bool Buffer_Needs_Data() { return m_index != 0; }

private:
    int32_t m_crc;
    int m_index;

    union
    {
        int32_t integer;
        int8_t bytes[4];
    } m_buffer;
};

class CRC32Engine
{
public:
    CRC32Engine(int32_t initial = 0) : m_crc(initial), m_index(0) { m_buffer.integer = 0; }

    int32_t operator()(const void *buffer, unsigned length = 0);
    operator int32_t() { return Value(); }

private:
    void operator()(char datnum);
    int32_t Value();
    bool Buffer_Needs_Data() { return m_index != 0; }

private:
    int32_t m_crc;
    int m_index;

    union
    {
        int32_t integer;
        int8_t bytes[4];
    } m_buffer;
};

template<typename CRC>
int32_t Calculate_CRC(const void *buffer, unsigned length)
{
    return CRC()(buffer, length);
}

#endif // CRC_H
