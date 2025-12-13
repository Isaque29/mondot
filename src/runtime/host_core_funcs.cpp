#include "host_core_funcs.h"
#include <cstdio>
#include <charconv>
#include <random>
#include <chrono>
#include <iostream>
#include <limits>
#include <mutex>
#include <thread>
#include <algorithm>
#include <cmath>
#include <fstream>

namespace mondot_host
{
    static inline std::mutex io_mtx;

    static inline void format_value_to_string(const Value &v, std::string &out)
    {
        switch (v.tag)
        {
            case Tag::Number: {
                char buf[64];
                int n = std::snprintf(buf, sizeof(buf), "%.15g", v.num);
                if (n > 0) out.append(buf, (size_t)n);
                else out.append("<badnum>");
                break;
            }
            case Tag::String:
                out.append(*v.s);
                break;
            case Tag::Boolean:
                out.append(v.boolean ? "true" : "false");
                break;
            case Tag::Nil:
                out.append("nil");
                break;
            default:
                out.append("<val>");
                break;
        }
    }

    static inline std::string fast_to_string(const Value &v)
    {
        switch (v.tag) {
            case Tag::Number: {
                char buf[32];
                int n = std::snprintf(buf, sizeof(buf), "%.15g", v.num);
                if (n > 0) return std::string(buf, buf + n);
                return std::string("<badnum>");
            }
            case Tag::String:
                return *v.s;
            case Tag::Boolean:
                return v.boolean ? "true" : "false";
            case Tag::Nil:
                return "nil";
            default:
                return "<val>";
        }
    }

    static inline void fast_print_multi(const std::vector<Value> &args, bool add_newline, bool do_flush)
    {
        std::string buf;
        if (args.empty() && add_newline)
            buf = "nil\n";
        else
        {
            buf.reserve(args.size() * 16 + 32);
            for (size_t i = 0; i < args.size(); ++i)
            {
                format_value_to_string(args[i], buf);
                if (i + 1 < args.size()) buf.push_back(' ');
            }
            if (add_newline) buf.push_back('\n');
        }

        {
            std::lock_guard<std::mutex> lk(io_mtx);
            std::cout.write(buf.data(), static_cast<std::streamsize>(buf.size()));
            if (do_flush) {
                std::cout.flush();
                std::cerr.flush();
                std::fflush(nullptr);
            }
        }
    }

    RegisteredFunctionGuard::RegisteredFunctionGuard(HostBridge *h, std::string n)
        : host(h), name(std::move(n)) {}

    RegisteredFunctionGuard::~RegisteredFunctionGuard()
    {
        if (host && !name.empty()) host->unregister_function(name);
    }
    RegisteredFunctionGuard::RegisteredFunctionGuard(RegisteredFunctionGuard&& o) noexcept
        : host(o.host), name(std::move(o.name))
    {
        o.host = nullptr;
        o.name.clear();
    }
    RegisteredFunctionGuard& RegisteredFunctionGuard::operator=(RegisteredFunctionGuard&& o) noexcept
    {
        if (this != &o) {
            if (host && !name.empty()) host->unregister_function(name);
            host = o.host;
            name = std::move(o.name);
            o.host = nullptr;
            o.name.clear();
        }
        return *this;
    }
    RegisteredFunctionGuard RegisteredFunctionGuard::create(HostBridge *h, const std::string &name, const HostFn &fn)
    {
        if (!h) return RegisteredFunctionGuard();
        h->register_function(name, fn);
        return RegisteredFunctionGuard(h, name);
    }

