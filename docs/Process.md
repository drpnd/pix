# Process
## Memory structure

| Range                    | Description                            |
| :----------------------: | :------------------------------------- |
| `0x40000000--0x********` | Code segment (RO, Exec); 4KB-aligned   |
| `0x********--0x********` | Data segment (R/W); 4KB-aligned        |
| `0x********--0x********` | Userland memory                        |
| `0x********--0x********` | Stack (R/W)                            |
| `0x********--0xbfffffff` | Arguments in Stack (R/W); 256B-aligned |
| `0xc0000000--0xffffffff` | Kernel region                          |
| `>= 0x100000000`         | Userland memory                        |
