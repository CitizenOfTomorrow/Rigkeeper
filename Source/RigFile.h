#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

// Reads Kemper .krig files: a Standard MIDI File ("KThd"/"KTrk" instead of
// "MThd"/"MTrk") whose track is a list of Kemper SysEx messages.
// Names and addresses are from the Kemper MIDI Parameter Documentation v11.
namespace rigfile
{
    enum class Category { empty, wah, distortion, dynamics, modulation, equalizer,
                          booster, loop, pitch, delay, reverb, unknown };

    struct EffectType { int id; const char* name; };

    // Appendix B of the Kemper MIDI Parameter Documentation.
    constexpr EffectType effectTypes[] =
    {
        { 0, "Empty" }, { 1, "Wah Wah" }, { 2, "Wah Low Pass" }, { 3, "Wah High Pass" },
        { 4, "Wah Vowel Filter" }, { 6, "Wah Phaser" }, { 7, "Wah Flanger" },
        { 8, "Wah Rate Reducer" }, { 9, "Wah Ring Modulator" }, { 10, "Wah Freq Shifter" },
        { 11, "Pedal Pitch" }, { 12, "Wah Formant Shifter" }, { 13, "Pedal Vinyl Stop" },
        { 17, "Bit Shaper" }, { 18, "Octa Shaper" }, { 19, "Soft Shaper" }, { 20, "Hard Shaper" },
        { 21, "Wave Shaper" }, { 32, "Kemper Drive" }, { 33, "Green Scream" }, { 34, "Plus DS" },
        { 35, "One DS" }, { 36, "Muffin" }, { 37, "Mouse" }, { 38, "Kemper Fuzz" },
        { 39, "Metal DS" }, { 42, "Full OC" }, { 49, "Compressor" }, { 50, "Auto Swell" },
        { 57, "Noise Gate 2:1" }, { 58, "Noise Gate 4:1" }, { 64, "Space" },
        { 65, "Vintage Chorus" }, { 66, "Hyper Chorus" }, { 67, "Air Chorus" }, { 68, "Vibrato" },
        { 69, "Rotary Speaker" }, { 70, "Tremolo" }, { 71, "Micro Pitch" }, { 81, "Phaser" },
        { 82, "Phaser Vibe" }, { 83, "Phaser Oneway" }, { 89, "Flanger" },
        { 91, "Flanger Oneway" }, { 97, "Graphic Equalizer" }, { 98, "Studio Equalizer" },
        { 99, "Metal Equalizer" }, { 100, "Acoustic Simulator" }, { 101, "Stereo Widener" },
        { 102, "Phase Widener" }, { 103, "Delay Widener" }, { 104, "Double Tracker" },
        { 113, "Treble Booster" }, { 114, "Lead Booster" }, { 115, "Pure Booster" },
        { 116, "Wah Pedal Booster" }, { 121, "Loop Mono" }, { 122, "Loop Stereo" },
        { 123, "Loop Distortion" }, { 129, "Transpose" }, { 130, "Chromatic Pitch" },
        { 131, "Harmonic Pitch" }, { 132, "Analog Octaver" }, { 137, "Dual Chromatic" },
        { 138, "Dual Harmonic" }, { 139, "Dual Crystal" }, { 140, "Dual Loop Pitch" },
        { 145, "Legacy Delay" }, { 146, "Single Delay" }, { 147, "Dual Delay" },
        { 148, "Two Tap Delay" }, { 149, "Serial TwoTap Delay" }, { 150, "Crystal Delay" },
        { 151, "Loop Pitch Delay" }, { 152, "Freq Shifter Delay" }, { 161, "Rhythm Delay" },
        { 162, "Melody Chromatic" }, { 163, "Melody Harmonic" }, { 164, "Quad Delay" },
        { 165, "Quad Chromatic" }, { 166, "Quad Harmonic" }, { 177, "Legacy Reverb" },
        { 178, "Natural Reverb" }, { 179, "Easy Reverb" }, { 180, "Echo Reverb" },
        { 181, "Cirrus Reverb" }, { 182, "Formant Reverb" }, { 183, "Ionosphere Reverb" },
        { 193, "Spring Reverb" },
    };

    inline std::string effectName(int type)
    {
        for (const auto& t : effectTypes)
            if (t.id == type) return t.name;
        return "Type " + std::to_string(type);
    }

