// a command line utility for storing, modifying and managing a database of many items and their information.

#include <experimental/filesystem>
#include <functional>
#include <string>
#include <vector>
#include <fstream>

#include "include/argh.h"
#include "include/pretty.hpp"
#include "include/json.hpp"

namespace nm = nlohmann;
namespace fs = std::experimental::filesystem;

// used to determine whether a parameter's value is empty
#define ABSENT "  "
// github url for help
#define GITHUB "https://github.com/jibstack64/know-it-all"
// generic error strings
#define OUTFILE_GET_ERROR ": No outfile provided. Run with '-? o' for more information."
#define ITEM_GET_ERROR ": No item identifier provided. Run with '-? @' for more information."
#define ITEM_EXISTS_ERROR ": The item identifier provided does not exist in the database."
#define NO_KEY_ERROR ": No key provided."
#define INVALID_TYPE_ERROR ": Invalid type. Run with '-? t' for more information."
#define CANNOT_POP_KEY_IDENTIFIER_ERROR ": Cannot pop key - it is the name/identifier."
#define TYPE_CONVERSION_ERROR ": Error whilst converting provided value to type."
#define NO_ITEMS_TO_REMOVE_ERROR ": No items to remove."
#define KEY_NOT_PRESENT_ERROR ": Could not pop key; it could not be found in the item(s)."
#define OUTFILE_NO_EXIST_ERROR ": The outfile provided does not exist, or is prohibited."
#define NO_INSTANCE_ERROR ": Failed to find any instances of the term provided."
#define JSON_ERROR ": Error reading JSON from outfile. Check that the JSON format is valid."

// holds a parameter's data and its final value.
struct parameter {
    std::vector<std::string> names;
    std::string passed;
    std::string description;
    bool passedRequired;

    std::string result;

    std::function<bool(parameter&, const std::string)> execute;

    // forms a string containing all names seperated by a slash or 'sep'.
    const std::string prettify(const std::string sep = "/") {
        std::string o;
        for (const auto& n : names) {
            o += (names.back() == n) ? n : n + sep;
        }
        return o;
    }

    parameter(const std::initializer_list<std::string> names, const std::string description, 
            const std::string passed, std::function<bool(parameter&, const std::string)> execute, bool passedRequired = true)
            : names(names), description(description), passed(passed), passedRequired(passedRequired), execute(execute) {}
};

std::vector<parameter> mainParameters;

// returns a fatal error and exits.
template<typename T>
int fatal(T sad, int status = 1) {
    std::cout << pty::paint(sad, "lightred") << " [" << pty::paint(status, {"red", "dim"}) << "]" << std::endl;
    exit(status);
    return status;
}

// middlepoint of fatal and success.
template<typename T>
int warning(T headscratch, float status = 0.5) {
    for (const auto& p : mainParameters) {
        if (p.names[0] == "V") {
            if (p.passed == "verbose") {
                std::cout << pty::paint(headscratch, "yellow") << " [" << pty::paint(status, {"yellow", "dim"}) << "]" << std::endl;
            }
        }
    }
    return status;
}

// the opposite of fatal.
template<typename T>
int success(T hooray, int status = 0) {
    std::cout << pty::paint(hooray, "lightgreen") << " [" << pty::paint(status, "green") << "]" << std::endl;
    return status;
}

// highlights 'term' in 'value' and returns the result. paints non-highlighted with 'others'.
const std::string highlight(const std::string value, const std::string term, std::initializer_list<const char *> others) {
    // dont bother if the value is not present
    std::string first;
    std::string ter;
    std::string after;
    if (value.find(term) == std::string::npos) {
        return pty::paint(value, others);
    } else {
        int place = value.find(term);
        for (int i = 0; i < value.size(); i++) {
            // first
            if (i < place) {
                first += value[i];
            }
            // term
            else if (i >= place && i < term.size()) {
                ter += value[i];
            }
            // after
            else {
                after += value[i];
            }
        }
        // paint
        first = pty::paint(first, others);
        after = highlight(after, term, others);
    }

    // form and return
    std::vector<const char *> os = others;
    os.push_back("reversefield"); 
    return first + pty::paint(ter, os) + after;
}

