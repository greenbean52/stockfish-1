/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2009 Marco Costalba

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


////
//// Includes
////

#include <algorithm>
#include <cassert>
#include <map>
#include <string>
#include <sstream>
#include <vector>

#include "misc.h"
#include "thread.h"
#include "ucioption.h"

using std::string;

////
//// Local definitions
////

namespace {

  ///
  /// Types
  ///

  enum OptionType { SPIN, COMBO, CHECK, STRING, BUTTON };

  typedef std::vector<string> ComboValues;

  struct Option {

    string name, defaultValue, currentValue;
    OptionType type;
    size_t idx;
    int minValue, maxValue;
    ComboValues comboValues;

    Option();
    Option(const char* defaultValue, OptionType = STRING);
    Option(bool defaultValue, OptionType = CHECK);
    Option(int defaultValue, int minValue, int maxValue);

    bool operator<(const Option& o) const { return this->idx < o.idx; }
  };

  typedef std::map<string, Option> Options;

  ///
  /// Constants
  ///

  // load_defaults populates the options map with the hard
  // coded names and default values.

  void load_defaults(Options& o) {

    o["Use Search Log"] = Option(false);
    o["Search Log Filename"] = Option("SearchLog.txt");
    o["Book File"] = Option("book.bin");
    o["Mobility (Middle Game)"] = Option(100, 0, 200);
    o["Mobility (Endgame)"] = Option(100, 0, 200);
    o["Pawn Structure (Middle Game)"] = Option(100, 0, 200);
    o["Pawn Structure (Endgame)"] = Option(100, 0, 200);
    o["Passed Pawns (Middle Game)"] = Option(100, 0, 200);
    o["Passed Pawns (Endgame)"] = Option(100, 0, 200);
    o["Space"] = Option(100, 0, 200);
    o["Aggressiveness"] = Option(100, 0, 200);
    o["Cowardice"] = Option(100, 0, 200);
    o["King Safety Curve"] = Option("Quadratic", COMBO);

       o["King Safety Curve"].comboValues.push_back("Quadratic");
       o["King Safety Curve"].comboValues.push_back("Linear");  /*, "From File"*/

    o["King Safety Coefficient"] = Option(40, 1, 100);
    o["King Safety X Intercept"] = Option(0, 0, 20);
    o["King Safety Max Slope"] = Option(30, 10, 100);
    o["King Safety Max Value"] = Option(500, 100, 1000);
    o["Queen Contact Check Bonus"] = Option(3, 0, 8);
    o["Queen Check Bonus"] = Option(2, 0, 4);
    o["Rook Check Bonus"] = Option(1, 0, 4);
    o["Bishop Check Bonus"] = Option(1, 0, 4);
    o["Knight Check Bonus"] = Option(1, 0, 4);
    o["Discovered Check Bonus"] = Option(3, 0, 8);
    o["Mate Threat Bonus"] = Option(3, 0, 8);
    o["Check Extension (PV nodes)"] = Option(2, 0, 2);
    o["Check Extension (non-PV nodes)"] = Option(1, 0, 2);
    o["Single Reply Extension (PV nodes)"] = Option(2, 0, 2);
    o["Single Reply Extension (non-PV nodes)"] = Option(2, 0, 2);
    o["Mate Threat Extension (PV nodes)"] = Option(0, 0, 2);
    o["Mate Threat Extension (non-PV nodes)"] = Option(0, 0, 2);
    o["Pawn Push to 7th Extension (PV nodes)"] = Option(1, 0, 2);
    o["Pawn Push to 7th Extension (non-PV nodes)"] = Option(1, 0, 2);
    o["Passed Pawn Extension (PV nodes)"] = Option(1, 0, 2);
    o["Passed Pawn Extension (non-PV nodes)"] = Option(0, 0, 2);
    o["Pawn Endgame Extension (PV nodes)"] = Option(2, 0, 2);
    o["Pawn Endgame Extension (non-PV nodes)"] = Option(2, 0, 2);
    o["Full Depth Moves (PV nodes)"] = Option(14, 1, 100);
    o["Full Depth Moves (non-PV nodes)"] = Option(3, 1, 100);
    o["Threat Depth"] = Option(5, 0, 100);
    o["LSN filtering"] = Option(false);
    o["LSN Time Margin (sec)"] = Option(4, 1, 10);
    o["LSN Value Margin"] = Option(200, 100, 600);
    o["Randomness"] = Option(0, 0, 10);
    o["Minimum Split Depth"] = Option(4, 4, 7);
    o["Maximum Number of Threads per Split Point"] = Option(5, 4, 8);
    o["Threads"] = Option(1, 1, 8);
    o["Hash"] = Option(32, 4, 4096);
    o["Clear Hash"] = Option(false, BUTTON);
    o["Ponder"] = Option(true);
    o["OwnBook"] = Option(true);
    o["MultiPV"] = Option(1, 1, 500);
    o["UCI_ShowCurrLine"] = Option(false);
    o["UCI_Chess960"] = Option(false);

    // Any option should know its name so to be easily printed
    for (Options::iterator it = o.begin(); it != o.end(); ++it)
        it->second.name = it->first;
  }

  ///
  /// Variables
  ///

  Options options;