    // Types are grouped in ranges in the documentation.
    inline Category effectCategory(int type)
    {
        if (type <= 0)   return Category::empty;
        if (type < 17)   return Category::wah;
        if (type < 49)   return Category::distortion;
        if (type < 64)   return Category::dynamics;
        if (type < 97)   return Category::modulation;
        if (type < 113)  return Category::equalizer;
        if (type < 121)  return Category::booster;
        if (type < 129)  return Category::loop;
        if (type < 145)  return Category::pitch;
        if (type < 177)  return Category::delay;
        if (type < 200)  return Category::reverb;
        return Category::unknown;
    }

    // Effect parameters. Every slot stores all of them; which ones an effect
    // uses is not documented, so these lists follow the parameter names in the
    // docs, grouped by effect family. Units are approximate.
    enum class Scale { unipolar, bipolar, percent, semitones, toggle, raw };

    struct Param { int number; const char* name; Scale scale; };

    inline std::vector<Param> effectParameters(int type)
    {
        using S = Scale;
        const Param mix { 4, "Mix", S::percent }, volume { 6, "Volume", S::bipolar },
                    ducking { 53, "Ducking", S::bipolar }, fxMix { 69, "Mix", S::percent };
        const auto join = [] (std::vector<Param> a, const std::vector<Param>& b)
        {
            a.insert(a.end(), b.begin(), b.end());
            return a;
        };

        if (type <= 0) return {};
        if (type == 11 || type == 13)
            return { { 56, "Toe Pitch", S::semitones }, { 57, "Heel Pitch", S::semitones },
                     { 12, "Pedal Mode", S::raw }, mix, volume };
        if (type < 17)
        {
            std::vector<Param> wah { { 8, "Manual", S::unipolar }, { 9, "Peak", S::unipolar },
                                     { 52, "Peak Range", S::bipolar }, { 10, "Pedal Range", S::bipolar },
                                     { 12, "Pedal Mode", S::raw }, { 13, "Touch Attack", S::unipolar },
                                     { 14, "Touch Release", S::unipolar }, { 15, "Touch Boost", S::unipolar } };
            if (type == 6) wah = join(wah, { { 26, "Peak Spread", S::bipolar }, { 27, "Stages", S::raw } });
            if (type == 12) wah = join(wah, { { 57, "Formant Shift", S::semitones } });
            return join(wah, { mix, volume, ducking });
        }
        if (type < 32)
            return { { 16, "Drive", S::unipolar }, { 17, "Tone", S::bipolar }, mix, volume };
        if (type < 49)
        {
            std::vector<Param> drive { { 16, "Drive", S::unipolar }, { 17, "Tone", S::bipolar } };
            if (type == 32) drive = join(drive, { { 21, "Definition", S::bipolar }, { 23, "Slim Down", S::unipolar } });
            if (type == 38) drive = join(drive, { { 18, "Octa", S::unipolar }, { 20, "Transistor Shape", S::unipolar },
                                                  { 21, "Transistor Tone", S::bipolar }, { 23, "Definition", S::bipolar },
                                                  { 13, "Impedance LP", S::unipolar } });
            if (type == 39) drive = join(drive, { { 42, "Low", S::bipolar }, { 46, "Middle", S::bipolar },
                                                  { 47, "Mid Frequency", S::unipolar }, { 44, "High", S::bipolar } });
            if (type == 42) drive = join(drive, { { 100, "HP/LP", S::toggle } });
            return join(drive, { volume, mix });
        }
        if (type == 49)
            return { { 18, "Intensity", S::unipolar }, { 19, "Attack", S::unipolar },
                     { 33, "Squash", S::bipolar }, mix, volume };
        if (type == 50)
            return { { 20, "Swell", S::unipolar }, { 18, "Compressor", S::unipolar }, volume };
        if (type < 64)
            return { { 18, "Threshold", S::unipolar } };
        if (type == 64)
            return { { 54, "Intensity", S::percent }, mix, volume, ducking };
        if (type == 69)
            return { { 30, "Speed (Fast)", S::toggle }, { 31, "Distance", S::unipolar },
                     { 32, "Low-High Balance", S::bipolar }, mix, volume };
        if (type == 71)
            return { { 21, "Detune", S::unipolar }, { 23, "Crossover", S::unipolar }, mix, volume };
        if (type < 81)
        {
            std::vector<Param> mod { { 20, "Rate", S::unipolar }, { 21, "Depth", S::unipolar },
                                     { 23, "Crossover", S::unipolar } };
            if (type == 66) mod = join(mod, { { 24, "Amount", S::unipolar } });
            return join(mod, { mix, volume, ducking });
        }
        if (type < 97)
        {
            const bool oneway = type == 83 || type == 91;
            std::vector<Param> phaser { { oneway ? 73 : 20, "Rate", S::unipolar }, { 21, "Depth", S::unipolar },
                                        { 25, "Manual", S::unipolar }, { 22, "Feedback", S::bipolar } };
            if (type < 89) phaser = join(phaser, { { 26, "Peak Spread", S::bipolar }, { 27, "Stages", S::raw } });
            return join(phaser, { mix, volume, ducking });
        }
        if (type == 97)
            return { { 34, "80 Hz", S::bipolar }, { 35, "160 Hz", S::bipolar }, { 36, "320 Hz", S::bipolar },
                     { 37, "640 Hz", S::bipolar }, { 38, "1250 Hz", S::bipolar }, { 39, "2500 Hz", S::bipolar },
                     { 40, "5000 Hz", S::bipolar }, { 41, "10000 Hz", S::bipolar }, volume };
        if (type == 98)
            return { { 42, "Low Gain", S::bipolar }, { 43, "Low Frequency", S::unipolar },
                     { 46, "Mid 1 Gain", S::bipolar }, { 47, "Mid 1 Frequency", S::unipolar },
                     { 48, "Mid 1 Q", S::unipolar }, { 49, "Mid 2 Gain", S::bipolar },
                     { 50, "Mid 2 Frequency", S::unipolar }, { 51, "Mid 2 Q", S::unipolar },
                     { 44, "High Gain", S::bipolar }, { 45, "High Frequency", S::unipolar },
                     { 67, "Low Cut", S::unipolar }, { 68, "High Cut", S::unipolar }, volume };
        if (type == 99)
            return { { 42, "Low", S::bipolar }, { 46, "Middle", S::bipolar },
                     { 47, "Mid Frequency", S::unipolar }, { 44, "High", S::bipolar }, volume };
        if (type == 100)
            return { { 42, "Body", S::bipolar }, { 44, "Sparkle", S::bipolar },
                     { 46, "Bronze", S::bipolar }, { 49, "Pickup", S::bipolar }, volume };
        if (type < 104)
            return { { 20, "Tune", S::unipolar }, { 21, "Intensity", S::unipolar }, volume };
        if (type < 113)
            return { { 21, "Looseness", S::unipolar }, mix, volume };
        if (type < 121)
        {
            std::vector<Param> boost { volume, { 17, "Tone", S::bipolar } };
            if (type == 116) boost.push_back({ 10, "Pedal Range", S::bipolar });
            return boost;
        }
        if (type < 129)
            return { mix, volume, { 70, "Post", S::toggle } };
        if (type == 129)
            return { { 56, "Pitch", S::semitones }, volume };
        if (type == 130 || type == 137)
            return { { 56, "Voice 1 Pitch", S::semitones }, { 57, "Voice 2 Pitch", S::semitones },
                     { 58, "Detune", S::unipolar }, { 55, "Voice Balance", S::bipolar },
                     { 54, "Pitch Mix", S::percent }, { 60, "Smooth Chords", S::toggle },
                     { 61, "Pure Tuning", S::toggle }, { 66, "Formant Offset", S::bipolar }, volume };
        if (type == 131 || type == 138)
            return { { 62, "Voice 1 Interval", S::raw }, { 63, "Voice 2 Interval", S::raw },
                     { 64, "Key", S::raw }, { 55, "Voice Balance", S::bipolar },
                     { 54, "Pitch Mix", S::percent }, { 61, "Pure Tuning", S::toggle }, volume };
        if (type == 132)
            return { { 54, "Octaver Mix", S::percent }, { 23, "Low Cut", S::unipolar }, volume };
        if (type == 139 || type == 140 || type == 150 || type == 151)
            return { { 56, "Voice 1 Pitch", S::semitones }, { 57, "Voice 2 Pitch", S::semitones },
                     { 55, "Voice Balance", S::bipolar }, { 54, "Crystal Mix", S::percent },
                     fxMix, { 71, "Delay Time", S::unipolar }, { 93, "Feedback", S::percent },
                     { 99, "High Cut", S::unipolar }, ducking, volume };
        if (type < 177)
        {
            std::vector<Param> delay { fxMix, { 71, "Time", S::unipolar }, { 93, "Feedback", S::percent } };
            if (type == 147 || type == 148 || type == 149)
                delay = join(delay, { { 72, "Time 2", S::unipolar }, { 96, "Feedback 2", S::percent },
                                      { 55, "Balance", S::bipolar } });
            return join(delay, { { 76, "Note Value", S::raw }, { 80, "To Tempo", S::toggle },
                                 { 98, "Low Cut", S::unipolar }, { 99, "High Cut", S::unipolar },
                                 { 101, "Modulation", S::unipolar }, { 102, "Chorus", S::unipolar },
                                 { 103, "Flutter", S::unipolar }, { 104, "Flutter Rate", S::unipolar },
                                 { 105, "Grit", S::unipolar }, { 106, "Reverse Mix", S::percent },
                                 { 107, "Swell", S::unipolar }, { 108, "Smear", S::unipolar },
                                 ducking, volume });
        }
        if (type == 177)
            return { fxMix, { 27, "Room Size", S::unipolar }, { 93, "Decay Time", S::unipolar },
                     { 19, "Bandwidth", S::unipolar }, { 33, "Mid Frequency", S::unipolar }, ducking, volume };
        if (type == 193)
            return { fxMix, { 71, "Spring Size", S::unipolar }, { 93, "Decay Time", S::unipolar },
                     { 25, "Spectral Balance", S::bipolar }, { 104, "Dripstone", S::unipolar },
                     { 105, "Dwell", S::unipolar }, ducking, volume };
        if (type < 200)
        {
            std::vector<Param> reverb { fxMix, { 71, "Room Size", S::unipolar }, { 72, "Predelay", S::unipolar },
                                        { 93, "Decay Time", S::unipolar } };
            if (type == 180) reverb.push_back({ 96, "Echo Feedback", S::percent });
            if (type == 183) reverb.push_back({ 96, "Buildup", S::unipolar });
            if (type == 182) reverb = join(reverb, { { 16, "Formant Mix", S::percent }, { 22, "Vowel", S::raw },
                                                     { 25, "Formant Offset", S::bipolar }, { 26, "Formant Peak", S::unipolar } });
            return join(reverb, { { 98, "Low Decay", S::bipolar }, { 99, "High Decay", S::bipolar },
                                  { 68, "High Cut", S::unipolar }, { 103, "Modulation", S::unipolar },
                                  { 104, "Early Diffusion", S::unipolar }, { 107, "Swell", S::unipolar },
                                  { 108, "Smear", S::unipolar }, ducking, volume });
        }
        return { mix, volume };
    }

