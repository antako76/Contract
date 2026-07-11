#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <cstddef>
#include <contract/visit.hpp>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iomanip>
#include <map>
#include <optional>
#include <ostream>
#include <random>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <utility>
#include <vector>
#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#endif

namespace benchmark_base {

inline constexpr std::uint64_t default_order_seed = 0x5eed'1234'c0de'beefull;

struct quantiles {
    double p25 = 0.0;
    double p50 = 0.0;
    double p75 = 0.0;
};

struct measurement {
    quantiles target_cost;
    quantiles base_cost;
    quantiles delta_cost;
    quantiles ratio;
};

struct run_config {
    int iterations = 0;
    int repeats = 0;
};

template<std::size_t ColumnCount, class MeasurementT = measurement>
struct table_row {
    std::string name;
    int iterations = 0;
    std::array<std::optional<MeasurementT>, ColumnCount> values{};
};

template<class Fixture>
struct fixture_bank {
    std::string name;
    std::vector<Fixture> fixtures;
    std::vector<std::size_t> order;

    std::size_t pick(int i) const {
        if (fixtures.empty()) {
            return 0;
        }
        if (order.empty()) {
            return static_cast<std::size_t>(i) % fixtures.size();
        }
        return order[static_cast<std::size_t>(i) % order.size()] % fixtures.size();
    }
};

template<std::size_t ColumnCount>
struct table_spec {
    std::string title;
    std::array<const char*, ColumnCount> columns{};
    std::size_t separator_after = ColumnCount > 0 ? ColumnCount - 1 : 0;
    std::size_t value_width = 16;
    std::size_t gap = 3;
};

template<class Fn>
inline std::uint64_t measure_once(Fn&& fn) {
    const auto start = std::chrono::steady_clock::now();
    fn();
    const auto end = std::chrono::steady_clock::now();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
}

template<class Fn>
inline std::uint64_t measure_cycles_once(Fn&& fn) {
#if defined(__x86_64__) || defined(__i386__)
    const auto start = __rdtsc();
    fn();
    const auto end = __rdtsc();
    return static_cast<std::uint64_t>(end - start);
#else
    return measure_once(fn);
#endif
}

template<class Fn>
inline double measure_per_op(int iterations, Fn&& fn) {
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        fn(i);
    }
    const auto end = std::chrono::steady_clock::now();
    const auto total = std::chrono::duration<double, std::nano>(end - start).count();
    return total / static_cast<double>(iterations);
}

template<class Fn>
inline double median_per_op(int repeats, int iterations, Fn&& fn) {
    std::vector<double> samples;
    samples.resize(static_cast<std::size_t>(repeats));
    for (double& sample : samples) {
        sample = measure_per_op(iterations, fn);
    }
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

inline std::vector<std::size_t> make_order(
    std::size_t count,
    std::size_t modulo,
    std::uint64_t seed = default_order_seed)
{
    std::vector<std::size_t> order(count);
    std::mt19937_64 rng{seed};
    if (modulo == 0) {
        std::fill(order.begin(), order.end(), 0);
        return order;
    }

    std::uniform_int_distribution<std::size_t> dist(0, modulo - 1);
    for (std::size_t i = 0; i < order.size(); ++i) {
        order[i] = dist(rng);
    }
    return order;
}

inline std::size_t selection_slot(const std::vector<std::size_t>& order, int i) {
    if (order.empty()) {
        return 0;
    }
    return order[static_cast<std::size_t>(i) % order.size()];
}

inline std::size_t seed_at(const std::vector<std::size_t>& order, std::size_t i) {
    if (order.empty()) {
        return 0;
    }
    return order[i % order.size()];
}

inline std::uint64_t fold_bytes(const unsigned char* data, std::size_t size, std::uint64_t seed = 0) {
    std::uint64_t hash = seed;
    for (std::size_t i = 0; i < size; ++i) {
        hash = (hash << 5) | (hash >> 59);
        hash ^= static_cast<std::uint64_t>(data[i]);
    }
    hash ^= static_cast<std::uint64_t>(size) << 56u;
    return hash;
}

inline void consume_bytes(volatile std::uint64_t& sink, const void* data, std::size_t size) {
    sink ^= fold_bytes(static_cast<const unsigned char*>(data), size, sink);
}

template<class T>
inline void escape(T& value) {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : "+m"(value) : : "memory");
#else
    (void)value;
#endif
}