  // stringify converts a value of type T to a std::string
  template<typename T>
  string stringify(const T& v) {

     std::ostringstream ss;
     ss << v;
     return ss.str();
  }


  // get_option_value implements the various get_option_value_<type>
  // functions defined later, because only the option value
  // type changes a template seems a proper solution.

  template<typename T>
  T get_option_value(const string& optionName) {

      T ret = T();
      if (options.find(optionName) == options.end())
          return ret;

      std::istringstream ss(options[optionName].currentValue);
      ss >> ret;
      return ret;
  }

}

////
//// Functions
////

/// init_uci_options() initializes the UCI options.  Currently, the only
/// thing this function does is to initialize the default value of the
/// "Threads" parameter to the number of available CPU cores.

void init_uci_options() {

  load_defaults(options);

  // Limit the default value of "Threads" to 7 even if we have 8 CPU cores.
  // According to Ken Dail's tests, Glaurung plays much better with 7 than
  // with 8 threads.  This is weird, but it is probably difficult to find out
  // why before I have a 8-core computer to experiment with myself.
  assert(options.find("Threads") != options.end());
  assert(options.find("Minimum Split Depth") != options.end());

  options["Threads"].defaultValue = stringify(Min(cpu_count(), 7));
  options["Threads"].currentValue = stringify(Min(cpu_count(), 7));

  // Increase the minimum split depth when the number of CPUs is big.
  // It would probably be better to let this depend on the number of threads
  // instead.
  if (cpu_count() > 4)
  {
      options["Minimum Split Depth"].defaultValue = "6";
      options["Minimum Split Depth"].currentValue = "6";
  }
}


/// print_uci_options() prints all the UCI options to the standard output,
/// in the format defined by the UCI protocol.

void print_uci_options() {

  static const char optionTypeName[][16] = {
    "spin", "combo", "check", "string", "button"
  };

  // Build up a vector out of the options map and sort it according to idx
  // field, that is the chronological insertion order in options map.
  std::vector<Option> vec;
  for (Options::const_iterator it = options.begin(); it != options.end(); ++it)
      vec.push_back(it->second);

  std::sort(vec.begin(), vec.end());

  for (std::vector<Option>::const_iterator it = vec.begin(); it != vec.end(); ++it)
  {
      std::cout << "\noption name " << it->name
                << " type "         << optionTypeName[it->type];

      if (it->type == BUTTON)
          continue;

      if (it->type == CHECK)
          std::cout << " default " << (it->defaultValue == "1" ? "true" : "false");
      else
          std::cout << " default " << it->defaultValue;

      if (it->type == SPIN)
          std::cout << " min " << it->minValue << " max " << it->maxValue;
      else if (it->type == COMBO)
          for (ComboValues::const_iterator itc = it->comboValues.begin();
              itc != it->comboValues.end(); ++itc)
              std::cout << " var " << *itc;
  }
  std::cout << std::endl;
}


/// get_option_value_bool() returns the current value of a UCI parameter of
/// type "check".

bool get_option_value_bool(const string& optionName) {

  return get_option_value<bool>(optionName);
}


/// get_option_value_int() returns the value of a UCI parameter as an integer.
/// Normally, this function will be used for a parameter of type "spin", but
/// it could also be used with a "combo" parameter, where all the available
/// values are integers.

int get_option_value_int(const string& optionName) {

  return get_option_value<int>(optionName);
}


/// get_option_value_string() returns the current value of a UCI parameter as
/// a string. It is used with parameters of type "combo" and "string".

string get_option_value_string(const string& optionName) {

   return get_option_value<string>(optionName);
}


/// set_option_value() inserts a new value for a UCI parameter. Note that
/// the function does not check that the new value is legal for the given
/// parameter: This is assumed to be the responsibility of the GUI.

void set_option_value(const string& name, const string& value) {

  // UCI protocol uses "true" and "false" instead of "1" and "0", so convert
  // value according to standard C++ convention before to store it.
  string v(value);
  if (v == "true")
      v = "1";
  else if (v == "false")
      v = "0";

  if (options.find(name) != options.end())
      options[name].currentValue = v;
  else
      std::cout << "No such option: " << name << std::endl;
}


/// push_button() is used to tell the engine that a UCI parameter of type
/// "button" has been selected:

void push_button(const string& buttonName) {

  set_option_value(buttonName, "true");
}


/// button_was_pressed() tests whether a UCI parameter of type "button" has
/// been selected since the last time the function was called, in this case
/// it also resets the button.

bool button_was_pressed(const string& buttonName) {

  if (!get_option_value<bool>(buttonName))
	  return false;

  set_option_value(buttonName, "false");
  return true;
}


namespace {

  // Define constructors of Option class.

  Option::Option() {} // To allow insertion in a std::map

  Option::Option(const char* def, OptionType t)
  : defaultValue(def), currentValue(def), type(t), idx(options.size()), minValue(0), maxValue(0) {}

  Option::Option(bool def, OptionType t)
  : defaultValue(stringify(def)), currentValue(stringify(def)), type(t), idx(options.size()), minValue(0), maxValue(0) {}

  Option::Option(int def, int minv, int maxv)
  : defaultValue(stringify(def)), currentValue(stringify(def)), type(SPIN), idx(options.size()), minValue(minv), maxValue(maxv) {}

}