bool help(parameter& parent, const std::string param) {
    std::string toc = "";

    for (auto& p : mainParameters) {
        // if param is provided
        bool in;
        for (const auto& n : p.names) {
            if (n == param) {
                in = true;
                break;
            } else {
                in = false;
            }
        }
        // add to toc
        if (param == ABSENT || in) {
            // name
            toc += "# " + pty::paint(p.prettify(), {"magenta", "underlined"}) + " ";
            // takes
            if (p.passed != "") {
                toc += pty::paint(p.passedRequired ? "<" : "[", "grey") + pty::paint(p.passed, "yellow") +
                        pty::paint(p.passedRequired ? ">" : "]", "grey");
            }
            // desc
            toc += "\n" + pty::paint(p.description, {"grey", "italic"}) + "\n\n";
        }
    }

    // if the param does not exist
    if (toc == "" && param != ABSENT) {
        return fatal(parent.prettify() + ": Parameter '" + param + "' does not exist.");
    }

    // add github for extra help
    toc = pty::paint("Need extra help?\n" + (std::string)GITHUB + "\n\n", {"turqoise", "underlined"}) + toc;
    
    // feed to console
    std::cout << toc;

    return true;
}

bool outfile(parameter& parent, const std::string path) {
    // check if path exists
    if (!fs::exists(path)) {
        return fatal(parent.prettify() + OUTFILE_NO_EXIST_ERROR);
    }

    parent.result = path;
    return false;
}

// used for reading json in conjunction with parameters.
nm::json read(parameter& parent, const std::string path) {
    // read json data
    std::ifstream file(path);
    nm::json jf;
    try {
        jf = nm::json::parse(file);
    } catch (nm::json::exception) { // catch json errors
        return fatal(parent.prettify() + JSON_ERROR);
    }
    file.close(); // safe and snuggly!
    
    // make sure is array
    if (!jf.is_array()) {
        return fatal(parent.prettify() + JSON_ERROR);
    }

    return jf;
}

// iterates all parameters until the one with a name of 'firstName' is found, then returns that parameters final value.
// writes 'error' string to the console if the final value is not present.
const std::string getfinal(parameter& parent, const std::string firstName, const std::string error, bool raises = true) {
    std::string result;
    for (const auto& p : mainParameters) {
        if (p.names[0] == firstName) {
            if (p.result == "") {
                if (raises) {
                    fatal(parent.prettify() + error);
                } else {
                    continue;
                }
            } else {
                result = p.result;
                break;
            }
        }
    }
    return result;
}

// gets outfile path
const std::string getout(parameter& parent, bool raises = false) {
    std::string out = getfinal(parent, "o", OUTFILE_GET_ERROR, raises);
    if (out == "") {
        warning(parent.prettify() + ": No outfile provided, defaulting to './database.json'.");
        out = "./database.json";
        if (!fs::exists(out)) {
            fatal(parent.prettify() + OUTFILE_NO_EXIST_ERROR);
        } else {
            // set as outfile final
            for (auto& p : mainParameters) {
                if (p.names[0] == "o") {
                    p.result = out;
                }
            }
        }
    }
    return out;
}

// gets item identifier
const std::string getitem(parameter& parent, bool raises = true) {
    return getfinal(parent, "@", ITEM_GET_ERROR, raises);
}

// gets key for modification
const std::string getkey(parameter& parent, bool raises = true) {
    return getfinal(parent, "k", NO_KEY_ERROR, raises);
}

// gets type for modification
const std::string gettype(parameter& parent) {
    std::string fin = getfinal(parent, "t", "", false);
    if (fin == "") {
        fin = "string";
    }
    return fin;
}

// used for writing json in conjunction with parameters.
void write(parameter& parent, const std::string path, nm::json& value) {
    // write to file
    std::ofstream out(path);
    out << std::setw(4) << value << std::endl;
    out.close();
}

