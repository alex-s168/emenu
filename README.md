# emenu
extension to dmenu, meant to be used with dmenu.

Usage: just execute `emenu_run` the same way you would execute `dmenu_run`

## Advantages over normal `dmenu_run`
Programs that you used more than others will show first in the search results,
increasing productivity

## Building
First make sure that you have all the GIT submodules.

Simply run `./build.sh build`, which will automatically detect things like your C compiler.
If you need to overwrite it however, you can manually set `CC`, `CFLAGS`, and `LDFLAGS`.

This will create a `emenu_run` and a `emenu_path`, which both need to be installed.

You can also just run `./build.sh install` which will build and install to `$(DESTDIR)$(PREFIX)`, which equals to `/usr` if unset.
