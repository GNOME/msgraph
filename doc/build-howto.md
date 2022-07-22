Title: Compiling with Libmsgraph
Slug: building

# Compiling with Libmsgraph

If you need to build Libmsgraph, get the source from
[here](https://gitlab.gnome.org/jbrummer/msgraph/) and see the `README.md` file.

## Using `pkg-config`

Like other GNOME libraries, Libmsgraph uses `pkg-config` to provide compiler
options. The package name is `libmsgraph-0`.

When using the Meson build system you can declare a dependency like:

```meson
dependency('libmsgraph-0')
```

The `0` in the package name is the "API version" (indicating "the version of the
Libmsgraph API that first appeared in version 0") and is essentially just part
of the package name.

