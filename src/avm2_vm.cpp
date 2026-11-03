/*
 * MIT License
 *
 * Copyright (c) 2026 MiokoTech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "avm2_vm.h"
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <limits>
#include <chrono>
#include <algorithm>
#include <sstream>

namespace avm2 {

/* helper bikin object value */
Avm2Value Avm2Value::object(std::shared_ptr<Avm2Object> o) {
    Avm2Value v;
    v.type = Avm2Type::Object;
    v.objVal = std::move(o);
    return v;
}

bool Avm2Value::toBool() const {
    switch (type) {
        case Avm2Type::Bool: return bVal;
        case Avm2Type::Int: return iVal != 0;
        case Avm2Type::Double: return dVal != 0.0 && !std::isnan(dVal);
        case Avm2Type::String: return !sVal.empty() && sVal != "false" && sVal != "0";
        case Avm2Type::Object:
        case Avm2Type::Array: return objVal != nullptr;
        default: return false;
    }
}

int32_t Avm2Value::toInt() const {
    switch (type) {
        case Avm2Type::Bool: return bVal ? 1 : 0;
        case Avm2Type::Int: return iVal;
        case Avm2Type::Double: {
            if (std::isnan(dVal) || std::isinf(dVal)) return 0;
            return static_cast<int32_t>(dVal);
        }
        case Avm2Type::String: {
            double d = toDouble();
            if (std::isnan(d) || std::isinf(d)) return 0;
            return static_cast<int32_t>(d);
        }
        default: return 0;
    }
}

double Avm2Value::toDouble() const {
    switch (type) {
        case Avm2Type::Undefined: return std::numeric_limits<double>::quiet_NaN();
        case Avm2Type::Null: return 0.0;
        case Avm2Type::Bool: return bVal ? 1.0 : 0.0;
        case Avm2Type::Int: return static_cast<double>(iVal);
        case Avm2Type::Double: return dVal;
        case Avm2Type::String: {
            if (sVal.empty()) return 0.0;
            try {
                size_t idx = 0;
                double val = std::stod(sVal, &idx);
                while (idx < sVal.size() && (sVal[idx] == ' ' || sVal[idx] == '\t' || sVal[idx] == '\r' || sVal[idx] == '\n')) {
                    idx++;
                }
                if (idx < sVal.size()) return std::numeric_limits<double>::quiet_NaN();
                return val;
            } catch (...) {
                return std::numeric_limits<double>::quiet_NaN();
            }
        }
        default: return std::numeric_limits<double>::quiet_NaN();
    }
}

std::string Avm2Value::toString() const {
    switch (type) {
        case Avm2Type::Undefined: return "undefined";
        case Avm2Type::Null: return "null";
        case Avm2Type::Bool: return bVal ? "true" : "false";
        case Avm2Type::Int: return std::to_string(iVal);
        case Avm2Type::Double: {
            if (std::isnan(dVal)) return "NaN";
            if (std::isinf(dVal)) return dVal > 0 ? "Infinity" : "-Infinity";
            if (dVal == static_cast<double>(static_cast<int64_t>(dVal))) {
                return std::to_string(static_cast<int64_t>(dVal));
            }
            std::ostringstream ss;
            ss << dVal;
            return ss.str();
        }
        case Avm2Type::String: return sVal;
        case Avm2Type::Object: return "[object Object]";
        case Avm2Type::Array: return "[object Array]";
        default: return "";
    }
}

bool Avm2Value::equals(const Avm2Value& other) const {
    if (type == other.type) return strictEquals(other);
    if (isNullOrUndefined() && other.isNullOrUndefined()) return true;
    if (isNumber() && other.isNumber()) return toDouble() == other.toDouble();
    if (isString() || other.isString()) return toString() == other.toString();
    return false;
}

bool Avm2Value::strictEquals(const Avm2Value& other) const {
    if (type != other.type) return false;
    switch (type) {
        case Avm2Type::Undefined:
        case Avm2Type::Null: return true;
        case Avm2Type::Bool: return bVal == other.bVal;
        case Avm2Type::Int: return iVal == other.iVal;
        case Avm2Type::Double: return dVal == other.dVal;
        case Avm2Type::String: return sVal == other.sVal;
        case Avm2Type::Object:
        case Avm2Type::Array: return objVal == other.objVal;
    }
    return false;
}

bool Avm2Value::lessThan(const Avm2Value& other) const {
    return toDouble() < other.toDouble();
}

bool Avm2Value::lessEquals(const Avm2Value& other) const {
    return toDouble() <= other.toDouble();
}

bool Avm2Value::greaterThan(const Avm2Value& other) const {
    return toDouble() > other.toDouble();
}

bool Avm2Value::greaterEquals(const Avm2Value& other) const {
    return toDouble() >= other.toDouble();
}

/* Operasi aritmatika & concat string ala ActionScript */
Avm2Value Avm2Value::add(const Avm2Value& a, const Avm2Value& b) {
    if (a.isString() || b.isString()) {
        return Avm2Value::string(a.toString() + b.toString());
    }
    if (a.type == Avm2Type::Double || b.type == Avm2Type::Double) {
        return Avm2Value::number(a.toDouble() + b.toDouble());
    }
    return Avm2Value::integer(a.toInt() + b.toInt());
}

Avm2Value Avm2Value::subtract(const Avm2Value& a, const Avm2Value& b) {
    if (a.type == Avm2Type::Double || b.type == Avm2Type::Double) {
        return Avm2Value::number(a.toDouble() - b.toDouble());
    }
    return Avm2Value::integer(a.toInt() - b.toInt());
}

Avm2Value Avm2Value::multiply(const Avm2Value& a, const Avm2Value& b) {
    if (a.type == Avm2Type::Double || b.type == Avm2Type::Double) {
        return Avm2Value::number(a.toDouble() * b.toDouble());
    }
    return Avm2Value::integer(a.toInt() * b.toInt());
}

Avm2Value Avm2Value::divide(const Avm2Value& a, const Avm2Value& b) {
    double denom = b.toDouble();
    if (denom == 0.0) return Avm2Value::number(0.0);
    return Avm2Value::number(a.toDouble() / denom);
}

Avm2Value Avm2Value::modulo(const Avm2Value& a, const Avm2Value& b) {
    int32_t denom = b.toInt();
    if (denom == 0) return Avm2Value::integer(0);
    return Avm2Value::integer(a.toInt() % denom);
}

/* Default dynamic object get/set */
Avm2Value Avm2Object::getProperty(const std::string& name) {
    if (name == "length") {
        /* note: array length di AS3 ngembaliin ukuran m_arrayItems kalau objeknya array */
        if (!m_arrayItems.empty() || properties.find("length") == properties.end()) {
            return Avm2Value::integer(static_cast<int32_t>(m_arrayItems.size()));
        }
    }
    auto it = properties.find(name);
    return (it != properties.end()) ? it->second : Avm2Value::undefined();
}

void Avm2Object::setProperty(const std::string& name, const Avm2Value& val) {
    properties[name] = val;
}

