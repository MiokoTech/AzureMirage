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

#ifndef AVM2_VM_H
#define AVM2_VM_H

/* 
 * AVM2 Mini VM - Standalone ActionScript 3 (DoABC) interpreter
 * Gw bikin ini murni C++17 tanpa dependency external apapun.
 * Tujuannya biar bisa ngeksekusi method AS3 modder (suction, reflect, wallbounce)
 * langsung dari binary tanpa perlu Flash Player / Adobe AIR.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace avm2 {
enum class Avm2Type : uint8_t {
    Undefined = 0,
    Null,
    Bool,
    Int,
    Double,
    String,
    Object,
    Array
}; 

class Avm2Object;

/* Dynamic value wrapper ala AS3 untyped (*) / Object */
struct Avm2Value {
    Avm2Type type = Avm2Type::Undefined;
    bool bVal = false;
    int32_t iVal = 0;
    double dVal = 0.0;
    std::string sVal;
    std::shared_ptr<Avm2Object> objVal;
    /* factory helpers biar ga repot ngetik terus */
    static Avm2Value undefined() { return Avm2Value{}; }
    static Avm2Value null() { Avm2Value v; v.type = Avm2Type::Null; return v; }
    static Avm2Value boolean(bool b) { Avm2Value v; v.type = Avm2Type::Bool; v.bVal = b; return v; }
    static Avm2Value integer(int32_t i) { Avm2Value v; v.type = Avm2Type::Int; v.iVal = i; return v; }
    static Avm2Value number(double d) { Avm2Value v; v.type = Avm2Type::Double; v.dVal = d; return v; }
    static Avm2Value string(const std::string& s) { Avm2Value v; v.type = Avm2Type::String; v.sVal = s; return v; }
    static Avm2Value object(std::shared_ptr<Avm2Object> o);
    inline bool isUndefined() const { return type == Avm2Type::Undefined; }
    inline bool isNull() const { return type == Avm2Type::Null; }
    inline bool isNullOrUndefined() const { return type == Avm2Type::Undefined || type == Avm2Type::Null; }
    inline bool isNumber() const { return type == Avm2Type::Int || type == Avm2Type::Double || type == Avm2Type::Bool; }
    inline bool isString() const { return type == Avm2Type::String; }

    /* casting & konversi tipe */
    bool toBool() const;
    int32_t toInt() const;
    double toDouble() const;
    std::string toString() const;

    /* perbandingan */
    bool equals(const Avm2Value& other) const;
    bool strictEquals(const Avm2Value& other) const;
    bool lessThan(const Avm2Value& other) const;
    bool lessEquals(const Avm2Value& other) const;
    bool greaterThan(const Avm2Value& other) const;
    bool greaterEquals(const Avm2Value& other) const;

    /* operator bawaan */
    static Avm2Value add(const Avm2Value& a, const Avm2Value& b);
    static Avm2Value subtract(const Avm2Value& a, const Avm2Value& b);
    static Avm2Value multiply(const Avm2Value& a, const Avm2Value& b);
    static Avm2Value divide(const Avm2Value& a, const Avm2Value& b);
    static Avm2Value modulo(const Avm2Value& a, const Avm2Value& b);
};

/* 
 * Base object AVM2 buat binding ke C++.
 * Turunin class ini kalo mau ekspos entitas game (misal Player, Hitbox, Effect) ke script AS3.
 */
class Avm2Object {
public:
    virtual ~Avm2Object() = default;

    /* override ini di game host buat get/set/method custom */
    virtual Avm2Value getProperty(const std::string& name);
    virtual void setProperty(const std::string& name, const Avm2Value& val);
    virtual bool hasProperty(const std::string& name) const {
        return properties.find(name) != properties.end();
    }
    virtual Avm2Value callMethod(const std::string& name, const std::vector<Avm2Value>& args, void* hostContext);

    /* buat MovieClip / Array dinamis */
    virtual size_t arrayLength() const { return m_arrayItems.size(); }
    virtual Avm2Value getElement(size_t index) const {
        return (index < m_arrayItems.size()) ? m_arrayItems[index] : Avm2Value::undefined();
    }
    virtual void pushElement(const Avm2Value& val) { m_arrayItems.push_back(val); }

    std::unordered_map<std::string, Avm2Value> properties;
    std::vector<Avm2Value> m_arrayItems; 
};

/* 
 * Constant pool hasil parsing tag DoABC (Tag 82).
 * Di-load sekali pas init / pas baca binary cache AMC.
 */
struct Avm2ConstantPool {
    std::vector<int32_t> ints;
    std::vector<uint32_t> uints;
    std::vector<double> doubles; 
    std::vector<std::string> strings;
    std::vector<std::string> multinames;

    const std::string& getString(uint32_t idx) const {
        static const std::string empty;
        return (idx > 0 && idx < strings.size()) ? strings[idx] : empty;
    }
    const std::string& getMultiname(uint32_t idx) const {
        static const std::string empty;
        return (idx > 0 && idx < multinames.size()) ? multinames[idx] : empty;
    }
    double getDouble(uint32_t idx) const {
        return (idx > 0 && idx < doubles.size()) ? doubles[idx] : 0.0;
    }
    int32_t getInt(uint32_t idx) const {
        return (idx > 0 && idx < ints.size()) ? ints[idx] : 0;
    }
};

/* Metadata + instruksi mentah bytecode method */
struct Avm2MethodBody {
    uint16_t nameId = 0;
    std::string name;
    uint16_t maxStack = 32;
    uint16_t localCount = 16;
    uint16_t initScopeDepth = 0;
    uint16_t maxScopeDepth = 0;
    std::vector<uint8_t> code;
    uint16_t poolIdx = 0; /* id pool constant AVM2/AVM3 */
};

/*
 * Interpreter stack machine utama.
 * Singleton aja biar gampang dipanggil dari mana-mana tanpa alloc ulang.
 */
class Avm2Interpreter {
public:
    static Avm2Interpreter& getInstance();

    /* Delegate buat dispatch method bytecode custom ke objek dinamis (misal class GeneralAnimal::createYanWu) */
    using CustomMethodDispatcherFn = std::function<Avm2Value(
        std::shared_ptr<Avm2Object> targetObj,
        const std::string& methodName,
        const std::vector<Avm2Value>& args,
        void* hostContext
    )>;

    void setCustomMethodDispatcher(CustomMethodDispatcherFn dispatcher);
    CustomMethodDispatcherFn getCustomMethodDispatcher() const;

    /* Hook matcher kesetaraan string (misal bvnAnimLabelsMatch untuk mencocokkan label animasi Flash vs kanonikal) */
    using StringEqualityMatcherFn = std::function<bool(const std::string&, const std::string&)>;
    void setStringEqualityMatcher(StringEqualityMatcherFn matcher);
    StringEqualityMatcherFn getStringEqualityMatcher() const;

    /* Eksekusi bytecode method */
    Avm2Value execute(
        const Avm2MethodBody& method,
        const Avm2ConstantPool& pool,
        std::shared_ptr<Avm2Object> thisObject = nullptr,
        const std::vector<Avm2Value>& args = {},
        void* hostContext = nullptr
    );

private:
    Avm2Interpreter() = default;
    ~Avm2Interpreter() = default;
    CustomMethodDispatcherFn m_customDispatcher = nullptr;
    StringEqualityMatcherFn m_stringMatcher = nullptr;
};

} // namespace avm2

#endif // AVM2_VM_H
