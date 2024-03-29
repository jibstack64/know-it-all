// a command line utility for storing, modifying and managing a database of many items and their information.
// please, ignore the messiness (and laziness) of some of this code, a cleanup is due!

#include <experimental/filesystem>
#include <functional>
#include <string>
#include <vector>
#include <fstream>
#include <istream>

#ifdef _WIN32
#include <iostream> // required for win compilers
#endif
#include "include/argh.h"
#include "include/pretty.hpp"
#include "include/json.hpp"

namespace nm = nlohmann;
namespace fs = std::experimental::filesystem;

// min lengths
#define MINPASS 8
// used to determine whether a parameter's value is empty
#define ABSENT "  "
#define DATABASE (std::string)"./database.json"
#define DECRYPT (std::string)"./decrypted.json"
#define ENCRYPT (std::string)"./encrypted.json"
// github url for help
#define GITHUB "https://github.com/jibstack64/know-it-all"
// generic error strings
#define OUTFILE_GET_ERROR ": No outfile provided. Run with '-? o' for more information."
#define ITEM_GET_ERROR ": No item identifier provided. Run with '-? @' for more information."
#define ITEM_EXISTS_ERROR ": The item identifier provided does not exist in the database."
#define NO_KEY_ERROR ": No key(s) provided."
#define INVALID_TYPE_ERROR ": Invalid type. Run with '-? t' for more information."
#define CANNOT_POP_KEY_IDENTIFIER_ERROR ": Popping the 'identifier' key is prohibited.."
#define TYPE_CONVERSION_ERROR ": Error whilst converting provided value to type."
#define NO_ITEMS_TO_REMOVE_ERROR ": No items to remove."
#define KEY_NOT_PRESENT_ERROR ": Could not pop key(s); they could not be found in the item(s)."
#define OUTFILE_NO_EXIST_ERROR ": The outfile provided does not exist, or is prohibited."
#define NO_INSTANCE_ERROR ": Failed to find any instances of the term provided."
#define JSON_ERROR ": Error reading JSON from outfile. Check that the JSON format is valid."
#define DECRYPT_FAIL_ERROR ": Decryption and reading of the JSON failed. Either the phrase is wrong, or the JSON format."
#define PASSCODE_SERIES_ERROR ": Your passphrase cannot be a series of the same character."
#define PHRASE_TOO_SHORT_ERROR ": Your passphrase must be atleast "+std::to_string(MINPASS).c_str()+" characters long."
#define PASSCODE_END_SERIES_ERROR ": Your passcode cannot end with a series of characters (it is obsolete)."
#define KEY_VAL_MATCH_ERROR ": Not enough keys for the values provided, or vice versa."
#define TOO_MANY_OTYPES_ERROR ": Too many types provided for the number of values."

// holds a parameter's data and its final value.
struct parameter {
    std::vector<std::string> names;
    std::string passed;
    std::string description;
    bool blockingFunc;
    bool passedRequired;

    std::string result;

    std::function<void(parameter&, const std::string)> execute;

    // forms a string containing all names seperated by a slash or 'sep'.
    const std::string prettify(const std::string sep = "/") {
        std::string o;
        for (const auto& n : names) {
            o += (names.back() == n) ? n : n + sep;
        }
        return o;
    }

    parameter(const std::initializer_list<std::string> names, const std::string description, 
            const std::string passed, std::function<void(parameter&, const std::string)> execute, bool passedRequired = true,
            bool blockingFunc = false) : names(names), description(description), passed(passed), passedRequired(passedRequired),
            blockingFunc(blockingFunc), execute(execute) {}
};

std::vector<parameter> mainParameters;

/*///////////////////////////////*
//  PARAMETER VALUE MODIF/READ  //
*///////////////////////////////*/

void setParameterValue(std::initializer_list<const char *> names, const std::string value) {
    std::vector<const char *> ns = names;
    for (auto& p : mainParameters) {
        if (ns[0] == p.names[0]) {
            p.result = value;
            return;
        }
    }
}

void setParameterValue(const char * name, const std::string value) {
    return setParameterValue({name}, value);
}

parameter* getParameter(const char * name) {
    for (auto& p : mainParameters) {
        for (auto& n : p.names) {
            if (n == name) {
                return &p;
            }
        }
    }
    return nullptr; 
}