template<class T>
inline void consume(volatile std::uint64_t& sink, const T& value);

template<class T>
inline void consume(volatile std::uint64_t& sink, const std::optional<T>& value) {
    if (value.has_value()) {
        sink ^= 0x9e3779b97f4a7c15ull;
        consume(sink, *value);
    } else {
        sink ^= 0x243f6a8885a308d3ull;
    }
}

template<class T, std::size_t N>
inline void consume(volatile std::uint64_t& sink, const std::array<T, N>& value) {
    if constexpr (std::is_trivially_copyable_v<T>) {
        consume_bytes(sink, value.data(), sizeof(T) * value.size());
    } else {
        for (const auto& item : value) {
            consume(sink, item);
        }
    }
}

template<class T, std::size_t N>
inline void consume(volatile std::uint64_t& sink, const std::bitset<N>& value) {
    if constexpr (N <= 64) {
        consume(sink, value.to_ullong());
    } else {
        consume(sink, value.to_string());
    }
}

template<class T>
inline void consume(volatile std::uint64_t& sink, const std::vector<T>& value) {
    if constexpr (std::is_trivially_copyable_v<T>) {
        if (!value.empty()) {
            consume_bytes(sink, value.data(), sizeof(T) * value.size());
        }
    } else {
        for (const auto& item : value) {
            consume(sink, item);
        }
    }
}

template<class... Ts>
inline void consume(volatile std::uint64_t& sink, const std::tuple<Ts...>& value) {
    std::apply([&](const auto&... items) {
        (consume(sink, items), ...);
    }, value);
}

template<class... Ts>
inline void consume(volatile std::uint64_t& sink, const std::variant<Ts...>& value) {
    std::visit([&](const auto& item) {
        consume(sink, item);
    }, value);
}

template<class K, class V, class... Rest>
inline void consume(volatile std::uint64_t& sink, const std::map<K, V, Rest...>& value) {
    for (const auto& [key, mapped] : value) {
        consume(sink, key);
        consume(sink, mapped);
    }
}

template<class K, class V, class... Rest>
inline void consume(volatile std::uint64_t& sink, const std::unordered_map<K, V, Rest...>& value) {
    for (const auto& [key, mapped] : value) {
        consume(sink, key);
        consume(sink, mapped);
    }
}

template<class T>
inline void consume(volatile std::uint64_t& sink, const T& value) {
    if constexpr (std::is_same_v<T, std::string>) {
        consume_bytes(sink, value.data(), value.size());
    } else if constexpr (std::is_same_v<T, std::string_view>) {
        consume_bytes(sink, value.data(), value.size());
    } else if constexpr (std::is_same_v<T, const char*>) {
        consume_bytes(sink, value, std::char_traits<char>::length(value));
    } else if constexpr (std::is_arithmetic_v<T>) {
        consume_bytes(sink, &value, sizeof(value));
    } else if constexpr (std::is_trivially_copyable_v<T>) {
        consume_bytes(sink, &value, sizeof(value));
    } else {
        static_assert(std::is_trivially_copyable_v<T>, "benchmark_base::consume needs an overload for this type");
    }
}

template<class Object>
inline void consume_object(volatile std::uint64_t& sink, const Object& object) {
    contract::for_each_field<Object>([&](const auto&... fields) {
        (consume(sink, fields.get(object)), ...);
    });
}