bool add(parameter& parent, const std::string identifier) {
    // get path
    std::string path = getout(parent);

    // read json data
    nm::json jf = read(parent, path);

    // check if item with matching identifier is already in the database
    for (const auto& j : jf) {
        if (j["identifier"] == identifier) {
            return fatal(parent.prettify() + ": An item with the identifier '" + identifier + "' is already present within the database.");
        }
    }

    // form json object
    nm::json it;
    it["identifier"] = identifier;

    jf.push_back(it); // add item

    // write json
    write(parent, path, jf);

    // ALSO set item to working item
    for (auto& p : mainParameters) {
        if (p.names[0] == "@") {
            p.result = identifier;
        }
    }

    // success message
    success("Item of identifier '" + identifier + "' has been added to the database.");

    return false;
}

bool erase(parameter& parent, const std::string _) {
    // get outpath and item
    std::string path = getout(parent);
    std::string identifier = getitem(parent);

    // read json
    nm::json jf = read(parent, path);

    // if no items to remove
    if (jf.empty()) {
        return fatal(parent.prettify() + NO_ITEMS_TO_REMOVE_ERROR);
    }

    // iterate until found
    if (identifier != "[ALL]") {
        int p;
        for (int i = 0; i < jf.size(); i++) {
            if (jf[i]["identifier"] == identifier) {
                p = i;
                break;
            }
        }
        jf.erase(p); // remove
    } else {
        jf.clear();
    }

    // success message
    success("Removed item(s) '" + identifier + "'.");

    // write new json
    write(parent, path, jf);

    return true;
}

bool item(parameter& parent, const std::string identifier) {
    // all?
    if (identifier != "[ALL]") {
        // read json
        nm::json jf = read(parent, getout(parent));

        // ensure that the item exists
        bool found = false;
        for (const auto& j : jf) {
            if (j["identifier"] == identifier) {
                found = true;
            }
        }

        // if not found, ararrghH!!!!
        if (!found) {
            return fatal(parent.prettify() + ITEM_EXISTS_ERROR);
        }
    }

    parent.result = identifier;
    return false;
}

bool key(parameter& parent, const std::string key_name) {
    parent.result = key_name;
    return false;
}

bool value(parameter& parent, const std::string new_value) {
    // get the path and identifier
    std::string key = getkey(parent);
    std::string otype = gettype(parent);
    std::string path = getout(parent);
    std::string identifier = getitem(parent);

    std::string fval = new_value; // eek!

    // read the json
    nm::json jf = read(parent, path);

    // find the element and replace the key
    bool said = false;
    for (auto& j : jf) {
        if (j["identifier"] == identifier || identifier == "[ALL]") {
            // attempt at conversion
            if (otype == "null") {
                j[key] = {};
            } else if (otype == "string") {
                j[key] = fval;
            } else if (otype == "int/integer") {
                int val;
                try {
                    val = std::stoi(fval);
                } catch (std::exception) {
                    return fatal(parent.prettify() + TYPE_CONVERSION_ERROR);
                }
                j[key] = val;
            } else if (otype == "float/decimal") {
                double val;
                try {
                    val = std::stod(fval);
                } catch (std::exception) {
                    return fatal(parent.prettify() + TYPE_CONVERSION_ERROR);
                }
                j[key] = val;
            } else {
                // impossible, but you never know
                return fatal(parent.prettify() + INVALID_TYPE_ERROR);
            }
            if (!said) {
                success("Value of key '" + key + "' has been assigned the value '" + fval + "' (of type '" + otype + "') for item(s) '" + identifier + "'.");
                said = true;
            }; // basically only say once for [ALL]
            if (identifier != "[ALL]") {
                break;
            }
        }
    }

    // write to json
    write(parent, path, jf);

    return false;
}