/*//////////////*
//  COLOURING  //
*//////////////*/

// a proxy for the paint function, used for c/colourless parameter.
template<typename T>
const std::string paint(T value, std::vector<const char *> cns) {
    parameter* call = getParameter("c");
    if (call->result == "colourless") {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    } else {
        return pty::paint(value, cns);
    }
}

template<typename T>
const std::string paint(T value, std::initializer_list<const char *> cns) {
    return paint(value, std::vector(cns));
}

template<typename T>
const std::string paint(T value, const char * cn) {
    return paint(value, {cn});
}

// highlights 'term' in 'value' and returns the result. paints non-highlighted with 'others'.
const std::string highlight(const std::string value, const std::string term, std::initializer_list<const char *> others) {
    // dont bother if the value is not present
    std::string first;
    std::string ter;
    std::string after;
    if (value.find(term) == std::string::npos) {
        return paint(value, others);
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
        first = paint(first, others);
        after = highlight(after, term, others);
    }

    // form and return
    std::vector<const char *> os = others;
    os.push_back("reversefield"); 
    return first + paint(ter, os) + after;
}

/*////////////*
//  LOGGING  //
*////////////*/

// returns a fatal error and exits.
template<typename T>
int fatal(T sad, int status = 1) {
    std::cout << paint(sad, "lightred") << " [" << paint(status, {"red", "dim"}) << "]" << std::endl;
    if (getParameter("F")->passed != "force") {
        exit(status);
    }
    return status;
}

// middlepoint of fatal and success.
template<typename T>
float warning(T headscratch, float status = 0.5) {
    if (getParameter("V")->passed == "verbose") {
        std::cout << paint(headscratch, "yellow") << " [" << paint(status, {"yellow", "dim"}) << "]" << std::endl;
    }
    return status;
}

// the opposite of fatal.
template<typename T>
int success(T hooray, int status = 0) {
    std::cout << paint(hooray, "lightgreen") << " [" << paint(status, "green") << "]" << std::endl;
    return status;
}

/*///////////////*
//  READ/WRITE  //
*///////////////*/

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

// used for writing json in conjunction with parameters.
void write(parameter& parent, const std::string path, nm::json& value) {
    // write to file
    std::ofstream out(path);
    out << std::setw(4) << value << std::endl;
    out.close();
}

/*////////////////////////*
//  PARAM VALUE GETTERS  //
*////////////////////////*/

