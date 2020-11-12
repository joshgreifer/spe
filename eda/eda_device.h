//
// Created by josh on 10/11/2020.
//

#ifndef EDA_EDA_DEVICE_H
#define EDA_EDA_DEVICE_H
#include "../eng6/eng6.h"
#include <Windows.h>
#include <setupapi.h>
#pragma comment(lib,  "setupapi.lib")

#include "formatted_string.h"

namespace stdproc = sel::eng6::proc;
namespace eng = sel::eng6;

struct eda_device  : public stdproc::data_source<1> {

    mutable OVERLAPPED overlapped;
    static VOID CALLBACK IOCompletionRoutine( DWORD err, DWORD nBytes, LPOVERLAPPED lpOverlapped ) {
        if (err)
            throw std::system_error(std::error_code(err, std::system_category()));

        auto device = reinterpret_cast<eda_device *>(lpOverlapped->hEvent);
        assert((nBytes % sizeof(EDA_Packet)) == 0);
        device->queue.endwrite(nBytes / sizeof(EDA_Packet));
    }
// from hidclass.h in WinDDK
    GUID GUID_DEVINTERFACE_HID = { 0x4D1E55B2L, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 };

//const char *STR_GUID_DEVINTERFACE_HID = "{4d1e55b2-f16f-11cf-88cb-001111000030}";

//const char *DEVICE_PATH = "\\\\?\\hid#vid_1781&pid_0870#7&9a6b3c5&1&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}";

    static const int HEALTHSMART_VENDOR_ID = 0x04d8;
    static const int HEALTHSMART_PRODUCT_ID_SMART_HEART_AGENT = 0xef61;
    char device_path[MAX_PATH];

    HANDLE file_hand_;
    const formatted_string DEVICE_PATH_SEARCH_STRING;
    eda_device() :  DEVICE_PATH_SEARCH_STRING ("\\\\?\\hid#vid_%4.4x&pid_%4.4x#", HEALTHSMART_VENDOR_ID, HEALTHSMART_PRODUCT_ID_SMART_HEART_AGENT) {
        memset(&overlapped, 0, sizeof(overlapped));
        overlapped.hEvent = this;
    }

    ~eda_device() {
        close();
    }

private:
    bool found_device(const char *path)
    {
        return _strnicmp(path, DEVICE_PATH_SEARCH_STRING, strlen(DEVICE_PATH_SEARCH_STRING)) == 0;
    }

    bool scan_for_connected_device( const char *physical_path, const int physical_path_len )
    {

        bool found = false;

        SP_INTERFACE_DEVICE_DATA did;
        did.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);

        // Open a handle to the plug and play dev node.
        HDEVINFO hdi = SetupDiGetClassDevs ( &GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

        for (DWORD devicenum = 0; !found ; ++devicenum) {
            PSP_INTERFACE_DEVICE_DETAIL_DATA device_data;
            ULONG  requiredLength;
            try {
                if (0 == SetupDiEnumDeviceInterfaces (hdi, 0, &GUID_DEVINTERFACE_HID, devicenum, &did)) {
                    auto err_c = GetLastError();
                    if (err_c == ERROR_NO_MORE_ITEMS)
                        break; // no more devices
                    else
                        throw std::system_error(std::error_code(err_c, std::system_category()));
                }
                // http://msdn.microsoft.com/en-us/library/windows/hardware/ff551120(v=vs.85).aspx

                // Call SetupDiGetDeviceInterfaceDetail	with a NULL device data return buffer, so we can learn the required buffer size
                requiredLength = 0;
                if (0 == SetupDiGetDeviceInterfaceDetail ( hdi, &did, NULL, 0, &requiredLength, NULL)) {
                    auto err_c = GetLastError();
                    if (err_c != ERROR_INSUFFICIENT_BUFFER)
                        throw std::system_error(std::error_code(err_c, std::system_category()));
                }

                // now we know the size of the actual SP_INTERFACE_DEVICE_DETAIL_DATA we'll be getting, we can allocate it
                device_data = (PSP_INTERFACE_DEVICE_DETAIL_DATA) new BYTE[requiredLength];

                device_data->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);

                SetupDiGetDeviceInterfaceDetail ( hdi, &did, device_data, requiredLength, &requiredLength, NULL );

                // See if the the device path matches (starts with) the pattern we're looking for
                if (found_device( device_data->DevicePath )) {
                    strncpy_s(const_cast<char *>(physical_path), physical_path_len, device_data->DevicePath, physical_path_len);
                    found = true;
                }

                delete[] device_data;


            } catch (...) {
                found = false;
            }
        }
        // Clean up
        SetupDiDestroyDeviceInfoList (hdi);

        return found;

    }

#pragma pack(push, 1)
    struct EDA_Packet
    {
        BYTE reportId; //= 0;
        BYTE status; // = 0x80;      // Status (fixed for now)
        BYTE data_type; // = 0x01;      // Type of data - 01 = EDA, 50 samples/second
        short data;
        BYTE reserved[4];
    };
    static_assert(sizeof(EDA_Packet) == 9);
#pragma pack(pop)

    mutable sel::quick_queue<EDA_Packet, 64> queue;
public:
    [[nodiscard]] const EDA_Packet& get_packet() const
    {
        ;
        DWORD bytes_read;

        auto n_packets_to_read = queue.put_avail();
        if (n_packets_to_read)
            if (!::ReadFileEx(file_hand_, queue.acquirewrite(), sizeof(EDA_Packet) * n_packets_to_read, &overlapped, IOCompletionRoutine))
                throw std::system_error(std::error_code(::GetLastError(), std::system_category()));

        do {
            SleepEx(0, TRUE);
        } while (!queue.get_avail());

        const EDA_Packet& p = queue.get();
        if (p.reportId != 0)
            throw std::exception("ReportId != 0");

        return p;

    }

    [[nodiscard]] unsigned char get_byte() const {
        DWORD numread;
        unsigned char b;
        if (!::ReadFile(file_hand_, &b, 1, &numread, NULL))
            throw std::system_error(std::error_code(::GetLastError(), std::system_category()));
        return b;
    }

    [[nodiscard]] short get_data() const {
        return get_packet().data;
    }

    void process() final
    {
        *this->out = static_cast<samp_t>(get_data());
        raise();
    }
    void init(eng::schedule * context) final {
        open();
    }
    void term(eng::schedule * context) final {
        close();
    }
public:
    void open()
    {
        for(;;) {
            if (!scan_for_connected_device(device_path, MAX_PATH)) {
                if (IDCANCEL == ::MessageBox(NULL,
                                             "No EDA device is detected.\n"
                                             "Please connect a device and then select Retry,  or select Cancel to exit the program.",
                                             "No device detected", MB_RETRYCANCEL | MB_ICONSTOP))
                    throw std::exception("No EDA device found.");
            } else
                break;
        }
        fprintf(stderr, "Detected EDA device on USB path %s.\n", device_path);

        file_hand_= ::CreateFile(device_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL, OPEN_EXISTING,FILE_ATTRIBUTE_READONLY | FILE_FLAG_OVERLAPPED , NULL );
        if (file_hand_ == INVALID_HANDLE_VALUE)
            throw std::system_error(std::error_code(::GetLastError(), std::system_category()));

        raise(); // start the read pump
    }

    bool is_open()
    {
        return file_hand_ != INVALID_HANDLE_VALUE;
    }

    void close()
    {
        if (is_open())
            ::CloseHandle(file_hand_);
    }




};


#endif //EDA_EDA_DEVICE_H