bool pop(parameter& parent, const std::string _) {
    // get stuffs
    std::string path = getout(parent);
    std::string key = getkey(parent);
    std::string identifier = getitem(parent);

    // this causes... a lot of issues
    if (key == "identifier") { 
        return fatal(parent.prettify() + CANNOT_POP_KEY_IDENTIFIER_ERROR);
    }

    // read json
    nm::json jf = read(parent, path);
    
    // pop value
    nm::json jfinal;
    bool changed;
    for (auto& j : jf) {
        if (j["identifier"] == identifier || identifier == "[ALL]") {
            nm::json cp;
            for (auto& i : j.items()) {
                if (i.key() != key) {
                    cp[i.key()] = i.value();
                } else {
                    changed = true;
                }
            }
            jfinal.push_back(cp);
        } else {
            jfinal.push_back(j);
        }
    }

    // if nothing changed
    if (!changed) {
        return fatal(parent.prettify() + KEY_NOT_PRESENT_ERROR);
    }

    // write json
    write(parent, path, jfinal);

    // success
    success("Key '" + key + "' removed from item(s) '" + identifier + "'.");

    return false;
}

bool type(parameter& parent, const std::string object_type) {
    // might as well get it over with
    if (object_type == "string" || object_type == ABSENT) {
        parent.result = "string";
    } else if (object_type == "int" || object_type == "integer") {
        parent.result = "int/integer";
    } else if (object_type == "float" || object_type == "decimal") {
        parent.result = "float/decimal";
    } else if (object_type == "null") {
        parent.result = "null";
    } else {
        return fatal(parent.prettify() + INVALID_TYPE_ERROR);
    }

    return false;
}

bool readable(parameter& parent, const std::string _identifier) {
    // fetch items and all
    std::string path = getout(parent);
    std::string identifier;
    if (_identifier != ABSENT) {
        identifier = _identifier;
    } else {
        identifier = getitem(parent, false);
    }

    // if no identifier
    if (identifier == "" || identifier == "[ALL]") {
        for (const auto& j : read(parent, path)) {
            readable(parent, j["identifier"]);
        }
    }

    read:
    // read all of contents
    std::string out; // string to be pushed to console
    nm::json jf = read(parent, path);
    for (const auto& j : jf) {
        if (j["identifier"] == identifier || identifier == "[ALL]") {
            out += pty::paint("> ", "grey") + pty::paint(j["identifier"].get<std::string>(), {"yellow", "bold"}) + "\n";
            int i = 0; // tracker
            for (auto& kav : j.items()) {
                i++;
                if (kav.key() == "identifier") {
                    continue;
                }
                out += pty::paint(kav.key(), {"turqoise", "italic"}) + " : ";
                out += pty::paint(kav.value(), "yellow") + "\n";
            }
            if (i < 2) {
                out += pty::paint("N/A", "grey") + "\n";
            }
        }
    }

    // out to console
    std::cout << out;

    return true;
}

bool verbose(parameter& parent, const std::string _) {
    parent.passed = "verbose";
    return false;
}

bool search(parameter& parent, const std::string term) {
    // get values
    std::string path = getout(parent);

    // read json
    nm::json jf = read(parent, path);

    // iterate
    std::string out = "";
    for (const auto& j : jf) {
        int p = 0;
        std::string temp = "";
        for (const auto& kav : j.items()) {
            std::ostringstream oss;
            oss << kav.value();
            std::string val = oss.str();
            if ((kav.key().find(term) != std::string::npos || val.find(term) != std::string::npos)) {
                if (kav.key() != "identifier") {    
                    temp += highlight(kav.key(), term, {"turqoise", "italic"}) + " : ";
                    temp += highlight(val, term, {"yellow"}) + "\n";
                }
                p++;
            }
        }
        // if something was found
        if (p != 0) {
            temp = pty::paint("> ", "grey") + highlight(j["identifier"], term, {"yellow", "bold"}) + "\n" + temp;
        }
        out += temp;
    }

    // out to terminal if out was modified
    if (out == "") {
        return fatal(parent.prettify() + NO_INSTANCE_ERROR);
    } else {
        std::cout << out;
    }

    return true;
}

