/**
 * @file
 *
 * @author CCHyper
 * @author OmniBlade
 *
 * @brief Class for reading data from a memory buffer using FileClass IO interface.
 *
 * @copyright Chronoshift is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#ifndef RAMFILE_H
#define RAMFILE_H

#include "always.h"
#include "basefile.h"

class RAMFileClass : public FileClass
{
public:
    RAMFileClass();
    RAMFileClass(void *data, int size);
    virtual ~RAMFileClass();

    virtual const char *File_Name() const override;
    virtual const char *Set_Name(const char *filename) override;
    virtual bool Create() override;
    virtual bool Delete() override;
    virtual bool Is_Available(bool forced = false) override;
    virtual bool Is_Open() const override { return m_IsOpen; }
    virtual bool Open(const char *filename, int rights = FM_READ) override;
    virtual bool Open(int rights = FM_READ) override;
    virtual int Read(void *buffer, int length) override;
    virtual off_t Seek(off_t offset, int whence = FS_SEEK_CURRENT) override;
    virtual off_t Size() override;
    virtual int Write(const void *buffer, int size) override;
    virtual void Close() override { m_IsOpen = false; };
    virtual time_t Get_Date_Time() override;
    virtual bool Set_Date_Time(time_t date_time) override;
    virtual void Error(int error, bool can_retry = false, const char *filename = nullptr) override;

private:
    char *m_Buffer;
    int m_BufferSize;
    int m_BufferAvailable;
    int m_BufferOffset;
    int m_BufferRights;
    bool m_IsOpen;
    bool m_IsAllocated;
};

#endif // RAMFILE_H
