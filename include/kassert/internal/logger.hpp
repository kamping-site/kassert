// This file is part of KAssert.
//
// Copyright 2021-2022 The KAssert Authors

/// @file
/// @brief Logger utility class to build error messages for failed assertins.

#pragma once

#include <ostream>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

namespace @KASSERT_NAMESPACE@::internal {
// If partially specialized template is not applicable, set value to false.
template <typename, typename, typename = void>
struct is_streamable_type_impl : std::false_type {};

// Partially specialize template if StreamT::operator<<(ValueT) is valid.
template <typename StreamT, typename ValueT>
struct is_streamable_type_impl<
    StreamT,
    ValueT,
    std::void_t<decltype(std::declval<StreamT&>() << std::declval<ValueT>())>> : std::true_type {};

/// @brief Determines whether a value of type \c ValueT can be streamed into an output stream of type \c StreamT.
/// @ingroup expression-expansion
/// @tparam StreamT An output stream overloading the \c << operator.
/// @tparam ValueT A value type that may or may not be used with \c StreamT::operator<<.
template <typename StreamT, typename ValueT>
constexpr bool is_streamable_type = is_streamable_type_impl<StreamT, ValueT>::value;
} // namespace kassert::internal

namespace @KASSERT_NAMESPACE@ {
/// @brief Simple wrapper for output streams that is used to stringify values in assertions and exceptions.
///
/// To enable stringification for custom types, overload the \c << operator of this class.
/// The library overloads this operator for the following STL types:
///
/// * \c std::vector<T>
/// * \c std::pair<K, V>
///
/// @tparam StreamT The underlying streaming object (e.g., \c std::ostream or \c std::ostringstream).
template <typename StreamT>
class Logger {
public:
    /// @brief Construct the object with an underlying streaming object.
    /// @param out The underlying streaming object.
    explicit Logger(StreamT&& out) : _out_buffer(), _out(std::forward<StreamT>(out)) {
        _out_buffer << std::boolalpha;
    }

    /// @brief Forward all values for which \c StreamT::operator<< is defined to the underlying streaming object.
    /// @param value Value to be stringified.
    /// @tparam ValueT Type of the value to be stringified.
    template <typename ValueT, std::enable_if_t<internal::is_streamable_type<std::ostream, ValueT>, int> = 0>
    Logger<StreamT>& operator<<(ValueT&& value) {
        // we buffer logged values and only flush when the destructor is called or the buffer is flushed manually
        // this prevents interleaving of the outputs of multiple processes (e.g. MPI ranks)
        _out_buffer << std::forward<ValueT>(value);
        return *this;
    }

    /// @brief Get the underlying streaming object.
    /// Flushes all buffered logs to the underlying stream before returning a reference to the stream.
    /// @return The underlying streaming object.
    StreamT&& stream() {
        flush();
        return std::forward<StreamT>(_out);
    }

    /// @brief Flushes all buffered logs to the underlying stream.
    void flush() {
        _out << _out_buffer.str() << std::flush;
        _out_buffer.str(std::string{});
    }

    /// @brief Destructor of the logger stream, which flushes all buffered logs to the underlying stream upon
    /// destruction.
    ~Logger() {
        flush();
    }

private:
    std::stringstream _out_buffer; ///> @brief The output buffer.
    StreamT&&         _out;        ///> @brief The underlying streaming object.
};
} // namespace kassert

namespace @KASSERT_NAMESPACE@::internal {
/// @addtogroup expression-expansion
/// @{

/// @brief Stringify a value using the given assertion logger. If the value cannot be streamed into the logger, print
/// \c <?> instead.
/// @tparam StreamT The underlying streaming object of the assertion logger.
/// @tparam ValueT The type of the value to be stringified.
/// @param out The assertion logger.
/// @param value The value to be stringified.
template <typename StreamT, typename ValueT>
void stringify_value(Logger<StreamT>& out, ValueT const& value) {
    if constexpr (is_streamable_type<Logger<StreamT>, ValueT>) {
        out << value;
    } else {
        out << "<?>";
    }
}

/// @brief Logger writing all output to a \c std::ostream. This specialization is used to generate the KASSERT error
/// messages.
using OStreamLogger = Logger<std::ostream&>;

/// @brief Logger writing all output to a rvalue \c std::ostringstream. This specialization is used to generate the
/// custom error message for THROWING_KASSERT exceptions.
using RrefOStringstreamLogger = Logger<std::ostringstream&&>;

/// @}
} // namespace kassert::internal

namespace @KASSERT_NAMESPACE@ {

/// @brief Stringification of `std::vector<T>` in assertions.
///
/// Outputs a `std::vector<T>` in the following format, where `element i` are the stringified elements of the
/// vector: `[element 1, element 2, ...]`
///
/// @tparam StreamT The underlying output stream of the Logger.
/// @tparam ValueT The type of the elements contained in the vector.
/// @tparam AllocatorT The allocator of the vector.
/// @param logger The assertion logger.
/// @param container The vector to be stringified.
/// @return The stringified vector as described above.
template <typename StreamT, typename ValueT, typename AllocatorT>
Logger<StreamT>& operator<<(Logger<StreamT>& logger, std::vector<ValueT, AllocatorT> const& container) {
    logger << "[";
    bool first = true;
    for (auto const& element: container) {
        if (!first) {
            logger << ", ";
        }
        logger << element;
        first = false;
    }
    return logger << "]";
}

/// @brief Stringification of `std::pair<K, V>` in assertions.
///
/// Outputs a `std::pair<K, V>` in the following format, where `first` and `second` are the stringified
/// components of the pair: `(first, second)`.
///
/// @tparam StreamT The underlying output stream of the Logger.
/// @tparam Key Type of the first component of the pair.
/// @tparam Value Type of the second component of the pair.
/// @param logger The assertion logger.
/// @param pair The pair to be stringified.
/// @return The stringification of the pair as described above.
template <typename StreamT, typename Key, typename Value>
Logger<StreamT>& operator<<(Logger<StreamT>& logger, std::pair<Key, Value> const& pair) {
    return logger << "(" << pair.first << ", " << pair.second << ")";
}
} // namespace kassert
