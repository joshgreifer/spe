#pragma once
// Input: biased autocorrelation with MAXLAG==LPC_ORDER
// Output: 0 .. LPC_ORDER : lpc coeffs
//		   LPC_ORDER+1 .. 2*LPC_ORDER : reflection coeffs
template<size_t ORDER>class sp_lpc : public  RegisterableSigProc<sp_lpc<ORDER>, ORDER, ORDER + ORDER + 1>
{
	static constexpr size_t N_LPC_COEFFS = ORDER + 1;
	static constexpr size_t N_LPC_REFLECTION_COEFFS = ORDER;

//	friend class unit_test_lpc;

	static constexpr bool USE_LEVINSON_DURBIN = true;
public:
	void process(void)
	{
		// input - normalized autocorrelation
		const SIGNAL_T *R = in_as_array();


		double e;
		if (USE_LEVINSON_DURBIN)
			e = levinson_durbin2(R, out, out + N_LPC_COEFFS);
		else {
			e = memcof2(R, out);

		}


	}

	void init(Schedule *context)
	{
		in_as_array(); // throws exception if inputs aren't an array
	}

	// default constuctor needed for factory creation
	explicit sp_lpc() {}

	sp_lpc(params& args)
	{

	}

	virtual ~sp_lpc()
	{
	}
private:
	static double levinson_durbin(const double *r, double *a, double *k) {
		const size_t N = LPC_ORDER;
		double a_temp[N + 1];       /* n <= N = constant  */
		double k_local_stack_alloc[N + 1];            // used if we havent provided reflection coeff array


		if (!k)
			k = k_local_stack_alloc;

		k[0] = 0.0;                          /* unused */

		a[0] = 1.0;
		a_temp[0] = 1.0;                  /* unnecessary but consistent */


		double alpha = r[0];



		for (size_t i = 1; i <= N; ++i) {

			double epsilon = 0.0;

			for (size_t j = 0; j < i; j++) {
				epsilon += a[j] * r[i - j];
			}

			a[i] = k[i] = -epsilon / alpha;

			const double current_k = k[i];
			alpha *= (1.0 - current_k * current_k);

			/* update a[ ] array into temporary array */
			for (size_t j = i - 1; j > 0; j--) {
				a_temp[j] = a[j] + current_k * a[i - j];
			}
			/* update a[ ] array */
			for (size_t j = i - 1; j > 0; j--) {
				a[j] = a_temp[j];
			}
		}
		a[N] = k[N];


		return alpha;
	}

	static double  memcof2(const double *r, double *a) {

		static constexpr size_t N = ORDER;
		static constexpr size_t M = ORDER + 1;

		double p = 0.0;
		double wk1[N], wk2[N], wkm[M];


		for (size_t j = 0; j < N; j++)
			p += SQR(r[j]);

		double xms = p / N;

		wk1[0] = r[0];
		wk2[N - 2] = r[N - 1];

		for (size_t j = 1; j < N - 1; j++) {
			wk1[j] = r[j];
			wk2[j - 1] = r[j];
		}
		/// TEST TEST
		// reverse R

		wk1[0] = r[0];
		wk2[N - 2] = r[N - 1];
		for (size_t j = 1; j < N - 1; j++) {
			wk1[j] = r[j];
			wk2[j - 1] = r[j];
		}
		// END TEST TEST

		for (size_t k = 0; k < M; k++) {

			double num = 0.0, denom = 0.0;

			for (size_t j = 0; j < (N - k - 1); j++) {
				num += (wk1[j] * wk2[j]);
				denom += (SQR(wk1[j]) + SQR(wk2[j]));
			}

			a[k] = 2.0*num / denom;
			xms *= (1.0 - SQR(a[k]));

			for (size_t i = 0; i < k; i++)
				a[i] = wkm[i] - a[k] * wkm[k - 1 - i];

			/*		if (k == M - 1)
			return xms;*/

			for (size_t i = 0; i <= k; i++)
				wkm[i] = a[i];

			for (size_t j = 0; j < (N - k - 2); j++) {
				wk1[j] -= (wkm[k] * wk2[j]);
				wk2[j] = wk2[j + 1] - wkm[k] * wk1[j + 1];
			}
		}
		return xms;
		throw("never get here in memcof");
	}

	static double memcof(const double * data, double *d) {
		static constexpr size_t N = LPC_ORDER + 1;
		size_t k, j, i;
		double p = 0.0;
		double wk1[SZ], wk2[SZ], wkm[N];
		for (j = 0; j<SZ; j++) p += SQR(data[j]);
		double xms = p / SZ;

		wk1[0] = data[0];
		wk2[SZ - 2] = data[SZ - 1];
		for (j = 1; j<SZ - 1; j++) {
			wk1[j] = data[j];
			wk2[j - 1] = data[j];
		}
		for (k = 0; k<N; k++) {
			Doub num = 0.0, denom = 0.0;
			for (j = 0; j<(SZ - k - 1); j++) {
				num += (wk1[j] * wk2[j]);
				denom += (SQR(wk1[j]) + SQR(wk2[j]));
			}
			d[k] = 2.0*num / denom;
			xms *= (1.0 - SQR(d[k]));
			for (i = 0; i<k; i++)
				d[i] = wkm[i] - d[k] * wkm[k - 1 - i];
			if (k == LPC_ORDER)
				return xms;
			for (i = 0; i <= k; i++) wkm[i] = d[i];
			for (j = 0; j<(SZ - k - 2); j++) {
				wk1[j] -= (wkm[k] * wk2[j]);
				wk2[j] = wk2[j + 1] - wkm[k] * wk1[j + 1];
			}
		}
		throw("never get here in memcof");
	}

	// http://musicdsp.org/showone.php?id=137

	// Calculate the Levinson-Durbin recursion for the autocorrelation sequence R of length P+1 and return the autocorrelation coefficients a and reflection coefficients K
	static double levinson_durbin2(const double *R, double *A, double *K)
	{
		const size_t P = ORDER;
		double k_local_stack_alloc[P + 1];            // used if we havent provided reflection coeff array
		double err;

		if (!K)
			K = k_local_stack_alloc;

		double Am1[P + 1];

		if (R[0] == 0.0) {
			for (size_t i = 1; i <= P; i++)
			{
				K[i] = 0.0;
				A[i] = 0.0;
			}
		}
		else {

			for (size_t k = 0; k <= P; k++) {
				A[0] = 0.0;
				Am1[0] = 0.0;
			}
			A[0] = 1.0;
			Am1[0] = 1.0;
			double km = 0;
			double Em = R[0];
			for (size_t m = 1; m <= P; m++)                    //m=2:N+1
			{
				err = 0.0;                    //err = 0;
				for (size_t k = 1; k <= m - 1; k++)            //for k=2:m-1
					err += Am1[k] * R[m - k];        // err = err + am1(k)*R(m-k+1);
				km = (R[m] - err) / Em;            //km=(R(m)-err)/Em1;
				K[m - 1] = -km;
				A[m] = km;                        //am(m)=km;
				for (size_t k = 1; k <= m - 1; k++)            //for k=2:m-1
					A[k] = Am1[k] - km * Am1[m - k];  // am(k)=am1(k)-km*am1(m-k+1);
				Em = (1 - km * km)*Em;                //Em=(1-km*km)*Em1;
				for (size_t s = 0; s <= P; s++)                //for s=1:N+1
					Am1[s] = A[s];                // am1(s) = am(s)


			}
		}
		for (size_t m = 1; m <= P; m++)
			A[m] = -A[m];

		return err;
	}
};