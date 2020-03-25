#pragma once
#include <cstring> 
#include <stdio.h> 
#include <fcntl.h> 
#include <time.h>
#include <sys/timeb.h>
#include <sys/stat.h>

#include "../processor.h"
#include "../msg_and_error.h"

#ifndef __iscsymf
#define __iscsymf(c)  (isalpha(c) || ((c) == '_'))
#endif

#ifndef __iscsym
#define __iscsym(c)   (isalnum(c) || ((c) == '_'))
#endif
namespace sel {
	namespace eng {
		namespace proc {
			struct matlab_file_output : ProcessorXx0, virtual public creatable<matlab_file_output>
			{
			virtual const std::string type() const override { return "matlab file writer"; }
			private:
				using file_descriptor = FILE * ;
				using byte = unsigned int;

				file_descriptor fp_;

				static constexpr int FILE_OFFSET_ROWS = 4;
				static constexpr int FILE_OFFSET_COLS = 8;

				mutable int rows_;		// number of inputs 
				int cols_;
				char matlabfilename_[FILENAME_MAX];

				mutable double fs_;		// sample rate -- gets written to Matlab var
				double start_time_matlab_datenum_;  // start time in Matlab DateNum format  -- gets written to Matlab var
				char varname_[33];	// Max allowed length of matlab variable name, plus terminating \0

				bool vectorize_;

				void Open()
				{
					start_time_matlab_datenum_ = MatlabDateNum();
					cols_ = 0;
					fp_ = ::fopen(matlabfilename_, "wb");
					if (!fp_)
						throw eng_ex(os_err_message<2048U>(errno).data());

					WriteMatrixHeader(varname_, vectorize_ ? 1 : rows_, cols_);
				}
				void SetName(const uri& uri)
				{
					bool was_open = (fp_ != nullptr);

					if (was_open) {
						Close();
					}

					::strcpy(matlabfilename_, uri.path());

					// Append ".mat" if necessary
					if (_stricmp(&matlabfilename_[strlen(matlabfilename_) - 4], ".mat"))
						strcat(matlabfilename_, ".mat");
					if (strchr(matlabfilename_, '%'))
						throw eng_ex("Illegal escape char in matlab filename");

					char temp[FILENAME_MAX];

					char *datepos;

					datepos = strstr(matlabfilename_, "$T");
					if (datepos) {
						datepos[0] = '%';
						datepos[1] = 's';
						sprintf(temp, matlabfilename_, CurrentTimeString());
						strcpy(matlabfilename_, temp);
					}

					datepos = strstr(matlabfilename_, "$D");
					if (datepos) {
						datepos[0] = '%';
						datepos[1] = 's';
						sprintf(temp, matlabfilename_, CurrentDateString());
						strcpy(matlabfilename_, temp);
					}

					char *numpos = strstr(matlabfilename_, "$N");

					if (numpos) {
						numpos[0] = '%';
						numpos[1] = 'd';

						int n = 0;
						struct stat buffer;

						// loop until no file found with this name
						do {
							++n;
							sprintf(temp, matlabfilename_, n);
						} while (!stat(temp, &buffer));

						// found filename
						strcpy(matlabfilename_, temp);

					}

					if (was_open)
						Open();

				}
				void Close()
				{
					if (fp_) {
						// write sample rate to end of file
						WriteDoubleVar(fs_, "sample_rate");

						// write start time to end of file
						WriteDoubleVar(start_time_matlab_datenum_, "start_time");

						// update the number of columns written
						if (::fseek(fp_, FILE_OFFSET_COLS, SEEK_SET) == -1)
							throw eng_ex(os_err_message<2048U>(errno).data());

						::fwrite(&cols_, sizeof(int), 1, fp_);

						::fclose(fp_);
						fp_ = nullptr;
					}
				}


				// utility function:  Convert current system time to Matlab Datenum format
				double MatlabDateNum()
				{
					std::time_t timer_ = std::time(nullptr);		// convert to Matlab Datenum format (double precision number of days since Jan 1 0000)
															// 719529 == DateNum(1 Jan 1970)
															// 86400 == num. seconds in a day
					return 719529.0 + timer_ / 86400.0;
				}

