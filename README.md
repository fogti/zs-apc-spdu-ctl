# apc-spdu-ctl

## License

The most parts of this project are licensed under the MIT license, but we use code parts from [net-snmp](docs/COPYING.net-snmp).

We also call the 'fping' command using 'system()', look at [COPYING.fping](docs/COPYING.fping).

## improve git diff

add the following to [.git/config](https://tante.cc/2010/06/23/managing-zip-based-file-formats-in-git/):

```
[diff "zip"]
textconv = unzip -c -a
```
