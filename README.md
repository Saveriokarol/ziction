# Ziction
The niche lightweight tool for concatenating files in one go.

## Description
Ziction is a niche lightweight tool that concatenates files without a escaped separator.
It is highly versatile and can be parsed easily on every platform, including low resource embedded.

## File Structure
The header of the `.zict` format is simple:

| Type                      | Description                   |
|---------------------------|-------------------------------|
| `LSB uint32_t indl`       | The size of the offset table. |
| `LSB uint32_t vect[indl]` | The offset table.             |
| `void* ...`               | The concatenated files.       |

## Building & Installing
To build, simply do:
```make```
To install, simply do:
```sudo make install```
Note: If you want to directly build & install, `install` rule arleady depends on the `all` rule, so you would:
```sudo make install```

## License
Covered by the [MIT License](LICENSE).
