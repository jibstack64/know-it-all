# know-it-all

![GitHub](https://img.shields.io/github/license/jibstack64/know-it-all)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/jibstack64/know-it-all)
![GitHub all releases](https://img.shields.io/github/downloads/jibstack64/know-it-all/total)

*A command line utility for indexing, modifying and managing a database of many items.*
> <img src="https://pyxis.nymag.com/v1/imgs/a33/a1b/ff252c077aab7eaa9980c702142ae3abff-wojak-00.2x.w710.jpg" alt="Big brain wojak" width="250"/>

## Parameters

### Illustration

`[]` means non-blocking.

`{}` means blocks all parameters after it, as illustrated below.

`kial {-?/help} [-V/verbose] [-o/outfile] [-d/decrypt] {-e/encrypt} {-s/search} [-@/item] [-+/add] {-!/erase} {-r/readable} [-t/type] [-k/key] [-p/pop] [-v/value]`
> **NOTE** For instance, the `+/add` parameter overwrites the `o/outfile` parameter's value as it is after it. This goes for `d/decrypt` too - if you decrypt a file, the decrypted filepath (`./decrypted.json`) will overwrite the provided `o/outfile`.
> The order in which you provide these arguments is obsolete; they are parsed in the order above regardless.

### Modification and appendage
- `-o/outfile <path>` - Specifies the target database JSON file. If none is provided, a `./database.json` will be assumed.
- `-+/add <name/identifier>` - Adds an item to the database by its name/identifier.
- `-!/erase` - Removes the `@/item` from the database.
- `-k/key <key>` - Specifies the key to be modified on the `@/item`.
- `-v/value <new-value>` - The value to be assigned to the `k/key`.
- `-t/type <type-name>` - Specifies the type of contents that `v/value` holds. Can be 'string', 'int'/'integer', 'float'/'decimal', 'bool'/'boolean' or 'null'.
- `-@/item <name/identifier>` - Specifies the item to be used with the `!/erase`, `k/key` and `r/readable` parameters. If `[ALL]` is provided, then **all** items in the database will be selected.
- `-p/pop` - Pops `key`, removing it from the `@/item`.
  
### Catalog and iteration
- `-s/search <term>` - Iterates through all items in the database; if an item's name/identifier or inner value(s) contain `term`, its name/identifier and the value(s) in which `term` was found in are written to the console in a similar style to `r/readable`.
  
### Encryption and decryption
> **NOTE**
> The encryption algorithm is a very basic two-dimensional bitshift algorithm. It works, but make sure that your password is long, or your data may be at risk from bruteforce attacks. **Your phrase cannot be a series of characters!** Otherwise your passcode is encrypted (and decrypted) as being the single character that is repeated, reducing the entire point of the encrypt/decrypt functions!
>
> If you have a repeating sequence in your phrase, for example, `testtest`, `test` will work for decrypting as well as `testtest` due to the way the algorithm works. For this reason, I suggest avoiding repeating words in your phrase.
>
> This is *in progress* and is prone to change soon.
- `-e/encrypt <phrase>` - Encrypts the `o/outfile` provided to `./encrypted.json`.
- `-d/decrypt <phrase>` - Decrypts the `o/outfile` provided to `./decrypted.json`. If no `o/outfile` is provided, an `./encrypted.json` will be used (if found).

### Other
- `-?/help [parameter]` - Provides a help-sheet for all parameters, (almost) identical to that of this README. If `parameter` is passed, only help for that single parameter will be written to the console.
- `-r/readable` - If an `@/item` is specified, the contents of that specific item will be beautified and fed to the console; if not, all items will be displayed in a neat, readable style.
- `-V/verbose` - If passed, warning errors will be shown. Use this if you are unsure to the issue at hand.
  
### Builds
Releases currently ship with **Windows 7+** and **Linux** builds. You may build it yourself, add preprocessor fields for OS-compatibility, and create a PR, if you so wish.

#### Examples:
<img src="https://cdn.discordapp.com/attachments/870419973607129139/1046425653328740413/image.png" alt="Example 1" width="525"/>
<img src="https://cdn.discordapp.com/attachments/870419973607129139/1046431951705362513/image.png" alt="Example 2" width="240"/>
<img src="https://cdn.discordapp.com/attachments/870419973607129139/1046437175547412530/image.png" alt="Example 3" width=250>

### Features to-come
- [x] Encryption and decryption.
- [ ] Multi-key, value and type appending (e.g. `-k mykey1,mykey2 -t int,string -v 5,hi`).
- [ ] ~~Catalog search (search the web for suitable matches to an item based on its data).~~ Scrapped - not needed.
- [ ] Write/read from URL/cloud-file (e.g. `-o https://.../db.json`)
- [ ] Write/read from SSH (scp).
- [x] Compile Windows executables.

### Credits (non-std libraries)
[pretty.hpp](https://github.com/jibstack64/pretty) for colours, [argh.h](https://github.com/adishavit/argh) for argument parsing and [json.hpp](https://github.com/nlohmann/json) for JSON modification.