    void register_core_host_functions(HostBridge &host)
    {
        // IO
        host.register_function("io.print", [](const std::vector<Value> &args)->Value {
            fast_print_multi(args, true, true);
            return Value::make_nil();
        });

        host.register_function("io.println", [](const std::vector<Value> &args)->Value {
            fast_print_multi(args, true, true);
            return Value::make_nil();
        });

        host.register_function("io.write", [](const std::vector<Value> &args)->Value {
            if (args.empty()) return Value::make_nil();
            std::string s;
            format_value_to_string(args[0], s);
            {
                std::lock_guard<std::mutex> lk(io_mtx);
                std::cout.write(s.data(), static_cast<std::streamsize>(s.size()));
            }
            return Value::make_nil();
        });

        host.register_function("io.writeln", [](const std::vector<Value> &args)->Value {
            if (args.empty()) {
                std::lock_guard<std::mutex> lk(io_mtx);
                std::cout.put('\n');
                return Value::make_nil();
            }
            std::string s;
            format_value_to_string(args[0], s);
            s.push_back('\n');
            {
                std::lock_guard<std::mutex> lk(io_mtx);
                std::cout.write(s.data(), static_cast<std::streamsize>(s.size()));
            }
            return Value::make_nil();
        });

        host.register_function("io.flush", [](const std::vector<Value> &args)->Value {
            std::lock_guard<std::mutex> lk(io_mtx);
            std::cout.flush();
            std::cerr.flush();
            std::fflush(nullptr);
            return Value::make_nil();
        });

        host.register_function("io.set_auto_flush", [](const std::vector<Value> &args)->Value {
            bool on = false;
            if (!args.empty() && args[0].tag == Tag::Number) on = (args[0].num != 0.0);
            if (on) {
                std::cout.setf(std::ios::unitbuf);
                std::cerr.setf(std::ios::unitbuf);
            } else {
                std::cout.unsetf(std::ios::unitbuf);
                std::cerr.unsetf(std::ios::unitbuf);
            }
            return Value::make_nil();
        });

        host.register_function("io.flush_and_exit", [](const std::vector<Value> &args)->Value {
            int code = 0;
            if (!args.empty() && args[0].tag == Tag::Number) code = static_cast<int>(args[0].num);
            {
                std::lock_guard<std::mutex> lk(io_mtx);
                std::cout.flush();
                std::cerr.flush();
                std::fflush(nullptr);
            }
            std::exit(code);
            return Value::make_nil();
        });

        // Strings & introspection
        host.register_function("strlen", [](const std::vector<Value> &args)->Value {
            if (!args.empty() && args[0].tag == Tag::String)
                return Value::make_number(static_cast<double>(args[0].s->size()));
            return Value::make_number(0.0);
        });

        host.register_function("len", [](const std::vector<Value> &args)->Value {
            if (args.empty()) return Value::make_number(0.0);
            const Value &v = args[0];
            switch (v.tag) {
                case Tag::String: return Value::make_number(static_cast<double>(v.s->size()));
                // If you have arrays/objects, add cases here.
                default: return Value::make_number(0.0);
            }
        });

        host.register_function("str_char_at", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2 && args[0].tag == Tag::String && args[1].tag == Tag::Number) {
                int idx = static_cast<int>(args[1].num);
                const std::string &s = *args[0].s;
                if (idx >= 0 && idx < static_cast<int>(s.size())) {
                    std::string r;
                    r.reserve(1);
                    r.push_back(s[idx]);
                    return Value::make_string(std::move(r));
                }
            }
            return Value::make_string(std::string());
        });

        host.register_function("tostring", [](const std::vector<Value> &args)->Value {
            if (args.empty()) return Value::make_string(std::string("nil"));
            return Value::make_string(fast_to_string(args[0]));
        });

        host.register_function("typeof", [](const std::vector<Value> &args)->Value {
            if (args.empty()) return Value::make_string(std::string("nil"));
            switch (args[0].tag) {
                case Tag::Number:  return Value::make_string(std::string("number"));
                case Tag::String:  return Value::make_string(std::string("string"));
                case Tag::Boolean: return Value::make_string(std::string("boolean"));
                case Tag::Nil:     return Value::make_string(std::string("nil"));
                default:           return Value::make_string(std::string("object"));
            }
        });

        // Arithmetic & string concat
        host.register_function("add", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2) {
                const Value &a = args[0];
                const Value &b = args[1];
                if (a.tag == Tag::Number && b.tag == Tag::Number)
                    return Value::make_number(a.num + b.num);
                if (a.tag == Tag::String && b.tag == Tag::String) {
                    const std::string &sa = *a.s;
                    const std::string &sb = *b.s;
                    std::string out;
                    out.reserve(sa.size() + sb.size());
                    out.append(sa);
                    out.append(sb);
                    return Value::make_string(std::move(out));
                }
                std::string out = fast_to_string(a);
                out.append(fast_to_string(b));
                return Value::make_string(std::move(out));
            }
            return Value::make_number(0.0);
        });

        host.register_function("sub", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2 && args[0].tag == Tag::Number && args[1].tag == Tag::Number)
                return Value::make_number(args[0].num - args[1].num);
            return Value::make_number(0.0);
        });

        host.register_function("mul", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2 && args[0].tag == Tag::Number && args[1].tag == Tag::Number)
                return Value::make_number(args[0].num * args[1].num);
            return Value::make_number(0.0);
        });

        host.register_function("div", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2 && args[0].tag == Tag::Number && args[1].tag == Tag::Number) {
                double d = args[1].num;
                if (d != 0.0) return Value::make_number(args[0].num / d);
            }
            return Value::make_number(0.0);
        });

        host.register_function("lt", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2 && args[0].tag == Tag::Number && args[1].tag == Tag::Number)
                return Value::make_number(args[0].num < args[1].num ? 1.0 : 0.0);
            return Value::make_number(0.0);
        });

        host.register_function("gt", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2 && args[0].tag == Tag::Number && args[1].tag == Tag::Number)
                return Value::make_number(args[0].num > args[1].num ? 1.0 : 0.0);
            return Value::make_number(0.0);
        });

        host.register_function("eq", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2) {
                const Value &a = args[0], &b = args[1];
                if (a.tag != b.tag) return Value::make_number(0.0);
                switch (a.tag) {
                    case Tag::Number: return Value::make_number(a.num == b.num ? 1.0 : 0.0);
                    case Tag::String: return Value::make_number((*a.s == *b.s) ? 1.0 : 0.0);
                    case Tag::Boolean: return Value::make_number(a.boolean == b.boolean ? 1.0 : 0.0);
                    case Tag::Nil: return Value::make_number(1.0);
                    default: return Value::make_number(0.0);
                }
            }
            return Value::make_number(0.0);
        });

        host.register_function("neq", [](const std::vector<Value> &args)->Value {
            if (args.size() < 2) return Value::make_number(0.0);
            const Value &a = args[0];
            const Value &b = args[1];
            if (a.tag != b.tag) return Value::make_number(1.0);
            switch (a.tag) {
                case Tag::Number: return Value::make_number(a.num != b.num ? 1.0 : 0.0);
                case Tag::String: return Value::make_number((*a.s != *b.s) ? 1.0 : 0.0);
                case Tag::Boolean: return Value::make_number(a.boolean != b.boolean ? 1.0 : 0.0);
                case Tag::Nil: return Value::make_number(0.0);
                default: return Value::make_number(1.0);
            }
        });

        // bitwise helpers (treat numbers as int64)
        host.register_function("shift", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2 && args[0].tag == Tag::Number && args[1].tag == Tag::Number) {
                int64_t a = static_cast<int64_t>(args[0].num);
                int64_t b = static_cast<int64_t>(args[1].num);
                if (b >= 0 && b < 63) return Value::make_number(static_cast<double>(a << b));
            }
            return Value::make_number(0.0);
        });

        host.register_function("bitwise", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2 && args[0].tag == Tag::Number && args[1].tag == Tag::Number) {
                int64_t a = static_cast<int64_t>(args[0].num);
                int64_t b = static_cast<int64_t>(args[1].num);
                if (b >= 0 && b < 63) return Value::make_number(static_cast<double>(a >> b));
            }
            return Value::make_number(0.0);
        });

        // conversions / parsing
        host.register_function("tonumber", [](const std::vector<Value> &args)->Value {
            if (args.empty()) return Value::make_number(0.0);
            const Value &v = args[0];
            if (v.tag == Tag::Number) return Value::make_number(v.num);
            if (v.tag == Tag::String) {
                const std::string &s = *v.s;
                // try std::from_chars
                double out = 0.0;
                // from_chars for double isn't fully supported portably -> use std::strtod fallback
                char *end = nullptr;
                const char *cstr = s.c_str();
                errno = 0;
                double val = std::strtod(cstr, &end);
                if (end != cstr && errno == 0) return Value::make_number(val);
            }
            return Value::make_number(0.0);
        });

        host.register_function("toint", [](const std::vector<Value> &args)->Value {
            if (args.empty()) return Value::make_number(0.0);
            const Value &v = args[0];
            if (v.tag == Tag::Number) return Value::make_number(std::floor(v.num));
            if (v.tag == Tag::String) {
                const std::string &s = *v.s;
                char *end = nullptr;
                long val = std::strtol(s.c_str(), &end, 10);
                if (end != s.c_str()) return Value::make_number(static_cast<double>(val));
            }
            return Value::make_number(0.0);
        });

        // simple math helpers
        host.register_function("floor", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 1 && args[0].tag == Tag::Number) return Value::make_number(std::floor(args[0].num));
            return Value::make_number(0.0);
        });
        host.register_function("ceil", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 1 && args[0].tag == Tag::Number) return Value::make_number(std::ceil(args[0].num));
            return Value::make_number(0.0);
        });
        host.register_function("abs", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 1 && args[0].tag == Tag::Number) return Value::make_number(std::fabs(args[0].num));
            return Value::make_number(0.0);
        });
        host.register_function("min", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2 && args[0].tag==Tag::Number && args[1].tag==Tag::Number)
                return Value::make_number(std::min(args[0].num, args[1].num));
            return Value::make_number(0.0);
        });
        host.register_function("max", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2 && args[0].tag==Tag::Number && args[1].tag==Tag::Number)
                return Value::make_number(std::max(args[0].num, args[1].num));
            return Value::make_number(0.0);
        });
        host.register_function("pow", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2 && args[0].tag==Tag::Number && args[1].tag==Tag::Number)
                return Value::make_number(std::pow(args[0].num, args[1].num));
            return Value::make_number(0.0);
        });
        host.register_function("sqrt", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 1 && args[0].tag==Tag::Number)
                return Value::make_number(std::sqrt(args[0].num));
            return Value::make_number(0.0);
        });
        host.register_function("sin", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 1 && args[0].tag==Tag::Number) return Value::make_number(std::sin(args[0].num));
            return Value::make_number(0.0);
        });
        host.register_function("cos", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 1 && args[0].tag==Tag::Number) return Value::make_number(std::cos(args[0].num));
            return Value::make_number(0.0);
        });
        host.register_function("tan", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 1 && args[0].tag==Tag::Number) return Value::make_number(std::tan(args[0].num));
            return Value::make_number(0.0);
        });
        host.register_function("log", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 1 && args[0].tag==Tag::Number) return Value::make_number(std::log(args[0].num));
            return Value::make_number(0.0);
        });
        host.register_function("exp", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 1 && args[0].tag==Tag::Number) return Value::make_number(std::exp(args[0].num));
            return Value::make_number(0.0);
        });
    }

    void register_extra_host_functions(HostBridge &host)
    {
        host.register_function("io.input", [](const std::vector<Value> &args)->Value {
            std::string line;
            if (!std::getline(std::cin, line)) return Value::make_string(std::string());
            return Value::make_string(std::move(line));
        });

        host.register_function("sleep_ms", [](const std::vector<Value> &args)->Value {
            if (!args.empty() && args[0].tag == Tag::Number) {
                int ms = static_cast<int>(args[0].num);
                if (ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
            return Value::make_nil();
        });

        host.register_function("time_ms", [](const std::vector<Value> &args)->Value {
            using namespace std::chrono;
            auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            return Value::make_number(static_cast<double>(now));
        });

        // random: thread_local rng
        host.register_function("rand", [](const std::vector<Value> &args)->Value {
            thread_local std::mt19937_64 rng_local((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            return Value::make_number(dist(rng_local));
        });

        host.register_function("substr", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2 && args[0].tag == Tag::String && args[1].tag == Tag::Number) {
                const std::string &s = *args[0].s;
                int start = static_cast<int>(args[1].num);
                if (start < 0) start = 0;
                if (start >= (int)s.size()) return Value::make_string(std::string());
                size_t len = s.size() - start;
                if (args.size() >= 3 && args[2].tag == Tag::Number) {
                    int l = static_cast<int>(args[2].num);
                    if (l >= 0) len = static_cast<size_t>(std::min<int>(l, static_cast<int>(len)));
                }
                return Value::make_string(s.substr(static_cast<size_t>(start), len));
            }
            return Value::make_string(std::string());
        });

        host.register_function("index_of", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2 && args[0].tag == Tag::String && args[1].tag == Tag::String) {
                const std::string &s = *args[0].s, &sub = *args[1].s;
                size_t pos = s.find(sub);
                if (pos == std::string::npos) return Value::make_number(-1.0);
                return Value::make_number(static_cast<double>(pos));
            }
            return Value::make_number(-1.0);
        });

        host.register_function("read_file", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 1 && args[0].tag == Tag::String) {
                const std::string &path = *args[0].s;
                std::ifstream ifs(path, std::ios::binary);
                if (!ifs) return Value::make_string(std::string());
                std::string out;
                ifs.seekg(0, std::ios::end);
                std::streampos sz = ifs.tellg();
                if (sz > 0) {
                    out.resize(static_cast<size_t>(sz));
                    ifs.seekg(0, std::ios::beg);
                    ifs.read(&out[0], sz);
                }
                return Value::make_string(std::move(out));
            }
            return Value::make_string(std::string());
        });

        host.register_function("write_file", [](const std::vector<Value> &args)->Value {
            if (args.size() >= 2 && args[0].tag == Tag::String && args[1].tag == Tag::String) {
                const std::string &path = *args[0].s;
                const std::string &content = *args[1].s;
                std::ofstream ofs(path, std::ios::binary);
                if (!ofs) return Value::make_number(0.0);
                ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
                if (!ofs) return Value::make_number(0.0);
                return Value::make_number(1.0);
            }
            return Value::make_number(0.0);
        });
    }
}