				charbuf_short CurrentTimeString(void)
				{
					charbuf_short msg;
					static char _str_tm[256];
					timeb tstruct;

					ftime(&tstruct);

					auto gmt = *gmtime(&tstruct.time);

					msg.sprintf("%2.2d%2.2d%2.2d%3.3d", gmt.tm_hour, gmt.tm_min, gmt.tm_sec, tstruct.millitm);
					return msg;
				}

				charbuf_short CurrentDateString(void)
				{
					charbuf_short msg;
					timeb tstruct;
					ftime(&tstruct);

					auto gmt = *gmtime(&tstruct.time);

					msg.sprintf("%4.4d%2.2d%2.2d", 1900 + gmt.tm_year, gmt.tm_mon + 1, gmt.tm_mday);
					return msg;
				}
				// utility function: Write Matrix header to file
				void WriteMatrixHeader(const char *varname, int r, int c)
				{
					int type = 00;	// Matlab double array
					int imagf = 0;	// not using complex numbers
					int varnamelen = (int)strlen(varname) + 1;

					fwrite(&type, sizeof(int), 1, fp_);
					fwrite(&r, sizeof(int), 1, fp_);
					fwrite(&c, sizeof(int), 1, fp_); // will update to final column count in dtor
					fwrite(&imagf, sizeof(int), 1, fp_);
					fwrite(&varnamelen, sizeof(int), 1, fp_);
					// write matrix name + terminating null
					fwrite(varname, sizeof(char), varnamelen, fp_);
				}

				// utility function: Write double value to Matlab var
				void WriteDoubleVar(double d, const char *varname)
				{
					WriteMatrixHeader(varname, 1, 1);
					::fwrite(&d, sizeof(double), 1, fp_);
				}

				// utility function: Convert a character buffer to a legal Matlab var name
				static void MakeLegalMatlabVarName(char *var_name)
				{
					size_t var_len = strlen(var_name);

					if (var_len > 32)
						var_name[32] = '\0'; // truncate name

											 // make legal matlab variable name -- same rule as C symbols

											 // first char -- underscore or alpha
					if (!__iscsymf(var_name[0]))
						var_name[0] = '_';
					// remaining chars -- underscore, alpha or digit
					for (size_t j = 1; j < var_len; ++j)
						if (!__iscsym(var_name[j]))
							var_name[j] = '_';
				}
			public:


				matlab_file_output() {}

				matlab_file_output(const uri& u, bool vectorize = false) : fp_(0), vectorize_(vectorize)
				{
					u.assert_scheme(uri::FILE);
					SetName(u);

					strcpy(varname_, "sig");

				}

				matlab_file_output(params& params) : fp_(0), vectorize_(false)
				{
					uri u = params.get<const char *>("uri");
					u.assert_scheme(uri::FILE);
					SetName(u);

					// If true, output [ncols X 1] vector
					// If false, out [ncols X port.width] matrix
					vectorize_ = params.get<bool>("vectorize", false);

					strncpy(varname_, params.get<const char*>("var", "unnamed_data"), sizeof(varname_));
					MakeLegalMatlabVarName(varname_);
				}

				void init(schedule* context)  final
				{

					rows_ = 0;
					for (auto& port : inports)
						rows_ += (int)port->width();


					fs_ = context ? static_cast<double>(context->trigger()->rate().expected()) : 0.0;

					Open();
				}

				void term(schedule* context) final
				{
					Close();
				}

				void process() override
				{
					for (auto port : inports)
						::fwrite(port->as_array(), sizeof(samp_t), (unsigned int)port->width(), fp_);

					if (vectorize_)
						cols_ += rows_;
					else
						++cols_;
				}


			};
		} // proc
	} // eng
} // sel


#if 0
#pragma once

#include "DateUtil.h"
#include "stream_out.h"
#include "sp.h"
#include <sys/stat.h>





