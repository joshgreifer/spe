//
// Created by josh on 26/10/2020.
//
#include <iostream>
#include <opencv2/core.hpp>

#include <opencv2/dnn/dnn.hpp>
#include "../eng/melspec_impl.h"

using MelSpec = melspec_impl<float, 16000, 80, 512>;


int main()
{
    try {
        std::string onnx_filename = "netG.onnx";
        const int num_layers = 24; // TODO: Obtain this from net
        MelSpec::Ptr melspec;
        cv::dnn::Net net = cv::dnn::readNetFromONNX(onnx_filename);


//        std::cout << "[\n";
//    for (auto layer_id  = 0; layer_id < num_layers; ++layer_id) {
//        auto layer = net.getLayer(layer_id);
//        std::cout << '\t' << layer->name << ":\t";
//        std::cout << layer->type << '\n';
//    }
//    std::cout << "]\n";
        auto x_vec = sel::numpy::load<float>(std::string(onnx_filename + ".x.npy").c_str());
        auto x_hat_vec = sel::numpy::load<float>(std::string(onnx_filename + ".x_hat.npy").c_str());
        std::vector<int> input_shape = {static_cast<int>(x_vec.size() / 80 / 80), 80, 80};

        cv::Mat x_cvmat(input_shape, CV_32FC1, x_vec.data());
        cv::Mat x_hat_pytorch_cvmat(input_shape, CV_32FC1, x_hat_vec.data());


        auto x = cv::dnn::blobFromImages(x_cvmat);
        std::cout << "*** Running inference with batch size " <<  x_vec.size() / 80 / 80 << "...";
        net.setInput(x);
        auto x_hat_cvmat = net.forward();

        std::cout << " done. Output size:" <<  x_hat_cvmat.size << '\n';

        // compare
        bool eq = std::equal(x_hat_cvmat.begin<float>(), x_hat_cvmat.end<float>(), x_hat_pytorch_cvmat.begin<float>());
        const float *x_cv_data = reinterpret_cast<float *>(x_hat_cvmat.data);
        const float *x_py_data = reinterpret_cast<float *>(x_hat_pytorch_cvmat.data);
        size_t n_diffs = 0;
        for (size_t i = 0; i < x_vec.size(); ++i) {
            const auto x_py = x_cv_data[i];
            const auto x_cv = x_py_data[i];
            if (abs(x_py - x_cv) > 1e-5) {
                ++n_diffs;
                std::cerr << i << ": " << x_cv << ", " << x_py << '\n';
            }
        }
        if (n_diffs == 0)
            std::cout << "*** No differences.\n";
        else
            std::cout << "*** " << n_diffs  << " difference" << (n_diffs == 1 ? ".\n" : "s.\n");
    } catch (std::exception &ex) {
        std::cerr << ex.what();
    }
}


