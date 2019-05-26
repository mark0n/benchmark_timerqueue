// g++ -I /home/marko/work/EPICS/epics-base/include -I /home/marko/work/EPICS/epics-base/include/compiler/gcc/ -I /home/marko/work/EPICS/epics-base/include/os/Linux/ timerQueue.cpp -g -fno-omit-frame-pointer -L /home/marko/work/EPICS/epics-base/lib/linux-x86_64-debug/ -l Com -pthread

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

int main()
{
  epicsTimerQueueActive &tq = epicsTimerQueueActive::allocate(false);
  vector<thread> threads;
  vector<double> start_durations;
  mutex start_durations_m;
  vector<double> cancel_durations;
  mutex cancel_durations_m;
  mutex cv_m;
  condition_variable cv;
  bool run = false;
  const int number_of_threads = 100;
  for (int i = 0; i < number_of_threads; i++)
  {
    threads.push_back(thread([&tq, i, &cv, &cv_m, &run, &start_durations, &start_durations_m, &cancel_durations, &cancel_durations_m](){
      const int timers_per_thread = 300;
      vector<unique_ptr<handler>> hv;
      for (int j = 0; j < timers_per_thread; j++)
      {
        string name = "timer " + to_string(i) + "." + to_string(j);
        hv.push_back(make_unique<handler>(name.c_str(), tq));
      }
      {
        unique_lock<mutex> lk(cv_m);
        cv.wait(lk, [&]{ return run == true; });
      }
//       cerr << "Start\n";

      auto start = chrono::steady_clock::now();

      for (int j = 0; j < timers_per_thread; j++)
        hv[j]->start(1.0);

      auto middle = chrono::steady_clock::now();

      for (int j = 0; j < timers_per_thread; j++)
        hv[j]->cancel();

      auto end = chrono::steady_clock::now();

      auto start_duration = chrono::duration_cast<chrono::microseconds>(middle - start).count();
      auto cancel_duration = chrono::duration_cast<chrono::microseconds>(end - middle).count();
//       cerr << "Done (" << start_duration << " us, " << cancel_duration << " us)\n";
      {
        lock_guard<mutex> lg(start_durations_m);
        start_durations.push_back(start_duration);
      }
      {
        lock_guard<mutex> lg(cancel_durations_m);
        cancel_durations.push_back(cancel_duration);
      }
    }));
  }
  epicsThreadSleep(.1);
  cerr << "Starting threads\n";
  run = true;
  cv.notify_all();
  for (auto& t : threads)
  {
    t.join();
  }
  
  auto total_start_durations = 0;
  for(auto sd: start_durations)
  {
    total_start_durations += sd;
  }
  cerr << "Total start time: " << total_start_durations/1000 << " ms\n";
  
  auto total_cancel_durations = 0;
  for(auto cd: cancel_durations)
  {
    total_cancel_durations += cd;
  }
  cerr << "Total cancel time: " << total_cancel_durations/1000 << " ms\n";

  tq.release();
}