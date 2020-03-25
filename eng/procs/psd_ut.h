#pragma once
// fft unit test
#include "fft.h"
#include "psd.h"
#include "../unit_test.h"

SEL_UNIT_TEST(psd)

class ut_traits : public eng_traits<64, 16000> {};


using fft = sel::eng::proc::fft_t<ut_traits>;
using psd = sel::eng::proc::psd<ut_traits>;


sel::eng::Const input = std::vector<double>(
	{
		-4911.43080257359, -4648.75282177202, -2049.90893844119, -1941.83692490218,
		-1757.64936335238, -804.438841549709, -735.172838921937, -554.898542043838,
		-542.549799169880, -517.934644663490, -456.347218936102, -424.653394382480,
		-352.079161485175, -343.446646570997, -306.270240885125, -303.642673849313,
		-301.209207465972, -288.778228758572, -230.728361813264, -217.743527148850,
		-194.634938699500, -169.166329820594, -168.551445313991, -153.723394003659,
		-151.230944621563, -146.780147037939, -135.148833700298, -126.693676935488,
		-125.552098064491, -122.704445307867, -120.582494617052, -118.740309356804,
		-117.746385186952, -117.218568438222, -116.583578868798, -110.799421039188,
		-106.892373019838, -105.603222157213, -100.079692464756, -96.1833578542874,
		-94.7379934704439, -93.2463242585609, -93.0786685060479, -92.7785224714423,
		-90.8551296913823, -90.5138921710392, -88.6267757962204, -86.8779026580548,
		-84.2174719564973, -83.5656659635882, -80.0311450839270, -79.2560931323279,
		-75.9927469681787, -75.4177304654045, -75.2733826216122, -74.9360776766520,
		-74.4290610751853, -69.1942449581439, -68.9462387427999, -68.7165676256769,
		-67.8152204254259, -66.9516267299665, -64.2061183905124, -64.1172832722024
	});

std::array<samp_t, psd::OUTW> matlab_psd_result = { {

		670.242556474339,
		606.955224094522,
		414.143238234035,
		316.777058962669,
		269.172636613206,
		217.545351186600,
		175.605508714318,
		142.866297889710,
		118.272317096306,
		91.7651507693226,
		71.9702040950375,
		62.7175457315403,
		58.4586806394712,
		56.0675544907948,
		54.4606489756662,
		54.3031701605388,
		54.1446501171176,
		51.6917681765122,
		48.6242384364012,
		45.6801533489208,
		41.7820165009488,
		35.9047438574175,
		28.9503101701580,
		23.8045457551338,
		18.0039699195128,
		12.1133853667972,
		8.03918197854466,
		5.40360489311977,
		4.22605636029506,
		4.68509114740154,
		5.45393920773927,
		5.54702262384500,
		2.75378181037514
	} };


void run() {

	fft fft;
	psd psd;
	
	fft.ConnectFrom(input);
	psd.ConnectFrom(fft);

	fft.freeze();
	psd.freeze();

	fft.process();
	psd.process();

	samp_t *my_psd_result = psd.out;

	// compare matlab psd
	
	for (size_t i = 0; i < psd::OUTW; ++i) {
		samp_t e = abs(my_psd_result[i] - matlab_psd_result[i]);
		//if (e >= 1e-10)
		SEL_UNIT_TEST_ASSERT(e < 1e-10);

	}

}

SEL_UNIT_TEST_END