Avm2Value Avm2Object::callMethod(const std::string& name, const std::vector<Avm2Value>& args, void* /*hostContext*/) {
    /* note: method Array native AS3 (indexOf, push, pop) wajib ada di Avm2Object biar logika suMoveTarget ama hitsDamage jalan */
    if (name == "indexOf" && !args.empty()) {
        const auto& target = args[0];
        for (size_t i = 0; i < m_arrayItems.size(); ++i) {
            if (m_arrayItems[i].equals(target)) {
                return Avm2Value::integer(static_cast<int32_t>(i));
            }
        }
        return Avm2Value::integer(-1);
    }
    if (name == "push") {
        for (const auto& a : args) {
            m_arrayItems.push_back(a);
        }
        return Avm2Value::integer(static_cast<int32_t>(m_arrayItems.size()));
    }
    if (name == "pop") {
        if (m_arrayItems.empty()) return Avm2Value::undefined();
        auto val = m_arrayItems.back();
        m_arrayItems.pop_back();
        return val;
    }
    if (name == "shift") {
        if (m_arrayItems.empty()) return Avm2Value::undefined();
        auto val = m_arrayItems.front();
        m_arrayItems.erase(m_arrayItems.begin());
        return val;
    }
    if (name == "unshift") {
        for (auto it = args.rbegin(); it != args.rend(); ++it) {
            m_arrayItems.insert(m_arrayItems.begin(), *it);
        }
        return Avm2Value::integer(static_cast<int32_t>(m_arrayItems.size()));
    }
    if (name == "slice") {
        int sz = static_cast<int>(m_arrayItems.size());
        int start = args.empty() ? 0 : args[0].toInt();
        int end = (args.size() >= 2) ? args[1].toInt() : sz;
        if (start < 0) start = std::max(0, sz + start);
        if (end < 0) end = std::max(0, sz + end);
        start = std::clamp(start, 0, sz);
        end = std::clamp(end, start, sz);
        auto newArr = std::make_shared<Avm2Object>();
        newArr->m_arrayItems.assign(m_arrayItems.begin() + start, m_arrayItems.begin() + end);
        return Avm2Value::object(newArr);
    }
    if (name == "splice") {
        /* note: splice(start, deleteCount, ...items) standar AS3, ngembaliin array elemen yang dihapus */
        int sz = static_cast<int>(m_arrayItems.size());
        if (args.empty()) {
            return Avm2Value::object(std::make_shared<Avm2Object>());
        }
        int start = args[0].toInt();
        if (start < 0) start = std::max(0, sz + start);
        start = std::clamp(start, 0, sz);

        int delCount = (args.size() >= 2) ? args[1].toInt() : (sz - start);
        delCount = std::clamp(delCount, 0, sz - start);

        auto removedArr = std::make_shared<Avm2Object>();
        removedArr->m_arrayItems.assign(m_arrayItems.begin() + start, m_arrayItems.begin() + start + delCount);

        m_arrayItems.erase(m_arrayItems.begin() + start, m_arrayItems.begin() + start + delCount);

        if (args.size() > 2) {
            m_arrayItems.insert(m_arrayItems.begin() + start, args.begin() + 2, args.end());
        }
        return Avm2Value::object(removedArr);
    }
    if (name == "concat") {
        auto resultArr = std::make_shared<Avm2Object>();
        resultArr->m_arrayItems = m_arrayItems;
        for (const auto& a : args) {
            if (a.type == Avm2Type::Object && a.objVal && !a.objVal->m_arrayItems.empty()) {
                resultArr->m_arrayItems.insert(resultArr->m_arrayItems.end(), a.objVal->m_arrayItems.begin(), a.objVal->m_arrayItems.end());
            } else {
                resultArr->m_arrayItems.push_back(a);
            }
        }
        return Avm2Value::object(resultArr);
    }
    if (name == "join") {
        std::string delim = args.empty() ? "," : args[0].toString();
        std::string res;
        for (size_t i = 0; i < m_arrayItems.size(); ++i) {
            if (i > 0) res += delim;
            res += m_arrayItems[i].toString();
        }
        return Avm2Value::string(res);
    }
    if (name == "reverse") {
        std::reverse(m_arrayItems.begin(), m_arrayItems.end());
        return Avm2Value::undefined();
    }
    if (name == "hasOwnProperty" && !args.empty()) {
        std::string prop = args[0].toString();
        if (properties.find(prop) != properties.end()) {
            return Avm2Value::boolean(true);
        }
        try {
            int idx = std::stoi(prop);
            if (idx >= 0 && idx < static_cast<int>(m_arrayItems.size())) {
                return Avm2Value::boolean(true);
            }
        } catch (...) {}
        return Avm2Value::boolean(false);
    }
    if (name == "sort") {
        std::sort(m_arrayItems.begin(), m_arrayItems.end(), [](const Avm2Value& a, const Avm2Value& b) {
            if (a.isNumber() && b.isNumber()) {
                return a.toDouble() < b.toDouble();
            }
            return a.toString() < b.toString();
        });
        return Avm2Value::undefined();
    }
    if (name == "toString") {
        if (!m_arrayItems.empty()) {
            std::string res;
            for (size_t i = 0; i < m_arrayItems.size(); ++i) {
                if (i > 0) res += ",";
                res += m_arrayItems[i].toString();
            }
            return Avm2Value::string(res);
        }
        return Avm2Value::string("[object Object]");
    }
    if (name == "indexOf" && !args.empty()) {
        const auto& search = args[0];
        int from = (args.size() >= 2) ? args[1].toInt() : 0;
        int sz = static_cast<int>(m_arrayItems.size());
        if (from < 0) from = std::max(0, sz + from);
        for (int i = from; i < sz; ++i) {
            if (m_arrayItems[i].strictEquals(search) || m_arrayItems[i].equals(search)) {
                return Avm2Value::integer(i);
            }
        }
        return Avm2Value::integer(-1);
    }
    if (name == "lastIndexOf" && !args.empty()) {
        const auto& search = args[0];
        int sz = static_cast<int>(m_arrayItems.size());
        int from = (args.size() >= 2) ? args[1].toInt() : (sz - 1);
        if (from >= sz) from = sz - 1;
        for (int i = from; i >= 0; --i) {
            if (m_arrayItems[i].strictEquals(search) || m_arrayItems[i].equals(search)) {
                return Avm2Value::integer(i);
            }
        }
        return Avm2Value::integer(-1);
    }
    if (name == "intersects" && !args.empty() && args[0].type == Avm2Type::Object) {
        auto other = args[0].objVal;
        if (other) {
            double rx1 = getProperty("x").toDouble();
            double ry1 = getProperty("y").toDouble();
            double rw1 = getProperty("width").toDouble();
            double rh1 = getProperty("height").toDouble();

            double rx2 = other->getProperty("x").toDouble();
            double ry2 = other->getProperty("y").toDouble();
            double rw2 = other->getProperty("width").toDouble();
            double rh2 = other->getProperty("height").toDouble();

            bool noOverlap = (rx1 + rw1 <= rx2) || (rx2 + rw2 <= rx1) ||
                             (ry1 + rh1 <= ry2) || (ry2 + rh2 <= ry1);
            return Avm2Value::boolean(!noOverlap && rw1 > 0.0 && rh1 > 0.0 && rw2 > 0.0 && rh2 > 0.0);
        }
        return Avm2Value::boolean(false);
    }
    if (name == "valueOf") {
        return Avm2Value::undefined();
    }
    return Avm2Value::undefined();
}