int main(int argc, char ** argv) {
    // all parameters
    // organised in such a manner that, during iteration, parameters will work no matter the order
    mainParameters = {
        (parameter({"?", "help"},
        "Provides help for all/a parameter(s).",
        "parameter", help, false)),

        (parameter({"V", "verbose"},
        "Enables warning errors.",
        "", verbose, false)),

        (parameter({"o", "outfile"},
        "Specifies the target database JSON file. If none is provided, a './database.json' will be assumed.",
        "path", outfile, true)),

        (parameter({"s", "search"}, 
        "Searches through the database for the provided term. Prints matching to the console.",
        "term", search, true)),

        (parameter({"@", "item"},
        "Specifies the item to be used with the !/erase, k/key and r/readable parameters.",
        "name/identifier", item, true)),
        
        (parameter({"+", "add"},
        "Adds an item to the database by its name/identifier.",
        "name/identifier", add, true)),
        
        (parameter({"!", "erase"},
        "Removes the specified item.",
        "", erase, false)),

        (parameter({"r", "readable"},
        "Reads the entirety of a item's contents in a readable format. If no item is specified with @/item, all items in the database will be displayed neatly.",
        "", readable, false)),

        (parameter({"t", "type"},
        "Specifies the type of contents that v/value holds. Can be 'string', 'int'/'integer', 'float'/'decimal' or 'null'.",
        "type-name", type, true)),

        (parameter({"k", "key"},
        "Specifies the key to be modified on the item.",
        "key", key, true)),

        (parameter({"p", "pop"},
        "Pops key, removing it from the item specified.",
        "", pop, false)),

        (parameter({"v", "value"},
        "The value to be assigned to the k/key. Spaces in the input stream must be replaced with :/s.",
        "new-value", value, true)),

    };

    // add arguments via argh
    argh::parser parser;
    for (const auto& param : mainParameters) {
        for (const auto& name : param.names) {
            parser.add_param(name);
        }
    }

    // parse for arguments
    parser.parse(argc, argv);

    // loop through args and assign end values
    std::vector<std::string> ignored; // contains ignored params
    bool received = false;
    for (auto& param : mainParameters) {
        // if the parameter has been passed
        std::string value = ABSENT;
        for (const auto& arg : parser.params()) {
            if (value != ABSENT) {
                break;
            }
            for (const auto& name : param.names) {
                if (arg.first == name) {
                    value = arg.second;
                    break;
                }
            }
        }

        // if the parameter can also be a flag, check for that
        bool flag = false;
        for (const auto& fl : parser.flags()) {
            if (flag) {
                break;
            }
            for (const auto& name : param.names) {
                if (fl == name) {
                    flag = true;
                    break;
                }
            }
        }

        // if no value, but requires, throw
        if (flag && param.passedRequired) {
            return fatal("Parameter '" + param.prettify("'/'") + "' missing argument(s).");
        }

        // check for collides
        if (flag && value != ABSENT) {
            return fatal("Parameter '" + param.prettify("'/'") + "' passed twice.");
        } else if (!flag && value == ABSENT) {
            //std::cout << param.prettify() << " <- NOT PASSED" << std::endl;
            continue; // if not passed
        }
        //std::cout << param.prettify() << " <- YES PASSED : " << ((value == ABSENT) ? "N/A" : value) << std::endl;

        // add spaces to value
        if (value != ABSENT) {
            // add spaces in new_value
            std::string fval;
            bool possible = false;
            for (int i = 0; i < value.size(); i++) {
                if (value[i] == ':' && value[i+1] == '/' && value[i+2] == 's') {
                    fval += " ";
                    i += 2;
                    continue;
                } else {
                    fval += value[i];
                }
            }
            value = fval;
        }

        // run execute() function
        bool terminating = param.execute(param, value);
        received = true;
        if (terminating) {
            return 0;
        }
    }

    // if no parameters provided, return error
    if (!received) {
        return fatal("No parameters provided.");
    }
}
