#include <benchmark/benchmark.h>
#include <vector>
#include <iostream>
#include <memory>
#include <thread>
#include <condition_variable>

#include <epicsTimer.h>

using namespace std;

class handler : public epicsTimerNotify
{
public:
  handler(const char* nameIn, epicsTimerQueueActive& queue) : name(nameIn), timer(queue.createTimer()) {}
  ~handler() { timer.destroy(); }
  void start(double delay) { timer.start(*this, delay); }
  void cancel() { timer.cancel(); }
  expireStatus expire(const epicsTime& currentTime) {
    cout << name << "\n";
    return expireStatus(restart, 0.1);
//     return expireStatus(noRestart);
  }
private:
  const string name;
  epicsTimer &timer;
};

static void withPassiveTimersCreateTimer(benchmark::State& state) {
  epicsTimerQueueActive& tq = epicsTimerQueueActive::allocate(false);
  {
    vector<handler> hv;
    hv.reserve(state.range(0));
    for (int i = 0; i < state.range(0); i++) {
      hv.emplace_back("arbitrary name", tq);
    }

    for (auto _ : state) {
      // create and immediately destroy a timer
      handler h = handler("arbitrary name", tq);
    }
  }
  tq.release();
}
BENCHMARK(withPassiveTimersCreateTimer)->Arg(0)->Arg(30000);

static void withActiveTimersCreateTimer(benchmark::State& state) {
  epicsTimerQueueActive& tq = epicsTimerQueueActive::allocate(false);
  {
    vector<handler> hv;
    hv.reserve(state.range(0));
    for (int i = 0; i < state.range(0); i++) {
      hv.emplace_back("arbitrary name", tq);
    }
    for (auto& h : hv) {
      h.start(60.0f);
    }

    for (auto _ : state) {
      // create and immediately destroy a timer
      handler h = handler("arbitrary name", tq);
    }
  }

  tq.release();
}
BENCHMARK(withActiveTimersCreateTimer)->Arg(0)->Arg(30000);

static void withActiveTimersCreateAndStartTimer(benchmark::State& state) {
  epicsTimerQueueActive& tq = epicsTimerQueueActive::allocate(false);
  {
    vector<handler> hv;
    hv.reserve(state.range(0));
    for (int i = 0; i < state.range(0); i++) {
      hv.emplace_back("arbitrary name", tq);
    }
    for (auto& h : hv) {
      h.start(60.0f);
    }

    for (auto _ : state) {
      handler h = handler("arbitrary name", tq);
      h.start(state.range(1));
    }
  }

  tq.release();
}
BENCHMARK(withActiveTimersCreateAndStartTimer)
  ->Args({0,      30})  // start timer while timer queue is empty, new timer has smallest expiration time
  ->Args({30000,  30})  // start timer while timer queue has 30,000 entries, new timer has smallest expiration time
  ->Args({0,     120})  // start timer while timer queue is empty, new timer has largest expiration time
  ->Args({30000, 120}); // start timer while timer queue has 30,000 entries, new timer has largest expiration time


static void withActiveTimersCreateAndStartTimerMultiThreaded(benchmark::State& state) {
  epicsTimerQueueActive& tq = epicsTimerQueueActive::allocate(false); // share timer queue between threads
  {
    vector<handler> hv;
    if (state.thread_index == 0) {
      hv.reserve(state.range(0));
      for (int i = 0; i < state.range(0); i++) {
        hv.emplace_back("arbitrary name", tq);
      }
      for (auto& h : hv) {
        h.start(60.0f);
      }
    }

    for (auto _ : state) {
      handler h = handler("arbitrary name", tq);
      h.start(state.range(1));
    }
  }

  tq.release();
}
BENCHMARK(withActiveTimersCreateAndStartTimerMultiThreaded)
  ->Args({0,      30}) // start timer while timer queue is empty, new timer has smallest expiration time
  ->Args({30000,  30}) // start timer while timer queue has 30,000 entries, new timer has smallest expiration time
  ->Args({0,     120}) // start timer while timer queue is empty, new timer has largest expiration time
  ->Args({30000, 120}) // start timer while timer queue has 30,000 entries, new timer has largest expiration time
  ->Threads(1)
  ->Threads(100);

static void withActiveTimersCancelTimer(benchmark::State& state) {
  epicsTimerQueueActive& tq = epicsTimerQueueActive::allocate(false);
  {
    vector<handler> hv;
    hv.reserve(state.range(0));
    for (int i = 0; i < state.range(0); i++) {
      hv.emplace_back("arbitrary name", tq);
    }
    for (auto& h : hv) {
      h.start(60.0f);
    }

    for (auto _ : state) {
      state.PauseTiming();
      handler h = handler("arbitrary name", tq);
      h.start(state.range(1));
      state.ResumeTiming();
      h.cancel();
    }
  }

  tq.release();
}
BENCHMARK(withActiveTimersCancelTimer)
  ->Args({0,      30})  // start timer while timer queue is empty, new timer has smallest expiration time
  ->Args({30000,  30})  // start timer while timer queue has 30,000 entries, new timer has smallest expiration time
  ->Args({0,     120})  // start timer while timer queue is empty, new timer has largest expiration time
  ->Args({30000, 120}); // start timer while timer queue has 30,000 entries, new timer has largest expiration time

// static void epicsThreadSleep(benchmark::State& state) {
// //   cout << "epicsThreadSleepQuantum: " << epicsThreadSleepQuantum() << "\n";
//   auto t = 1.23456789 / state.range(0);
//   for (auto _ : state) {
//     epicsThreadSleep(t);
//   }
// }
// BENCHMARK(epicsThreadSleep)->RangeMultiplier(10)->Range(10, 1000000);
// 
// static void stdThisThreadSleepFor(benchmark::State& state) {
//   auto t = 1.23456789s / state.range(0);
//   for (auto _ : state) {
//     std::this_thread::sleep_for(t);
//   }
// }
// BENCHMARK(stdThisThreadSleepFor)->RangeMultiplier(10)->Range(10, 1000000);

BENCHMARK_MAIN();