namespace {

/* 
 * Modder bvn sering bener pake Math.abs, min, max buat rumus hitbox & tarikan gravitasi.
 * Sengaja di-mock langsung di C++ biar ga perlu link runtime Flash player 100mb.
 */
class MathMockObject : public Avm2Object {
public:
    Avm2Value getProperty(const std::string& name) override {
        if (name == "PI") return Avm2Value::number(3.141592653589793);
        if (name == "E") return Avm2Value::number(2.718281828459045);
        if (name == "LN10") return Avm2Value::number(2.302585092994046);
        if (name == "LN2") return Avm2Value::number(0.6931471805599453);
        if (name == "LOG10E") return Avm2Value::number(0.4342944819032518);
        if (name == "LOG2E") return Avm2Value::number(1.4426950408889634);
        if (name == "SQRT1_2") return Avm2Value::number(0.7071067811865476);
        if (name == "SQRT2") return Avm2Value::number(1.4142135623730951);
        return Avm2Object::getProperty(name);
    }

    Avm2Value callMethod(const std::string& name, const std::vector<Avm2Value>& args, void* hostContext) override {
        if (name == "min") {
            if (args.empty()) return Avm2Value::number(std::numeric_limits<double>::infinity());
            double val = args[0].toDouble();
            for (size_t i = 1; i < args.size(); ++i) {
                double v = args[i].toDouble();
                if (std::isnan(v)) return Avm2Value::number(v);
                val = std::min(val, v);
            }
            return Avm2Value::number(val);
        }
        if (name == "max") {
            if (args.empty()) return Avm2Value::number(-std::numeric_limits<double>::infinity());
            double val = args[0].toDouble();
            for (size_t i = 1; i < args.size(); ++i) {
                double v = args[i].toDouble();
                if (std::isnan(v)) return Avm2Value::number(v);
                val = std::max(val, v);
            }
            return Avm2Value::number(val);
        }
        if (name == "abs") {
            return args.empty() ? Avm2Value::number(0.0) : Avm2Value::number(std::abs(args[0].toDouble()));
        }
        if (name == "floor") {
            if (args.empty()) return Avm2Value::number(std::numeric_limits<double>::quiet_NaN());
            double d = args[0].toDouble();
            if (std::isnan(d)) return Avm2Value::number(d);
            return Avm2Value::number(std::floor(d));
        }
        if (name == "ceil") {
            if (args.empty()) return Avm2Value::number(std::numeric_limits<double>::quiet_NaN());
            double d = args[0].toDouble();
            if (std::isnan(d)) return Avm2Value::number(d);
            return Avm2Value::number(std::ceil(d));
        }
        if (name == "round") {
            if (args.empty()) return Avm2Value::number(std::numeric_limits<double>::quiet_NaN());
            double d = args[0].toDouble();
            if (std::isnan(d)) return Avm2Value::number(d);
            return Avm2Value::number(std::round(d));
        }
        if (name == "random") {
            return Avm2Value::number(static_cast<double>(rand()) / static_cast<double>(RAND_MAX));
        }
        if (name == "sqrt") {
            return args.empty() ? Avm2Value::number(0.0) : Avm2Value::number(std::sqrt(args[0].toDouble()));
        }
        if (name == "atan2" && args.size() >= 2) {
            return Avm2Value::number(std::atan2(args[0].toDouble(), args[1].toDouble()));
        }
        if (name == "sin") {
            return args.empty() ? Avm2Value::number(0.0) : Avm2Value::number(std::sin(args[0].toDouble()));
        }
        if (name == "cos") {
            return args.empty() ? Avm2Value::number(0.0) : Avm2Value::number(std::cos(args[0].toDouble()));
        }
        if (name == "tan") {
            return args.empty() ? Avm2Value::number(0.0) : Avm2Value::number(std::tan(args[0].toDouble()));
        }
        if (name == "asin") {
            return args.empty() ? Avm2Value::number(0.0) : Avm2Value::number(std::asin(args[0].toDouble()));
        }
        if (name == "acos") {
            return args.empty() ? Avm2Value::number(0.0) : Avm2Value::number(std::acos(args[0].toDouble()));
        }
        if (name == "atan") {
            return args.empty() ? Avm2Value::number(0.0) : Avm2Value::number(std::atan(args[0].toDouble()));
        }
        if (name == "pow" && args.size() >= 2) {
            return Avm2Value::number(std::pow(args[0].toDouble(), args[1].toDouble()));
        }
        if (name == "log") {
            return args.empty() ? Avm2Value::number(0.0) : Avm2Value::number(std::log(args[0].toDouble()));
        }
        if (name == "exp") {
            return args.empty() ? Avm2Value::number(0.0) : Avm2Value::number(std::exp(args[0].toDouble()));
        }
        return Avm2Object::callMethod(name, args, hostContext);
    }
};

static std::shared_ptr<MathMockObject> s_mathMock = std::make_shared<MathMockObject>();

/* Helper cari properti di target object atau cari ke this */
static Avm2Value resolveProperty(const Avm2Value& obj, const std::string& name, std::shared_ptr<Avm2Object> thisObj) {
    if (name == "Math") return Avm2Value::object(s_mathMock);
    if (obj.type == Avm2Type::String && name == "length") {
        return Avm2Value::integer(static_cast<int32_t>(obj.sVal.size()));
    }
    if (obj.type == Avm2Type::Object && obj.objVal) {
        return obj.objVal->getProperty(name);
    }
    if (thisObj) {
        return thisObj->getProperty(name);
    }
    return Avm2Value::undefined();
}

static void assignProperty(Avm2Value& obj, const std::string& name, const Avm2Value& val, std::shared_ptr<Avm2Object> thisObj) {
    if (obj.type == Avm2Type::Object && obj.objVal) {
        obj.objVal->setProperty(name, val);
        return;
    }
    if (thisObj) {
        thisObj->setProperty(name, val);
    }
}

static Avm2Value invokeMethod(Avm2Value& obj, const std::string& name, const std::vector<Avm2Value>& args, std::shared_ptr<Avm2Object> thisObj, void* hostContext) {
    /* String prototype methods */
    if (obj.type == Avm2Type::String) {
        const std::string& str = obj.sVal;
        if (name == "indexOf" && !args.empty()) {
            std::string sub = args[0].toString();
            size_t pos = str.find(sub);
            return Avm2Value::integer((pos != std::string::npos) ? static_cast<int32_t>(pos) : -1);
        }
        if (name == "substr" && !args.empty()) {
            int start = args[0].toInt();
            int len = (args.size() >= 2) ? args[1].toInt() : static_cast<int>(str.size());
            if (start < 0) start = std::max(0, static_cast<int>(str.size()) + start);
            if (start >= static_cast<int>(str.size()) || len <= 0) return Avm2Value::string("");
            return Avm2Value::string(str.substr(start, len));
        }
        if (name == "charAt" && !args.empty()) {
            int idx = args[0].toInt();
            if (idx >= 0 && idx < static_cast<int>(str.size())) {
                return Avm2Value::string(std::string(1, str[idx]));
            }
            return Avm2Value::string("");
        }
        if (name == "charCodeAt" && !args.empty()) {
            int idx = args[0].toInt();
            if (idx >= 0 && idx < static_cast<int>(str.size())) {
                return Avm2Value::integer(static_cast<uint8_t>(str[idx]));
            }
            return Avm2Value::number(0.0);
        }
        if (name == "substring" && !args.empty()) {
            int sz = static_cast<int>(str.size());
            int start = std::max(0, args[0].toInt());
            int end = (args.size() >= 2) ? std::max(0, args[1].toInt()) : sz;
            start = std::clamp(start, 0, sz);
            end = std::clamp(end, 0, sz);
            if (start > end) std::swap(start, end);
            return Avm2Value::string(str.substr(start, end - start));
        }
        if (name == "split") {
            auto arr = std::make_shared<Avm2Object>();
            if (args.empty() || args[0].isNullOrUndefined()) {
                arr->pushElement(Avm2Value::string(str));
                return Avm2Value::object(arr);
            }
            std::string delim = args[0].toString();
            if (delim.empty()) {
                for (char c : str) arr->pushElement(Avm2Value::string(std::string(1, c)));
            } else {
                size_t start = 0;
                size_t pos = 0;
                while ((pos = str.find(delim, start)) != std::string::npos) {
                    arr->pushElement(Avm2Value::string(str.substr(start, pos - start)));
                    start = pos + delim.size();
                }
                arr->pushElement(Avm2Value::string(str.substr(start)));
            }
            return Avm2Value::object(arr);
        }
        if (name == "toLowerCase") {
            std::string res = str;
            for (char& c : res) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return Avm2Value::string(res);
        }
        if (name == "toUpperCase") {
            std::string res = str;
            for (char& c : res) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return Avm2Value::string(res);
        }
        if (name == "replace" && args.size() >= 2) {
            std::string from = args[0].toString();
            std::string to = args[1].toString();
            size_t pos = str.find(from);
            if (pos != std::string::npos) {
                std::string res = str;
                res.replace(pos, from.length(), to);
                return Avm2Value::string(res);
            }
            return Avm2Value::string(str);
        }
    }

    /* Global helper functions */
    if (name == "isNaN" && !args.empty()) {
        return Avm2Value::boolean(std::isnan(args[0].toDouble()));
    }
    if (name == "isFinite" && !args.empty()) {
        return Avm2Value::boolean(std::isfinite(args[0].toDouble()));
    }
    if (name == "parseInt" && !args.empty()) {
        try {
            return Avm2Value::integer(std::stoi(args[0].toString()));
        } catch (...) {
            return Avm2Value::number(std::numeric_limits<double>::quiet_NaN());
        }
    }
    if (name == "parseFloat" && !args.empty()) {
        try {
            return Avm2Value::number(std::stod(args[0].toString()));
        } catch (...) {
            return Avm2Value::number(std::numeric_limits<double>::quiet_NaN());
        }
    }
    if (name == "getTimer") {
        static auto s_appStartTime = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        int32_t ms = static_cast<int32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now - s_appStartTime).count());
        return Avm2Value::integer(ms);
    }
    if (name == "escape" && !args.empty()) {
        const std::string& input = args[0].toString();
        std::string res;
        for (unsigned char c : input) {
            if (std::isalnum(c) || c == '@' || c == '*' || c == '_' || c == '+' || c == '-' || c == '.' || c == '/') {
                res += static_cast<char>(c);
            } else {
                char buf[4];
                snprintf(buf, sizeof(buf), "%%%02X", c);
                res += buf;
            }
        }
        return Avm2Value::string(res);
    }
    if (name == "unescape" && !args.empty()) {
        const std::string& input = args[0].toString();
        std::string res;
        for (size_t i = 0; i < input.size(); ++i) {
            if (input[i] == '%' && i + 2 < input.size() && std::isxdigit(input[i + 1]) && std::isxdigit(input[i + 2])) {
                int hexVal = std::stoi(input.substr(i + 1, 2), nullptr, 16);
                res += static_cast<char>(hexVal);
                i += 2;
            } else {
                res += input[i];
            }
        }
        return Avm2Value::string(res);
    }

    if (obj.type == Avm2Type::Object && obj.objVal) {
        return obj.objVal->callMethod(name, args, hostContext);
    }
    if (thisObj) {
        return thisObj->callMethod(name, args, hostContext);
    }
    return Avm2Value::undefined();
}

} // anon namespace

