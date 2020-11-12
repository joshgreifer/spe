//
// Created by josh on 26/10/2020.
//
#include <iostream>
#include <array>
#include <opencv2/core.hpp>

#include <opencv2/dnn/dnn.hpp>
#include "../eng6/melspec_impl.h"

#define USE_OPENCV_DNN

template<class T>T&& heap_alloc() {
    return std::move(*std::make_unique<T>());
}

struct pix2pix {
    static constexpr size_t sr = 16000;
    static constexpr size_t n_fft = 512;
    static constexpr size_t n_mel = 80;
    static constexpr size_t n_frames = 80;

    using data_t = float;
    using cdata_t = std::complex<data_t>;

    static_assert(std::is_floating_point<data_t>::value , "data_t must be float");
    static_assert(sizeof(data_t) == 4, "data_t size must be 4 bytes");

    using MelSpec = melspec_impl<data_t, sr, n_mel, n_fft>;

    using mel_t = std::vector<data_t>;
    using fft_t = std::vector<cdata_t>;
    using mag_t = std::vector<data_t>;

    using input_t = std::vector<data_t> ;
    using output_t = input_t;

    std::array<fft_t, n_frames> in_buffer;
    std::array<fft_t, n_frames> out_buffer;

    input_t pix2pix_input;
    output_t pix2pix_output;

    MelSpec&& melspec;
    const int batch_size;

    pix2pix(const int batch_size) :
    batch_size(batch_size),
    melspec(heap_alloc<MelSpec>())
    {

    }
#ifdef USE_OPENCV_DNN
    std::string onnx_filename = "netG.onnx";
    cv::dnn::Net net = cv::dnn::readNetFromONNX(onnx_filename);
#endif
    struct global_scale {
        constexpr static  data_t max = 3.0f;
        constexpr static  data_t min = -13.0f;
        constexpr static  data_t range = max - min;

        static data_t down( const data_t x ) { return 2 * (x - min) / range - 1; }
        static data_t up( const data_t x ) { return ((x + 1) / 2) * range + min; }
    };

    // Implement Neurence spectrum scaling
    fft_t&  spectrum_scale(const input_t& mels_in, input_t& mels_out, const fft_t& fft_in)
    {
//        ratio_mask = mels_out / mels_in  # 80,time
//        ratio_weights = np.sum(np.expand_dims(mel_weights, -1) * np.expand_dims(ratio_mask, 1), 0) * 37. / 2
//        specs_out = specs_in * ratio_weights


    }

    fft_t& run1fft(fft_t& input_fft)
    {
        assert(this->batch_size == 1);

        static size_t frame_count;


        in_buffer[frame_count++] = input_fft;
        if (frame_count == n_frames) {
            frame_count = 0;

            // process fft buffer: fft -> mag -> log -> scale down -> forward net -> scale up -> exp ->
            size_t offset = 0;  // bump this by mel.size() in for loop
            for (auto& fft : in_buffer ) {
                mag_t mag;
                mel_t mel;

                // get the magnitude spec
                for (auto i = 0; i < fft.size(); ++i) mag[i] = abs(fft[i]);


                // melspec
                melspec.fft_mag2mel(mag.data(), mel.data());



                // add to pix2pix input buffer
                std::copy(mel.begin(), mel.end(), &pix2pix_input[offset]);
                // bump offset
                offset += mel.size();
            }
            // TODO: Need to stash unscaled values for ratio mask

            auto input_scaled = pix2pix_input;

            // log the melspec, and prevent zero values in log,  then scale down
            for (auto &v : input_scaled) v = global_scale::down(log(std::max(v, 1e-5f)));

            // now pix2pix_input is ready for processing, run inference
            pix2pix_output = forward(input_scaled);

            // invert  the scaling, then invert the log to restore magnitude spec
            for (auto &v : pix2pix_output) v = exp(global_scale::up(v));

            //

        }
        return out_buffer[frame_count];

    }

#ifdef USE_OPENCV_DNN

    void dump_layers() {
//    const int num_layers = 24; // TODO: Obtain this from net
//    std::cout << "[\n";
//    for (auto layer_id  = 0; layer_id < num_layers; ++layer_id) {
//        auto layer = net.getLayer(layer_id);
//        std::cout << '\t' << layer->name << ":\t";
//        std::cout << layer->type << '\n';
//    }
//    std::cout << "]\n";
    }

   output_t forward(input_t& input, bool residual_net = true) {
        output_t output;

        cv::Mat input_mat({ batch_size, n_frames, n_mel }, CV_32FC1, input.data());
        auto blob = cv::dnn::blobFromImages(input_mat);
        std::cout << "*** Running inference with batch size " <<  batch_size << "...";
        net.setInput(blob);
        auto output_mat = net.forward();

        std::copy(output_mat.begin<data_t>(), output_mat.end<data_t>(), std::back_inserter(output));

        assert((output.size() == input.size()) &&  (output.size() == batch_size * n_frames * n_mel));

        if (residual_net)
            for (size_t i = 0; i < output.size(); ++i)
                output[i] += input[i];

        return output;


    }
#endif
};

int main()
{
    try {

        std::string onnx_filename = "netG.onnx";

        auto input_vec = sel::numpy::load<float>(std::string(onnx_filename + ".x.npy").c_str());
        auto output_vec_pytorch = sel::numpy::load<float>(std::string(onnx_filename + ".x_hat.npy").c_str());

        std::cout << std::string(onnx_filename + ".x.npy") << ": Batch size: " << input_vec.size() / 80 / 80 << '\n';

        auto n_mels_in_file = input_vec.size() / 80 / 80;
        pix2pix p2p(n_mels_in_file);

        auto output_vec = p2p.forward(input_vec, false);
        std::cout << " done.";

        // compare
        // bool eq = std::equal(x_hat_cvmat.begin<float>(), x_hat_cvmat.end<float>(), x_hat_pytorch_cvmat.begin<float>());

        size_t n_diffs = 0;
        for (size_t i = 0; i < output_vec.size(); ++i) {
            const auto x_py = output_vec_pytorch[i];
            const auto x_cv = output_vec[i];
            if (abs(x_py - x_cv) > 1e-5) {
                ++n_diffs;
                if (n_diffs < 10)
                    std::cerr << i << ": " << x_cv << ", " << x_py << '\n';
                else if (n_diffs == 100)
                    std::cerr << "(Too many differences to display)";

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