    inline std::string formatValue(Scale scale, int raw)
    {
        char text[32];
        switch (scale)
        {
            case Scale::unipolar:  std::snprintf(text, sizeof(text), "%.1f", raw * 10.0 / 16383.0); break;
            case Scale::bipolar:   std::snprintf(text, sizeof(text), "%+.1f", (raw - 8192) * 5.0 / 8192.0); break;
            case Scale::percent:   std::snprintf(text, sizeof(text), "%d %%", (raw * 100 + 8191) / 16383); break;
            case Scale::semitones: std::snprintf(text, sizeof(text), "%+d st", raw - 64); break;
            case Scale::toggle:    return raw != 0 ? "On" : "Off";
            case Scale::raw:       std::snprintf(text, sizeof(text), "%d", raw); break;
        }
        return text;
    }

    // Position on a bar, 0..1; negative when a bar makes no sense.
    inline double barPosition(Scale scale, int raw)
    {
        switch (scale)
        {
            case Scale::unipolar:
            case Scale::bipolar:
            case Scale::percent:   return raw / 16383.0;
            case Scale::semitones: return (raw - 64 + 36) / 72.0;
            case Scale::toggle:    return raw != 0 ? 1.0 : 0.0;
            case Scale::raw:       return -1.0;
        }
        return -1.0;
    }

