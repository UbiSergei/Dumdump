// Copyright � 1996-2017, Valve Corporation, All rights reserved.

#ifndef SOURCE_TIER0_FASTTIMER_H_
#define SOURCE_TIER0_FASTTIMER_H_

#if _WIN32
#include <intrin.h>
#endif

#include <cassert>
#include "tier0/platform.h"

static int64_t g_ClockSpeed;
static unsigned long g_dwClockSpeed;

static double g_ClockSpeedMicrosecondsMultiplier;
static double g_ClockSpeedMillisecondsMultiplier;
static double g_ClockSpeedSecondsMultiplier;

class PLATFORM_CLASS CCycleCount {
  friend class CFastTimer;

 public:
  CCycleCount();
  CCycleCount(uint64_t cycles);

  // Sample the clock.
  void Sample();

  // Set to zero.
  void Init();
  void Init(float initTimeMsec);
  void Init(double initTimeMsec) { Init((float)initTimeMsec); }
  void Init(uint64_t cycles);

  // Compare two counts.
  bool IsLessThan(CCycleCount const &other) const;

  // Convert to other time representations. These functions are slow, so it's
  // preferable to call them during display rather than inside a timing block.
  unsigned long GetCycles() const;
  uint64_t GetLongCycles() const;

  unsigned long GetMicroseconds() const;
  uint64_t GetUlMicroseconds() const;
  double GetMicrosecondsF() const;
  void SetMicroseconds(unsigned long nMicroseconds);

  unsigned long GetMilliseconds() const;
  double GetMillisecondsF() const;
  double GetSeconds() const;

  CCycleCount &operator+=(CCycleCount const &other);

  // dest = rSrc1 + rSrc2
  static void Add(CCycleCount const &rSrc1, CCycleCount const &rSrc2,
                  CCycleCount &dest);  // Add two samples together.

  // dest = rSrc1 - rSrc2
  static void Sub(CCycleCount const &rSrc1, CCycleCount const &rSrc2,
                  CCycleCount &dest);  // Add two samples together.

  static uint64_t GetTimestamp();

  uint64_t m_Int64;
};

class CClockSpeedInit {
 public:
  CClockSpeedInit() { Init(); }

  static void Init() {
    const CPUInformation &pi = GetCPUInformation();

    g_ClockSpeed = pi.m_Speed;
    g_dwClockSpeed = (unsigned long)g_ClockSpeed;

    g_ClockSpeedMicrosecondsMultiplier = 1000000.0 / (double)g_ClockSpeed;
    g_ClockSpeedMillisecondsMultiplier = 1000.0 / (double)g_ClockSpeed;
    g_ClockSpeedSecondsMultiplier = 1.0f / (double)g_ClockSpeed;
  }
};

class PLATFORM_CLASS CFastTimer {
 public:
  // These functions are fast to call and should be called from your sampling
  // code.
  void Start();
  void End();

  const CCycleCount &GetDuration()
      const;  // Get the elapsed time between Start and End calls.
  CCycleCount GetDurationInProgress()
      const;  // Call without ending. Not that cheap.

  // Return number of cycles per second on this processor.
  static inline unsigned long GetClockSpeed();

 private:
  CCycleCount m_Duration;
#ifdef DEBUG_FASTTIMER
  bool m_bRunning;  // Are we currently running?
#endif
};

// This is a helper class that times whatever block of code it's in
class CTimeScope {
 public:
  CTimeScope(CFastTimer *pTimer);
  ~CTimeScope();

 private:
  CFastTimer *m_pTimer;
};

inline CTimeScope::CTimeScope(CFastTimer *pTotal) {
  m_pTimer = pTotal;
  m_pTimer->Start();
}

inline CTimeScope::~CTimeScope() { m_pTimer->End(); }

// This is a helper class that times whatever block of code it's in and
// adds the total (int microseconds) to a global counter.
class CTimeAdder {
 public:
  CTimeAdder(CCycleCount *pTotal);
  ~CTimeAdder();

  void End();

 private:
  CCycleCount *m_pTotal;
  CFastTimer m_Timer;
};

inline CTimeAdder::CTimeAdder(CCycleCount *pTotal) {
  m_pTotal = pTotal;
  m_Timer.Start();
}

inline CTimeAdder::~CTimeAdder() { End(); }

inline void CTimeAdder::End() {
  if (m_pTotal) {
    m_Timer.End();
    *m_pTotal += m_Timer.GetDuration();
    m_pTotal = 0;
  }
}

