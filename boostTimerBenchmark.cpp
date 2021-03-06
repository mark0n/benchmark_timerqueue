#include <benchmark/benchmark.h>
#include <boost/asio.hpp>
#include <iostream>
#include <functional>

using namespace std::literals::chrono_literals;

void handler(const boost::system::error_code& ec, std::string msg)
{
  if (ec == boost::asio::error::operation_aborted) {
    std::cout << "timer has been aborted\n";
  } else {
    std::cout << "timer expired (message was " << msg << ")\n";
  }
}

class withActiveTimers : public ::benchmark::Fixture {
public:
  boost::asio::io_context ctx;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type> wg = boost::asio::make_work_guard(ctx); // ensure ctx.run() doesn't run out of work before we're done
  std::thread t;
  std::condition_variable cv;
  std::mutex cv_mtx;
  bool threadReady = false;
  std::vector<boost::asio::steady_timer> tv;

  void SetUp(const ::benchmark::State& state) {
    if (state.thread_index == 0) {
      t = std::thread([this]() {
        {
          std::lock_guard<std::mutex> lck(cv_mtx);
          threadReady = true;
          cv.notify_all();
        }
        ctx.run();
      });
      tv.reserve(state.range(0));
      for (int i = 0; i < state.range(0); i++) {
        tv.emplace_back(ctx, 60s);
        std::string msg = "handler " + std::to_string(i) + "\n";
        tv.back().async_wait([msg](const boost::system::error_code& ec) {
          handler(ec, msg);
        });
      }
      {
        std::unique_lock<std::mutex> lck(cv_mtx);
        cv.wait(lck, [this]{ return threadReady; });
      }
    }
  }
  
  void TearDown(const ::benchmark::State& state) {
    wg.reset();
    if (state.thread_index == 0) {
      t.join();
    }
  }
};

BENCHMARK_DEFINE_F(withActiveTimers, createAndStartTimer)(benchmark::State& state) {
  for (auto _ : state) {
    auto tmr = boost::asio::steady_timer(ctx, std::chrono::seconds(state.range(1)));
    std::string msg = "handler2\n";
    tmr.async_wait([msg](const boost::system::error_code& ec) {
      handler(ec, msg);
    });
//     tmr.cancel();
  }
}
BENCHMARK_REGISTER_F(withActiveTimers, createAndStartTimer)
  ->Args({0,      30})  // start timer while no other timers exist, new timer has smallest expiration time
  ->Args({30000,  30})  // start timer while 30,000 timers exist, new timer has smallest expiration time
  ->Args({0,     120})  // start timer while no other timers exist, new timer has largest expiration time
  ->Args({30000, 120}); // start timer while 30,000 timers exist, new timer has largest expiration time

BENCHMARK_DEFINE_F(withActiveTimers, createAndStartTimerMultiThreaded)(benchmark::State& state) {
  for (auto _ : state) {
    auto tmr = boost::asio::steady_timer(ctx, std::chrono::seconds(state.range(1)));
    std::string msg = "handler2\n";
    tmr.async_wait([msg](const boost::system::error_code& ec) {
      handler(ec, msg);
    });
  }
}
BENCHMARK_REGISTER_F(withActiveTimers, createAndStartTimerMultiThreaded)
  ->Args({0,      30}) // start timer while timer queue is empty, new timer has smallest expiration time
  ->Args({30000,  30}) // start timer while timer queue has 30,000 entries, new timer has smallest expiration time
  ->Args({0,     120}) // start timer while timer queue is empty, new timer has largest expiration time
  ->Args({30000, 120}) // start timer while timer queue has 30,000 entries, new timer has largest expiration time
  ->Threads(1)
  ->Threads(100);

BENCHMARK_DEFINE_F(withActiveTimers, cancelTimer)(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    auto tmr = boost::asio::steady_timer(ctx, std::chrono::seconds(state.range(1)));
    std::string msg = "handler2\n";
    tmr.async_wait([msg](const boost::system::error_code& ec) {
      handler(ec, msg);
    });
    state.ResumeTiming();
    tmr.cancel();
  }
}
BENCHMARK_REGISTER_F(withActiveTimers, cancelTimer)
  ->Args({0,      30}) // start/cancel timer while timer queue is empty, new timer has smallest expiration time
  ->Args({30000,  30}) // start/cancel timer while timer queue has 30,000 entries, new timer has smallest expiration time
  ->Args({0,     120}) // start/cancel timer while timer queue is empty, new timer has largest expiration time
  ->Args({30000, 120}) // start/cancel timer while timer queue has 30,000 entries, new timer has largest expiration time
  ->Threads(1)
  ->Threads(100);
BENCHMARK_MAIN();
