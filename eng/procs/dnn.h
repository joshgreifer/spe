#pragma once
#include <opencv2/core.hpp>

#include <opencv2/dnn/dnn.hpp>
#include "../processor.h"

namespace sel
{
	namespace eng
	{
		namespace proc 
		{
			template<size_t InW, size_t OutW>class dnn : public Processor1A1B<InW, OutW>
			{
				cv::dnn::Net trained_net_;
				const std::string output_layer_name_;
				
			public:
				void process() final
				{
					// convert to vector of floats for cv::dnn::Net
					vector<float_t> fv;
					for (auto& v: this->inports[0]->as_vector())
						fv.push_back(static_cast<float>(v));
					
					const auto blob = cv::dnn::blobFromImage(fv);
					
					trained_net_.setInput(blob);
					auto mat = trained_net_.forward(output_layer_name_);
					
					for (size_t i = 0; i < OutW; ++i) 
						this->out[i] = mat.at<float>((int)i);
				}
				// default constuctor needed for factory creation
				explicit dnn() = default;

				explicit dnn(const std::string& onnx_filename, const char*output_layer_name = "softmax") :
				trained_net_(cv::dnn::readNetFromONNX(onnx_filename)),
				output_layer_name_(output_layer_name)
				{
				}

				explicit dnn(params& args) : dnn( args.get<std::string>("filename"), args.get<const char*>("output_layer", "softmax"))
				{
					
				}

			};
		}
	}
}

#if defined(COMPILE_UNIT_TESTS)
#include "../unit_test.h"

SEL_UNIT_TEST(dnn)


struct ut_traits
{
	static constexpr size_t num_input_features = 10;
	static constexpr size_t num_output_features = 2;
};

std::array<samp_t, ut_traits::num_input_features> input_a = {{
	
	0.607155466949657,
	0.056974873123022,
	0.415280593486775,
	0.473014170033608,
	0.282925172645024,
	0.271188460619838,
	0.583191063128763,
	0.535374600383481,
	0.359134236659897,
	0.548199609490952

	} };

// Matlab results are actually floats
std::array<double, ut_traits::num_output_features> matlab_results = {
	{
		0.9814663,
		0.0185338
	}};

static bool file_exists(const char *filename)
{
	struct stat buffer;
	return stat(filename, &buffer) == 0;
}

void run()
{
	const std::string onnx_filename = "./dnn_ut.onnx";

	SEL_UNIT_TEST_ASSERT(file_exists(onnx_filename.c_str()));

	cv::dnn::Net net(cv::dnn::readNetFromONNX(onnx_filename));
	std::cout <<"[\n";
	for (auto& layer_name : net.getLayerNames())
		std::cout << '\t' << layer_name << '\n';
	std::cout <<"]\n";


	sel::eng::proc::dnn<ut_traits::num_input_features, ut_traits::num_output_features> dnn(onnx_filename);

	sel::eng::Const input = input_a;
	dnn.ConnectFrom(input);
	dnn.freeze();

	dnn.process();
	const samp_t* matlab_result = matlab_results.data();
	for (auto& v : dnn.oport)
	{
		SEL_UNIT_TEST_ASSERT_ALMOST_EQUAL(v, *matlab_result++)
	}
}
SEL_UNIT_TEST_END
#endif
