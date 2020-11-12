
#ifdef _MSC_VER
#define _WIN32_WINNT 0x0601
#include <conio.h>
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif
#endif
#include  "../eng6/eng6.h"
#include "eda_device.h"
namespace stdproc = sel::eng6::proc;
namespace eng = sel::eng6;

struct console_writer : eng::Sink<1> {
    void process() final {
        printf("%4.4x ", static_cast<short>(this->in[0]));
        std::cout << std::flush;
    }
};

int main() {
    try {
        eng::websocket_stream output_stream {};
        stdproc::number_stream_writer<short, 1, 16>output_writer { &output_stream };
        console_writer log {};
        eda_device device {};

        eng::scheduler& s = eng::scheduler::get();

        stdproc::processor_graph graph {s};
        graph.connect(device, output_writer);
//        graph.connect(device, log);

        output_stream.connect("ws://0.0.0.0:1102", []() { std::cout << "Websocket client connected.\n"; });

        s.do_measure_performance_at_start = true;
        auto t = s.start();

        printf("Press a key to stop measure\n");
        getch();
        s.stop();
        t.join();
        printf("Device Rate: %4.4f\n", device.rate().actual());
    } catch ( std::exception& ex) {
        std::cerr << ex.what();
    }
    return 0;
}
