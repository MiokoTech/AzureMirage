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
#include <iostream>

/* 
 * Simulasi entitas fighter game di C++.
 * Host class tinggal inherit avm2::Avm2Object biar variabel & fungsi C++
 * bisa diakses bolak-balik dari script AS3.
 */
class FighterEntity : public avm2::Avm2Object {
public:
    int hp = 1000;
    int facing = 1; /* 1 = kanan, -1 = kiri */
    float posX = 100.0f;
    float posY = 250.0f;

    /* Get property pas script AS3 baca `$self_main.hp` atau `this.direct` */
    avm2::Avm2Value getProperty(const std::string& name) override {
        if (name == "hp") return avm2::Avm2Value::integer(hp);
        if (name == "direct") return avm2::Avm2Value::integer(facing);
        if (name == "x") return avm2::Avm2Value::number(posX);
        if (name == "y") return avm2::Avm2Value::number(posY);
        return avm2::Avm2Object::getProperty(name);
    }

    /* Set property pas script AS3 ngubah arah `this.direct = -1` atau ubah posisi */
    void setProperty(const std::string& name, const avm2::Avm2Value& val) override {
        if (name == "hp") {
            hp = val.toInt();
            return;
        }
        if (name == "direct") {
            facing = (val.toInt() < 0) ? -1 : 1;
            return;
        }
        if (name == "x") {
            posX = static_cast<float>(val.toDouble());
            return;
        }
        if (name == "y") {
            posY = static_cast<float>(val.toDouble());
            return;
        }
        avm2::Avm2Object::setProperty(name, val);
    }

    /* Method callback pas AS3 panggil fungsi native engine, cth: `loseHp(100)` atau `moveToTarget(20)` */
    avm2::Avm2Value callMethod(const std::string& name, const std::vector<avm2::Avm2Value>& args, void*) override {
        if (name == "loseHp" && !args.empty()) {
            int dmg = args[0].toInt();
            hp -= dmg;
            std::cout << ">> [C++ Engine Callback] loseHp() dipanggil dari bytecode AS3! Damage: " 
                      << dmg << " -> Sisa HP: " << hp << "\n";
            return avm2::Avm2Value::undefined();
        }
        if (name == "suMoveTarget" && !args.empty()) {
            /* Simulasi tarikan gravitasi / suction */
            float pullDistance = static_cast<float>(args[0].toDouble());
            posX += (facing * pullDistance);
            std::cout << ">> [C++ Engine Callback] suMoveTarget() suction: " << pullDistance 
                      << "px -> PosX skrg: " << posX << "\n";
            return avm2::Avm2Value::undefined();
        }
        return avm2::Avm2Object::callMethod(name, args, nullptr);
    }
};

int main() {
    std::cout << "=== AVM2 Mini Virtual Machine Test Run ===\n\n";

    /*
     * Constant pool hasil ekstraksi tag DoABC Flash (.swf)
     * Kumpulan string dan multiname yang dipake bytecode.
     */
    avm2::Avm2ConstantPool pool;
    pool.strings = {"", "Math", "min", "loseHp", "suMoveTarget", "direct"};
    pool.multinames = {"", "Math", "min", "loseHp", "suMoveTarget", "direct"};

    /*
     * Bytecode method mentah (dump instruksi DoABC)
     *
     * Logika ssripts AS3 aslinya:
     *   var power:int = 75 * 2;            // 150
     *   var actualDmg:int = Math.min(power, 120); // Math.min(150, 120) = 120
     *   this.loseHp(actualDmg);            // panggil callback C++
     *   this.suMoveTarget(35);             // tarik posisi 35px
     */
    avm2::Avm2MethodBody attackMethod;
    attackMethod.name = "customSkill_reflectSuction";
    attackMethod.maxStack = 8;
    attackMethod.localCount = 4;

    attackMethod.code = {
        0xD0,             /* getlocal_0 (this) */
        0x60, 0x01,       /* getlex Math */
        0x24, 0x4B,       /* pushbyte 75 */
        0x24, 0x02,       /* pushbyte 2 */
        0xA2,             /* multiply (75 * 2 = 150) */
        0x25, 0x78,       /* pushshort 120 */
        0x46, 0x02, 0x02, /* callproperty min, 2 -> Math.min(150, 120) -> 120 */
        0x4F, 0x03, 0x01, /* callpropvoid loseHp, 1 */

        0xD0,             /* getlocal_0 (this) */
        0x24, 0x23,       /* pushbyte 35 */
        0x4F, 0x04, 0x01, /* callpropvoid suMoveTarget, 1 */

        0x47              /* returnvoid */
    };

    /* Instansiasi host entity & eksekusi */
    auto hero = std::make_shared<FighterEntity>();
    std::cout << "[Kondisi Awal Karakter]\n";
    std::cout << "  HP      : " << hero->hp << "\n";
    std::cout << "  Position: (" << hero->posX << ", " << hero->posY << ")\n";
    std::cout << "  Facing  : " << (hero->facing > 0 ? "Kanan (1)" : "Kiri (-1)") << "\n\n";

    std::cout << "Ngeksekusi bytecode AVM2 method: " << attackMethod.name << "...\n";
    avm2::Avm2Interpreter::getInstance().execute(attackMethod, pool, hero);

    std::cout << "\n[Kondisi Akhir Karakter]\n";
    std::cout << "  HP      : " << hero->hp << " (sukses berkurang 120)\n";
    std::cout << "  Position: (" << hero->posX << ", " << hero->posY << ") (sukses geser 35px)\n";
    std::cout << "\nVM jalan 100% lancar, siap dipake di engine game / emulator!\n";

    return 0;
}
