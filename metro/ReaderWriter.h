#pragma once

#include <ppltasks.h>

// A reader/writer class that provides support for reading and writing
// files on disk. Provides synchronous and asynchronous methods.
ref class ReaderWriter
{
private:
   Windows::Storage::StorageFolder ^ m_location;
   Platform::String ^ m_locationPath;

internal:
   ReaderWriter();
   ReaderWriter(
      _In_ Windows::Storage::StorageFolder ^ folder
      );

   Platform::Array<byte> ^ ReadData(
      _In_ Platform::String ^ filename
      );

   concurrency::task<Platform::Array<byte> ^> ReadDataAsync(
      _In_ Platform::String ^ filename
      );

   uint32 WriteData(
      _In_ Platform::String ^ filename,
      _In_ const Platform::Array<byte> ^ fileData
      );

   concurrency::task<void> WriteDataAsync(
      _In_ Platform::String ^ filename,
      _In_ const Platform::Array<byte> ^ fileData
      );
};
