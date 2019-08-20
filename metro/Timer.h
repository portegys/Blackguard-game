#pragma once

// A simple timer class used to drive the render loop.
// Uses the QueryPerformanceCounter function to retrieve
// high-resolution timing information.
ref class Timer
{
private:
   LARGE_INTEGER m_frequency;
   LARGE_INTEGER m_currentTime;
   LARGE_INTEGER m_startTime;
   LARGE_INTEGER m_lastTime;
   float         m_total;
   float         m_delta;

internal:
   Timer();
   void Reset();
   void Update();

   property float Total
   {
      float get();
   }
   property float Delta
   {
      float get();
   }
};
