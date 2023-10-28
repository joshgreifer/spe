//
// Created by josh on 17/11/2022.
//
#ifndef SPE_JULIUSRESAMPLER_H
#define SPE_JULIUSRESAMPLER_H
using data_t = float;
template<typename data_t, size_t SZ>struct CircularFifo : std::array<data_t, SZ * 2>
{
    sel::idx<SZ> back {};
    sel::idx<SZ> front {};
    bool isEmpty = true;
    template<typename It>void write(It begin, It end)
    {
        while (begin != end) {
            *(front+SZ) = *begin;
            *front++ = *begin++;
        }
        isEmpty = false;
    }

    template<typename It>void readAll(It dest)
    {
        if (!isEmpty) {
            while (back != front)
                *dest++ = *back++;
            isEmpty = true;
        }
    }

    template<typename It>void read1(It dest)
    {
        if (!isEmpty)
            *dest++ = *back++;
        isEmpty = (back == front);
    }

    size_t num_items() const
    {
        const auto d = front - back;
        return d == 0 ? (isEmpty ? 0 : SZ) : d;
    }

};

template<size_t SR_IN, size_t SR_OUT, size_t MAX_IBUF_SZ> struct JuliusResampler
{
    static constexpr auto num = std::ratio<SR_IN, SR_OUT>::num;
    static constexpr auto den = std::ratio<SR_IN, SR_OUT>::den;
    CircularFifo<float, MAX_IBUF_SZ>input_buffer;

    size_t total_input = 0;
    template<typename It>void resample(It begin, It end)
    {
        size_t num_coeffs = 11;
        // Add data to buffer
        input_buffer.template write(begin, end);
        total_input += end - begin;
        while (input_buffer.num_items() >= num_coeffs) {

        }
        // resample as much of the data as we can



    }
};
#endif //SPE_JULIUSRESAMPLER_H
