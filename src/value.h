#ifndef MONDOT_VALUE_H
#define MONDOT_VALUE_H

#include <memory>
#include <string>
#include <cstdint>

struct Rule { uint16_t type = 0; uint32_t id = 0; };

enum class Tag { Nil, Number, String, Rule };

struct Value {
    Tag tag = Tag::Nil;
    double num = 0.0;
    std::shared_ptr<std::string> s;
    std::shared_ptr<Rule> r;

    Value() = default;
    static Value make_nil();
    static Value make_number(double n);
    static Value make_string(const std::string &str);
    static Value make_rule(const Rule &rule);
};

std::string value_to_string(const Value &v);

#endif // MONDOT_VALUE_H
