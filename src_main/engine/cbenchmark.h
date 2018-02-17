// Copyright � 1996-2018, Valve Corporation, All rights reserved.

#ifndef CBENCHMARK_H
#define CBENCHMARK_H

class CCommand;

#define MAX_BUFFER_SIZE 2048

// Purpose: Holds benchmark data & state
class CBenchmarkResults {
 public:
  CBenchmarkResults();

  bool IsBenchmarkRunning();
  void StartBenchmark(const CCommand &args);
  void StopBenchmark();
  void SetResultsFilename(const char *pFilename);
  void Upload();

 private:
  bool m_bIsTestRunning;
  char m_szFilename[256];

  float m_flStartTime;
  int m_iStartFrame;
};

inline CBenchmarkResults *GetBenchResultsMgr() {
  extern CBenchmarkResults g_BenchmarkResults;
  return &g_BenchmarkResults;
}

#endif  // CBENCHMARK_H
