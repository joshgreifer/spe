//
// Created by josh on 26/10/2020.
//
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

        std::vector<int> input_shape = {80, 80};

        std::cout << "[\n";
//    for (auto layer_id  = 0; layer_id < num_layers; ++layer_id) {
//        auto layer = net.getLayer(layer_id);
//        std::cout << '\t' << layer->name << ":\t";
//        std::cout << layer->type << '\n';
//    }
//    std::cout << "]\n";
        cv::Mat input_data(input_shape, CV_32FC1);

//    for (auto& v: this->inports[0]->as_vector())
//        fv.push_back(static_cast<float>(v));
        auto blob = cv::dnn::blobFromImage(input_data);
        net.setInput(blob);
        auto mat = net.forward();
        auto mat2 = mat;
        std::cout << "Output size:" <<  mat.size;
    } catch (std::exception &ex) {
        std::cerr << ex.what();
    }
}