inline int parse_iterations_arg(int argc, char** argv, int default_iterations) {
    int iterations = default_iterations;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view{argv[i]} == "--iterations" && i + 1 < argc) {
            iterations = std::max(1, std::atoi(argv[++i]));
        } else if (std::string_view{argv[i]}.rfind("--iterations=", 0) == 0) {
            iterations = std::max(1, std::atoi(std::string(argv[i]).c_str() + 13));
        }
    }
    return iterations;
}

template<std::size_t ColumnCount, class MeasurementT>
inline void render_table(
    const std::vector<table_row<ColumnCount, MeasurementT>>& rows,
    const table_spec<ColumnCount>& spec,
    bool color,
    bool show_quantiles,
    std::ostream& out)
{
    constexpr double warning_ratio = 1.2;
    constexpr const char* red = "\033[31m";
    constexpr const char* yellow = "\033[33m";
    constexpr const char* reset = "\033[0m";

    std::array<std::size_t, ColumnCount> widths{};
    for (std::size_t c = 0; c < ColumnCount; ++c) {
        widths[c] = std::max<std::size_t>(std::strlen(spec.columns[c]), spec.value_width);
    }

    std::size_t row_width = std::strlen("measurement");
    for (const auto& row : rows) {
        row_width = std::max<std::size_t>(row_width, row.name.size());
    }

    std::size_t runs_col_width = std::strlen("iters");
    for (const auto& row : rows) {
        runs_col_width = std::max<std::size_t>(runs_col_width, std::to_string(row.iterations).size());
    }

    struct cell_render {
        std::string text;
        std::string target_cost_p50;
        std::string base_cost_p50;
        std::string ratio_p25;
        std::string ratio_p50;
        bool highlight_red = false;
        bool highlight_yellow = false;
    };

    enum class row_highlight {
        none,
        yellow,
        red,
    };

    std::vector<std::array<cell_render, ColumnCount>> rendered;
    std::vector<row_highlight> row_highlights;
    rendered.reserve(rows.size());
    row_highlights.reserve(rows.size());

    for (const auto& row : rows) {
        std::array<cell_render, ColumnCount> line{};
        row_highlight highlight = row_highlight::none;
        for (std::size_t c = 0; c < ColumnCount; ++c) {
            if (!row.values[c].has_value()) {
                line[c].text = "-";
                continue;
            }

            const auto value = *row.values[c];
            std::ostringstream target_cost_p50;
            target_cost_p50 << std::fixed << std::setprecision(1) << value.target_cost.p50;

            std::ostringstream base_cost_p50;
            base_cost_p50 << std::fixed << std::setprecision(1) << value.base_cost.p50;

            std::ostringstream ratio_p25;
            ratio_p25 << std::fixed << std::setprecision(2) << value.ratio.p25;

            std::ostringstream ratio_p50;
            ratio_p50 << std::fixed << std::setprecision(2) << value.ratio.p50;

            line[c].target_cost_p50 = target_cost_p50.str();
            line[c].base_cost_p50 = base_cost_p50.str();
            line[c].ratio_p25 = ratio_p25.str();
            line[c].ratio_p50 = ratio_p50.str();
            line[c].highlight_red = color && value.ratio.p25 > warning_ratio;
            line[c].highlight_yellow = color && !line[c].highlight_red && value.ratio.p50 > warning_ratio;

            if (line[c].highlight_red) {
                highlight = row_highlight::red;
            } else if (highlight != row_highlight::red && line[c].highlight_yellow) {
                highlight = row_highlight::yellow;
            }

            if (show_quantiles) {
                line[c].text = line[c].target_cost_p50;
                line[c].text.append(" / ");
                line[c].text.append(line[c].base_cost_p50);
                line[c].text.append(" ~ ");
                line[c].text.append(line[c].ratio_p50);
                line[c].text.append(" (");
                line[c].text.append(line[c].ratio_p25);
                line[c].text.push_back(')');
            } else {
                line[c].text = line[c].target_cost_p50;
                line[c].text.push_back('/');
                line[c].text.append(line[c].ratio_p50);
            }
        }
        rendered.push_back(std::move(line));
        row_highlights.push_back(highlight);
    }

    for (const auto& row : rendered) {
        for (std::size_t c = 0; c < ColumnCount; ++c) {
            widths[c] = std::max<std::size_t>(widths[c], row[c].text.size());
        }
    }

    out << "\n" << spec.title << "\n";
    if (show_quantiles) {
        out << "quantiles: cost target[p50] / cost base[p50] ~ ratio[p50] (p25)\n";
        out << "note: ratio is the median of per-repeat ratios, not target[p50] / base[p50]\n";
        out << "legend: red=ratio p25>1.2; yellow=ratio p25<=1.2 && ratio p50>1.2\n";
        out << "color: red=whole cell, yellow=ratio p50 only\n";
    }

    out << std::left << std::setw(static_cast<int>(row_width)) << "measurement";
    out << std::string(spec.gap, ' ');
    out << std::left << std::setw(static_cast<int>(runs_col_width)) << "iters";
    out << std::string(spec.gap, ' ');
    for (std::size_t c = 0; c < ColumnCount; ++c) {
        out << std::left << std::setw(static_cast<int>(widths[c])) << spec.columns[c];
        if (c + 1 < ColumnCount) {
            if (show_quantiles) {
                out << " | ";
            } else {
                out << (c == spec.separator_after ? " | " : std::string(spec.gap, ' '));
            }
        }
    }
    out << "\n";

    std::size_t separator_width = row_width + spec.gap;
    separator_width += runs_col_width + spec.gap;
    for (std::size_t c = 0; c < ColumnCount; ++c) {
        separator_width += widths[c] + (c + 1 < ColumnCount ? spec.gap : 0);
    }
    out << std::string(separator_width, '-') << "\n";

    out << std::fixed << std::setprecision(3);
    for (std::size_t r = 0; r < rows.size(); ++r) {
        const auto& row = rows[r];
        const auto& cells = rendered[r];
        const auto highlight = row_highlights[r];

        const std::size_t row_padding = row_width > row.name.size() ? row_width - row.name.size() : 0;
        if (highlight == row_highlight::red) {
            out << red << row.name << reset;
        } else if (highlight == row_highlight::yellow) {
            out << yellow << row.name << reset;
        } else {
            out << row.name;
        }
        out << std::string(row_padding, ' ');
        out << std::string(spec.gap, ' ');
        const auto runs = row.iterations;
        const std::size_t runs_padding = runs_col_width > std::to_string(runs).size() ? runs_col_width - std::to_string(runs).size() : 0;
        out << std::string(runs_padding, ' ') << runs;
        out << std::string(spec.gap, ' ');
        for (std::size_t c = 0; c < ColumnCount; ++c) {
            const auto& cell = cells[c];
            const std::size_t padding = widths[c] > cell.text.size() ? widths[c] - cell.text.size() : 0;
            out << std::string(padding, ' ');
            if (cell.highlight_red) {
                out << red << cell.text << reset;
            } else if (show_quantiles && cell.highlight_yellow) {
                out << cell.target_cost_p50
                    << " / "
                    << cell.base_cost_p50
                    << " ~ "
                    << yellow << cell.ratio_p50 << reset
                    << " ("
                    << cell.ratio_p25
                    << ")";
            } else if (!show_quantiles && cell.highlight_yellow) {
                out << yellow << cell.text << reset;
            } else {
                out << cell.text;
            }
            if (c + 1 < ColumnCount) {
                if (show_quantiles) {
                    out << " | ";
                } else {
                    out << (c == spec.separator_after ? " | " : std::string(spec.gap, ' '));
                }
            }
        }
        out << "\n";
    }
}

