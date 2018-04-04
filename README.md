# apc-spdu-ctl

## improve git diff

add the following to [.git/config](https://tante.cc/2010/06/23/managing-zip-based-file-formats-in-git/):

```
[diff "zip"]
textconv = unzip -c -a
```
