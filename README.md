# dict
Query dict in terminal

## build

```bash
$ mkdir build
$ cmake -DCMAKE_BUILD_TYPE=Release .. (for release version) or cmake -DCMAKE_BUILD_TYPE=Debug .. (for debug version)
$ make
```
Then just copy the generated bin `dict` to $PATH.

## usage

```bash
$ dict word-to-query
```