Avm2Interpreter& Avm2Interpreter::getInstance() {
    static Avm2Interpreter s_inst;
    return s_inst;
}

Avm2Value Avm2Interpreter::execute(
    const Avm2MethodBody& method,
    const Avm2ConstantPool& pool,
    std::shared_ptr<Avm2Object> thisObject,
    const std::vector<Avm2Value>& args,
    void* hostContext
) {
    if (method.code.empty()) return Avm2Value::undefined();

    const uint8_t* code = method.code.data();
    size_t codeLen = method.code.size();
    size_t pc = 0;

    std::vector<Avm2Value> stack;
    stack.reserve(std::max(16, static_cast<int>(method.maxStack)));

    /* Siapin register local variables */
    size_t numLocals = std::max(static_cast<size_t>(method.localCount), args.size() + 1);
    std::vector<Avm2Value> locals(numLocals, Avm2Value::undefined());

    /* locals[0] selalu `this` di ActionScript 3 */
    if (thisObject) {
        locals[0] = Avm2Value::object(thisObject);
    }
    for (size_t i = 0; i < args.size() && (i + 1) < locals.size(); ++i) {
        locals[i + 1] = args[i];
    }

    auto push = [&](Avm2Value v) {
        stack.push_back(std::move(v));
    };

    auto pop = [&]() -> Avm2Value {
        if (stack.empty()) return Avm2Value::undefined();
        Avm2Value v = std::move(stack.back());
        stack.pop_back();
        return v;
    };

    /* Parser u30 variable-length DoABC */
    auto readU30 = [&]() -> uint32_t {
        uint32_t res = 0;
        int shift = 0;
        while (pc < codeLen) {
            uint8_t b = code[pc++];
            res |= static_cast<uint32_t>(b & 0x7F) << shift;
            if ((b & 0x80) == 0) break;
            shift += 7;
        }
        return res;
    };

    /* Signed 24-bit branch offset */
    auto readS24 = [&]() -> int32_t {
        if (pc + 3 > codeLen) return 0;
        int32_t b0 = code[pc++];
        int32_t b1 = code[pc++];
        int32_t b2 = code[pc++];
        int32_t res = b0 | (b1 << 8) | (b2 << 16);
        if (res & 0x800000) res |= ~0xFFFFFF; // sign extend
        return res;
    };

    /* Loop bytecode execution */
    while (pc < codeLen) {
        uint8_t op = code[pc++];
        switch (op) {
            case 0x01: /* debugnop */
            case 0x02: /* nop */
            case 0x09: /* label */
                break;

            case 0x08: { /* kill */
                uint32_t r = readU30();
                if (r < locals.size()) locals[r] = Avm2Value::undefined();
                break;
            }

            /* Branching & jumps */
            case 0x0C: { /* ifnlt */
                int32_t offset = readS24();
                Avm2Value b = pop(), a = pop();
                if (!a.lessThan(b)) pc += offset;
                break;
            }
            case 0x0D: { /* ifnle */
                int32_t offset = readS24();
                Avm2Value b = pop(), a = pop();
                if (!a.lessEquals(b)) pc += offset;
                break;
            }
            case 0x0E: { /* ifngt */
                int32_t offset = readS24();
                Avm2Value b = pop(), a = pop();
                if (!a.greaterThan(b)) pc += offset;
                break;
            }
            case 0x0F: { /* ifnge */
                int32_t offset = readS24();
                Avm2Value b = pop(), a = pop();
                if (!a.greaterEquals(b)) pc += offset;
                break;
            }
            case 0x10: { /* jump */
                int32_t offset = readS24();
                pc += offset;
                break;
            }
            case 0x11: { /* iftrue */
                int32_t offset = readS24();
                Avm2Value cond = pop();
                if (cond.toBool()) pc += offset;
                break;
            }
            case 0x12: { /* iffalse */
                int32_t offset = readS24();
                Avm2Value cond = pop();
                if (!cond.toBool()) pc += offset;
                break;
            }
            case 0x13: { /* ifeq */
                int32_t offset = readS24();
                Avm2Value b = pop(), a = pop();
                if (a.equals(b)) pc += offset;
                break;
            }
            case 0x14: { /* ifne */
                int32_t offset = readS24();
                Avm2Value b = pop(), a = pop();
                if (!a.equals(b)) pc += offset;
                break;
            }
            case 0x15: { /* iflt */
                int32_t offset = readS24();
                Avm2Value b = pop(), a = pop();
                if (a.lessThan(b)) pc += offset;
                break;
            }
            case 0x16: { /* ifle */
                int32_t offset = readS24();
                Avm2Value b = pop(), a = pop();
                if (a.lessEquals(b)) pc += offset;
                break;
            }
            case 0x17: { /* ifgt */
                int32_t offset = readS24();
                Avm2Value b = pop(), a = pop();
                if (a.greaterThan(b)) pc += offset;
                break;
            }
            case 0x18: { /* ifge */
                int32_t offset = readS24();
                Avm2Value b = pop(), a = pop();
                if (a.greaterEquals(b)) pc += offset;
                break;
            }
            case 0x19: { /* ifstricteq */
                int32_t offset = readS24();
                Avm2Value b = pop(), a = pop();
                if (a.strictEquals(b)) pc += offset;
                break;
            }
            case 0x1A: { /* ifstrictne */
                int32_t offset = readS24();
                Avm2Value b = pop(), a = pop();
                if (!a.strictEquals(b)) pc += offset;
                break;
            }
            case 0x1B: { /* lookupswitch default_offset, case_count, case_offsets... */
                size_t basePc = pc - 1; // target offset dihitung relatif dari posisi opcode 0x1B
                int32_t defaultOffset = readS24();
                uint32_t maxIndex = readU30();
                std::vector<int32_t> caseOffsets(maxIndex + 1);
                for (uint32_t i = 0; i <= maxIndex; ++i) {
                    caseOffsets[i] = readS24();
                }
                int32_t index = pop().toInt();
                if (index >= 0 && static_cast<uint32_t>(index) <= maxIndex) {
                    pc = basePc + caseOffsets[index];
                } else {
                    pc = basePc + defaultOffset;
                }
                break;
            }
            case 0x1D: break; /* popscope */
            case 0x1E: { /* nextname */
                Avm2Value idxVal = pop();
                Avm2Value objVal = pop();
                int32_t idx = idxVal.toInt() - 1;
                if (objVal.type == Avm2Type::Object && objVal.objVal) {
                    if (!objVal.objVal->m_arrayItems.empty()) {
                        push(Avm2Value::integer(idx));
                    } else if (idx >= 0 && static_cast<size_t>(idx) < objVal.objVal->properties.size()) {
                        auto it = objVal.objVal->properties.begin();
                        std::advance(it, idx);
                        push(Avm2Value::string(it->first));
                    } else {
                        push(Avm2Value::undefined());
                    }
                } else {
                    push(Avm2Value::undefined());
                }
                break;
            }
            case 0x23: { /* nextvalue */
                /* AVM2 nextvalue buat loop `for each (var x in list)` */
                Avm2Value idxVal = pop();
                Avm2Value objVal = pop();
                int32_t idx = idxVal.toInt() - 1;
                if (objVal.type == Avm2Type::Object && objVal.objVal) {
                    if (idx >= 0 && static_cast<size_t>(idx) < objVal.objVal->m_arrayItems.size()) {
                        push(objVal.objVal->m_arrayItems[idx]);
                    } else if (idx >= 0 && static_cast<size_t>(idx) < objVal.objVal->properties.size()) {
                        auto it = objVal.objVal->properties.begin();
                        std::advance(it, idx);
                        push(it->second);
                    } else {
                        push(Avm2Value::undefined());
                    }
                } else {
                    push(Avm2Value::undefined());
                }
                break;
            }

            /* Push constants */
            case 0x20: push(Avm2Value::null()); break;
            case 0x21: push(Avm2Value::undefined()); break;
            case 0x24: { /* pushbyte */
                int8_t b = static_cast<int8_t>(code[pc++]);
                push(Avm2Value::integer(b));
                break;
            }
            case 0x25: { /* pushshort */
                int32_t s = static_cast<int32_t>(readU30());
                push(Avm2Value::integer(s));
                break;
            }
            case 0x26: push(Avm2Value::boolean(true)); break;
            case 0x27: push(Avm2Value::boolean(false)); break;
            case 0x28: push(Avm2Value::number(std::numeric_limits<double>::quiet_NaN())); break; /* pushnan */
            case 0x29: pop(); break; /* pop stack */
            case 0x2A: { /* dup */
                if (!stack.empty()) push(stack.back());
                break;
            }
            case 0x2B: { /* swap */
                if (stack.size() >= 2) std::swap(stack[stack.size() - 1], stack[stack.size() - 2]);
                break;
            }
            case 0x2C: { /* pushstring */
                uint32_t idx = readU30();
                push(Avm2Value::string(pool.getString(idx)));
                break;
            }
            case 0x2D: { /* pushint */
                uint32_t idx = readU30();
                push(Avm2Value::integer(pool.getInt(idx)));
                break;
            }
            case 0x2E: { /* pushuint */
                uint32_t idx = readU30();
                uint32_t u = (idx > 0 && idx < pool.uints.size()) ? pool.uints[idx] : 0;
                push(Avm2Value::integer(static_cast<int32_t>(u)));
                break;
            }
            case 0x2F: { /* pushdouble */
                uint32_t idx = readU30();
                push(Avm2Value::number(pool.getDouble(idx)));
                break;
            }
            case 0x30: { /* pushscope */
                /* tiap awal method selalu getlocal_0 terus pushscope, pop doang biat gak nyampah di stack */
                pop();
                break;
            }
            case 0x32: { /* hasnext2 object_reg, index_reg */
                /*
                 * hasnext2 itu opcode sakti di AS3 buat iterator loop.  
                 * Operasinya langsung ngutak-atik locals[index_reg] dan push boolean ke stack.
                 */
                uint32_t objReg = readU30();
                uint32_t idxReg = readU30();
                int32_t curIdx = (idxReg < locals.size()) ? locals[idxReg].toInt() : 0;
                bool hasMore = false;
                if (objReg < locals.size() && locals[objReg].type == Avm2Type::Object && locals[objReg].objVal) {
                    auto o = locals[objReg].objVal;
                    size_t count = !o->m_arrayItems.empty() ? o->m_arrayItems.size() : o->properties.size();
                    if (static_cast<size_t>(curIdx) < count) {
                        hasMore = true;
                        if (idxReg < locals.size()) locals[idxReg] = Avm2Value::integer(curIdx + 1);
                    } else {
                        if (idxReg < locals.size()) locals[idxReg] = Avm2Value::integer(0);
                    }
                } else {
                    if (idxReg < locals.size()) locals[idxReg] = Avm2Value::integer(0);
                }
                push(Avm2Value::boolean(hasMore));
                break;
            }
            /* Object & function instantiation */
            case 0x40: { /* newfunction method_id */
                /* note: closure sementara dipass string id buat delayCall / event handler */
                uint32_t mid = readU30();
                push(Avm2Value::string("method_" + std::to_string(mid)));
                break;
            }
            case 0x42: { /* construct arg_count */
                uint32_t argCount = readU30();
                for (uint32_t i = 0; i < argCount; ++i) pop();
                pop();
                push(Avm2Value::object(std::make_shared<Avm2Object>()));
                break;
            }
            case 0x44: { /* callstatic method_id, arg_count */
                readU30();
                uint32_t argCount = readU30();
                for (uint32_t i = 0; i < argCount; ++i) pop();
                pop();
                push(Avm2Value::undefined());
                break;
            }
            case 0x4a: { /* constructprop mn_idx, arg_count */
                uint32_t mnIdx = readU30();
                uint32_t argCount = readU30();
                std::vector<Avm2Value> cArgs(argCount);
                for (int i = static_cast<int>(argCount) - 1; i >= 0; --i) cArgs[i] = pop();
                pop(); /* pop receiver */
                const std::string& clsName = pool.getMultiname(mnIdx);
                auto obj = std::make_shared<Avm2Object>();
                if (clsName == "Point" || clsName.find("Point") != std::string::npos) {
                    obj->setProperty("x", cArgs.size() >= 1 ? cArgs[0] : Avm2Value::number(0.0));
                    obj->setProperty("y", cArgs.size() >= 2 ? cArgs[1] : Avm2Value::number(0.0));
                } else if (clsName == "Rectangle" || clsName.find("Rectangle") != std::string::npos) {
                    obj->setProperty("x", cArgs.size() >= 1 ? cArgs[0] : Avm2Value::number(0.0));
                    obj->setProperty("y", cArgs.size() >= 2 ? cArgs[1] : Avm2Value::number(0.0));
                    obj->setProperty("width", cArgs.size() >= 3 ? cArgs[2] : Avm2Value::number(0.0));
                    obj->setProperty("height", cArgs.size() >= 4 ? cArgs[3] : Avm2Value::number(0.0));
                }
                push(Avm2Value::object(obj));
                break;
            }
            case 0x47: return Avm2Value::undefined(); /* returnvoid */
            case 0x48: return pop(); /* returnvalue */
            case 0x49: { /* constructsuper arg_count */
                uint32_t argCount = readU30();
                for (uint32_t i = 0; i < argCount; ++i) pop();
                pop(); /* pop receiver (this) */
                break;
            }
            case 0x4C: { /* callproplex mn_idx, arg_count */
                uint32_t mnIdx = readU30();
                uint32_t argCount = readU30();
                const std::string& methName = pool.getMultiname(mnIdx);
                std::vector<Avm2Value> margs(argCount);
                for (int i = static_cast<int>(argCount) - 1; i >= 0; --i) margs[i] = pop();
                Avm2Value obj = pop();
                push(invokeMethod(obj, methName, margs, thisObject, hostContext));
                break;
            }
            case 0x4E: { /* callsupervoid mn_idx, arg_count */
                readU30();
                uint32_t argCount = readU30();
                for (uint32_t i = 0; i < argCount; ++i) pop();
                pop(); /* pop receiver */
                break;
            }

            case 0x55: { /* newobject arg_count */
                uint32_t argCount = readU30();
                auto obj = std::make_shared<Avm2Object>();
                std::vector<Avm2Value> kvs(argCount * 2);
                for (int i = static_cast<int>(argCount * 2) - 1; i >= 0; --i) kvs[i] = pop();
                for (uint32_t i = 0; i < argCount; ++i) {
                    obj->setProperty(kvs[i * 2].toString(), kvs[i * 2 + 1]);
                }
                push(Avm2Value::object(obj));
                break;
            }
            case 0x56: { /* newarray arg_count */
                uint32_t argCount = readU30();
                auto arr = std::make_shared<Avm2Object>();
                arr->m_arrayItems.resize(argCount);
                for (int i = static_cast<int>(argCount) - 1; i >= 0; --i) {
                    arr->m_arrayItems[i] = pop();
                }
                push(Avm2Value::object(arr));
                break;
            }
            case 0x57: { /* newactivation (scope activation record) */
                push(Avm2Value::object(std::make_shared<Avm2Object>()));
                break;
            }
            case 0x58: { /* newclass class_idx */
                readU30();
                pop(); /* pop basetype */
                push(Avm2Value::object(std::make_shared<Avm2Object>()));
                break;
            }
            case 0x59: { /* getdescendants mn_idx */
                readU30();
                pop();
                push(Avm2Value::undefined());
                break;
            }
            case 0x5A: { /* newcatch index */
                readU30();
                push(Avm2Value::object(std::make_shared<Avm2Object>()));
                break;
            }
            case 0x5D: { /* findpropstrict mn_idx */
                readU30();
                push(locals[0]); /* fallback ke this */
                break;
            }
            case 0x5E: { /* findproperty mn_idx */
                readU30();
                push(locals[0]); /* fallback ke this */
                break;
            }
            case 0x60: { /* getlex mn_idx */
                /*
                 * getlex (0x60) itu JANGAN pernah nge-pop stack!
                 * Di AVM2 dia cuma resolve nama dari multiname pool trus push hasilnya.
                 * Kemaren crash berjam-jam gara-gara stack underflow di sini.
                 */
                uint32_t mnIdx = readU30();
                const std::string& propName = pool.getMultiname(mnIdx);
                push(resolveProperty(Avm2Value::undefined(), propName, thisObject));
                break;
            }
            case 0x64: { /* getglobalslot slot_idx */
                readU30();
                push(Avm2Value::undefined());
                break;
            }
            case 0x65: { /* getscopeobject scope_idx */
                if (pc < codeLen) ++pc;
                push(locals[0]);
                break;
            }

            /* Property access */
            case 0x66: { /* getproperty mn_idx */
                uint32_t mnIdx = readU30();
                const std::string& propName = pool.getMultiname(mnIdx);
                /* 
                 * Jebakan spek flash: multinameL (0x1b/0x1c) itu buat dynamic index `obj[key]`.
                 * Urutan pop di stack: KEY dulu baru OBJECT
                 */
                if (propName == "<MultinameL>" || propName == "<RTQNameL>") {
                    Avm2Value key = pop();
                    Avm2Value obj = pop();
                    if (obj.type == Avm2Type::Object && obj.objVal) {
                        if (key.type == Avm2Type::Int && key.iVal >= 0 && static_cast<size_t>(key.iVal) < obj.objVal->m_arrayItems.size()) {
                            push(obj.objVal->getElement(static_cast<size_t>(key.iVal)));
                        } else {
                            push(obj.objVal->getProperty(key.toString()));
                        }
                    } else {
                        push(resolveProperty(obj, key.toString(), thisObject));
                    }
                } else {
                    Avm2Value obj = pop();
                    push(resolveProperty(obj, propName, thisObject));
                }
                break;
            }
            case 0x61:   /* setproperty mn_idx */
            case 0x68: { /* initproperty mn_idx */
                uint32_t mnIdx = readU30();
                const std::string& propName = pool.getMultiname(mnIdx);
                if (propName == "<MultinameL>" || propName == "<RTQNameL>") {
                    Avm2Value val = pop();
                    Avm2Value key = pop();
                    Avm2Value obj = pop();
                    if (obj.type == Avm2Type::Object && obj.objVal) {
                        obj.objVal->setProperty(key.toString(), val);
                    } else {
                        assignProperty(obj, key.toString(), val, thisObject);
                    }
                } else {
                    Avm2Value val = pop();
                    Avm2Value obj = pop();
                    assignProperty(obj, propName, val, thisObject);
                }
                break;
            }
            case 0x6A: { /* deleteproperty mn_idx */
                uint32_t mnIdx = readU30();
                const std::string& propName = pool.getMultiname(mnIdx);
                if (propName == "<MultinameL>" || propName == "<RTQNameL>") {
                    Avm2Value key = pop();
                    Avm2Value obj = pop();
                    if (obj.type == Avm2Type::Object && obj.objVal) {
                        obj.objVal->properties.erase(key.toString());
                    }
                } else {
                    Avm2Value obj = pop();
                    if (obj.type == Avm2Type::Object && obj.objVal) {
                        obj.objVal->properties.erase(propName);
                    }
                }
                push(Avm2Value::boolean(true));
                break;
            }
            case 0x6B: { /* deletepropertylate */
                Avm2Value key = pop();
                Avm2Value obj = pop();
                if (obj.type == Avm2Type::Object && obj.objVal) {
                    obj.objVal->properties.erase(key.toString());
                }
                push(Avm2Value::boolean(true));
                break;
            }
            case 0x6c: { /* getslot slot_idx */
                uint32_t sidx = readU30();
                Avm2Value obj = pop();
                push(resolveProperty(obj, "slot_" + std::to_string(sidx), thisObject));
                break;
            }
            case 0x6d: { /* setslot slot_idx */
                uint32_t sidx = readU30();
                Avm2Value val = pop();
                Avm2Value obj = pop();
                assignProperty(obj, "slot_" + std::to_string(sidx), val, thisObject);
                break;
            }

            /* Method calls */
            case 0x46: { /* callproperty mn_idx, arg_count */
                uint32_t mnIdx = readU30();
                uint32_t argCount = readU30();
                const std::string& methName = pool.getMultiname(mnIdx);
                std::vector<Avm2Value> margs(argCount);
                for (int i = static_cast<int>(argCount) - 1; i >= 0; --i) margs[i] = pop();
                Avm2Value obj = pop();
                push(invokeMethod(obj, methName, margs, thisObject, hostContext));
                break;
            }
            case 0x4F: { /* callpropvoid mn_idx, arg_count */
                uint32_t mnIdx = readU30();
                uint32_t argCount = readU30();
                const std::string& methName = pool.getMultiname(mnIdx);
                std::vector<Avm2Value> margs(argCount);
                for (int i = static_cast<int>(argCount) - 1; i >= 0; --i) margs[i] = pop();
                Avm2Value obj = pop();
                invokeMethod(obj, methName, margs, thisObject, hostContext);
                break;
            }

            /* Register locals */
            case 0x62: { /* getlocal n */
                uint32_t n = readU30();
                push(n < locals.size() ? locals[n] : Avm2Value::undefined());
                break;
            }
            case 0x63: { /* setlocal n */
                uint32_t n = readU30();
                if (n >= locals.size()) locals.resize(n + 1, Avm2Value::undefined());
                locals[n] = pop();
                break;
            }
            case 0xD0: push(locals[0]); break; /* getlocal_0 */
            case 0xD1: push(locals.size() > 1 ? locals[1] : Avm2Value::undefined()); break;
            case 0xD2: push(locals.size() > 2 ? locals[2] : Avm2Value::undefined()); break;
            case 0xD3: push(locals.size() > 3 ? locals[3] : Avm2Value::undefined()); break;
            case 0xD4: locals[0] = pop(); break; /* setlocal_0 */
            case 0xD5: if (locals.size() > 1) locals[1] = pop(); else pop(); break;
            case 0xD6: if (locals.size() > 2) locals[2] = pop(); else pop(); break;
            case 0xD7: if (locals.size() > 3) locals[3] = pop(); else pop(); break;

            /* Type conversions */
            case 0x70: { /* convert_s */
                if (!stack.empty()) stack.back() = Avm2Value::string(stack.back().toString());
                break;
            }
            case 0x73: { /* convert_i */
                if (!stack.empty()) stack.back() = Avm2Value::integer(stack.back().toInt());
                break;
            }
            case 0x74: { /* convert_u */
                if (!stack.empty()) stack.back() = Avm2Value::integer(static_cast<int32_t>(static_cast<uint32_t>(stack.back().toInt())));
                break;
            }
            case 0x75: { /* convert_d */
                if (!stack.empty()) stack.back() = Avm2Value::number(stack.back().toDouble());
                break;
            }
            case 0x76: { /* convert_b */
                if (!stack.empty()) stack.back() = Avm2Value::boolean(stack.back().toBool());
                break;
            }
            case 0x80: readU30(); break; /* coerce mn_idx */
            case 0x82: break; /* coerce_a */
            case 0x85: { /* coerce_s */
                if (!stack.empty() && !stack.back().isNullOrUndefined()) {
                    stack.back() = Avm2Value::string(stack.back().toString());
                }
                break;
            }
            case 0x86: { /* astype type_idx */
                readU30();
                break;
            }
            case 0x87: { /* astypelate (val as Class) */
                pop(); /* pop class object */
                break;
            }

            /* Math & logic */
            case 0x90: push(Avm2Value::number(-pop().toDouble())); break; /* negate */
            case 0x91: { /* increment */
                Avm2Value v = pop();
                push(Avm2Value::number(v.toDouble() + 1.0));
                break;
            }
            case 0x92: { /* decrement */
                Avm2Value v = pop();
                push(Avm2Value::number(v.toDouble() - 1.0));
                break;
            }
            case 0x93: { /* inclocal r */
                uint32_t r = readU30();
                if (r < locals.size()) locals[r] = Avm2Value::number(locals[r].toDouble() + 1.0);
                break;
            }
            case 0x94: { /* declocal r */
                uint32_t r = readU30();
                if (r < locals.size()) locals[r] = Avm2Value::number(locals[r].toDouble() - 1.0);
                break;
            }
            case 0x95:
            case 0x96: push(Avm2Value::boolean(!pop().toBool())); break; /* not */
            case 0x97: push(Avm2Value::integer(~pop().toInt())); break; /* bitnot */

            case 0xA0: { Avm2Value b = pop(), a = pop(); push(Avm2Value::add(a, b)); break; }
            case 0xA1: { Avm2Value b = pop(), a = pop(); push(Avm2Value::subtract(a, b)); break; }
            case 0xA2: { Avm2Value b = pop(), a = pop(); push(Avm2Value::multiply(a, b)); break; }
            case 0xA3: { Avm2Value b = pop(), a = pop(); push(Avm2Value::divide(a, b)); break; }
            case 0xA4: { Avm2Value b = pop(), a = pop(); push(Avm2Value::modulo(a, b)); break; }

            /* Bitwise shifts */
            case 0xA5: { int32_t b = pop().toInt(), a = pop().toInt(); push(Avm2Value::integer(a << (b & 0x1F))); break; } /* lshift */
            case 0xA6: { int32_t b = pop().toInt(), a = pop().toInt(); push(Avm2Value::integer(a >> (b & 0x1F))); break; } /* rshift */
            case 0xA7: { /* urshift */
                uint32_t b = static_cast<uint32_t>(pop().toInt());
                uint32_t a = static_cast<uint32_t>(pop().toInt());
                push(Avm2Value::integer(static_cast<int32_t>(a >> (b & 0x1F))));
                break;
            }

            /* Bitwise ops */
            case 0xA8: { int32_t b = pop().toInt(), a = pop().toInt(); push(Avm2Value::integer(a & b)); break; }
            case 0xA9: { int32_t b = pop().toInt(), a = pop().toInt(); push(Avm2Value::integer(a | b)); break; }
            case 0xAA: { int32_t b = pop().toInt(), a = pop().toInt(); push(Avm2Value::integer(a ^ b)); break; }

            /* Comparison ops */
            case 0xAB: { Avm2Value b = pop(), a = pop(); push(Avm2Value::boolean(a.equals(b))); break; }
            case 0xAC: { Avm2Value b = pop(), a = pop(); push(Avm2Value::boolean(a.strictEquals(b))); break; }
            case 0xAD: { Avm2Value b = pop(), a = pop(); push(Avm2Value::boolean(a.lessThan(b))); break; }
            case 0xAE: { Avm2Value b = pop(), a = pop(); push(Avm2Value::boolean(a.lessEquals(b))); break; }
            case 0xAF: { Avm2Value b = pop(), a = pop(); push(Avm2Value::boolean(a.greaterThan(b))); break; }
            case 0xB0: { Avm2Value b = pop(), a = pop(); push(Avm2Value::boolean(a.greaterEquals(b))); break; }
            case 0xB1: { /* instanceof */
                pop(); /* pop class object */
                Avm2Value val = pop();
                push(Avm2Value::boolean(!val.isNullOrUndefined()));
                break;
            }
            case 0xB2: { /* istype type_idx */
                readU30();
                Avm2Value val = pop();
                push(Avm2Value::boolean(!val.isNullOrUndefined()));
                break;
            }
            case 0xB3: { /* istypelate (val is Class) */
                pop(); /* pop class object */
                Avm2Value val = pop();
                push(Avm2Value::boolean(!val.isNullOrUndefined()));
                break;
            }
            case 0xB4: { /* in (prop in obj) */
                Avm2Value obj = pop(), key = pop();
                bool has = (obj.type == Avm2Type::Object && obj.objVal && obj.objVal->properties.count(key.toString()) > 0);
                push(Avm2Value::boolean(has));
                break;
            }

            case 0xC0: { /* increment_i */
                if (!stack.empty()) stack.back() = Avm2Value::integer(stack.back().toInt() + 1);
                break;
            }
            case 0xC1: { /* decrement_i */
                if (!stack.empty()) stack.back() = Avm2Value::integer(stack.back().toInt() - 1);
                break;
            }
            case 0xC2: { /* inclocal_i */
                uint32_t r = readU30();
                if (r < locals.size()) locals[r] = Avm2Value::integer(locals[r].toInt() + 1);
                break;
            }
            case 0xC3: { /* declocal_i */
                uint32_t r = readU30();
                if (r < locals.size()) locals[r] = Avm2Value::integer(locals[r].toInt() - 1);
                break;
            }
            case 0xC4: { /* negate_i */
                if (!stack.empty()) stack.back() = Avm2Value::integer(-stack.back().toInt());
                break;
            }
            case 0xC5: { int32_t b = pop().toInt(), a = pop().toInt(); push(Avm2Value::integer(a + b)); break; } /* add_i */
            case 0xC6: { int32_t b = pop().toInt(), a = pop().toInt(); push(Avm2Value::integer(a - b)); break; } /* subtract_i */
            case 0xC7: { int32_t b = pop().toInt(), a = pop().toInt(); push(Avm2Value::integer(a * b)); break; } /* multiply_i */

            /* Debug info opcodes (wajib konsumsi operand DoABC biar PC ga slip) */
            case 0xEF: { /* debug type, name_idx, reg, extra */
                if (pc < codeLen) ++pc;
                readU30();
                if (pc < codeLen) ++pc;
                readU30();
                break;
            }
            case 0xF0: { /* debugline line_num */
                readU30();
                break;
            }
            case 0xF1: { /* debugfile file_idx */
                readU30();
                break;
            }

            default:
                /* abaikan opcode debug/unsupported */
                break;
        }
    }

    return stack.empty() ? Avm2Value::undefined() : stack.back();
}

} // namespace avm2

