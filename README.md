# know-it-all

![GitHub](https://img.shields.io/github/license/jibstack64/know-it-all)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/jibstack64/know-it-all)
![GitHub all releases](https://img.shields.io/github/downloads/jibstack64/know-it-all/total)

*A command line utility for storing, modifying and managing a database of many items.*
> <img src="https://pyxis.nymag.com/v1/imgs/a33/a1b/ff252c077aab7eaa9980c702142ae3abff-wojak-00.2x.w710.jpg" alt="Big brain wojak" width="250"/>

## Parameters

### How parameter values are parsed
Spaces in the input stream must be replaced with `:/s` to be considered a full string (e.g. `hello:/sworld` would be parsed as `hello world`).
Arguments are parsed in order. Essentially, `o/outfile`, `@/item` and `+/add` are among some arguments that are parsed first. This means that you can do some useful things, such as `-+ myitem -k mykey -t int -v 1234`, but could lead to some issues if you push the boundaries and start using `+/add` alongside `!/erase`. For this reason, I recommend only using `+/add` in this way, otherwise the database may break or lose data.

### Modification and appendage
- `-o/outfile <path>` - Specifies the target database JSON file. If none is provided, a `./database.json` will be assumed.
- `-+/add <name/identifier>` - Adds an item to the database by its name/identifier.
- `-!/erase` - Removes the `@/item` from the database.
- `-k/key <key>` - Specifies the key to be modified on the `@/item`.
- `-v/value <new-value>` - The value to be assigned to the `k/key`.
- `-t/type <type-name>` - Specifies the type of contents that `v/value` holds. Can be 'string', 'int'/'integer', 'float'/'decimal' or 'null'.
- `-@/item <name/identifier>` - Specifies the item to be used with the `!/erase`, `k/key` and `r/readable` parameters. If `[ALL]` is provided, then **all** items in the database will be selected.
- `-p/pop` - Pops `key`, removing it from the `@/item`.
  
### Catalog and iteration
- `-s/search <term>` - Iterates through all items in the database; if an item's name/identifier or inner value(s) contain `term`, its name/identifier and the value(s) in which `term` was found in are written to the console in a similar style to `r/readable`.

### Other
- `-?/help [parameter]` - Provides a help-sheet for all parameters, (almost) identical to that of this README. If `parameter` is passed, only help for that single parameter will be written to the console.
- `-r/readable` - If an `@/item` is specified, the contents of that specific item will be beautified and fed to the console; if not, all items will be displayed in a neat, readable style.
- `-V/verbose` - If passed, warning errors will be shown. Use this if you are unsure to the issue at hand.
  
#### Example:
<img src="https://cdn.discordapp.com/attachments/870419973607129139/1046425653328740413/image.png" alt="Example" width="525"/>

### Features to-come
- [ ] Multi-key, value and type appending (e.g. `-k mykey1,mykey2 -t int,string -v 5,hi`).
- [ ] Catalog search (search the web for suitable matches to an item based on its data).
- [ ] Write to URL/cloud-file (e.g. `-o https://.../db.json`)
- [ ] Compile Windows executables.

### Credits (non-std libraries)
[pretty.hpp](https://github.com/jibstack64/pretty) for colours, [argh.h](https://github.com/adishavit/argh) for argument parsing and [json.hpp](https://github.com/nlohmann/json) for JSON modification.