    // Address pages.
    constexpr int rigPage = 4, ampPage = 10, eqPage = 11, cabPage = 12;
    constexpr std::array<int, 8> slotPages { 50, 51, 52, 53, 56, 58, 60, 61 };
    constexpr std::array<const char*, 8> slotLabels { "A", "B", "C", "D", "X", "MOD", "DLY", "REV" };

    struct Rig
    {
        std::map<std::pair<int, int>, std::string> strings;  // (page, param)
        std::map<std::pair<int, int>, int> values;           // 14-bit raw values

        std::string text(int page, int param) const
        {
            const auto it = strings.find({ page, param });
            return it != strings.end() ? it->second : std::string();
        }
        int value(int page, int param, int fallback = -1) const
        {
            const auto it = values.find({ page, param });
            return it != values.end() ? it->second : fallback;
        }

        std::string name() const      { return text(0, 1); }
        std::string author() const    { return text(0, 2); }
        std::string date() const      { return text(0, 3); }
        std::string ampName() const   { return text(0, 0x10); }
        std::string ampAuthor() const { return text(0, 0x11); }
        std::string cabName() const   { return text(0, 0x20); }
        std::string cabAuthor() const { return text(0, 0x21); }
        std::string firmware() const
        {
            const auto full = text(0x4e, 0x0e);  // e.g. "14.2.1.prof20.public.20260624"
            const auto dot = full.find(".prof");
            return dot == std::string::npos ? full : full.substr(0, dot);
        }
        int slotType(std::size_t slot) const { return value(slotPages[slot], 0); }
        bool slotOn(std::size_t slot) const  { return value(slotPages[slot], 3, 0) != 0; }
    };

