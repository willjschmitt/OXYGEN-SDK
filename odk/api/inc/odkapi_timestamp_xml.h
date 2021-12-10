// Copyright DEWETRON GmbH 2018
#pragma once

#include "odkuni_xpugixml_fwd.h"
#include <cstdint>
#include <string>

namespace odk
{
    class Timestamp
    {
    public:
        Timestamp();
        Timestamp(std::uint64_t ticks, double frequency);

        bool parse(const char* xml_string);
        std::string generate() const;

        bool parseTickFrequencyAttributes(const pugi::xml_node& node);
        void writeTickFrequencyAttributes(pugi::xml_node& node) const;

        bool timestampValid() const;

        std::uint64_t m_ticks;
        double m_frequency;
    };

    class AbsoluteTime
    {
    public:
        AbsoluteTime();

        bool parse(const char* xml_string);
        std::string generate() const;

        int m_year;
        int m_month;
        int m_day;
        int m_hour;
        int m_minute;
        int m_second;
        std::uint32_t m_nanosecond;

        std::uint64_t m_nanoseconds_since_1970;

        std::string m_timezone_name;
        std::string m_timezone_location;

        int m_timezone_utc_offset_seconds;
        int m_timezone_std_offset_seconds;
        int m_timezone_dst_offset_seconds;
    };
}
