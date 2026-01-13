#pragma once
#include <map>
#include <string>
#include "search.h"
#include "types.h"

namespace Nebula{
  class Position;

  namespace Uci{
    class Option;

    struct CaseInsensitiveLess{
      bool operator()(const std::string&, const std::string&) const;
    };

    using OptionsMap = std::map<std::string, Option, CaseInsensitiveLess>;

    class Option{
    public:
      using OnChange = void(*)(const Option&);

      explicit Option(const OnChange f=nullptr)
        : type("string"), min(0), max(0), on_change(f){}

      explicit Option(const bool v, const OnChange f=nullptr)
        : defaultValue(v?"true":"false"), currentValue(defaultValue), type("check"), min(0), max(0), on_change(f){}

      explicit Option(const char* v, const OnChange f=nullptr)
        : defaultValue(v), currentValue(v), type("string"), min(0), max(0), on_change(f){}

      Option(const double v, const int minv, const int maxv, const OnChange f=nullptr)
        : defaultValue(std::to_string(v)), currentValue(defaultValue), type("spin"), min(minv), max(maxv),
        on_change(f){}

      Option(const char* v, const char* cur, const OnChange f=nullptr)
        : defaultValue(v), currentValue(cur), type("combo"), min(0), max(0), on_change(f){}

      Option& operator=(const std::string& v){
        currentValue=v;
        if (on_change)
          on_change(*this);
        return *this;
      }

      void operator<<(const Option& o){
        defaultValue=o.defaultValue;
        currentValue=o.currentValue;
        type=o.type;
        min=o.min;
        max=o.max;
        on_change=o.on_change;
      }

      // ----- Typed accessors -----
      [[nodiscard]] bool asBool() const{ return currentValue=="true"||currentValue=="1"; }
      [[nodiscard]] int asInt() const{ return std::stoi(currentValue); }
      [[nodiscard]] size_t asSize() const{ return std::stoull(currentValue); }
      [[nodiscard]] double asDouble() const{ return std::stod(currentValue); }
      [[nodiscard]] const std::string& asString() const{ return currentValue; }
      bool operator==(const char* s) const{ return currentValue==s; }
    private:
      friend std::ostream& operator<<(std::ostream&, const OptionsMap&);
      std::string defaultValue;
      std::string currentValue;
      std::string type;
      int min, max;
      OnChange on_change;
    };

    void init(OptionsMap&);
    void loop(int argc, char* argv[]);
    std::string value(Value v);
    std::string square(Square s);
    std::string move(Move m, bool chess960);
    std::string pv(const Position& pos, Depth depth);
    Move toMove(const Position& pos, std::string& str);
  }

  extern Uci::OptionsMap options;
}