// Simple tool to support timing a block of code, and reporting the results on
// program exit or at each iteration
// Macros used because dbg.h uses this header, thus Msg() is unavailable
#define PROFILE_SCOPE(name)                                                 \
  class C##name##ACC : public CAverageCycleCounter {                        \
   public:                                                                  \
    ~C##name##ACC() {                                                       \
      Msg("%-48s: %6.3f avg (%8.1f total, %7.3f peak, %5d iters)\n", #name, \
          GetAverageMilliseconds(), GetTotalMilliseconds(),                 \
          GetPeakMilliseconds(), GetIters());                               \
    }                                                                       \
  };                                                                        \
  static C##name##ACC name##_ACC;                                           \
  CAverageTimeMarker name##_ATM(&name##_ACC)

#define TIME_SCOPE(name)                                                     \
  class CTimeScopeMsg_##name {                                               \
   public:                                                                   \
    CTimeScopeMsg_##name() { m_Timer.Start(); }                              \
    ~CTimeScopeMsg_##name() {                                                \
      m_Timer.End();                                                         \
      Msg(#name "time: %.4fms\n", m_Timer.GetDuration().GetMillisecondsF()); \
    }                                                                        \
                                                                             \
   private:                                                                  \
    CFastTimer m_Timer;                                                      \
  } name##_TSM;

class CAverageCycleCounter {
 public:
  CAverageCycleCounter();

  void Init();
  void MarkIter(const CCycleCount &duration);

  unsigned GetIters() const;

  double GetAverageMilliseconds() const;
  double GetTotalMilliseconds() const;
  double GetPeakMilliseconds() const;

 private:
  CCycleCount m_Total;
  CCycleCount m_Peak;
  uint32_t m_nIters;
  bool m_fReport;
  const char *m_pszName;
};

class CAverageTimeMarker {
 public:
  CAverageTimeMarker(CAverageCycleCounter *pCounter);
  ~CAverageTimeMarker();

 private:
  CAverageCycleCounter *m_pCounter;
  CFastTimer m_Timer;
};

inline CCycleCount::CCycleCount() { Init(0ui64); }

inline CCycleCount::CCycleCount(uint64_t cycles) { Init(cycles); }

inline void CCycleCount::Init() { Init(0ui64); }

inline void CCycleCount::Init(float initTimeMsec) {
  if (g_ClockSpeedMillisecondsMultiplier > 0.0)
    Init((uint64_t)(initTimeMsec / g_ClockSpeedMillisecondsMultiplier));
  else
    Init(0ui64);
}

inline void CCycleCount::Init(uint64_t cycles) { m_Int64 = cycles; }

inline void CCycleCount::Sample() {
#if defined(_WIN32)
  m_Int64 = __rdtsc();
#elif defined(_LINUX)
  unsigned long *pSample = (unsigned long *)&m_Int64;
  __asm__ __volatile__(
      "rdtsc\n\t"
      "movl %%eax,  (%0)\n\t"
      "movl %%edx, 4(%0)\n\t"
      : /* no output regs */
      : "D"(pSample)
      : "%eax", "%edx");
#endif
}

inline CCycleCount &CCycleCount::operator+=(CCycleCount const &other) {
  m_Int64 += other.m_Int64;
  return *this;
}

inline void CCycleCount::Add(CCycleCount const &rSrc1, CCycleCount const &rSrc2,
                             CCycleCount &dest) {
  dest.m_Int64 = rSrc1.m_Int64 + rSrc2.m_Int64;
}

inline void CCycleCount::Sub(CCycleCount const &rSrc1, CCycleCount const &rSrc2,
                             CCycleCount &dest) {
  dest.m_Int64 = rSrc1.m_Int64 - rSrc2.m_Int64;
}

inline uint64_t CCycleCount::GetTimestamp() {
  CCycleCount c;
  c.Sample();
  return c.GetLongCycles();
}

inline bool CCycleCount::IsLessThan(CCycleCount const &other) const {
  return m_Int64 < other.m_Int64;
}

inline unsigned long CCycleCount::GetCycles() const {
  return (unsigned long)m_Int64;
}

inline uint64_t CCycleCount::GetLongCycles() const { return m_Int64; }

inline unsigned long CCycleCount::GetMicroseconds() const {
  return (unsigned long)(m_Int64 * 1000000 / g_ClockSpeed);
}