    // Kemper knobs: unipolar 0..10, bipolar -5..+5.
    inline double unipolar(int raw) { return raw * 10.0 / 16383.0; }
    inline double bipolar(int raw)  { return (raw - 8192) * 5.0 / 8192.0; }

    namespace detail
    {
        inline std::uint32_t read32(const std::uint8_t* p)
        {
            return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16)
                 | (std::uint32_t(p[2]) << 8) | std::uint32_t(p[3]);
        }

        inline bool readVarLen(const std::uint8_t*& p, const std::uint8_t* end, std::size_t& out)
        {
            out = 0;
            for (int i = 0; i < 4 && p < end; ++i)
            {
                const auto c = *p++;
                out = (out << 7) | (c & 0x7f);
                if (c < 0x80) return true;
            }
            return false;
        }

        inline std::string readString(const std::uint8_t* p, const std::uint8_t* end)
        {
            std::string s;
            while (p < end && *p != 0) s.push_back(static_cast<char>(*p++));
            return s;
        }

        // `m` is a SysEx body after F0: 00 20 33 <product> <device> <fn> <instance> <page> <param> ...
        inline void applyMessage(Rig& rig, const std::uint8_t* m, std::size_t size)
        {
            if (size < 9 || m[0] != 0x00 || m[1] != 0x20 || m[2] != 0x33) return;
            const auto* end = m + size - (m[size - 1] == 0xf7 ? 1 : 0);
            const int fn = m[5], page = m[7], param = m[8];
            const auto* data = m + 9;
            switch (fn)
            {
                case 0x01:  // single parameter
                    if (end - data >= 2) rig.values[{ page, param }] = (data[0] << 7) | data[1];
                    break;
                case 0x02:  // consecutive parameters
                    for (int i = 0; end - data >= 2; ++i, data += 2)
                        rig.values[{ page, param + i }] = (data[0] << 7) | data[1];
                    break;
                case 0x03:  // string
                    rig.strings[{ page, param }] = readString(data, end);
                    break;
                default:
                    break;
            }
        }
    }

    // Returns false if the data is not a .krig file.
    inline bool parse(const std::uint8_t* data, std::size_t size, Rig& rig)
    {
        using namespace detail;
        rig = {};
        const auto* end = data + size;
        if (size < 14 || std::string(reinterpret_cast<const char*>(data), 4) != "KThd") return false;
        const auto* p = data + 8 + read32(data + 4);
        if (end - p < 8 || std::string(reinterpret_cast<const char*>(p), 4) != "KTrk") return false;
        const auto trackLength = read32(p + 4);
        p += 8;
        if (static_cast<std::size_t>(end - p) > trackLength) end = p + trackLength;

        std::uint8_t runningStatus = 0;
        while (p < end)
        {
            std::size_t delta = 0, length = 0;
            if (!readVarLen(p, end, delta) || p >= end) break;
            auto status = *p;
            if (status >= 0x80) ++p;
            else status = runningStatus;

            if (status == 0xf0 || status == 0xf7)
            {
                if (!readVarLen(p, end, length) || static_cast<std::size_t>(end - p) < length) break;
                applyMessage(rig, p, length);
                p += length;
            }
            else if (status == 0xff)
            {
                if (end - p < 1) break;
                ++p;  // meta type
                if (!readVarLen(p, end, length) || static_cast<std::size_t>(end - p) < length) break;
                p += length;
            }
            else if (status >= 0x80)
            {
                runningStatus = status;
                const auto type = status & 0xf0;
                p += (type == 0xc0 || type == 0xd0) ? 1 : 2;
            }
            else
                break;
        }
        return !rig.strings.empty() || !rig.values.empty();
    }
}