// iterates all parameters until the one with a name of 'firstName' is found, then returns that parameters final value.
// writes 'error' string to the console if the final value is not present.
const std::string getFinal(parameter& parent, const std::string firstName, const std::string error, bool raises = true) {
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
const std::string getOut(parameter& parent, bool raises = false, const std::string default_to = DATABASE) {
    std::string out = getFinal(parent, "o", OUTFILE_GET_ERROR, raises);
    if (out == "") {
        warning(parent.prettify() + ": No outfile provided, defaulting to '"+ default_to +"'.");
        out = default_to;
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
const std::string getItem(parameter& parent, bool raises = true) {
    return getFinal(parent, "@", ITEM_GET_ERROR, raises);
}

// gets key for modification
const std::vector<std::string> getKeys(parameter& parent, bool raises = true) {
    std::stringstream raw = std::stringstream(getFinal(parent, "k", NO_KEY_ERROR, raises));
    std::vector<std::string> keys;
    std::string segment;
    while(std::getline(raw, segment, ',')) {
        keys.push_back(segment);
    }
    return keys;
}

// gets vals for modification
const std::vector<std::string> getVals(const std::string _raw) {
    std::stringstream raw = std::stringstream(_raw);
    std::vector<std::string> vals;
    std::string segment;
    while(std::getline(raw, segment, ',')) {
        vals.push_back(segment);
    }
    return vals;
}

// gets type for modification
const std::vector<std::string> getTypes(parameter& parent) {
    std::stringstream raw = std::stringstream(getFinal(parent, "t", "", false));
    std::vector<std::string> types;
    std::string segment;
    while(std::getline(raw, segment, ',')) {
        types.push_back(segment);
    }
    return types;
}

/*//////////////////////
//  PARAM CORE FUNCS  //
*///////////////////////

void help(parameter& parent, const std::string param) {
    std::string toc = "";
    std::string order = paint("kial ", "magenta");

    for (auto& p : mainParameters) {
        // add to ordered text at end
        const char * eas = (p.blockingFunc ? "{}" : "[]");
        order += paint(eas[0], "grey") + paint(p.prettify(), "yellow") + paint(eas[1], "grey") + " ";
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
            toc += "⮑ " + paint("-" + p.prettify(), "magenta") + " ";
            // takes
            if (p.passed != "") {
                toc += paint(p.passedRequired ? "<" : "[", "grey") + paint(p.passed, "yellow") +
                        paint(p.passedRequired ? ">" : "]", "grey");
            }
            // desc
            toc += "\n  " + paint(p.description, {"grey", "italic"}) + "\n";
        }
    }

    // if the param does not exist
    if (toc == "" && param != ABSENT) {
        fatal(parent.prettify() + ": Parameter '" + param + "' does not exist.");
    }

    if (param == ABSENT) {
        // add github for extra help
        toc = paint("Need extra help?\n", "turqoise") + paint((std::string)GITHUB + "\n\n", {"turqoise", "underlined"}) + toc;
        toc += paint("\nParsing order:", {"bold"}) + "\n" + order;
    } else {
        toc = "\n" + toc;
    }

    // feed to console :D
    std::cout << toc << std::endl;
}

void outfile(parameter& parent, const std::string path) {
    // check if path exists
    if (!fs::exists(path)) {
        fatal(parent.prettify() + OUTFILE_NO_EXIST_ERROR);
    }

    parent.result = path;
}
        
void add(parameter& parent, const std::string identifier) {
    // get path
    std::string path = getOut(parent);

    // read json data
    nm::json jf = read(parent, path);

    // check if item with matching identifier is already in the database
    for (const auto& j : jf) {
        if (j["identifier"] == identifier) {
            fatal(parent.prettify() + ": An item with the identifier '" + identifier + "' is already present within the database.");
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
}

void erase(parameter& parent, const std::string _) {
    // get outpath and item
    std::string path = getOut(parent);
    std::string identifier = getItem(parent);

    // read json
    nm::json jf = read(parent, path);

    // if no items to remove
    if (jf.empty()) {
        fatal(parent.prettify() + NO_ITEMS_TO_REMOVE_ERROR);
    }

    auto n = nlohmann::json::array({});
    // iterate until found
    for (int i = 0; i < jf.size(); i++) {
        if (jf[i]["identifier"] == identifier || identifier == "[ALL]") {
            success("Removed item '" + std::string(jf[i]["identifier"]) + "' from the database.");
        } else {
            n.push_back(jf[i]);
        }
    }

    // write new json
    write(parent, path, n);    
}

void item(parameter& parent, const std::string identifier) {
    // all?
    if (identifier != "[ALL]") {
        // read json
        nm::json jf = read(parent, getOut(parent));

        // ensure that the item exists
        bool found = false;
        for (const auto& j : jf) {
            if (j["identifier"] == identifier) {
                found = true;
            }
        }

        // if not found, ararrghH!!!!
        if (!found) {
            fatal(parent.prettify() + ITEM_EXISTS_ERROR);
        }
    }

    parent.result = identifier;
}

void key(parameter& parent, const std::string key_name) {
    parent.result = key_name;
}

void value(parameter& parent, const std::string new_value) {
    std::vector<std::string> keys = getKeys(parent);
    std::vector<std::string> fvals = getVals(new_value);
    std::vector<std::string> otypes = getTypes(parent);
    // check for inconsistency
    if (keys.size() != fvals.size()) {
        fatal(parent.prettify() + KEY_VAL_MATCH_ERROR);
    }
    // check for overflow in otypes
    if (otypes.size() > fvals.size()) {
        fatal(parent.prettify() + TOO_MANY_OTYPES_ERROR);
    }
    // fill otypes
    for (int i = 0; i < fvals.size()-otypes.size(); i++) {
        otypes.push_back("string");
    }

    std::string path = getOut(parent);
    std::string identifier = getItem(parent);

    // read the json
    nm::json jf = read(parent, path);

    // find the element and replace the key
    for (auto& j : jf) {
        if (j["identifier"] == identifier || identifier == "[ALL]") {
            bool out = false; // break out
            for (int k = 0; k < keys.size(); k++) {
                std::string key = keys[k];
                std::string fval = fvals[k];
                std::string otype = otypes[k];
                
                // attempt at conversion
                if (otype == "null") {
                    j[key] = {};
                } else if (otype == "string") {
                    j[key] = fval;
                } else if (otype == "int" || otype == "integer") {
                    int val;
                    try {
                        val = std::stoi(fval);
                    } catch (std::exception) {
                        fatal(parent.prettify() + TYPE_CONVERSION_ERROR);
                    }
                    j[key] = val;
                } else if (otype == "float" || otype == "decimal") {
                    double val;
                    try {
                        val = std::stod(fval);
                    } catch (std::exception) {
                        fatal(parent.prettify() + TYPE_CONVERSION_ERROR);
                    }
                    j[key] = val;
                } else if (otype == "boolean" || otype == "bool") {
                    j[key] = fval[0] == 't' ? true : false;
                    fval = j[key] ? "true" : "false";
                } else {
                    // impossible, but you never know
                    fatal(parent.prettify() + INVALID_TYPE_ERROR);
                }
                success("Value of key '" + key + "' has been assigned the value '" + fval + "' (of type '" + otype + "') for item '" + std::string(j["identifier"]) + "'.");
                // basically only say once for [ALL]
                if (identifier != "[ALL]") {
                    out = true;
                }
            }
            if (out) {
                break;
            }
        }
    }

    // write to json
    write(parent, path, jf);
}

void pop(parameter& parent, const std::string _) {
    // get stuffs
    std::string path = getOut(parent);
    std::string identifier = getItem(parent);

    std::vector<std::string> keys = getKeys(parent);

    // this causes... a lot of issues
    for (auto& key : keys) {
        if (key == "identifier") { 
            fatal(parent.prettify() + CANNOT_POP_KEY_IDENTIFIER_ERROR);
        }
    }

    // read json
    nm::json jf = read(parent, path);
    
    // pop value
    nm::json jfinal;
    bool overall = false;
    for (auto& j : jf) {
        auto clone = j;
        if (j["identifier"] == identifier || identifier == "[ALL]") {
            for (auto& k : keys) {
                bool changed = false;
                for (auto& i : j.items()) {
                    if (i.key() == k) {
                        changed = true;
                        overall = true;
                        clone.erase(k);
                    }
                }
                if (!changed) {
                    warning("Key '" + k + "' not present in '" + std::string(j["identifier"]) + "'.");
                } else {
                    success("Key '" + k + "' removed from item '" + std::string(j["identifier"]) + "'.");
                }
            }
        }
        jfinal.push_back(clone);
    }

    if (!overall) {
        fatal("Nothing changed.");
    }

    // write json
    write(parent, path, jfinal);
}

void type(parameter& parent, const std::string object_type) {
    // might as well get it over with
    parent.result = object_type;
}

void readable(parameter& parent, const std::string _identifier) {
    // fetch items and all
    std::string path = getOut(parent);
    std::string identifier;
    if (_identifier != ABSENT) {
        identifier = _identifier;
    } else {
        identifier = getItem(parent, false);
    }

    // if no identifier
    if (identifier == "" || identifier == "[ALL]") {
        for (const auto& j : read(parent, path)) {
            readable(parent, j["identifier"]);
        }
    }

    // read all of contents
    std::string out; // string to be pushed to console
    nm::json jf = read(parent, path);
    for (const auto& j : jf) {
        if (j["identifier"] == identifier || identifier == "[ALL]") {
            out += paint("╔═: ", "grey") + paint(j["identifier"].get<std::string>(), {"yellow", "bold"}) + "\n";
            int i = 0; // tracker
            for (auto& kav : j.items()) {
                i++;
                if (kav.key() == "identifier") {
                    continue;
                }
                if (i == j.size()) {
                    out += paint("╚ ", "grey");
                } else {
                    out += paint("╠ ", "grey");
                }
                out += paint(kav.key(), {"turqoise", "italic"}) + " : ";
                out += paint(kav.value(), "yellow") + "\n";
            }
            if (i < 2) {
                out += paint("╚ ", "grey") + paint("N/A", "lightred") + "\n";
            }
        }
    }

    // out to console
    std::cout << out;
}

void verbose(parameter& parent, const std::string _) {
    parent.passed = "verbose";
}

void search(parameter& parent, const std::string term) {
    // get values
    std::string path = getOut(parent);

    // read json
    nm::json jf = read(parent, path);

    // iterate
    std::string out = "";
    int i = 0;
    for (const auto& j : jf) {
        i += 1;
        // loop through items
        bool saidIdentifier = false;
        std::string idstr = paint("⮩ ", "grey") + highlight(j["identifier"], term, {"yellow", "bold"}) + "\n";
        // add identifier beforehand if contains term
        if (std::string(j["identifier"]).find(term) != std::string::npos) {
            out += idstr;
            saidIdentifier = true;
        }
        for (auto& it : j.items()) {
            if (it.key() == "identifier") {
                continue;
            } else {
                // get true value of value to string
                std::ostringstream oss;
                oss << it.value();
                std::string val = oss.str();
                // if string, remove quotations
                if (it.value().is_string()) {
                    val = val.replace(0, 1, "");
                    val = val.replace(val.size()-1, val.size(), "");
                }
                // if exists
                if (it.key().find(term) != std::string::npos || val.find(term) != std::string::npos) {
                    if (!saidIdentifier) { // add name to out if not already
                        out += idstr;
                        saidIdentifier = true;
                    }
                    out += paint(" ⬥ ", "grey");
                    // feed to out
                    out += highlight(it.key(), term, {"turqoise", "italic"}) + " : ";
                    std::string qt = paint("\"", "yellow");
                    out += qt + highlight(val, term, {"yellow"}) + qt + "\n";
                }
            }
        }
    }

    // if nothing
    if (out == "") {
        fatal(parent.prettify() + NO_INSTANCE_ERROR);
    }

    // feed to console
    std::cout << out;
}

void encrypt(parameter& parent, const std::string phrase) {
    // get outfile for encrypting
    std::string path = getOut(parent);

    // check if all are the same
    bool same = true;
    int last = -1;
    for (int c : phrase) {
        if (last == -1) {
            last = c;
            continue;
        }
        if (c != last) {
            same = false;
            break;
        } else {
            same = true;
            continue;
        }
    }
    // has to be a proper series - otherwise considered single char
    if (same) {
        fatal(parent.prettify() + PASSCODE_SERIES_ERROR);
    }

    // check if ends in trailing characters
    // basically identical to the codeblock above, but backwards
    same = true;
    int count = 0;
    last = -1;
    for (int i = phrase.size()-1; i > -1; i--) {
        int c = (int)(phrase[i]);
        if (last == -1) {
            last = c;
        }
        if (c == last) {
            same = true;
            last = c;
            count++;
            continue;
        } else {
            break;
        }
    }
    if (count > 1) {
        fatal(parent.prettify() + PASSCODE_END_SERIES_ERROR);
    }

    // check if too short
    if (phrase.size() < MINPASS) {
        fatal(parent.prettify() + PHRASE_TOO_SHORT_ERROR);
    }

    // in and out
    std::fstream fin(path, std::fstream::in);
    std::fstream fout(ENCRYPT, std::fstream::out);

    // charshift algorithm
    char i;
    while (fin >> std::noskipws >> i) {
        int temp;
        for (int x = 0; x < phrase.size(); x++) {
            temp = i + phrase[x];
        }
        fout << (char)temp;
    }

    // finished
    fin.close();
    fout.close();

    // success
    success("Successfully encrypted '" + path + "'.");
}

void decrypt(parameter& parent, const std::string phrase) {
    // get outfile for decrypting
    std::string path = getOut(parent, false, ENCRYPT);

    // in and out
    std::fstream fin(path, std::fstream::in);
    std::ostringstream comp;

    // essentially do the opposite of encrypt
    // shift all bytes left by c as integer for every c in phrase
    char i;
    while (fin >> std::noskipws >> i) {
        int temp;
        for (int x = 0; x < phrase.size(); x++) {
            temp = i - phrase[x];
        }
        comp << (char)temp;
    }

    // finished-ish
    fin.close();

    // now attempt to parse
    bool valid = true;
    nm::json jp;
    try {
        jp = nm::json::parse(comp.str());
    } catch (nm::json_abi_v3_11_2::detail::parse_error) {
        valid = false;
    }
    if (jp.is_null() || !valid) {
        fatal(parent.prettify() + DECRYPT_FAIL_ERROR);
    }

    // since all went well, write
    std::fstream fout(DECRYPT, std::fstream::out);
    fout << comp.str();
    fout.close();

    // set outfile to new decrypted
    setParameterValue("o", DECRYPT);

    // success
    success("Successfully decrypted '" + path + "'.");    
}

void colourless(parameter& parent, const std::string _) {
    parent.result = "colourless";
    warning("Disabled colours.");
}

void force(parameter& parent, const std::string _) {
    warning("In force mode.");
    parent.result = "colourless";
}

void count(parameter& parent, const std::string _) {
    std::string path = getOut(parent);
    nm::json jf = read(parent, path);
    std::cout << paint("There are ", "green") << paint(std::to_string(jf.size()), (jf.size() > 0 ? "magenta" : "lightred")) << paint(" items in the database.", "green") << std::endl;
}

/*/////////*
//  MAIN  //
*/////////*/

int main(int argc, char ** argv) {
    // all parameters
    // organised in such a manner that, during iteration, parameters will work no matter the order
    mainParameters = {

        (parameter({"V", "verbose"},
        "Enables warning errors.",
        "", verbose, false, false)),

        (parameter({"F", "force"},
        "Forces the program to run despite fatal errors. Use with caution.",
        "", force, false, false)),

        (parameter({"c", "colourless"},
        "Disables colours.",
        "", colourless, false, false)),

        (parameter({"C", "count"},
        "Returns the number of elements in the database.",
        "", count, false, true)),

        (parameter({"?", "help"},
        "Provides help for all/a parameter(s).",
        "parameter", help, false, true)),

        (parameter({"o", "outfile"},
        "Specifies the target database JSON file. If none is provided, a '" + DATABASE + "' will be assumed.",
        "path", outfile, true, false)),

        (parameter({"d", "decrypt"},
        "Attempts to decrypt the o/outfile specified with phrase provided - dumps to '" + DECRYPT + "'.",
        "phrase", decrypt, true, false)),

        (parameter({"e", "encrypt"},
        "Encrypts the outfile with the phrase provided - dumps to '" + ENCRYPT + "'.",
        "phrase", encrypt, true, true)),

        (parameter({"s", "search"}, 
        "Searches through the database for the provided term. Prints matching to the console.",
        "term", search, true, true)),

        (parameter({"@", "item"},
        "Specifies the item to be used with the !/erase, k/key and r/readable parameters.",
        "name/identifier", item, true, false)),
        
        (parameter({"+", "add"},
        "Adds an item to the database by its name/identifier.",
        "name/identifier", add, true, false)),
        
        (parameter({"!", "erase"},
        "Removes the specified item.",
        "", erase, false, true)),

        (parameter({"r", "readable"},
        "Reads the entirety of a item's contents in a readable format. If no item is specified with @/item, all items in the database will be displayed neatly.",
        "", readable, false, true)),

        (parameter({"t", "type"},
        "Specifies the types of each of the values provided. Can be 'string', 'int'/'integer', 'float'/'decimal' or 'null'. Seperated by commas.",
        "type,type,...", type, true, false)),

        (parameter({"k", "key"},
        "Specifies the keys to be modified on the item. Seperated by commas.",
        "key,key,...", key, true, false)),

        (parameter({"p", "pop"},
        "Pops key, removing it from the item specified.",
        "", pop, false, false)),

        (parameter({"v", "value"},
        "The values to be assigned to each given key. Seperated by commas.",
        "value,value,...", value, true, false)),

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
    int i = 0;
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
                    i += 1;
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

        // add to passed
        param.execute(param, value);
        if (param.blockingFunc) {
            return 0;
        } else {
            continue;
        }
    }

    // if no parameters provided, return error
    if (i == 0) {
        return fatal("No parameters provided.");
    }
}