inline uint64_t CCycleCount::GetUlMicroseconds() const {
  return m_Int64 * 1000000 / g_ClockSpeed;
}

inline double CCycleCount::GetMicrosecondsF() const {
  return (double)(m_Int64 * g_ClockSpeedMicrosecondsMultiplier);
}

inline void CCycleCount::SetMicroseconds(unsigned long nMicroseconds) {
  m_Int64 = ((uint64_t)nMicroseconds * g_ClockSpeed) / 1000000;
}

inline unsigned long CCycleCount::GetMilliseconds() const {
  return (unsigned long)(m_Int64 * 1000 / g_ClockSpeed);
}

inline double CCycleCount::GetMillisecondsF() const {
  return (double)(m_Int64 * g_ClockSpeedMillisecondsMultiplier);
}

inline double CCycleCount::GetSeconds() const {
  return (double)(m_Int64 * g_ClockSpeedSecondsMultiplier);
}

inline void CFastTimer::Start() {
  m_Duration.Sample();
#ifdef DEBUG_FASTTIMER
  m_bRunning = true;
#endif
}

inline void CFastTimer::End() {
  CCycleCount cnt;
  cnt.Sample();

  m_Duration.m_Int64 = cnt.m_Int64 - m_Duration.m_Int64;

#ifdef DEBUG_FASTTIMER
  m_bRunning = false;
#endif
}

inline CCycleCount CFastTimer::GetDurationInProgress() const {
  CCycleCount cnt;
  cnt.Sample();

  CCycleCount result;
  result.m_Int64 = cnt.m_Int64 - m_Duration.m_Int64;

  return result;
}

inline unsigned long CFastTimer::GetClockSpeed() { return g_dwClockSpeed; }

inline CCycleCount const &CFastTimer::GetDuration() const {
#ifdef DEBUG_FASTTIMER
  assert(!m_bRunning);
#endif
  return m_Duration;
}

inline CAverageCycleCounter::CAverageCycleCounter()
    : m_nIters(0), m_fReport(false), m_pszName(nullptr) {}

inline void CAverageCycleCounter::Init() {
  m_Total.Init();
  m_Peak.Init();
  m_nIters = 0;
}

inline void CAverageCycleCounter::MarkIter(const CCycleCount &duration) {
  ++m_nIters;
  m_Total += duration;
  if (m_Peak.IsLessThan(duration)) m_Peak = duration;
}

inline unsigned CAverageCycleCounter::GetIters() const { return m_nIters; }

inline double CAverageCycleCounter::GetAverageMilliseconds() const {
  if (m_nIters) return (m_Total.GetMillisecondsF() / (double)m_nIters);

  return 0.0;
}

inline double CAverageCycleCounter::GetTotalMilliseconds() const {
  return m_Total.GetMillisecondsF();
}

inline double CAverageCycleCounter::GetPeakMilliseconds() const {
  return m_Peak.GetMillisecondsF();
}

inline CAverageTimeMarker::CAverageTimeMarker(CAverageCycleCounter *pCounter) {
  m_pCounter = pCounter;
  m_Timer.Start();
}

inline CAverageTimeMarker::~CAverageTimeMarker() {
  m_Timer.End();
  m_pCounter->MarkIter(m_Timer.GetDuration());
}

// Use this to time whether a desired interval of time has passed.  It's
// extremely fast to check while running.
class CLimitTimer {
 public:
  void SetLimit(uint64_t m_cMicroSecDuration);
  bool BLimitReached();

 private:
  uint64_t m_lCycleLimit;
};

// Purpose: Initializes the limit timer with a period of time to measure.
// Input  : cMicroSecDuration -		How long a time period to measure
inline void CLimitTimer::SetLimit(uint64_t m_cMicroSecDuration) {
  uint64_t dlCycles = m_cMicroSecDuration * g_dwClockSpeed / 1000000;
  CCycleCount cycleCount;
  cycleCount.Sample();
  m_lCycleLimit = cycleCount.GetLongCycles() + dlCycles;
}

// Purpose: Determines whether our specified time period has passed
// Output:	true if at least the specified time period has passed
inline bool CLimitTimer::BLimitReached() {
  CCycleCount cycleCount;
  cycleCount.Sample();
  return cycleCount.GetLongCycles() >= m_lCycleLimit;
}

#endif  // SOURCE_TIER0_FASTTIMER_H_
