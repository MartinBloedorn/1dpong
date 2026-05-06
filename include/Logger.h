#pragma once

#include <Arduino.h>

namespace Logger 
{

namespace details 
{

static inline bool sEnabled{true};

template<typename Stream, typename Arg>
void print_recursive(Stream& stream, Arg&& arg) {
    stream.println(std::move(arg));
}

template <typename Stream, typename Arg, typename... Args>
void print_recursive(Stream& stream, Arg&& arg, Args&&... args) {
    stream.print(std::move(arg));
    print_recursive(stream, std::forward<Args>(args)...);
}

inline const char* fmt_timestamp(long int t) {
    static char buffer[32];
    snprintf(buffer, sizeof(buffer), "%8lu", t);
    return buffer;
}

template<typename Stream, typename... Args>
void log(Stream& stream, const char* label, Args&&... args) {
    if(sEnabled)
        print_recursive(stream, details::fmt_timestamp(millis()), " [", label, "] ", std::forward<Args>(args)...);
}

} // details

inline void setEnabled(bool enabled) {
    details::sEnabled = enabled;
}

template<typename Stream, typename... Args>
void print(Stream& stream, Args&&... args) {
    details::print_recursive(stream, std::forward<Args>(args)...);
}

template<typename Stream, typename... Args>
void info(Stream& stream, Args&&... args) {
    details::log(stream, " INFO", std::forward<Args>(args)...);
}

template<typename Stream, typename... Args>
void debug(Stream& stream, Args&&... args) {
    details::log(stream, "DEBUG", std::forward<Args>(args)...);
}
template<typename Stream, typename... Args>
void warn(Stream& stream, Args&&... args) {
    details::log(stream, " WARN", std::forward<Args>(args)...);
}

}