template<class MeasureFn, class TargetFn, class TargetAfter, class BaseFn, class BaseAfter>
measurement measure_ratio(
    int repeats,
    int iteration_count,
    MeasureFn&& measure_fn,
    TargetFn&& target_fn,
    TargetAfter&& target_after,
    BaseFn&& base_fn,
    BaseAfter&& base_after)
{
    std::vector<double> cost_samples;
    cost_samples.reserve(static_cast<std::size_t>(repeats));
    std::vector<double> base_cost_samples;
    base_cost_samples.reserve(static_cast<std::size_t>(repeats));
    std::vector<double> delta_cost_samples;
    delta_cost_samples.reserve(static_cast<std::size_t>(repeats));
    std::vector<double> ratio_samples;
    ratio_samples.reserve(static_cast<std::size_t>(repeats));

    for (int r = 0; r < repeats; ++r) {
        double contract_sample = 0.0;
        double manual_sample = 0.0;

        if ((r & 1) == 0) {
            contract_sample = static_cast<double>(measure_fn(target_fn)) / static_cast<double>(iteration_count);
            target_after();
            manual_sample = static_cast<double>(measure_fn(base_fn)) / static_cast<double>(iteration_count);
            base_after();
        } else {
            manual_sample = static_cast<double>(measure_fn(base_fn)) / static_cast<double>(iteration_count);
            base_after();
            contract_sample = static_cast<double>(measure_fn(target_fn)) / static_cast<double>(iteration_count);
            target_after();
        }

        cost_samples.push_back(contract_sample);
        base_cost_samples.push_back(manual_sample);
        delta_cost_samples.push_back(contract_sample - manual_sample);
        ratio_samples.push_back(manual_sample > 0.0 ? contract_sample / manual_sample : 0.0);
    }

    std::sort(cost_samples.begin(), cost_samples.end());
    std::sort(base_cost_samples.begin(), base_cost_samples.end());
    std::sort(delta_cost_samples.begin(), delta_cost_samples.end());
    std::sort(ratio_samples.begin(), ratio_samples.end());

    const quantiles target_cost{
        cost_samples[cost_samples.size() / 4],
        cost_samples[cost_samples.size() / 2],
        cost_samples[(cost_samples.size() * 3) / 4],
    };
    const quantiles base_cost{
        base_cost_samples[base_cost_samples.size() / 4],
        base_cost_samples[base_cost_samples.size() / 2],
        base_cost_samples[(base_cost_samples.size() * 3) / 4],
    };
    const quantiles delta_cost{
        delta_cost_samples[delta_cost_samples.size() / 4],
        delta_cost_samples[delta_cost_samples.size() / 2],
        delta_cost_samples[(delta_cost_samples.size() * 3) / 4],
    };
    const quantiles ratio{
        ratio_samples[ratio_samples.size() / 4],
        ratio_samples[ratio_samples.size() / 2],
        ratio_samples[(ratio_samples.size() * 3) / 4],
    };
    return {target_cost, base_cost, delta_cost, ratio};
}

template<class MeasureFn, class TargetBody, class TargetAfter, class BaseBody, class BaseAfter>
measurement loop_ratio(
    int repeats,
    int iteration_count,
    const std::vector<std::size_t>& order,
    MeasureFn&& measure_fn,
    TargetBody&& target_body,
    TargetAfter&& target_after,
    BaseBody&& base_body,
    BaseAfter&& base_after)
{
    return measure_ratio(
        repeats,
        iteration_count,
        std::forward<MeasureFn>(measure_fn),
        [&] {
            for (int i = 0; i < iteration_count; ++i) {
                target_body(i, selection_slot(order, i));
            }
        },
        std::forward<TargetAfter>(target_after),
        [&] {
            for (int i = 0; i < iteration_count; ++i) {
                base_body(i, selection_slot(order, i));
            }
        },
        std::forward<BaseAfter>(base_after));
}

} // namespace benchmark_base