class MatFileWriter :
	public SigProc,
	public iRunTimeModifiable,
	public iRunTimeInspectable
{
protected:

	FILE * outfp;

	SIGNAL_T *packet_buf;

	static const int FILE_OFFSET_ROWS = 4;
	static const int FILE_OFFSET_COLS = 8;

	int rows;
	int cols;
	char matlabfilename[MAX_PATH];

	double fs_;		// sample rate -- gets written to Matlab var
	double start_time;  // start time in Matlab DateNum format  -- gets written to Matlab var

	bool paused;

	char varname_[33];	// Max allowed length of matlab variable name, plus terminating \0

						// utility function: Write Matrix header to file
	void WriteMatrixHeader(const char *varname, int r, int c)
	{
		int type = 00;	// Matlab double array
		int imagf = 0;	// not using complex numbers
		int varnamelen = (int)strlen(varname) + 1;

		fwrite(&type, sizeof(int), 1, outfp);
		fwrite(&r, sizeof(int), 1, outfp);
		fwrite(&c, sizeof(int), 1, outfp); // will update to final column count in dtor
		fwrite(&imagf, sizeof(int), 1, outfp);
		fwrite(&varnamelen, sizeof(int), 1, outfp);
		// write matrix name + terminating null
		fwrite(varname, sizeof(char), varnamelen, outfp);
	}

	// utility function: Write double value to Matlab var
	void WriteDoubleVar(double d, const char *varname)
	{
		WriteMatrixHeader(varname, 1, 1);
		fwrite(&d, sizeof(double), 1, outfp);
	}

	// utility function: Convert a character buffer to a legal Matlab var name
	static void MakeLegalMatlabVarName(char *var_name)
	{
		size_t var_len = strlen(var_name);

		if (var_len > 32)
			var_name[32] = '\0'; // truncate name

								 // make legal matlab variable name -- same rule as C symbols

								 // first char -- underscore or alpha
		if (!__iscsymf(var_name[0]))
			var_name[0] = '_';
		// remaining chars -- underscore, alpha or digit
		for (size_t j = 1; j < var_len; ++j)
			if (!__iscsym(var_name[j]))
				var_name[j] = '_';
	}

public:
	const char* name() const { return "MatFileWriter"; }
	PIN default_inpin() const { return NEW; }


	void Open()
	{
		if (outfp)
			throw eng_ex(CANT_OPEN_FILE, ". Attempted to open logfile twice, probably because the MatFilewriter is in more than one schedule block (which is not allowed)");
		start_time = DateUtil::MatlabDateNum();
		packet_buf = new SIGNAL_T[n_in];

		cols = 0;
		errno_t err = fopen_s(&outfp, matlabfilename, "wb");
		if (err) {
			char buf[4096];
			char msgbuf[4096];
			strerror_s(buf, 4096, err);
			sprintf_s(msgbuf, 4096, " '%s': %s", matlabfilename, buf);
			throw eng_ex(CANT_OPEN_FILE, msgbuf);
		}
		// reset file pointer
		if (outfp) fseek(outfp, 0L, SEEK_SET);
		WriteMatrixHeader(varname_, rows, cols);
	}

	void Modify(params& args)
	{
		if (is_arg(args, "uri")) {
			URI new_uri = mand_arg<URI>(args, "uri");
			SetName(new_uri);
		}
		if (is_arg(args, "pause")) {
			paused = mand_arg<bool>(args, "pause");
		}
		if (is_arg(args, "open")) {
			if (mand_arg<bool>(args, "open"))
				SetName(URI(matlabfilename));
			else
				Close();
		}
	}

	string Inspect(const char *attribute) const
	{
		if (!_stricmp(attribute, "filename"))
			return matlabfilename;
		else if (!_stricmp(attribute, "isopen"))
			return outfp != NULL ? "True" : "False";
		else if (!_stricmp(attribute, "rowswritten")) {
			char buf[32];
			sprintf_s(buf, 32, "%d", rows);
			return buf;
		}
		else if (!_stricmp(attribute, "columnswritten")) {
			char buf[32];
			sprintf_s(buf, 32, "%d", cols);
			return buf;
		}

		throw eng_ex(UNKNOWN_INSPECT_ATTRIBUTE, "Inspectable attributes for this object: 'filename', 'isopen', 'rowswritten', 'columnswritten'");
	}

	void SetName(const URI& uri)
	{
		bool was_open = (outfp != NULL);

		if (was_open) {
			Close();
		}

		strcpy_s(matlabfilename, MAX_PATH, uri.path());

		// Append ".mat" if necessary
		if (_stricmp(&matlabfilename[strlen(matlabfilename) - 4], ".mat"))
			strcat_s(matlabfilename, MAX_PATH, ".mat");
		if (strchr(matlabfilename, '%'))
			throw eng_ex(ILLEGAL_FILENAME_ESCAPE, matlabfilename);

		char temp[MAX_PATH];

		char *datepos;

		datepos = strstr(matlabfilename, "$T");
		if (datepos) {
			datepos[0] = '%';
			datepos[1] = 's';
			sprintf_s(temp, MAX_PATH, matlabfilename, DateUtil::CurrentTimeString());
			strcpy_s(matlabfilename, MAX_PATH, temp);
		}

		datepos = strstr(matlabfilename, "$D");
		if (datepos) {
			datepos[0] = '%';
			datepos[1] = 's';
			sprintf_s(temp, MAX_PATH, matlabfilename, DateUtil::CurrentDateString());
			strcpy_s(matlabfilename, MAX_PATH, temp);
		}

		char *numpos = strstr(matlabfilename, "$N");

		if (numpos) {
			numpos[0] = '%';
			numpos[1] = 'd';

			int n = 0;
			struct _stat buffer;

			// loop until no file found with this name
			do {
				++n;
				sprintf_s(temp, MAX_PATH, matlabfilename, n);
			} while (!_stat(temp, &buffer));

			// found filename
			strcpy_s(matlabfilename, MAX_PATH, temp);

		}

		if (was_open)
			Open();

	}
	void Close()
	{
		if (packet_buf) {
			delete[] packet_buf;
			packet_buf = NULL;
		}

		if (outfp) {
			// write sample rate to end of file
			WriteDoubleVar(fs_, "sample_rate");

			// write start time to end of file
			WriteDoubleVar(start_time, "start_time");

			// update the number of columns written
			fseek(outfp, FILE_OFFSET_COLS, SEEK_SET);
			fwrite(&cols, sizeof(int), 1, outfp);

			fflush(outfp);
			fclose(outfp);

			outfp = NULL;
		}
	}

	void init(Schedule *context)
	{
		checkinputs();
		rows = (int)n_in;
		fs_ = context ? (double)context->Fs() : 0.0;
		Open();

	}

	void term(Schedule *context)
	{
		Close();
	}

	void trace(void) const
	{
		EngineTrace("MatFileWriter: File %s: %d row%s X %d column%s written.", matlabfilename,
			rows, rows == 1 ? "" : "s",
			cols, cols == 1 ? "" : "s"
		);
	}

	MatFileWriter(const URI& uri, std::string& varname) : SigProc(0, 0), packet_buf(NULL), outfp(NULL)
	{
		strncpy_s(varname_, varname.c_str(), 32);
		paused = false;
		SetName(uri);
	}

	void process(void)
	{
		if (!paused) {
			if (outfp != NULL) {
				for (size_t i = 0; i < n_in; ++i)
					packet_buf[i] = *in[i];

				++cols;
				_fwrite_nolock(packet_buf, sizeof(SIGNAL_T), n_in, outfp);
				fflush(outfp);
			}
		}

	}

	virtual ~MatFileWriter()
	{
		Close();
	}
};

// WARNING: The XML in this comment section is used to generate both the XSD schema for the engine, 
// and the documentation for the signal processor library.
/**
<PROC NAME="MatFile">
<SUMMARY>
Matlab file output.  This processor's documentation is incomplete.
</SUMMARY>
<INPIN NAME="NEW" />
<CREATE_ARG NAME="uri" MANDATORY="true" TYPE="uri">
</CREATE_ARG>
</PROC>
*/
struct MatFileWriter_FACTORY : public FactoryClass2Args<MatFileWriter, URI, std::string>
{
	MatFileWriter_FACTORY() :
		FactoryClass2Args<MatFileWriter, URI, std::string>("uri", "varname")
	{
	}
};

#endif