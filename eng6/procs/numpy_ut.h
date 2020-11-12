//
// Created by josh on 27/10/2020.
//

#ifndef SPE_NUMPY_UT_H
#define SPE_NUMPY_UT_H
#if defined(COMPILE_UNIT_TESTS)
#pragma once
#include "../unit_test.h"
#include "../numpy.h"
#include "../scheduler.h"
#include "compound_processor.h"
#include "numpy_file_reader.h"
#include "numpy_file_writer.h"

SEL_UNIT_TEST(numpy)

        const std::string test_file = "test.npy";
        using npy_writer = sel::eng6::proc::numpy_file_writer<float, 20>;
        using npy_reader = sel::eng6::proc::numpy_file_reader<float, 20>;
        void run() {

        std::vector<int>shape = { 20, 30 };
        const auto sz = std::accumulate(shape.begin(), shape.end(),  1, std::multiplies<int>());
        std::vector<float> data1;
        std::vector<float> data2;
        std::vector<float> data3;
        for (auto i = 0; i < sz; ++i)
            data1.push_back(i);

        SEL_UNIT_TEST_ITEM("numpy::save / numpy::load");
        sel::numpy::save(data1.data(), test_file.c_str(), shape);

        data2 = sel::numpy::load<float>(test_file.c_str());
        SEL_UNIT_TEST_ASSERT(data1 == data2);

        SEL_UNIT_TEST_ITEM("reader and writer processors");
        auto &s = sel::eng6::scheduler::get();
        s.clear();
        sel::eng6::proc::processor_graph graph;

        auto reader1 = npy_reader(test_file);
        auto writer1 = npy_writer(test_file);

        graph.connect(reader1, writer1);
        s.run();
        data3 = sel::numpy::load<float>(test_file.c_str());

        SEL_UNIT_TEST_ASSERT(data1 == data2);



        }
SEL_UNIT_TEST_END


#endif
#endif //SPE_NUMPY_UT_H
